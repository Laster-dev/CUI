#include "PageRegistry.h"
#include "../ShowcaseHelpers.h"
#include "framework/core/CUIDsl.h"
#include <functional>
#include <vector>
#include "framework/controls/Control.h"
#include "framework/controls/ListBox.h"
#include "framework/controls/TextBox.h"
#include "framework/controls/TextBlock.h"
#include "framework/dnd/DragDropService.h"
#include "framework/style/ThemeManager.h"

using namespace CUI;
using namespace CUI::DSL;

namespace {

class DragChip : public Control, public CUI::IDragSource {
public:
    explicit DragChip(std::string payload)
        : m_payload(std::move(payload)) {
        SetText(m_payload);
        SetWidth(148.0f);
        SetHeight(34.0f);
        SetCornerRadius(17.0f);
        SetToolTip("按住拖到列表或输入框；Ctrl 为复制");
    }

    ~DragChip() override {
        if (auto* dnd = DragDropService::Current()) {
            dnd->AbortIfParticipant(this, nullptr);
        }
    }

    const char* GetClassName() const override { return "DragChip"; }

    Size Measure(Size availableSize) override {
        (void)availableSize;
        m_desiredSize = Size(GetWidth() > 0 ? GetWidth() : 148.0f, GetHeight() > 0 ? GetHeight() : 34.0f);
        return m_desiredSize;
    }

