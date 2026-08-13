#include "PageRegistry.h"
#include "../ShowcaseHelpers.h"
#include "framework/core/CUIDsl.h"
#include "framework/controls/LogView.h"
#include "framework/controls/Button.h"
#include "framework/controls/TextBlock.h"
#include "framework/controls/CheckBox.h"

using namespace CUI;
using namespace CUI::DSL;

namespace {
void SeedSample(LogView& log) {
    log.Append(LogLevel::Info, "app", "LogView 就绪 · 折叠只保留最新一条");
    log.Append(LogLevel::Debug, "ui", "虚拟化行高 22px，只绘制可视区");
    log.Append(LogLevel::Trace, "gpu", "Present 局部脏区，追加不触发布局");
    log.Append(LogLevel::Warn, "net", "重试 3 次后仍未连上 10.0.0.8:8080");
    log.Append(LogLevel::Error, "net", "connection refused");
    log.Append(LogLevel::Info, "fs", "配置已加载 config.json");
    log.Append(LogLevel::Fatal, "app", "未处理异常已记录，进程保持 UI 可操作");
}

void Burst(LogView& log, int count) {
    static const LogLevel kLv[] = {
        LogLevel::Trace, LogLevel::Debug, LogLevel::Info,
        LogLevel::Warn, LogLevel::Error, LogLevel::Info
    };
    static const char* kCat[] = { "net", "ui", "fs", "gpu", "app" };
    for (int i = 0; i < count; ++i) {
        const LogLevel lv = kLv[i % 6];
        const char* cat = kCat[i % 5];
        log.Append(lv, cat, "burst #" + std::to_string(i + 1) + " 模拟高频写入");
    }
}
} // namespace

ShowcasePage BuildLogViewPage(const ShowcaseContext& ctx) {
    auto log = std::make_shared<LogView>();
    log->SetWidth(-1.0f);
    log->SetHeight(280.0f);
    log->SetMaxEntries(16384);
    log->SetExpanded(true);
    log->SetToolTip("折叠后单行最新；展开后级别芯片 + 搜索 + 虚拟化列表。Ctrl+C 复制，Ctrl+A 全选，Ctrl+F 搜索。");
    SeedSample(*log);

    auto hint = std::static_pointer_cast<TextBlock>(
        CreateShowcaseText("展开 · 跟随尾部 · 未持久化", 12.0f, "textSecondary", false));
    auto refreshHint = [hint, log]() {
        std::string s = log->IsExpanded() ? "展开" : "折叠";
        s += log->GetFollowTail() ? " · 跟随尾部" : " · 已停跟随";
        s += log->GetPersistEnabled() ? " · 已落盘 " + log->GetPersistPath() : " · 未持久化";
        s += " · " + std::to_string(log->GetCount()) + " 条";
        hint->SetText(s);
    };
    log->OnExpandedChanged().Connect([refreshHint](LogView*) { refreshHint(); });

    auto btnToggle = std::make_shared<Button>(log->IsExpanded() ? "折叠" : "展开");
    btnToggle->OnClick().Connect([log, btnToggle, refreshHint](UIElement*) {
        log->SetExpanded(!log->IsExpanded());
        btnToggle->SetText(log->IsExpanded() ? "折叠" : "展开");
        refreshHint();
    });
    auto btnOne = std::make_shared<Button>("追加 1");
    btnOne->OnClick().Connect([log, refreshHint](UIElement*) {
        log->Append(LogLevel::Info, "ui", "手动追加一条");
        refreshHint();
    });
    auto btn100 = std::make_shared<Button>("追加 100");
    btn100->OnClick().Connect([log, refreshHint](UIElement*) {
        Burst(*log, 100);
        refreshHint();
    });
    auto btn1k = std::make_shared<Button>("追加 1000");
    btn1k->OnClick().Connect([log, refreshHint](UIElement*) {
        Burst(*log, 1000);
        refreshHint();
    });
    auto btnErr = std::make_shared<Button>("打一条 Error");
    btnErr->OnClick().Connect([log, refreshHint](UIElement*) {
        log->Append(LogLevel::Error, "diag", "复现：空指针检查失败 at FrameScheduler::Tick");
        refreshHint();
    });
    auto chkPersist = std::make_shared<CheckBox>("持久化到临时文件");
    chkPersist->OnCheckStateChanged().Connect([log, refreshHint](CheckBox*, CheckState st) {
        log->SetPersistEnabled(st == CheckState::Checked);
        refreshHint();
    });

    auto demo = Column(12).Children({
        CreateDemoSurface({
            CreateShowcaseText("1. 折叠 = 一条最新；展开 = 级别芯片 / 搜索 / 虚拟列表", 12.0f, "textSecondary", false),
            hint,
            log,
            Row(8).Children({ btnToggle, btnOne, btn100, btn1k, btnErr, chkPersist }).Build(),
        }, 10.0f),
        CreateDemoSurface({
            CreateShowcaseText(
                "复用：搜索 TextBox、复制/清空/跟随图标 Button（SVG）。其余（折叠头、级别芯片、虚拟化列表）为 LogView 自绘。\n"
                "点标题折叠 · 芯片过滤级别 · 拖选 / Ctrl+A / Ctrl+C / 双击复制。",
                12.0f, "textSecondary", false),
        }, 10.0f),
    }).Build();

    return { "LogView 日志框", CreatePage(
        "LogView 日志框",
        "搜索用 TextBox，操作为 SVG 图标按钮；折叠头、级别、日志列表为自绘虚拟化（环形缓冲，追加 O(1)）。",
        demo,
        CreatePropertyGrid(ctx, log), log) };
}
