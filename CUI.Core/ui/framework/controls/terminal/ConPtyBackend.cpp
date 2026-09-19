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

namespace {

// 按 Windows 命令行解析规则给参数加引号：只有含空格/制表符/引号的参数需要引号，
// 且内部的引号与反斜杠必须转义，否则 CreateProcess 会按错误边界切分（参数注入）。
std::wstring QuoteArgument(const std::wstring& argument) {
    if (argument.empty()) {
        return L"\"\"";
    }
    const bool needsQuotes =
        argument.find_first_of(L" \t\n\v\"") != std::wstring::npos;
    if (!needsQuotes) {
        return argument;
    }

    std::wstring out;
    out.push_back(L'"');
    size_t backslashCount = 0;
    for (const wchar_t ch : argument) {
        if (ch == L'\\') {
            ++backslashCount;
            out.push_back(ch);
            continue;
        }
        if (ch == L'"') {
            // 转义引号前的所有反斜杠，再加上转义引号本身的反斜杠。
            out.append(backslashCount, L'\\');
            out.push_back(L'\\');
            backslashCount = 0;
        }
        backslashCount = 0;
        out.push_back(ch);
    }
    // 结尾引号前的反斜杠同样要加倍。
    out.append(backslashCount, L'\\');
    out.push_back(L'"');
    return out;
}

} // namespace

ConPtyBackend::ConPtyBackend(const std::wstring& shellPath, const std::wstring& arguments)
    : m_shellPath(shellPath), m_arguments(arguments) {
    // 可执行文件与参数分开保存：CreateProcess 使用 lpApplicationName 指定可执行文件，
    // 不再依赖“把整行交给系统按 PATH/空格推断”的行为，杜绝路径劫持。
    m_commandLine = QuoteArgument(shellPath);
    if (!arguments.empty()) {
        m_commandLine += L" ";
        m_commandLine += arguments;
    }
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
    // 显式传入 lpApplicationName（可执行文件）：
    // 传 nullptr 时系统会从命令行字符串里按空格/引号规则重新切分出可执行文件，
    // 解析差异会导致加载到非预期的程序（路径劫持）。
    if (!CreateProcessW(m_shellPath.c_str(), commandBuffer.data(), nullptr, nullptr, FALSE,
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
    m_writeThread = std::thread(&ConPtyBackend::WriteLoop, this);
    return true;
}

void ConPtyBackend::Write(const char* data, size_t length) {
    if (!m_started || !data || length == 0 || m_inputWrite == nullptr) {
        return;
    }

    // Queue only; the WriteFile to the ConPTY input pipe happens on the
    // writer thread so the UI thread never blocks on a busy child.
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        // 背压：子进程停止消费时输入会无限堆积并吃掉内存。
        // 超出上限后丢弃最旧的块（保留最新输入），保证内存有界。
        if (m_queuedBytes + length > kMaxQueuedBytes) {
            while (!m_writeQueue.empty() && m_queuedBytes + length > kMaxQueuedBytes) {
                m_queuedBytes -= m_writeQueue.front().size();
                m_writeQueue.pop_front();
            }
        }
        if (length > kMaxQueuedBytes) {
            return; // 单块本身就超限，直接丢弃
        }
        m_writeQueue.emplace_back(data, length);
        m_queuedBytes += length;
    }
    m_writeCv.notify_one();
}

void ConPtyBackend::WriteLoop() {
    std::string chunk;
    for (;;) {
        {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            m_writeCv.wait(lock, [this] { return m_cancelled || !m_writeQueue.empty(); });
            if (m_cancelled) {
                break;
            }
            chunk = std::move(m_writeQueue.front());
            m_queuedBytes -= chunk.size();
            m_writeQueue.pop_front();
        }
        std::lock_guard<std::mutex> lock(m_writeMutex);
        if (m_inputWrite == nullptr) {
            break;
        }
        size_t offset = 0;
        while (offset < chunk.size()) {
            DWORD written = 0;
            const DWORD bytes = static_cast<DWORD>(
                (std::min)(chunk.size() - offset, static_cast<size_t>(64 * 1024)));
            if (!WriteFile(m_inputWrite, chunk.data() + offset, bytes, &written, nullptr) ||
                written == 0) {
                return;
            }
            offset += written;
        }
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

    // Drain the writer before the input handle goes away.
    m_writeCv.notify_all();
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_writeQueue.clear();
        m_queuedBytes = 0;
    }
    if (m_writeThread.joinable()) {
        if (m_writeThread.get_id() == std::this_thread::get_id()) {
            m_writeThread.detach();
        } else {
            m_writeThread.join();
        }
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

    // 关闭 ConPTY 后子进程一般自行退出；但它可能忽略控制台关闭（例如仍在等待子命令），
    // 从而变成残留进程占着句柄。这里先给它一个短暂宽限期，超时则强制终止。
    if (m_process != nullptr) {
        if (WaitForSingleObject(m_process, 2000) == WAIT_TIMEOUT) {
            TerminateProcess(m_process, 1);
            WaitForSingleObject(m_process, 1000);
        }
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
