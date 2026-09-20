#include "OverviewView.h"
#include "framework/controls/TextBlock.h"
#include "framework/controls/Panel.h"
#include "framework/controls/ScrollViewer.h"
#include "framework/style/ThemeTokenId.h"

using namespace CUI;
using namespace CUI::DSL;

namespace AutoGuard {

OverviewView::OverviewView(
    std::shared_ptr<MainViewModel> viewModel,
    std::function<void(const std::string&)> onNavigate)
    : m_viewModel(std::move(viewModel))
    , m_onNavigate(std::move(onNavigate)) {
}

std::shared_ptr<UIElement> OverviewView::Build() {
    const auto& summary = m_viewModel->GetSummary();
    const size_t total = summary.entries.size();
    const size_t enabled = summary.enabledCount;
    const size_t disabled = summary.disabledCount;
    const size_t missing = summary.missingCount;
    const size_t risks = summary.suspiciousCount + summary.highRiskCount;

    auto onNavigate = m_onNavigate;

    auto makeStat = [](const std::string& title, const std::string& val, Color col) {
        return Column(2.0f, {
            Text(title).FontSize(12.0f).ForegroundToken(ThemeTokenId::TextSecondary),
            Text(val).FontSize(24.0f).FontWeight(FontWeight::SemiBold).Foreground(col)
        })
        .MinWidth(140.0f)
        .Padding(10.0f)
        .CornerRadius(4.0f)
        .BackgroundToken(ThemeTokenId::CardBackground)
        .Build();
    };

    auto statRow = Row(8.0f, {
        makeStat("全部项", std::to_string(total), Color::Hex("#0078D4")),
        makeStat("已启用", std::to_string(enabled), Color::Hex("#107C41")),
        makeStat("潜在风险", std::to_string(risks), risks > 0 ? Color::Hex("#E81123") : Color::Hex("#107C41")),
        makeStat("目标失效", std::to_string(missing), missing > 0 ? Color::Hex("#FF8C00") : Color::Hex("#107C41")),
        makeStat("已禁用", std::to_string(disabled), Color::Hex("#767676"))
    }).Build();

    auto header = Row(12.0f, {
        Text("🛡 安全体检总览").FontSize(20.0f).FontWeight(FontWeight::SemiBold).ForegroundToken(ThemeTokenId::TextPrimary),
        Text("共监控 19 类自启动与持久化入口").FontSize(12.0f).ForegroundToken(ThemeTokenId::TextSecondary).Padding(Thickness(0, 4, 0, 0))
    }).Build();

    auto makeEntrance = [onNavigate](const std::string& icon, const std::string& title, const std::string& tag) {
        auto btn = Row(8.0f, {
            Text(icon).FontSize(16.0f).Width(20.0f),
            Text(title).FontSize(13.0f).FontWeight(FontWeight::Medium).ForegroundToken(ThemeTokenId::TextPrimary)
        })
        .MinWidth(180.0f)
        .Padding(Thickness(10, 8, 10, 8))
        .CornerRadius(4.0f)
        .BackgroundToken(ThemeTokenId::CardBackground)
        .Build();

        auto go = [onNavigate, tag](UIElement*) {
            if (onNavigate) onNavigate(tag);
        };
        btn->OnClick().Connect(go);
        for (const auto& c : btn->GetChildren()) {
            if (c) c->OnClick().Connect(go);
        }
        return btn;
    };

    auto entranceWrap = std::make_shared<WrapPanel>(Orientation::Horizontal);
        entranceWrap->SetGap(8.0f);
        entranceWrap->SetAlign(Alignment::Stretch);

    entranceWrap->AddChild(makeEntrance("👤", "登录自启动 (Run)", "logon"));
    entranceWrap->AddChild(makeEntrance("📁", "资源管理器 & COM", "explorer"));
    entranceWrap->AddChild(makeEntrance("◷", "计划任务 (Scheduler)", "tasks"));
    entranceWrap->AddChild(makeEntrance("⚙", "系统与驱动服务", "services"));
    entranceWrap->AddChild(makeEntrance("⚠", "镜像劫持 (IFEO)", "ifeo"));
    entranceWrap->AddChild(makeEntrance("🔑", "Winlogon 挂钩", "winlogon"));
    entranceWrap->AddChild(makeEntrance("📦", "已知 DLL & AppInit", "dlls"));
    entranceWrap->AddChild(makeEntrance("🔌", "Winsock & 网络", "network"));

    auto content = Column(12.0f, {
        header,
        statRow,
        Text("快捷入口").FontSize(14.0f).FontWeight(FontWeight::SemiBold).ForegroundToken(ThemeTokenId::TextPrimary),
        entranceWrap
    })
    .Padding(12.0f)
    .BackgroundToken(ThemeTokenId::WindowBackground)
    .Build();

    auto scroll = std::make_shared<ScrollViewer>();
        scroll->SetAlign(Alignment::Stretch);
        scroll->SetFlexGrow(1.0f);
        scroll->AddChild(content);
    return scroll;
}

} // namespace AutoGuard
