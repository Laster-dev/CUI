#pragma once

#include <chrono>
#include <cstdint>

namespace CUI {

struct PerfSnapshot {
    float privateMb = 0.0f;
    float workingSetMb = 0.0f;
    float cpuPct = 0.0f;
    float gpuPct = -1.0f; // <0 = unavailable
    float fps = 0.0f;
};

// Shared with GalleryPerfStatusBar so Chart and the chrome bar read one sample.
class PerfSampler {
public:
    static PerfSampler& Instance();

    const PerfSnapshot& Last() const { return m_last; }
    const PerfSnapshot& Sample();

private:
    PerfSnapshot m_last{};
    std::uint64_t m_lastCpu100ns = 0;
    std::chrono::steady_clock::time_point m_lastCpuSample{};
    std::chrono::steady_clock::time_point m_lastSampleAt{};
    bool m_hasCpuSample = false;
    bool m_hasSample = false;
};

} // namespace CUI
