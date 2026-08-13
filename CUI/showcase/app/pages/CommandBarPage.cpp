#include "PageRegistry.h"
#include "../ShowcaseHelpers.h"
#include "framework/core/CUIDsl.h"
#include "framework/controls/CommandBar.h"
#include "framework/controls/Button.h"
#include "framework/controls/TextBlock.h"
#include "framework/controls/Toast.h"
#include "framework/input/Command.h"
#include <functional>

using namespace CUI;
using namespace CUI::DSL;

namespace {

constexpr const char* kSvgNew =
    "<svg viewBox=\"0 0 16 16\" xmlns=\"http://www.w3.org/2000/svg\">"
    "<path d=\"M3 1.5h6.5L13 5v9.5H3zM9.5 1.5V5H13\"/>"
    "</svg>";
constexpr const char* kSvgOpen =
    "<svg viewBox=\"0 0 16 16\" xmlns=\"http://www.w3.org/2000/svg\">"
    "<path d=\"M1.5 3h4l1.5 1.5H14.5v9H1.5z\"/>"
    "</svg>";
constexpr const char* kSvgSave =
    "<svg viewBox=\"0 0 16 16\" xmlns=\"http://www.w3.org/2000/svg\">"
    "<path d=\"M2 2h9l3 3v9H2zM5 2h5v3H5zM4 9h8v5H4z\"/>"
    "</svg>";
constexpr const char* kSvgCut =
    "<svg viewBox=\"0 0 16 16\" xmlns=\"http://www.w3.org/2000/svg\">"
    "<path d=\"M2 11h4v4H2zM10 11h4v4h-4zM3 2l5 7 5-7\"/>"
    "</svg>";
constexpr const char* kSvgCopy =
    "<svg viewBox=\"0 0 16 16\" xmlns=\"http://www.w3.org/2000/svg\">"
    "<path d=\"M5 2h8v10H5zM3 4h2v9h7v2H3z\"/>"
    "</svg>";
constexpr const char* kSvgPaste =
    "<svg viewBox=\"0 0 16 16\" xmlns=\"http://www.w3.org/2000/svg\">"
    "<path d=\"M5 2h2V1h2v1h2v2H5zM3 4h10v11H3z\"/>"
    "</svg>";
constexpr const char* kSvgUndo =
    "<svg viewBox=\"0 0 16 16\" xmlns=\"http://www.w3.org/2000/svg\">"
    "<path d=\"M7 3L3 7l4 4V8c3 0 5 1 6 4-0.5-4-3-7-6-7z\"/>"
    "</svg>";
constexpr const char* kSvgRedo =
    "<svg viewBox=\"0 0 16 16\" xmlns=\"http://www.w3.org/2000/svg\">"
    "<path d=\"M9 3l4 4-4 4V8C6 8 4 9 3 12c0.5-4 3-7 6-7z\"/>"
    "</svg>";
constexpr const char* kSvgBold =
    "<svg viewBox=\"0 0 16 16\" xmlns=\"http://www.w3.org/2000/svg\">"
    "<path d=\"M4 2h5.2C11.4 2 13 3.4 13 5.2 13 6.4 12.3 7.4 11.2 8 12.6 8.5 13.6 9.7 13.6 11.2 13.6 13.2 11.8 14.8 9.4 14.8H4zM6.2 7.2h2.6c0.9 0 1.6-0.6 1.6-1.4S9.7 4.4 8.8 4.4H6.2zm0 5.2h3c1 0 1.8-0.6 1.8-1.5s-0.8-1.5-1.8-1.5h-3z\"/>"
    "</svg>";
constexpr const char* kSvgItalic =
    "<svg viewBox=\"0 0 16 16\" xmlns=\"http://www.w3.org/2000/svg\">"
    "<path d=\"M6 2h7v2H9.8l-2.6 8H10v2H3v-2h3.2L8.8 4H6z\"/>"
    "</svg>";

void FillBar(CommandBar& bar, const std::function<void(const char*)>& log) {
    auto cmd = [log](const char* name) {
        return std::make_shared<Command>([log, name]() { log(name); });
    };

    bar.AddButton("新建", kSvgNew, cmd("新建"));
    bar.AddButton("打开", kSvgOpen, cmd("打开"));
    bar.AddButton("保存", kSvgSave, cmd("保存"));
    bar.AddSeparator();
    bar.AddButton("剪切", kSvgCut, cmd("剪切"));
    bar.AddButton("复制", kSvgCopy, cmd("复制"));
    bar.AddButton("粘贴", kSvgPaste, cmd("粘贴"));
    bar.AddSeparator();
    bar.AddButton("撤销", kSvgUndo, cmd("撤销"));
    bar.AddButton("重做", kSvgRedo, cmd("重做"));
    bar.AddSeparator();
    bar.AddToggle("加粗", kSvgBold, cmd("加粗"));
    bar.AddToggle("斜体", kSvgItalic, cmd("斜体"));
    bar.AddSecondary("选项", {}, cmd("选项"));
    bar.AddSecondarySeparator();
    bar.AddSecondary("关于 CommandBar", {}, cmd("关于"));
}

} // namespace

