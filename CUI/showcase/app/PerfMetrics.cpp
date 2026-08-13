#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "PerfMetrics.h"
#include "framework/window/Window.h"
#include <d3d11.h>
#include <dxgi1_4.h>
#include <psapi.h>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>
#include <wrl/client.h>

#pragma comment(lib, "psapi.lib")

namespace CUI {

using Microsoft::WRL::ComPtr;

namespace {
SIZE_T SamplePrivateWorkingSetBytes(HANDLE process) {
    SYSTEM_INFO si{};
    GetSystemInfo(&si);
    const SIZE_T pageSize = si.dwPageSize ? si.dwPageSize : 4096;

    DWORD bytes = 64 * 1024;
    for (int attempt = 0; attempt < 8; ++attempt) {
        std::vector<unsigned char> buf(bytes);
        auto* ws = reinterpret_cast<PSAPI_WORKING_SET_INFORMATION*>(buf.data());
        if (QueryWorkingSet(process, ws, bytes)) {
            SIZE_T priv = 0;
            const ULONG_PTR n = ws->NumberOfEntries;
            const size_t maxEntries =
                (bytes >= sizeof(PSAPI_WORKING_SET_INFORMATION))
                    ? (bytes - offsetof(PSAPI_WORKING_SET_INFORMATION, WorkingSetInfo))
                          / sizeof(PSAPI_WORKING_SET_BLOCK)
                    : 0;
            const ULONG_PTR count = (std::min)(n, static_cast<ULONG_PTR>(maxEntries));
            for (ULONG_PTR i = 0; i < count; ++i) {
                if (!ws->WorkingSetInfo[i].Shared) {
                    priv += pageSize;
                }
            }
            return priv;
        }
        if (GetLastError() != ERROR_BAD_LENGTH) {
            break;
        }
        bytes *= 2;
    }
    return 0;
}

float SampleGpuUsage01(ID3D11Device* device) {
    if (!device) {
        return -1.0f;
    }
    ComPtr<IDXGIDevice> dxgiDevice;
    if (FAILED(device->QueryInterface(IID_PPV_ARGS(&dxgiDevice))) || !dxgiDevice) {
        return -1.0f;
    }
    ComPtr<IDXGIAdapter> adapter;
    if (FAILED(dxgiDevice->GetAdapter(&adapter)) || !adapter) {
        return -1.0f;
    }
    ComPtr<IDXGIAdapter3> adapter3;
    if (FAILED(adapter.As(&adapter3)) || !adapter3) {
        return -1.0f;
    }
    DXGI_QUERY_VIDEO_MEMORY_INFO info{};
    if (FAILED(adapter3->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &info))) {
        return -1.0f;
    }
    if (info.Budget == 0) {
        return -1.0f;
    }
    return static_cast<float>(
        (std::min)(1.0, static_cast<double>(info.CurrentUsage) / static_cast<double>(info.Budget)));
}
} // namespace

PerfSampler& PerfSampler::Instance() {
    static PerfSampler s;
    return s;
}

const PerfSnapshot& PerfSampler::Sample() {
    const auto now = std::chrono::steady_clock::now();
    if (m_hasSample && (now - m_lastSampleAt) < std::chrono::milliseconds(200)) {
        return m_last;
    }
    m_lastSampleAt = now;
    m_hasSample = true;

    SIZE_T workingSet = 0;
    PROCESS_MEMORY_COUNTERS_EX pmc{};
    pmc.cb = sizeof(pmc);
    if (GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc), sizeof(pmc))) {
        workingSet = pmc.WorkingSetSize;
    }
    SIZE_T privateWs = SamplePrivateWorkingSetBytes(GetCurrentProcess());
    if (privateWs > workingSet && workingSet > 0) {
        privateWs = workingSet;
    }
    m_last.privateMb = static_cast<float>(static_cast<double>(privateWs) / (1024.0 * 1024.0));
    m_last.workingSetMb = static_cast<float>(static_cast<double>(workingSet) / (1024.0 * 1024.0));

    FILETIME create{}, exit{}, kernel{}, user{};
    if (GetProcessTimes(GetCurrentProcess(), &create, &exit, &kernel, &user)) {
        ULARGE_INTEGER k{}, u{};
        k.LowPart = kernel.dwLowDateTime;
        k.HighPart = kernel.dwHighDateTime;
        u.LowPart = user.dwLowDateTime;
        u.HighPart = user.dwHighDateTime;
        const ULONGLONG cpu100ns = k.QuadPart + u.QuadPart;
        if (m_hasCpuSample) {
            const double deltaCpu = static_cast<double>(cpu100ns - m_lastCpu100ns);
            const double deltaWallSec = std::chrono::duration<double>(now - m_lastCpuSample).count();
            SYSTEM_INFO si{};
            GetSystemInfo(&si);
            const double cores = (std::max)(1.0, static_cast<double>(si.dwNumberOfProcessors));
            if (deltaWallSec > 0.001) {
                const double cpuSec = deltaCpu * 1e-7;
                m_last.cpuPct = static_cast<float>(std::clamp((cpuSec / deltaWallSec) / cores * 100.0, 0.0, 100.0));
            }
        }
        m_lastCpu100ns = cpu100ns;
        m_lastCpuSample = now;
        m_hasCpuSample = true;
    }

    m_last.fps = 0.0f;
    ID3D11Device* d3d = nullptr;
    if (Window* win = Window::Current()) {
        m_last.fps = win->GetDisplayFps();
        d3d = win->GetGraphicsContext().GetD3DDevice();
    }
    const float gpu01 = SampleGpuUsage01(d3d);
    if (gpu01 >= 0.0f) {
        m_last.gpuPct = gpu01 * 100.0f;
    }

    return m_last;
}

} // namespace CUI
