#include "PageRegistry.h"
#include "../ShowcaseHelpers.h"
#include "framework/controls/TerminalControl.h"

ShowcasePage BuildTerminalPage(const ShowcaseContext& ctx) {
    auto target = std::make_shared<CUI::TerminalControl>("cmd.exe");
    target->SetWidth(680.0f);
    target->SetHeight(420.0f);

    return { "Terminal 终端", CreatePage(
        "ConPty 高性能 ANSI/VT100 终端模拟器",
        "完整 xterm 状态机：SGR/TrueColor、备用屏幕、滚动区域、回流、OSC 8 超链接、"
        "鼠标上报、选区圆角高亮、Ctrl+滚轮缩放、Ctrl+F 查找。",
        CreateDemoSurface({ target }, 0.0f),
        CreatePropertyGrid(ctx, target), target) };
}