ShowcasePage BuildCommandBarPage(const ShowcaseContext& ctx) {
    auto log = std::static_pointer_cast<TextBlock>(
        CreateShowcaseText("命令日志：就绪", 12.0f, "#B5CEA8", false, "Consolas"));
    auto overflowHint = std::static_pointer_cast<TextBlock>(
        CreateShowcaseText("窄栏溢出：—", 12.0f, "textSecondary", false));

    auto logFn = [window = ctx.windowRef, log](const char* name) {
        log->SetText(std::string("[CommandBar] ") + name);
        if (window) {
            Toast::Show(window->GetRootElement().get(), "CommandBar", name,
                        ToastType::Info, ToastCorner::BottomRight, 1200);
        }
    };

    auto full = std::make_shared<CommandBar>();
    full->SetLabelPosition(CommandBarLabelPosition::Right);
    FillBar(*full, logFn);

    auto compact = std::make_shared<CommandBar>();
    compact->SetLabelPosition(CommandBarLabelPosition::Collapsed);
    FillBar(*compact, logFn);

    auto narrow = std::make_shared<CommandBar>();
    narrow->SetWidth(280.0f);
    narrow->SetAlign(Alignment::Start);
    narrow->SetLabelPosition(CommandBarLabelPosition::Right);
    FillBar(*narrow, logFn);
    narrow->OnOverflowOpened().Connect([overflowHint, narrow]() {
        overflowHint->SetText("窄栏溢出：已打开，共 "
            + std::to_string(narrow->GetOverflowCount()) + " 项进菜单");
    });

    auto btnLabels = std::make_shared<Button>("切换文字");
    btnLabels->OnClick().Connect([full, log](UIElement*) {
        const bool show = full->GetLabelPosition() != CommandBarLabelPosition::Right;
        full->SetLabelPosition(show ? CommandBarLabelPosition::Right : CommandBarLabelPosition::Collapsed);
        log->SetText(show ? "[CommandBar] 显示文字" : "[CommandBar] 仅图标");
    });

    auto demo = Column(12).Children({
        CreateDemoSurface({
            CreateShowcaseText("主命令条（图标 + 文字）", 13.0f, "textPrimary", true),
            CreateShowcaseText("挤不下的主命令和 Secondary 进「更多」菜单。加粗 / 斜体是 ToggleButton。", 12.0f, "textSecondary", false),
            full,
            Row(8).Children({ btnLabels }).Build(),
            log,
        }, 10.0f),
        CreateDemoSurface({
            CreateShowcaseText("仅图标", 13.0f, "textPrimary", true),
            compact,
        }, 10.0f),
        CreateDemoSurface({
            CreateShowcaseText("窄宽度强制溢出（280px）", 13.0f, "textPrimary", true),
            overflowHint,
            narrow,
        }, 10.0f),
    }).Build();

    return { "CommandBar 命令条", CreatePage(
        "CommandBar 命令条",
        "主命令 + 分隔 + 溢出菜单。按钮复用 Button / ToggleButton / Command，栏本身自绘。",
        demo) };
}
