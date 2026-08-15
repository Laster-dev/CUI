#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"
#include "framework/controls/Control.h"
#include "framework/controls/ListBox.h"
#include "framework/controls/TextBox.h"
#include "framework/controls/TextBlock.h"
#include "framework/controls/Button.h"
#include "framework/dnd/DragDropService.h"
#include "framework/style/ThemeManager.h"
#include <format>
#include <functional>
#include <memory>
#include <vector>

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

namespace {

/**
 * @brief 自定义可拖拽卡片芯片 (CUI::IDragSource 实现)
 */
class DragChip : public Control, public CUI::IDragSource {
public:
    explicit DragChip(std::string payload, std::string icon = "🏷️")
        : m_payload(std::move(payload)), m_icon(std::move(icon)) {
        SetText(m_payload);
        SetHeight(32.0f);
        SetCornerRadius(16.0f);
        SetToolTip("按住鼠标左键可拖拽此数据包至右侧列表、输入框或文件投放槽；按住 Ctrl 为复制。");
    }

    ~DragChip() override {
        if (auto* dnd = DragDropService::Current()) {
            dnd->AbortIfParticipant(this, nullptr);
        }
    }

    const char* GetClassName() const override { return "DragChip"; }

    Size Measure(Size availableSize) override {
        (void)availableSize;
        float w = GetWidth() > 0.0f ? GetWidth() : 180.0f;
        m_desiredSize = Size(w, 32.0f);
        return m_desiredSize;
    }

    void OnRender(GraphicsContext& ctx) override {
        const auto& tokens = ThemeManager::Instance().GetTokens();
        D2D1_COLOR_F bg = IsPressed() ? tokens.pressedBackground : (IsHovered() ? tokens.hoverBackground : tokens.cardBackground);
        ctx.FillRoundedRect(m_bounds, 16.0f, bg);
        ctx.DrawRoundedRect(m_bounds, 16.0f, tokens.cardBorder, 1.0f);

        std::string fullText = m_icon + " " + m_payload;
        ctx.DrawText(
            fullText,
            Rect(m_bounds.x + 12.0f, m_bounds.y, m_bounds.width - 24.0f, m_bounds.height),
            tokens.textPrimary,
            "微软雅黑",
            12.0f,
            DWRITE_TEXT_ALIGNMENT_LEADING,
            DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    }

    void OnMouseDown(Point pt) override {
        Control::OnMouseDown(pt);
        m_pressPt = pt;
    }

    void OnMouseMove(Point pt) override {
        Control::OnMouseMove(pt);
        if (!IsPressed()) {
            return;
        }
        const float dx = pt.x - m_pressPt.x;
        const float dy = pt.y - m_pressPt.y;
        if (dx * dx + dy * dy < 25.0f) {
            return;
        }
        if (auto* dnd = DragDropService::Current()) {
            if (!dnd->IsDragging()) {
                dnd->BeginDrag(this, pt, BeginDrag(pt), AllowedEffects());
            }
        }
    }

    DataPackage BeginDrag(Point pt) override {
        (void)pt;
        DataPackage pkg;
        pkg.SetText(m_payload);
        return pkg;
    }

    DragDropEffects AllowedEffects() const override {
        return DragDropEffects::Copy | DragDropEffects::Move;
    }

private:
    std::string m_payload;
    std::string m_icon;
    Point m_pressPt{};
};

/**
 * @brief 文件/数据投放收集槽 (CUI::IDropTarget 实现，支持应用内拖拽与 Windows 资源管理器 OLE 拖放)
 */
class DropZoneWell : public Control, public CUI::IDropTarget {
public:
    explicit DropZoneWell(std::function<void(const std::string&)> onReceived)
        : m_onReceived(std::move(onReceived)) {
        SetHeight(88.0f);
        SetCornerRadius(8.0f);
    }

    ~DropZoneWell() override {
        if (auto* dnd = DragDropService::Current()) {
            dnd->AbortIfParticipant(nullptr, this);
        }
    }

    const char* GetClassName() const override { return "DropZoneWell"; }

