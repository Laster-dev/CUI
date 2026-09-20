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

    void ApplyOutputCallback(OutputCallback callback) override { m_onOutput = std::move(callback); }

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
    std::wstring m_shellPath;   // 可执行文件全路径（CreateProcess 的 lpApplicationName）
    std::wstring m_arguments;   // 参数部分（拼接时带引号，避免路径劫持与参数注入）
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
    // 已排队但未写出的字节数：子进程卡死时输入会无限堆积，必须有上限（背压）。
    size_t m_queuedBytes = 0;
    static constexpr size_t kMaxQueuedBytes = 4u * 1024u * 1024u;

    OutputCallback m_onOutput;
};

} // namespace Term
} // namespace CUI
