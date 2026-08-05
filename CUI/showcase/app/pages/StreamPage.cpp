#include "PageRegistry.h"
#include "../ShowcaseHelpers.h"
#include "../Streaming.h"
#include "framework/core/CUIDsl.h"

using namespace CUI;
using namespace CUI::DSL;

ShowcasePage BuildStreamPage(const ShowcaseContext& ctx) {
    auto status = CreateShowcaseText("状态: 切换到本页后自动开始，离开自动停止。", 12.0f, "#9CDCFE", false, "Consolas");
    auto startBtn = ElevatedButton("开始推流").Background("#007ACC").Padding(12, 6, 12, 6).Build();
    auto stopBtn = ElevatedButton("暂停推流").Background("#5A5A5A").Padding(12, 6, 12, 6).Build();
    startBtn->OnClick().Connect([window = ctx.windowRef, image = ctx.streamImage, status](UIElement*) {
        StartStreamingThread(window, image);
        status->SetText("状态: 推流中 (手动启动)。");
        });
    stopBtn->OnClick().Connect([status](UIElement*) {
        StopStreamingThread();
        status->SetText("状态: 已暂停。");
        });

    auto side = CreateRightPanel({
        CreateShowcaseText("推流控制台", 12.0f, "#569CD6", true),
        status,
        Row(8).Children({ startBtn, stopBtn }).Build(),
        CreateShowcaseText("数据源: 桌面实时截图", 11.0f, "#AAAAAA"),
        CreateShowcaseText("目标尺寸: 420 x 240", 11.0f, "#AAAAAA")
        });

    return { "1000+ FPS 动态推流", CreatePage(
        "1000+ FPS 动态图像推流控制台",
        "Direct2D ID2D1Bitmap1 硬件位图实时推流，支持在线暂停与恢复播放。",
        CreateDemoSurface({ ctx.streamImage }, 0.0f),
        side) };
}