    Size Measure(Size availableSize) override {
        float w = GetWidth() > 0.0f ? GetWidth() : (availableSize.width > 0.0f ? availableSize.width : 360.0f);
        m_desiredSize = Size(w, 88.0f);
        return m_desiredSize;
    }

    void OnRender(GraphicsContext& ctx) override {
        const auto& tokens = ThemeManager::Instance().GetTokens();
        const float radius = 8.0f;

        if (m_hot) {
            const auto effect = DragDropService::Current()
                ? DragDropService::Current()->GetCurrentEffect()
                : DragDropEffects::Copy;
            DragDropService::PaintDropAccept(ctx, m_bounds, radius, effect, false);
        } else {
            ctx.FillRoundedRect(m_bounds, radius, tokens.cardBackground);
            ctx.DrawRoundedRect(m_bounds, radius, tokens.cardBorder, 1.0f);
        }

        std::string prompt = m_hot ? "⚡ 松开鼠标以完成放置 (Drop)" : m_statusText;
        D2D1_COLOR_F textColor = m_hot ? tokens.accentColor : tokens.textSecondary;

        ctx.DrawText(
            prompt,
            Rect(m_bounds.x + 16.0f, m_bounds.y + 12.0f, m_bounds.width - 32.0f, m_bounds.height - 24.0f),
            textColor,
            "微软雅黑",
            12.0f,
            DWRITE_TEXT_ALIGNMENT_CENTER,
            DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    }

    DragDropEffects OnDragOver(Point pt, const DataPackage& data, DragDropEffects allowed) override {
        (void)pt;
        if (!data.HasFiles() && !data.HasText()) {
            return DragDropEffects::None;
        }
        if (!m_hot) {
            m_hot = true;
            MarkRenderRectDirty(m_bounds);
        }
        if (HasEffect(allowed, DragDropEffects::Copy)) {
            return DragDropEffects::Copy;
        }
        return allowed;
    }

    void OnDragLeave() override {
        if (m_hot) {
            m_hot = false;
            MarkRenderRectDirty(m_bounds);
        }
    }

    bool OnDrop(Point pt, DataPackage& data, DragDropEffects effect) override {
        (void)pt;
        (void)effect;
        m_hot = false;

        if (data.HasFiles()) {
            const auto& files = data.GetFiles();
            m_statusText = std::format("📥 成功从资源管理器接收 {} 个文件投放：{}",
                                       files.size(),
                                       files.empty() ? "" : files.front() + (files.size() > 1 ? " 等" : ""));
            MarkRenderRectDirty(m_bounds);
            if (m_onReceived) {
                m_onReceived(m_statusText);
            }
            return true;
        }

        if (data.HasText()) {
            m_statusText = std::format("📥 成功接收文本数据包：【{}】", data.GetText());
            MarkRenderRectDirty(m_bounds);
            if (m_onReceived) {
                m_onReceived(m_statusText);
            }
            return true;
        }

        MarkRenderRectDirty(m_bounds);
        return false;
    }

    Rect DropHighlightRect() const override { return m_bounds; }

    void SetStatus(const std::string& text) {
        m_statusText = text;
        MarkRenderRectDirty(m_bounds);
    }

private:
    std::function<void(const std::string&)> m_onReceived;
    std::string m_statusText = "📥 放置目标槽位：可直接拖入上方数据包，或从 Windows 资源管理器拖入真实文件";
    bool m_hot = false;
};

} // namespace

/**
 * @brief 构建 DragDrop (拖放服务) 展示页面。
 * 遵循 Fluent 规范，全弹性流式布局，不硬编码固定宽度。
 */
Element BuildDragDropPage() {
    auto statusLabel = MakeStatus("提示：支持控件间数据拖拽（IDragSource -> IDropTarget）与 Windows 资源管理器外部文件拖放。");

    // ==========================================
    // 1. 列表间拖拽互换 (ListBox 跨列表拖拽)
    // ==========================================
    auto listA = std::make_shared<ListBox>();
    listA->SetHeight(180.0f);
    listA->SetAllowDrag(true);
    listA->SetAllowDrop(true);
    listA->SetItems({ "📄 MainWindow.cpp", "📄 AppStyles.xaml", "📄 CMakeLists.txt", "📄 README.md" });

    auto listB = std::make_shared<ListBox>();
    listB->SetHeight(180.0f);
    listB->SetAllowDrag(true);
    listB->SetAllowDrop(true);
    listB->SetItems({ "📦 CUI.Core.lib", "📦 Direct2D.dll", "📦 Assets.zip" });

    listA->OnSelectionChanged().Connect([statusLabel](ListBox*, int idx, const std::string& text) {
        if (idx >= 0) {
            statusLabel->Text = std::format("左侧列表选中：[{}] (可按住拖拽至右侧列表)", text);
        }
    });

    listB->OnSelectionChanged().Connect([statusLabel](ListBox*, int idx, const std::string& text) {
        if (idx >= 0) {
            statusLabel->Text = std::format("右侧列表选中：[{}] (可按住拖拽至左侧列表)", text);
        }
    });

    // ==========================================
    // 2. 自定义数据包芯片与放置井
    // ==========================================
    auto chip1 = std::make_shared<DragChip>("紧急任务: 修复拖放管线", "⚡");
    auto chip2 = std::make_shared<DragChip>("代码评审: DockManager.cpp", "🔍");
    auto chip3 = std::make_shared<DragChip>("主题 Token: AccentColor", "🎨");

    auto well = std::make_shared<DropZoneWell>([statusLabel](const std::string& msg) {
        statusLabel->Text = msg;
    });

    // 3. 接受拖放的文本输入框
    auto dropInput = TextField()
        .Text("可拖放文本或文件路径至此输入框...")
        .Height(36.0f);
    dropInput->SetAllowDrop(true);

    SamplePageSpec spec;
    spec.title = "Drag and Drop (拖放服务)";
    spec.subtitle = "跨控件与跨进程的高性能数据拖拽交换系统，支持 DataPackage 多格式封装、半透明 Ghost 拖影、吸附轮廓与 Windows OLE 外部文件拖放。";
    spec.sections = {
        {
            "双列表互拖 (ListBox 跨列表数据移动与复制)",
            "在左右两列表之间按住鼠标左键即可拖拽条目移动；按住 Ctrl 拖拽为复制，按 Esc 键可随时取消拖拽。",
            Column(12, {
                Row(12, {
                    Column(4, {
                        Text("📁 源码工程列表 (List A)").FontSize(12.0f).Foreground(D2D1::ColorF(0x94A3B8, 1.0f)),
                        listA,
                    }),
                    Column(4, {
                        Text("📦 输出资产列表 (List B)").FontSize(12.0f).Foreground(D2D1::ColorF(0x94A3B8, 1.0f)),
                        listB,
                    }),
                }),
            }),
        },
        {
            "数据包拖拽源 (IDragSource) 与通用接收槽 (IDropTarget)",
            "按住下方胶囊芯片可拖拽至放置井或文本输入框；亦支持从 Windows 资源管理器直接拖入真实文件。",
            Column(12, {
                Row(8, { chip1, chip2, chip3 }),
                well,
                dropInput,
                statusLabel,
            }),
        },
    };

    spec.source = R"cpp(// 1. 拖拽源控件：在 OnMouseMove 中检测拖动位移并启动
class MyDragSource : public Control, public IDragSource {
    DataPackage BeginDrag(Point pt) override {
        DataPackage pkg;
        pkg.SetText("Payload");
        return pkg;
    }
};

// 2. 放置目标控件：实现 IDropTarget 接收并响应高亮与数据落地
class MyDropTarget : public Control, public IDropTarget {
    DragDropEffects OnDragOver(Point pt, const DataPackage& data, DragDropEffects allowed) override {
        return DragDropEffects::Copy;
    }
    bool OnDrop(Point pt, DataPackage& data, DragDropEffects effect) override {
        if (data.HasFiles()) {
            auto files = data.GetFiles(); // 接收文件
        }
        return true;
    }
};
)cpp";

    return BuildSamplePage(spec);
}

} // namespace Gallery
