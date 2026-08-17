#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "framework/controls/StatusBar.h"
#include <chrono>
#include <cstdint>
#include <string>

namespace Gallery {

struct ResourceSnapshot {
    float privateMb = 0.0f;
    float workingSetMb = 0.0f;
    float cpuPct = 0.0f;
    float gpuPct = -1.0f; // <0 = 不可用
    uint32_t handleCount = 0;
    uint32_t threadCount = 0;
    float fps = 0.0f;
    int dpi = 96;
    int zoomPct = 100;
    bool isDComp = false;
    bool animationsEnabled = true;
};

/**
 * @brief CUI.Gallery 底部状态栏，常驻展示页面状态、渲染后端与性能监控数据。
 */
class GalleryStatusBar : public CUI::StatusBar {
public:
    GalleryStatusBar();
    virtual ~GalleryStatusBar() = default;

    virtual const char* GetClassName() const override { return "GalleryStatusBar"; }
    virtual void OnRender(CUI::GraphicsContext& ctx) override;
    virtual bool OnAnimationTick() override;
    virtual bool HasSelfAnimation() const override { return false; }

    void SetCurrentPage(const std::string& pageTitle);

private:
    void EnsureItems();
    void RefreshMetrics();
    void ScheduleNextSample();
    ResourceSnapshot SampleSnapshot();

    // 状态栏项目 ID
    int m_pageId = 0;
    int m_backendId = 0;
    int m_modeId = 0;
    int m_dpiId = 0;
    int m_handlesId = 0;
    int m_gpuId = 0;
    int m_cpuId = 0;
    int m_memId = 0;
    int m_fpsId = 0;

    bool m_itemsReady = false;
    bool m_kickstarted = false;
    std::string m_currentPage = "主页";

    // 采样缓存与历史时间戳
    std::uint64_t m_lastCpu100ns = 0;
    std::chrono::steady_clock::time_point m_lastCpuSample{};
    std::chrono::steady_clock::time_point m_lastSampleAt{};
    bool m_hasCpuSample = false;
    bool m_hasSample = false;
    ResourceSnapshot m_cachedSnapshot{};
};

} // namespace Gallery
