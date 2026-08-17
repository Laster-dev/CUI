#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "GalleryStatusBar.h"
#include "framework/window/Window.h"
#include "framework/animation/AnimationManager.h"
#include "framework/style/ThemeManager.h"
#include "framework/controls/UIElement.h"

#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <d3d11.h>
#include <dxgi1_4.h>
#include <wrl/client.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

#pragma comment(lib, "psapi.lib")

namespace Gallery {

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

uint32_t SampleThreadCount(DWORD pid) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (snap == INVALID_HANDLE_VALUE) {
        return 0;
    }
    THREADENTRY32 te{};
    te.dwSize = sizeof(te);
    uint32_t count = 0;
    if (Thread32First(snap, &te)) {
        do {
            if (te.th32OwnerProcessID == pid) {
                count++;
            }
        } while (Thread32Next(snap, &te));
    }
    CloseHandle(snap);
    return count;
}

uint32_t SampleHandleCount(HANDLE process) {
    DWORD handles = 0;
    if (GetProcessHandleCount(process, &handles)) {
        return static_cast<uint32_t>(handles);
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

GalleryStatusBar::GalleryStatusBar() {
    SetHeight(24.0f);
    SetWidth(-1.0f);
    SetBackgroundToken(CUI::ThemeTokenId::PaneBackground);
    SetBorderToken(CUI::ThemeTokenId::CardBorder);
    SetBorderThickness(1.0f);
    SetFontFamily("微软雅黑");
    SetFontSize(11.0f);
    EnsureItems();
}

void GalleryStatusBar::EnsureItems() {
    if (m_itemsReady) {
        return;
    }

    // 左侧区：当前页面、就绪状态、渲染后端
    m_pageId = AddTextItem("📍 页面: 主页 (就绪)", CUI::StatusBarItemAlignment::Left);
    AddSeparator(CUI::StatusBarItemAlignment::Left);
    m_backendId = AddTextItem("⚡ Direct2D 1.1", CUI::StatusBarItemAlignment::Left, 140.0f);

    // 右侧区：从右至左排列各项指标
    AddSeparator(CUI::StatusBarItemAlignment::Right);
    m_modeId = AddTextItem("✨ 动画开启", CUI::StatusBarItemAlignment::Right, 78.0f);

    AddSeparator(CUI::StatusBarItemAlignment::Right);
    m_dpiId = AddTextItem("DPI 96 (100%)", CUI::StatusBarItemAlignment::Right, 92.0f);

    AddSeparator(CUI::StatusBarItemAlignment::Right);
    m_handlesId = AddTextItem("句柄 0 · 线程 0", CUI::StatusBarItemAlignment::Right, 115.0f);

    AddSeparator(CUI::StatusBarItemAlignment::Right);
    m_gpuId = AddTextItem("GPU —", CUI::StatusBarItemAlignment::Right, 68.0f);

    AddSeparator(CUI::StatusBarItemAlignment::Right);
    m_cpuId = AddTextItem("CPU —", CUI::StatusBarItemAlignment::Right, 66.0f);

    AddSeparator(CUI::StatusBarItemAlignment::Right);
    m_memId = AddTextItem("内存: 私有 — / 完整 —", CUI::StatusBarItemAlignment::Right, 155.0f);

    AddSeparator(CUI::StatusBarItemAlignment::Right);
    m_fpsId = AddTextItem("— FPS", CUI::StatusBarItemAlignment::Right, 68.0f);

    m_itemsReady = true;
}

void GalleryStatusBar::SetCurrentPage(const std::string& pageTitle) {
    m_currentPage = pageTitle;
    char buf[128];
    std::snprintf(buf, sizeof(buf), "📍 页面: %s (就绪)", m_currentPage.c_str());
    SetItemText(m_pageId, buf);
}

ResourceSnapshot GalleryStatusBar::SampleSnapshot() {
    const auto now = std::chrono::steady_clock::now();
    if (m_hasSample && (now - m_lastSampleAt) < std::chrono::milliseconds(250)) {
        return m_cachedSnapshot;
    }
    m_lastSampleAt = now;
    m_hasSample = true;

    HANDLE proc = GetCurrentProcess();
    DWORD pid = GetCurrentProcessId();

    // 1. 内存采样
    SIZE_T workingSet = 0;
    PROCESS_MEMORY_COUNTERS_EX pmc{};
    pmc.cb = sizeof(pmc);
    if (GetProcessMemoryInfo(proc, reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc), sizeof(pmc))) {
        workingSet = pmc.WorkingSetSize;
    }
    SIZE_T privateWs = SamplePrivateWorkingSetBytes(proc);
    if (privateWs > workingSet && workingSet > 0) {
        privateWs = workingSet;
    }
    m_cachedSnapshot.privateMb = static_cast<float>(static_cast<double>(privateWs) / (1024.0 * 1024.0));
    m_cachedSnapshot.workingSetMb = static_cast<float>(static_cast<double>(workingSet) / (1024.0 * 1024.0));

    // 2. CPU 占用采样
    FILETIME create{}, exit{}, kernel{}, user{};
    if (GetProcessTimes(proc, &create, &exit, &kernel, &user)) {
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
                m_cachedSnapshot.cpuPct = static_cast<float>(
                    std::clamp((cpuSec / deltaWallSec) / cores * 100.0, 0.0, 100.0));
            }
        }
        m_lastCpu100ns = cpu100ns;
        m_lastCpuSample = now;
        m_hasCpuSample = true;
    }

    // 3. 句柄数与线程数
    m_cachedSnapshot.handleCount = SampleHandleCount(proc);
    m_cachedSnapshot.threadCount = SampleThreadCount(pid);

    // 4. 窗口、DPI、FPS 与 Direct3D 设备采样
    m_cachedSnapshot.fps = 0.0f;
    m_cachedSnapshot.dpi = 96;
    m_cachedSnapshot.zoomPct = 100;
    m_cachedSnapshot.isDComp = false;
    m_cachedSnapshot.animationsEnabled = CUI::UIElement::AreAnimationsEnabled();

    ID3D11Device* d3d = nullptr;
    if (CUI::Window* win = CUI::Window::Current()) {
        m_cachedSnapshot.fps = win->GetDisplayFps();
        float dpiScale = win->GetDpiScale();
        if (dpiScale <= 0.001f) {
            dpiScale = win->GetGraphicsContext().GetDpiScale();
        }
        m_cachedSnapshot.dpi = static_cast<int>(std::lround(dpiScale * 96.0f));
        m_cachedSnapshot.zoomPct = static_cast<int>(std::lround(dpiScale * 100.0f));
        m_cachedSnapshot.isDComp = win->GetGraphicsContext().UsesCompositionSwapChain();
        d3d = win->GetGraphicsContext().GetD3DDevice();
    }

    // 5. GPU 显存利用率
    const float gpu01 = SampleGpuUsage01(d3d);
    if (gpu01 >= 0.0f) {
        m_cachedSnapshot.gpuPct = gpu01 * 100.0f;
    } else {
        m_cachedSnapshot.gpuPct = -1.0f;
    }

    return m_cachedSnapshot;
}

