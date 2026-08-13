#pragma once
#include "Control.h"
#include <cstdint>
#include <string>
#include <vector>

namespace CUI {

enum class StatusBarItemAlignment : uint8_t {
    Left = 0,
    Fill = 1,
    Right = 2
};

enum class StatusBarItemKind : uint8_t {
    Text = 0,
    Progress = 1,
    Separator = 2
};

// Lightweight draw unit — owned and painted by StatusBar (not a UIElement child).
struct StatusBarItem {
    int id = 0;
    StatusBarItemKind kind = StatusBarItemKind::Text;
    StatusBarItemAlignment align = StatusBarItemAlignment::Left;
    std::string text;
    std::string icon;      // optional short glyph / emoji
    float progress = -1.0f; // 0..1 when kind == Progress; <0 hides bar fill
    float fixedWidth = -1.0f; // <0 = auto
    bool visible = true;
};

class StatusBar : public Control {
public:
    StatusBar();
    virtual ~StatusBar() = default;

    virtual const char* GetClassName() const override { return "StatusBar"; }
    virtual std::vector<PropertyMeta> GetPropertyMetas() const override;

    virtual Size Measure(Size availableSize) override;
    virtual void OnRender(GraphicsContext& ctx) override;

    int AddTextItem(
        const std::string& text,
        StatusBarItemAlignment align = StatusBarItemAlignment::Left,
        float fixedWidth = -1.0f);
    int AddProgressItem(
        const std::string& text,
        StatusBarItemAlignment align = StatusBarItemAlignment::Right,
        float fixedWidth = 120.0f);
    int AddSeparator(StatusBarItemAlignment align = StatusBarItemAlignment::Right);

    void SetItemText(int id, const std::string& text);
    void SetItemIcon(int id, const std::string& icon);
    void SetItemProgress(int id, float progress01);
    void SetItemVisible(int id, bool visible);
    void SetItemFixedWidth(int id, float width);
    void ClearItems();

    const std::vector<StatusBarItem>& GetItems() const { return m_items; }
    StatusBarItem* FindItem(int id);
    const StatusBarItem* FindItem(int id) const;

private:
    struct LaidOutItem {
        int id = 0;
        Rect bounds;
    };

    StatusBarItem* MutableFind(int id);
    float MeasureItemWidth(GraphicsContext& ctx, const StatusBarItem& item) const;
    void LayoutItems(GraphicsContext& ctx, std::vector<LaidOutItem>& out) const;
    void DrawItem(GraphicsContext& ctx, const StatusBarItem& item, const Rect& bounds) const;

    std::vector<StatusBarItem> m_items;
    int m_nextId = 1;
};

} // namespace CUI
