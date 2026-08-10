#pragma once
#include <cstddef>
#include <functional>
#include <string>

namespace CUI {
namespace Term {

// Pluggable I/O backend (ConPTY, process stdio, TCP, echo).
// Hosts implement this to feed bytes into Terminal via the output callback and
// receive keyboard/mouse bytes through Write().
class ITerminalBackend {
public:
    using OutputCallback = std::function<void(const char*, size_t)>;

    virtual ~ITerminalBackend() = default;

    // Callback is invoked on the backend's reader thread; implementations must
    // only queue the data.
    virtual void SetOutputCallback(OutputCallback callback) = 0;

    virtual bool Start(int cols, int rows) = 0;
    virtual void Write(const char* data, size_t length) = 0;
    virtual void Resize(int cols, int rows) = 0;
    virtual void Stop() = 0;
    virtual bool IsRunning() const = 0;

    void Write(const std::string& data) { Write(data.data(), data.size()); }
};

} // namespace Term
} // namespace CUI