    void OnRender(GraphicsContext& ctx) override {
        auto& theme = ThemeManager::Instance();
        const auto accent = theme.GetColor(ThemeTokenId::AccentColor);
        const auto onAccent = theme.GetColor(ThemeTokenId::AccentForeground);
        const float radius = GetCornerRadius() > 0.0f ? GetCornerRadius() : 17.0f;
        ctx.FillRoundedRect(m_bounds, radius, accent);
        ctx.DrawText(
            GetText(),
            m_bounds,
            onAccent,
            "微软雅黑",
            12.0f,
            DWRITE_TEXT_ALIGNMENT_CENTER);
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
        if (dx * dx + dy * dy < 36.0f) {
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
    Point m_pressPt{};
};

class FileDropWell : public Control, public CUI::IDropTarget {
public:
    explicit FileDropWell(std::function<void(const std::vector<std::string>&)> onFiles)
        : m_onFiles(std::move(onFiles)) {
        SetWidth(-1.0f);
        SetHeight(88.0f);
        SetCornerRadius(8.0f);
        SetToolTip("从资源管理器拖入文件");
    }

    ~FileDropWell() override {
        if (auto* dnd = DragDropService::Current()) {
            dnd->AbortIfParticipant(nullptr, this);
        }
    }

    const char* GetClassName() const override { return "FileDropWell"; }

    Size Measure(Size availableSize) override {
        float w = GetWidth();
        if (w < 0.0f) {
            w = availableSize.width > 0.0f ? availableSize.width : 360.0f;
        }
        m_desiredSize = Size(w, GetHeight() > 0.0f ? GetHeight() : 88.0f);
        return m_desiredSize;
    }

    void OnRender(GraphicsContext& ctx) override {
        auto& theme = ThemeManager::Instance();
        const auto card = theme.GetColor(ThemeTokenId::CardBackground);
        const auto border = theme.GetColor(ThemeTokenId::CardBorder);
        const auto text = theme.GetColor(m_hot ? ThemeTokenId::AccentColor : ThemeTokenId::TextSecondary);
        const float radius = GetCornerRadius() > 0.0f ? GetCornerRadius() : 8.0f;
        if (m_hot) {
            const auto effect = DragDropService::Current()
                ? DragDropService::Current()->GetCurrentEffect()
                : DragDropEffects::Copy;
            DragDropService::PaintDropAccept(ctx, m_bounds, radius, effect, false);
        } else {
            ctx.FillRoundedRect(m_bounds, radius, card);
            ctx.DrawRoundedRect(m_bounds, radius, border, 1.0f);
        }
        ctx.DrawText(
            m_hot ? "松开以放入文件" : m_status,
            Rect(m_bounds.x + 12.0f, m_bounds.y + 16.0f, m_bounds.width - 24.0f, m_bounds.height - 32.0f),
            text,
            "微软雅黑",
            13.0f,
            DWRITE_TEXT_ALIGNMENT_CENTER);
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
            m_status = files.empty()
                ? "没有文件"
                : ("已接收 " + std::to_string(files.size()) + " 个文件");
            MarkRenderRectDirty(m_bounds);
            if (m_onFiles) {
                m_onFiles(files);
            }
            return true;
        }
        if (data.HasText()) {
            m_status = data.GetText();
            MarkRenderRectDirty(m_bounds);
            return true;
        }
        MarkRenderRectDirty(m_bounds);
        return false;
    }

    Rect DropHighlightRect() const override { return m_bounds; }

private:
    std::function<void(const std::vector<std::string>&)> m_onFiles;
    std::string m_status = "把文件从资源管理器拖到这里";
    bool m_hot = false;
};

std::shared_ptr<ListBox> MakeDemoList(const std::vector<std::string>& items, float width) {
    auto list = std::make_shared<ListBox>();
    list->SetWidth(width);
    list->SetHeight(220.0f);
    list->SetAllowDrag(true);
    list->SetAllowDrop(true);
    list->SetItems(items);
    return list;
}

} // namespace

ShowcasePage BuildDragDropPage(const ShowcaseContext& ctx) {
    (void)ctx;

    auto left = MakeDemoList({ "苹果", "香蕉", "橙子", "葡萄", "西瓜" }, 220.0f);
    auto right = MakeDemoList({ "文档.docx", "截图.png", "笔记.md", "备份.zip" }, 220.0f);

    auto log = std::static_pointer_cast<TextBlock>(
        CreateShowcaseText("日志：就绪。两列表互拖；Ctrl=复制，Esc=取消。", 12.0f, "#B5CEA8", false, "Consolas"));

    auto appendLog = [log](const std::string& line) {
        log->SetText(line);
    };

    left->OnSelectionChanged().Connect([appendLog](ListBox*, int idx, const std::string& text) {
        if (idx >= 0) {
            appendLog("[左] 选中 " + std::to_string(idx) + " " + text);
        }
    });
    right->OnSelectionChanged().Connect([appendLog](ListBox*, int idx, const std::string& text) {
        if (idx >= 0) {
            appendLog("[右] 选中 " + std::to_string(idx) + " " + text);
        }
    });

    auto chipA = std::make_shared<DragChip>("任务：评审 PR");
    auto chipB = std::make_shared<DragChip>("标签：紧急");

    auto inbox = std::make_shared<TextBox>("拖放到此输入框（文本或文件路径）");
    inbox->SetAllowDrop(true);
    inbox->SetAcceptsReturn(true);
    inbox->SetTextWrapping(true);
    inbox->SetWidth(-1.0f);
    inbox->SetHeight(72.0f);
    inbox->OnTextChanged().Connect([appendLog](TextBox*, const std::string& text) {
        appendLog("[输入框] " + text);
    });

    auto well = std::make_shared<FileDropWell>([appendLog](const std::vector<std::string>& files) {
        std::string msg = "[文件] ";
        for (size_t i = 0; i < files.size(); ++i) {
            if (i) {
                msg += " | ";
            }
            msg += files[i];
        }
        appendLog(msg);
    });

    auto demo = Column(12).Children({
        CreateDemoSurface({
            CreateShowcaseText("1. 两列表互拖（ListBox 接入 IDragSource / IDropTarget）", 12.0f, "textSecondary", false),
            CreateShowcaseText("默认移动；按住 Ctrl 复制。拖到插入线位置。Esc 取消。", 12.0f, "textMuted", false),
            Row(12).Children({
                Column(6).Children({
                    CreateShowcaseText("水果", 12.0f, "textMuted", false),
                    left,
                }).Build(),
                Column(6).Children({
                    CreateShowcaseText("文件名", 12.0f, "textMuted", false),
                    right,
                }).Build(),
            }).Build(),
        }, 10.0f),
        CreateDemoSurface({
            CreateShowcaseText("2. 其它控件接入", 12.0f, "textSecondary", false),
            CreateShowcaseText("左侧徽章是自绘拖源；输入框与文件井是放置目标。", 12.0f, "textMuted", false),
            Row(10).Children({ chipA, chipB }).Build(),
            inbox,
            well,
            log,
        }, 10.0f),
    }).Build();

    return { "DragDrop", CreatePage(
        "拖放 API",
        "框架 DataPackage + IDragSource / IDropTarget + DragDropService。列表互拖、控件接入、外部文件拖入。",
        demo) };
}
