#pragma once
#include "ITerminalBackend.h"
#include <windows.h>
#include <atomic>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <string>
#include <thread>

namespace CUI {
namespace Term {

// ConPTY backend via kernel32 (dynamically resolved, no third-party deps).
class ConPtyBackend : public ITerminalBackend {
public:
    explicit ConPtyBackend(const std::wstring& shellPath = L"pwsh.exe",
                           const std::wstring& arguments = std::wstring());
    ~ConPtyBackend() override;

    ConPtyBackend(const ConPtyBackend&) = delete;
    ConPtyBackend& operator=(const ConPtyBackend&) = delete;

    void SetOutputCallback(OutputCallback callback) override { m_onOutput = std::move(callback); }

    bool Start(int cols, int rows) override;
    void Write(const char* data, size_t length) override;
    void Resize(int cols, int rows) override;
    void Stop() override;
    bool IsRunning() const override { return m_started && m_readerAlive; }

    // True when the ConPTY entry points exist on this Windows build.
    static bool IsSupported();

    const std::wstring& CommandLine() const { return m_commandLine; }

private:
    void ReadLoop();
    void WriteLoop();
    static void SafeClose(HANDLE& handle);

    std::wstring m_commandLine;
    HANDLE m_inputRead = nullptr;   // child side; closed after PTY creation
    HANDLE m_inputWrite = nullptr;  // we write pty input here
    HANDLE m_outputRead = nullptr;  // we read pty output here
    HANDLE m_outputWrite = nullptr; // child side; closed after PTY creation
    HANDLE m_process = nullptr;
    HANDLE m_thread = nullptr;
    PVOID m_pseudoConsole = nullptr; // HPCON
    LPPROC_THREAD_ATTRIBUTE_LIST m_attrList = nullptr;

    std::thread m_readThread;
    std::thread m_writeThread;
    std::atomic<bool> m_started{ false };
    std::atomic<bool> m_cancelled{ false };
    std::atomic<bool> m_readerAlive{ false };
    std::mutex m_writeMutex;
    // Keyboard input is queued and flushed on a worker thread: a synchronous
    // WriteFile to the ConPTY input pipe can block while the child is busy,
    // which froze the whole UI thread on every keystroke.
    std::mutex m_queueMutex;
    std::condition_variable m_writeCv;
    std::deque<std::string> m_writeQueue;

    OutputCallback m_onOutput;
};

} // namespace Term
} // namespace CUI