void GalleryStatusBar::RefreshMetrics() {
    EnsureItems();
    const ResourceSnapshot snap = SampleSnapshot();

    char buf[128];

    // 页面状态
    std::snprintf(buf, sizeof(buf), "📍 页面: %s (就绪)", m_currentPage.c_str());
    SetItemText(m_pageId, buf);

    // 图形后端
    if (snap.isDComp) {
        SetItemText(m_backendId, "⚡ Direct2D / DComp");
    } else {
        SetItemText(m_backendId, "🎨 Direct2D 1.1 / D3D11");
    }

    // 动画 / 低性能模式
    SetItemText(m_modeId, snap.animationsEnabled ? "✨ 动画开启" : "⚡ 低性能模式");

    // DPI 缩放
    std::snprintf(buf, sizeof(buf), "DPI %d (%d%%)", snap.dpi, snap.zoomPct);
    SetItemText(m_dpiId, buf);

    // 句柄与线程
    std::snprintf(buf, sizeof(buf), "句柄 %u · 线程 %u", snap.handleCount, snap.threadCount);
    SetItemText(m_handlesId, buf);

    // GPU
    if (snap.gpuPct >= 0.0f) {
        std::snprintf(buf, sizeof(buf), "GPU %.0f%%", snap.gpuPct);
    } else {
        std::snprintf(buf, sizeof(buf), "GPU —");
    }
    SetItemText(m_gpuId, buf);

    // CPU
    std::snprintf(buf, sizeof(buf), "CPU %.1f%%", snap.cpuPct);
    SetItemText(m_cpuId, buf);

    // 内存
    std::snprintf(buf, sizeof(buf), "内存: 私有%.0fMB / 完整%.0fMB", snap.privateMb, snap.workingSetMb);
    SetItemText(m_memId, buf);

    // FPS
    if (snap.fps > 0.5f) {
        std::snprintf(buf, sizeof(buf), "%.1f FPS", snap.fps);
    } else {
        std::snprintf(buf, sizeof(buf), "— FPS");
    }
    SetItemText(m_fpsId, buf);
}

void GalleryStatusBar::ScheduleNextSample() {
    if (CUI::AnimationManager* mgr = CUI::AnimationManager::Current()) {
        mgr->RequestWake(this, CUI::AnimationManager::clock::now() + std::chrono::milliseconds(500));
    }
}

void GalleryStatusBar::OnRender(CUI::GraphicsContext& ctx) {
    if (!m_kickstarted) {
        m_kickstarted = true;
        RefreshMetrics();
        RequestAnimationTicks();
    }
    CUI::StatusBar::OnRender(ctx);
}

bool GalleryStatusBar::OnAnimationTick() {
    RefreshMetrics();
    ScheduleNextSample();
    return false;
}

} // namespace Gallery
