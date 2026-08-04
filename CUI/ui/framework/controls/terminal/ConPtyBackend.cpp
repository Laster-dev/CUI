#include "ConPtyBackend.h"
#include <algorithm>
#include <vector>

#ifndef PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE
#define PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE 0x00020016
#endif

namespace CUI {
namespace Term {

namespace {
typedef HRESULT(WINAPI* PFN_CreatePseudoConsole)(COORD size, HANDLE hInput, HANDLE hOutput, DWORD dwFlags, PVOID* phPC);
typedef HRESULT(WINAPI* PFN_ResizePseudoConsole)(PVOID hPC, COORD size);
typedef VOID(WINAPI* PFN_ClosePseudoConsole)(PVOID hPC);

PFN_CreatePseudoConsole g_createPseudoConsole = nullptr;
PFN_ResizePseudoConsole g_resizePseudoConsole = nullptr;
PFN_ClosePseudoConsole g_closePseudoConsole = nullptr;

bool LoadConPtyApis() {
    static bool loaded = false;
    static bool available = false;
    if (loaded) {
        return available;
    }
    loaded = true;

    HMODULE kernel32 = GetModuleHandleW(L"kernel32.dll");
    if (kernel32) {
        g_createPseudoConsole = reinterpret_cast<PFN_CreatePseudoConsole>(
            GetProcAddress(kernel32, "CreatePseudoConsole"));
        g_resizePseudoConsole = reinterpret_cast<PFN_ResizePseudoConsole>(
            GetProcAddress(kernel32, "ResizePseudoConsole"));
        g_closePseudoConsole = reinterpret_cast<PFN_ClosePseudoConsole>(
            GetProcAddress(kernel32, "ClosePseudoConsole"));
    }
    available = g_createPseudoConsole && g_resizePseudoConsole && g_closePseudoConsole;
    return available;
}

SHORT ClampDim(int value) {
    return static_cast<SHORT>(std::clamp(value, 1, static_cast<int>(0x7FFF)));
}
}

bool ConPtyBackend::IsSupported() {
    return LoadConPtyApis();
}

ConPtyBackend::ConPtyBackend(const std::wstring& shellPath, const std::wstring& arguments) {
    m_commandLine = arguments.empty() ? shellPath : shellPath + L" " + arguments;
}

ConPtyBackend::~ConPtyBackend() {
    Stop();
}

bool ConPtyBackend::Start(int cols, int rows) {
    if (m_started) {
        return true;
    }
    if (!LoadConPtyApis()) {
        return false;
    }

    if (!CreatePipe(&m_inputRead, &m_inputWrite, nullptr, 0)) {
        return false;
    }
    if (!CreatePipe(&m_outputRead, &m_outputWrite, nullptr, 0)) {
        Stop();
        return false;
    }

    const COORD size{ ClampDim(cols), ClampDim(rows) };
    if (FAILED(g_createPseudoConsole(size, m_inputRead, m_outputWrite, 0, &m_pseudoConsole))) {
        Stop();
        return false;
    }

    // Close the handles the child side owns now that the PTY holds them.
    SafeClose(m_inputRead);
    SafeClose(m_outputWrite);

    SIZE_T attrListSize = 0;
    InitializeProcThreadAttributeList(nullptr, 1, 0, &attrListSize);
    m_attrList = static_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(
        HeapAlloc(GetProcessHeap(), 0, attrListSize));
    if (!m_attrList || !InitializeProcThreadAttributeList(m_attrList, 1, 0, &attrListSize)) {
        if (m_attrList) {
            HeapFree(GetProcessHeap(), 0, m_attrList);
            m_attrList = nullptr;
        }
        Stop();
        return false;
    }

    if (!UpdateProcThreadAttribute(m_attrList, 0, PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE,
                                   m_pseudoConsole, sizeof(PVOID), nullptr, nullptr)) {
        Stop();
        return false;
    }

    STARTUPINFOEXW startup{};
    startup.StartupInfo.cb = sizeof(STARTUPINFOEXW);
    startup.lpAttributeList = m_attrList;

    std::vector<wchar_t> commandBuffer(m_commandLine.begin(), m_commandLine.end());
    commandBuffer.push_back(L'\0');

    PROCESS_INFORMATION processInfo{};
    if (!CreateProcessW(nullptr, commandBuffer.data(), nullptr, nullptr, FALSE,
                        EXTENDED_STARTUPINFO_PRESENT, nullptr, nullptr,
                        &startup.StartupInfo, &processInfo)) {
        Stop();
        return false;
    }

    m_process = processInfo.hProcess;
    m_thread = processInfo.hThread;

    m_cancelled = false;
    m_started = true;
    m_readerAlive = true;
    m_readThread = std::thread(&ConPtyBackend::ReadLoop, this);
    return true;
}

void ConPtyBackend::Write(const char* data, size_t length) {
    if (!m_started || !data || length == 0 || m_inputWrite == nullptr) {
        return;
    }

    std::lock_guard<std::mutex> lock(m_writeMutex);
    size_t offset = 0;
    while (offset < length) {
        DWORD written = 0;
        const DWORD chunk = static_cast<DWORD>((std::min)(length - offset, static_cast<size_t>(64 * 1024)));
        if (!WriteFile(m_inputWrite, data + offset, chunk, &written, nullptr)) {
            break;
        }
        if (written == 0) {
            break;
        }
        offset += written;
    }
}

void ConPtyBackend::Resize(int cols, int rows) {
    if (m_pseudoConsole == nullptr || g_resizePseudoConsole == nullptr) {
        return;
    }
    const COORD size{ ClampDim(cols), ClampDim(rows) };
    g_resizePseudoConsole(m_pseudoConsole, size);
}

void ConPtyBackend::Stop() {
    m_cancelled = true;

    // Closing the pseudo console signals the child and unblocks the reader.
    if (m_pseudoConsole != nullptr && g_closePseudoConsole != nullptr) {
        g_closePseudoConsole(m_pseudoConsole);
        m_pseudoConsole = nullptr;
    }

    SafeClose(m_inputWrite);
    SafeClose(m_outputRead);
    SafeClose(m_inputRead);
    SafeClose(m_outputWrite);

    if (m_readThread.joinable()) {
        if (m_readThread.get_id() == std::this_thread::get_id()) {
            m_readThread.detach();
        } else {
            m_readThread.join();
        }
    }

    if (m_attrList != nullptr) {
        DeleteProcThreadAttributeList(m_attrList);
        HeapFree(GetProcessHeap(), 0, m_attrList);
        m_attrList = nullptr;
    }

    SafeClose(m_thread);
    SafeClose(m_process);

    m_started = false;
    m_readerAlive = false;
}

void ConPtyBackend::ReadLoop() {
    std::vector<char> buffer(64 * 1024);
    while (!m_cancelled && m_outputRead != nullptr) {
        DWORD read = 0;
        if (!ReadFile(m_outputRead, buffer.data(), static_cast<DWORD>(buffer.size()), &read, nullptr)) {
            break;
        }
        if (read == 0) {
            break;
        }
        if (m_onOutput) {
            m_onOutput(buffer.data(), static_cast<size_t>(read));
        }
    }
    m_readerAlive = false;
}

void ConPtyBackend::SafeClose(HANDLE& handle) {
    if (handle == nullptr) {
        return;
    }
    CloseHandle(handle);
    handle = nullptr;
}

} // namespace Term
} // namespace CUI
