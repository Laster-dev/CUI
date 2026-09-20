#include "DetailStripView.h"
#include "IconHelper.h"
#include "framework/style/ThemeTokenId.h"
#include "framework/render/GraphicsContext.h"

using namespace CUI;
using namespace CUI::DSL;

namespace AutoGuard {

class HIconElement : public UIElement {
public:
    void ApplyIcon(HICON hIcon) {
        if (m_hIcon != hIcon) {
            m_hIcon = hIcon;
            MarkRenderContentDirty();
        }
    }

    HICON GetIcon() const { return m_hIcon; }

protected:
    void OnRender(GraphicsContext& ctx) override {
        if (m_hIcon) {
            const float iconSize = 32.0f;
            float x = m_bounds.x + (std::max)(0.0f, (m_bounds.width - iconSize) * 0.5f);
            float y = m_bounds.y + (std::max)(0.0f, (m_bounds.height - iconSize) * 0.5f);
            ctx.DrawHIcon(m_hIcon, Rect(x, y, iconSize, iconSize));
        }
    }

private:
    HICON m_hIcon = nullptr;
};

DetailStripView::DetailStripView(std::shared_ptr<MainViewModel> viewModel)
    : m_viewModel(std::move(viewModel)) {
}

DetailStripView::~DetailStripView() = default;

std::shared_ptr<UIElement> DetailStripView::Build() {
    // 1. 32x32 Icon Container
    m_iconElement = std::make_shared<HIconElement>();
        m_iconElement.Width(36.0f);
    m_iconElement.Height(36.0f);
    m_iconElement.Margin(Thickness(2, 4, 12, 4));

    // 2. Left Main Section (Title, Description, Command/Path, Status Reason)
    m_titleName = Text("").FontSize(13.0f).FontWeight(FontWeight::SemiBold).ForegroundToken(ThemeTokenId::TextPrimary).Build();
    m_statusBadge = Text("").FontSize(11.0f).ForegroundToken(ThemeTokenId::TextSecondary).Build();
    m_locationBadge = Text("").FontSize(11.0f).ForegroundToken(ThemeTokenId::TextMuted).Build();
    auto line1 = Row(8.0f, { m_titleName, m_statusBadge, m_locationBadge }).Build();

    m_descriptionText = Text("").FontSize(11.0f).ForegroundToken(ThemeTokenId::TextPrimary).Build();
    m_commandText = Text("").FontSize(10.5f).ForegroundToken(ThemeTokenId::TextSecondary).Build();
    m_reasonText = Text("").FontSize(10.5f).Foreground(Color::Hex("#107C41")).Build();

    auto leftStack = Column(3.0f, {
        line1,
        m_descriptionText,
        m_commandText,
        m_reasonText
    })
    .FlexGrow(1.0f)
    .Build();

    // 3. Right Attribute Section (Publisher, Version/Size, Time, Source)
    m_publisherText = Text("").FontSize(11.0f).Foreground(Color::Hex("#107C41")).Build();
    m_versionSizeText = Text("").FontSize(10.5f).ForegroundToken(ThemeTokenId::TextMuted).Build();
    m_timeText = Text("").FontSize(10.5f).ForegroundToken(ThemeTokenId::TextMuted).Build();
    m_sourceText = Text("").FontSize(10.5f).ForegroundToken(ThemeTokenId::TextMuted).Build();

    auto rightStack = Column(3.0f, {
        m_publisherText,
        m_versionSizeText,
        m_timeText,
        m_sourceText
    })
    .Width(380.0f)
    .Padding(Thickness(10, 0, 8, 0))
    .Build();

    // Combined Modern Card Layout
    m_root = Row(10.0f, {
        m_iconElement,
        leftStack,
        rightStack
    })
    .Height(0.0f)
    .Visibility(Visibility::Collapsed)
    .Padding(Thickness(14, 6, 14, 6))
    .BackgroundToken(ThemeTokenId::CardBackground)
    .Align(Alignment::Stretch)
    .Build();

    return m_root;
}

void DetailStripView::Update() {
    if (!m_root) return;

    const auto* entry = m_viewModel->GetSelectedEntry();
    if (!entry) {
                m_root->ApplyHeight(0.0f);
                m_root->ApplyVisibility(Visibility::Collapsed);
        return;
    }

        m_root->ApplyHeight(90.0f);
        m_root->ApplyVisibility(Visibility::Visible);

    // 1. Update 32x32 Native Icon
    std::string iconPath = entry->executablePath.empty() ? entry->command : entry->executablePath;
    HICON hLargeIcon = nullptr;
    if (!iconPath.empty()) {
        hLargeIcon = IconHelper::Instance().GetFileIcon(iconPath, false);
    }
    if (!hLargeIcon) {
        hLargeIcon = IconHelper::Instance().GetDefaultExeIcon(false);
    }
    m_iconElement->ApplyIcon(hLargeIcon);

    // 2. Title & Badges
        m_titleName.Text(entry->name.empty() ? "(未命名条目)" : entry->name);

    std::string statusStr;
    Color statusColor = Color::Hex("#D8A000");
    if (entry->status == StartupStatus::Enabled) {
        statusStr = "[已启用]";
        statusColor = Color::Hex("#107C41");
    } else if (entry->status == StartupStatus::Disabled) {
        statusStr = "[已禁用]";
        statusColor = Color::Hex("#D8A000");
    } else if (entry->status == StartupStatus::Missing) {
        statusStr = "[目标失效]";
        statusColor = Color::Hex("#E81123");
    } else {
        statusStr = "[系统保护]";
        statusColor = Color::Hex("#107C41");
    }
        m_statusBadge.Text(statusStr);
    m_statusBadge.Color(statusColor);

    std::string locBadge = LocationName(entry->location);
    if (!entry->scope.empty()) {
        locBadge = "[" + entry->scope + " | " + locBadge + "]";
    }
    if (!entry->triggerInfo.empty()) {
        locBadge += " [触发: " + entry->triggerInfo + "]";
    }
        m_locationBadge.Text(locBadge);

    // 3. Description & Command
    std::string desc = entry->description.empty() ? "(无文件描述)" : entry->description;
        m_descriptionText.Text("描述: " + desc);

    std::string cmd = entry->command.empty() ? entry->executablePath : entry->command;
    if (!entry->fileExists && !entry->command.empty()) {
        cmd = "[File not found] " + cmd;
                m_commandText.Text("命令: " + cmd);
        m_commandText.Color(Color::Hex("#E81123"));
    } else if (cmd.empty()) {
                m_commandText.Text("命令: (系统内部指令/组件触发)");
        m_commandText.ColorToken(ThemeTokenId::TextMuted);
    } else {
                m_commandText.Text("命令: " + cmd);
        m_commandText.ColorToken(ThemeTokenId::TextSecondary);
    }

    // 4. Status Reason Diagnostic
    if (!entry->fileExists && !entry->command.empty()) {
                m_reasonText.Text("⚠ 诊断原因: 注册表或配置中存在自启动指向，但目标可执行文件/DLL在磁盘上已不存在（卸载残留或路径无效）。");
        m_reasonText.Color(Color::Hex("#E81123"));
    } else if (entry->status == StartupStatus::Disabled) {
                m_reasonText.Text("ℹ 状态原因: 该自启动条目已由系统注册表 StartupApproved 机制或服务/任务策略设为禁用。");
        m_reasonText.Color(Color::Hex("#D8A000"));
    } else {
                m_reasonText.Text("✔ 状态原因: 启动项配置有效，开机/登录时由 Windows 正常加载执行。");
        m_reasonText.Color(Color::Hex("#107C41"));
    }

    // 5. Publisher / Signature
    std::string pub = entry->publisher.empty() ? "(未验证签名)" : entry->publisher;
    Color pubColor = Color::Hex("#D8A000");
    if (pub.find("(Verified)") != std::string::npos || pub.find("Microsoft") != std::string::npos || pub.find("Google") != std::string::npos) {
        pubColor = Color::Hex("#107C41");
    } else if (pub.find("Revoked") != std::string::npos || pub.find("无效") != std::string::npos) {
        pubColor = Color::Hex("#E81123");
    }
        m_publisherText.Text("签名/出版商: " + pub);
    m_publisherText.Color(pubColor);

    // 6. Version & Size
    std::string ver = entry->fileVersion.empty() ? "--" : entry->fileVersion;
    std::string sz = entry->fileSizeStr.empty() ? "--" : entry->fileSizeStr;
        m_versionSizeText.Text("版本: " + ver + "  |  大小: " + sz);

    // 7. Time & Source
    std::string tm = entry->fileTimestamp.empty() ? "--" : entry->fileTimestamp;
        m_timeText.Text("时间: " + tm);

    std::string src = entry->source.empty() ? "--" : entry->source;
    if (src.size() > 50) {
        src = "..." + src.substr(src.size() - 47);
    }
        m_sourceText.Text("来源: " + src);
}

} // namespace AutoGuard
