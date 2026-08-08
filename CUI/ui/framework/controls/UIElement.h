#pragma once
#include "../core/Object.h"
#include "../core/PropertyDesc.h"
#include "../core/PropertyId.h"
#include "../render/GraphicsContext.h"
#include "../render/RenderNode.h"
#include "../style/ThemeTokenId.h"
#include "../layout/Layout.h"
#include "../input/RoutedEvent.h"
#include <vector>
#include <memory>
#include <string>
#include <unordered_map>
#include <chrono>
#include <limits>

namespace CUI {

class ContextMenu;

struct PropertyMeta {
    PropertyId id = PropertyId::None;
    std::string displayName;
    std::string category;
    std::string type;
    std::vector<std::string> options;

    PropertyMeta() = default;
    PropertyMeta(const char* name, std::string display, std::string cat, std::string propertyType,
                 std::vector<std::string> enumOptions = {})
        : id(PropertyIdFromName(name))
        , displayName(std::move(display))
        , category(std::move(cat))
        , type(std::move(propertyType))
        , options(std::move(enumOptions)) {}
};

class UIElement : public Object {
public:
    static constexpr float kAttachedUnset = -999999.0f;

    UIElement();
    virtual ~UIElement();
    virtual const char* GetClassName() const override { return "UIElement"; }

    virtual std::vector<PropertyMeta> GetPropertyMetas() const;
    virtual PropertyDescSpan GetPropertyDescs() const;

    float GetWidth() const { return m_width; }
    void SetWidth(float v);
    float GetHeight() const { return m_height; }
    void SetHeight(float v);
    float GetMinWidth() const { return m_minWidth; }
    void SetMinWidth(float v);
    float GetMinHeight() const { return m_minHeight; }
    void SetMinHeight(float v);
    Thickness GetMargin() const { return m_margin; }
    void SetMargin(const Thickness& margin);
    Thickness GetPadding() const { return m_padding; }
    void SetPadding(const Thickness& padding);

    Visibility GetVisibility() const { return m_visibility; }
    void SetVisibility(Visibility v);
    bool IsEnabled() const { return m_isEnabled; }
    void SetIsEnabled(bool enabled);

    float GetOpacity() const { return m_opacity; }
    void SetOpacity(float v);
    float GetCornerRadius() const { return m_cornerRadius; }
    void SetCornerRadius(float v);
    float GetBorderThickness() const { return m_borderThickness; }
    void SetBorderThickness(float v);
    float GetFlexGrow() const { return m_flexGrow; }
    void SetFlexGrow(float v);

    Alignment GetAlign() const { return m_align; }
    void SetAlign(Alignment a);
    Alignment GetAlignHorizontal() const { return m_alignHorizontal; }
    void SetAlignHorizontal(Alignment a);
    Alignment GetAlignVertical() const { return m_alignVertical; }
    void SetAlignVertical(Alignment a);

    Orientation GetOrientation() const { return m_orientation; }
    void SetOrientation(Orientation o);
    float GetGap() const { return m_gap; }
    void SetGap(float v);
    float GetItemWidth() const { return m_itemWidth; }
    void SetItemWidth(float v);
    float GetItemHeight() const { return m_itemHeight; }
    void SetItemHeight(float v);
    bool GetLastChildFill() const { return m_lastChildFill; }
    void SetLastChildFill(bool v);
    int GetRows() const { return m_rows; }
    void SetRows(int v);
    int GetColumns() const { return m_columns; }
    void SetColumns(int v);

    bool GetClipToBounds() const { return m_clipToBounds; }
    void SetClipToBounds(bool v);

    float GetCanvasLeft() const { return m_canvasLeft; }
    void SetCanvasLeft(float v);
    float GetCanvasTop() const { return m_canvasTop; }
    void SetCanvasTop(float v);
    float GetCanvasRight() const { return m_canvasRight; }
    void SetCanvasRight(float v);
    float GetCanvasBottom() const { return m_canvasBottom; }
    void SetCanvasBottom(float v);
    int GetGridColumn() const { return m_gridColumn; }
    void SetGridColumn(int v);
    int GetGridRow() const { return m_gridRow; }
    void SetGridRow(int v);
    int GetGridColumnSpan() const { return m_gridColumnSpan; }
    void SetGridColumnSpan(int v);
    int GetGridRowSpan() const { return m_gridRowSpan; }
    void SetGridRowSpan(int v);
    Dock GetDock() const { return m_dock; }
    void SetDock(Dock d);

    ThemeTokenId GetBackgroundToken() const { return m_backgroundToken; }
    void SetBackgroundToken(ThemeTokenId id);
    ThemeTokenId GetHoverBackgroundToken() const { return m_hoverBackgroundToken; }
    void SetHoverBackgroundToken(ThemeTokenId id);
    ThemeTokenId GetPressedBackgroundToken() const { return m_pressedBackgroundToken; }
    void SetPressedBackgroundToken(ThemeTokenId id);
    ThemeTokenId GetDisabledBackgroundToken() const { return m_disabledBackgroundToken; }
    void SetDisabledBackgroundToken(ThemeTokenId id);
    ThemeTokenId GetBorderToken() const { return m_borderToken; }
    void SetBorderToken(ThemeTokenId id);
    ThemeTokenId GetFocusedBorderToken() const { return m_focusedBorderToken; }
    void SetFocusedBorderToken(ThemeTokenId id);
    ThemeTokenId GetColorToken() const { return m_colorToken; }
    void SetColorToken(ThemeTokenId id);
    ThemeTokenId GetSecondaryColorToken() const { return m_secondaryColorToken; }
    void SetSecondaryColorToken(ThemeTokenId id);
    ThemeTokenId GetPlaceholderColorToken() const { return m_placeholderColorToken; }
    void SetPlaceholderColorToken(ThemeTokenId id);
    ThemeTokenId GetSelectedBackgroundToken() const { return m_selectedBackgroundToken; }
    void SetSelectedBackgroundToken(ThemeTokenId id);
    ThemeTokenId GetHeaderBackgroundToken() const { return m_headerBackgroundToken; }
    void SetHeaderBackgroundToken(ThemeTokenId id);
    ThemeTokenId GetPaneBackgroundToken() const { return m_paneBackgroundToken; }
    void SetPaneBackgroundToken(ThemeTokenId id);
    ThemeTokenId GetIndicatorColorToken() const { return m_indicatorColorToken; }
    void SetIndicatorColorToken(ThemeTokenId id);
    ThemeTokenId GetDropdownBackgroundToken() const { return m_dropdownBackgroundToken; }
    void SetDropdownBackgroundToken(ThemeTokenId id);
    ThemeTokenId GetSelectedItemBackgroundToken() const { return m_selectedItemBackgroundToken; }
    void SetSelectedItemBackgroundToken(ThemeTokenId id);
    ThemeTokenId GetFillColorToken() const { return m_fillColorToken; }
    void SetFillColorToken(ThemeTokenId id);
    ThemeTokenId GetTrackColorToken() const { return m_trackColorToken; }
    void SetTrackColorToken(ThemeTokenId id);
    ThemeTokenId GetActiveTrackColorToken() const { return m_activeTrackColorToken; }
    void SetActiveTrackColorToken(ThemeTokenId id);
    ThemeTokenId GetThumbColorToken() const { return m_thumbColorToken; }
    void SetThumbColorToken(ThemeTokenId id);
    ThemeTokenId GetOnColorToken() const { return m_onColorToken; }
    void SetOnColorToken(ThemeTokenId id);
    ThemeTokenId GetOffColorToken() const { return m_offColorToken; }
    void SetOffColorToken(ThemeTokenId id);
    ThemeTokenId GetKnobColorToken() const { return m_knobColorToken; }
    void SetKnobColorToken(ThemeTokenId id);
    ThemeTokenId GetCheckedBackgroundToken() const { return m_checkedBackgroundToken; }
    void SetCheckedBackgroundToken(ThemeTokenId id);
    ThemeTokenId GetAccentColorToken() const { return m_accentColorToken; }
    void SetAccentColorToken(ThemeTokenId id);
    ThemeTokenId GetActiveColorToken() const { return m_activeColorToken; }
    void SetActiveColorToken(ThemeTokenId id);
    ThemeTokenId GetUnderlineColorToken() const { return m_underlineColorToken; }
    void SetUnderlineColorToken(ThemeTokenId id);
    ThemeTokenId GetActiveUnderlineColorToken() const { return m_activeUnderlineColorToken; }
    void SetActiveUnderlineColorToken(ThemeTokenId id);
    ThemeTokenId GetActiveTabBackgroundToken() const { return m_activeTabBackgroundToken; }
    void SetActiveTabBackgroundToken(ThemeTokenId id);
    ThemeTokenId GetInactiveTabBackgroundToken() const { return m_inactiveTabBackgroundToken; }
    void SetInactiveTabBackgroundToken(ThemeTokenId id);
    ThemeTokenId GetGridLineBrushToken() const { return m_gridLineBrushToken; }
    void SetGridLineBrushToken(ThemeTokenId id);
    ThemeTokenId GetTitleColorToken() const { return m_titleColorToken; }
    void SetTitleColorToken(ThemeTokenId id);
    ThemeTokenId GetMessageColorToken() const { return m_messageColorToken; }
    void SetMessageColorToken(ThemeTokenId id);
    ThemeTokenId GetCaretColorToken() const { return m_caretColorToken; }
    void SetCaretColorToken(ThemeTokenId id);

    D2D1_COLOR_F ResolveThemeColor(ThemeTokenId token, ThemeTokenId fallback) const;
    D2D1_COLOR_F ResolveThemeColor(PropertyId tokenId, ThemeTokenId fallback) const;

    void SetBackground(D2D1_COLOR_F c);
    D2D1_COLOR_F GetBackgroundColor() const { return m_backgroundColor; }
    bool HasBackgroundColor() const { return m_hasBackgroundColor; }
    void SetHoverBackground(D2D1_COLOR_F c);
    D2D1_COLOR_F GetHoverBackgroundColor() const { return m_hoverBackgroundColor; }
    bool HasHoverBackgroundColor() const { return m_hasHoverBackgroundColor; }
    void SetPressedBackground(D2D1_COLOR_F c);
    D2D1_COLOR_F GetPressedBackgroundColor() const { return m_pressedBackgroundColor; }
    bool HasPressedBackgroundColor() const { return m_hasPressedBackgroundColor; }
    void SetBorderBrush(D2D1_COLOR_F c);
    D2D1_COLOR_F GetBorderBrushColor() const { return m_borderBrushColor; }
    bool HasBorderBrushColor() const { return m_hasBorderBrushColor; }
    void SetColor(D2D1_COLOR_F c);
    D2D1_COLOR_F GetColorValue() const { return m_colorValue; }
    bool HasColorValue() const { return m_hasColorValue; }

    const std::string& GetText() const { return m_text; }
    void SetText(const std::string& text);
    const std::string& GetPlaceholder() const { return m_placeholder; }
    void SetPlaceholder(const std::string& placeholder);
    const std::string& GetFontFamily() const { return m_fontFamily; }
    void SetFontFamily(const std::string& font);
    float GetFontSize() const { return m_fontSize; }
    void SetFontSize(float size);
    const std::string& GetFontWeight() const { return m_fontWeight; }
    void SetFontWeight(const std::string& weight);
    const std::string& GetToolTip() const { return m_toolTip; }
    void SetToolTip(const std::string& tip);
    const std::string& GetIcon() const { return m_icon; }
    void SetIcon(const std::string& icon);

    void SetProperty(PropertyId id, const Value& val) override;
    Value GetProperty(PropertyId id) const override;
    bool HasProperty(PropertyId id) const override;
    std::vector<std::pair<PropertyId, Value>> SnapshotProperties() const override;

    std::string GetId() const { return m_id; }
    void SetId(const std::string& id) { m_id = id; }

    std::string GetStyleClass() const { return m_styleClass; }
    void SetStyleClass(const std::string& styleClass) { m_styleClass = styleClass; }

    UIElement* GetParent() const { return m_parent; }
    void SetParent(UIElement* parent);

    const std::vector<std::shared_ptr<UIElement>>& GetChildren() const { return m_children; }
    void AddChild(std::shared_ptr<UIElement> child);
    // Same as AddChild/RemoveChild but without MarkRenderContentDirty (caller dirties locally).
    void AddChildQuiet(std::shared_ptr<UIElement> child);
    void RemoveChild(std::shared_ptr<UIElement> child);
    void RemoveChildQuiet(std::shared_ptr<UIElement> child);
    void RemoveChildRaw(UIElement* child);
    void ClearChildren();

    std::shared_ptr<UIElement> FindElementById(const std::string& id);

    Rect GetBounds() const { return m_bounds; }
    void SetBounds(const Rect& bounds);

    Size GetDesiredSize() const { return m_desiredSize; }

    virtual Size Measure(Size availableSize);
    virtual void Arrange(Rect finalRect);
    virtual bool ShouldClipToBounds() const;
    virtual void Render(GraphicsContext& ctx);
    virtual void OnRender(GraphicsContext& ctx);

    virtual void RenderOverlay(GraphicsContext& ctx);
    virtual void OnRenderOverlay(GraphicsContext& ctx) {}

    virtual UIElement* HitTest(float x, float y);
    virtual UIElement* HitTestOverlay(float x, float y);
    virtual UIElement* OnHitTestOverlay(float x, float y) { return nullptr; }

    bool IsHovered() const { return m_isHovered; }
    bool IsPressed() const { return m_isPressed; }
    bool IsFocused() const { return m_isFocused; }

    virtual HCURSOR GetCursor() const { return nullptr; }

    virtual void OnMouseEnter();
    virtual void OnMouseLeave();
    virtual void OnMouseDown(Point pt);
    virtual void OnMouseDblClick(Point pt) {}
    virtual void OnMouseRightClick(Point pt) {}
    virtual void OnMouseUp(Point pt);
    virtual void OnMouseMove(Point pt);
    virtual void OnMouseWheel(float delta);
    virtual void OnKeyDown(int vkCode);
    virtual void OnCharInput(wchar_t ch) {}
    virtual void OnAutoScrollTick() {}
    virtual bool NeedsAutoScrollTick() const { return false; }
    virtual bool OnAnimationTick();
    virtual bool HasSelfAnimation() const { return false; }
    // Modal overlays (ContentDialog) — Window freezes the scene layer while true.
    virtual bool IsModalOverlayOpen() const { return false; }
    // True while this animator updates via ComposePresent (DComp) and must not
    // dirty the scene cache or Present the HWND swapchain.
    virtual bool IsComposeOnlyAnimation() const { return false; }
    // Independent composition present (WinUI-style). Return true if handled.
    virtual bool ComposePresent(GraphicsContext& ctx) { (void)ctx; return false; }
    // Self contribution only (no child walk). Used by the animation pump dirty path.
    virtual void CollectSelfAnimationBounds(Rect& dirtyRect, bool& hasDirty) const;
    virtual void CollectAnimationBounds(Rect& dirtyRect, bool& hasDirty) const;
    virtual void CollectRenderDirtyRegion(DirtyRegion& dirtyRegion, bool consume = true);
    void RequestAnimationTicks();
    void CancelAnimationTicks();
    bool IsAnimationTicksRegistered() const { return m_animationTicksRegistered; }
    static void SetAnimationsEnabled(bool enabled);
    static bool AreAnimationsEnabled();
    static void SetAnimationDeltaSeconds(float dtSeconds);
    static float GetAnimationDeltaSeconds();
    RenderNode& GetRenderNode() { return m_renderNode; }
    const RenderNode& GetRenderNode() const { return m_renderNode; }
    virtual void SyncRenderState();
    virtual void MarkRenderContentDirty();
    virtual void MarkRenderRectDirty(const Rect& rect);
    virtual void OnThemeChanged();

    // Layout dirty axis (WPF InvalidateMeasure / Flutter markNeedsLayout).
    void InvalidateMeasure();
    void InvalidateArrange();
    bool IsMeasureDirty() const { return m_measureDirty; }
    bool IsArrangeDirty() const { return m_arrangeDirty; }
    // Incremental: only dirty subtrees. Window calls this each frame before animate.
    void FlushLayout(Size availableSize, const Rect& arrangeRect);

    // Composition properties — when promoted, opacity/offset animate without content re-raster.
    void PromoteLayer(bool promote);
    bool IsLayerPromoted() const { return m_layerPromoted; }
    void SetComposeOpacity(float opacity);
    float GetComposeOpacity() const { return m_composeOpacity; }
    void SetComposeOffset(float x, float y);
    float GetComposeOffsetX() const { return m_composeOffsetX; }
    float GetComposeOffsetY() const { return m_composeOffsetY; }
    bool HasComposeDirty() const { return m_composeDirty; }
    void ClearComposeDirty() { m_composeDirty = false; }

    // Page / subtree lifecycle (NavigationView content swap).
    virtual void OnNavigatedTo();
    virtual void OnNavigatedFrom();
    void PauseAnimationSubtree();
    // Re-arm HasSelfAnimation() ticks after attach / OnNavigatedTo (Build-time
    // RequestAnimationTicks is rejected while not under the live root).
    void ResumeAnimationSubtree();

    // Routed events (tunnel then bubble). Default forwards to classic OnMouse*/OnKey*.
    virtual void OnRoutedEvent(RoutedEventArgs& args);

    void SetContextMenu(std::shared_ptr<ContextMenu> menu) { m_contextMenu = menu; }
    std::shared_ptr<ContextMenu> GetContextMenu() const { return m_contextMenu; }

    virtual void OnFocus() { m_isFocused = true; NotifyFieldChanged(PropertyId::Focused, Value(true)); }
    virtual void OnBlur() { m_isFocused = false; NotifyFieldChanged(PropertyId::Focused, Value(false)); }

    Event<UIElement*>& OnClick() { return m_onClickEvent; }
    Event<UIElement*, Point>& OnMouseDownEvent() { return m_onMouseDownEvent; }

protected:
    void NotifyFieldChanged(PropertyId id, const Value& val);
    bool DescHasOptionalProperty(const PropertyDesc& desc) const;

    std::string m_id;
    std::string m_styleClass;
    std::string m_text;
    std::string m_placeholder;
    std::string m_fontFamily{ "微软雅黑" };
    std::string m_fontWeight{ "Normal" };
    std::string m_toolTip;
    std::string m_icon;
    float m_fontSize = 12.0f;

    float m_width = -1.0f;
    float m_height = -1.0f;
    float m_minWidth = 0.0f;
    float m_minHeight = 0.0f;
    Thickness m_margin{};
    Thickness m_padding{};
    Visibility m_visibility = Visibility::Visible;
    bool m_isEnabled = true;
    float m_opacity = 1.0f;
    float m_cornerRadius = 0.0f;
    float m_borderThickness = 0.0f;
    float m_flexGrow = 0.0f;
    Alignment m_align = Alignment::Stretch;
    Alignment m_alignHorizontal = Alignment::Stretch;
    Alignment m_alignVertical = Alignment::Stretch;
    Orientation m_orientation = Orientation::Vertical;
    float m_gap = 0.0f;
    float m_itemWidth = -1.0f;
    float m_itemHeight = -1.0f;
    bool m_lastChildFill = false;
    int m_rows = 1;
    int m_columns = 1;
    bool m_clipToBounds = false;
    bool m_subtreeRenderDirty = false;
    bool m_measureDirty = true;
    bool m_arrangeDirty = true;
    Size m_lastMeasureAvailable{ -1.0f, -1.0f };
    bool m_layerPromoted = false;
    bool m_composeDirty = false;
    float m_composeOpacity = 1.0f;
    float m_composeOffsetX = 0.0f;
    float m_composeOffsetY = 0.0f;
    Rect m_lastComposeScreenBounds{};

    float m_canvasLeft = kAttachedUnset;
    float m_canvasTop = kAttachedUnset;
    float m_canvasRight = kAttachedUnset;
    float m_canvasBottom = kAttachedUnset;
    int m_gridColumn = 0;
    int m_gridRow = 0;
    int m_gridColumnSpan = 1;
    int m_gridRowSpan = 1;
    Dock m_dock = Dock::Left;

    ThemeTokenId m_backgroundToken = ThemeTokenId::Unset;
    ThemeTokenId m_hoverBackgroundToken = ThemeTokenId::Unset;
    ThemeTokenId m_pressedBackgroundToken = ThemeTokenId::Unset;
    ThemeTokenId m_disabledBackgroundToken = ThemeTokenId::Unset;
    ThemeTokenId m_borderToken = ThemeTokenId::Unset;
    ThemeTokenId m_focusedBorderToken = ThemeTokenId::Unset;
    ThemeTokenId m_colorToken = ThemeTokenId::Unset;
    ThemeTokenId m_secondaryColorToken = ThemeTokenId::Unset;
    ThemeTokenId m_placeholderColorToken = ThemeTokenId::Unset;
    ThemeTokenId m_selectedBackgroundToken = ThemeTokenId::Unset;
    ThemeTokenId m_headerBackgroundToken = ThemeTokenId::Unset;
    ThemeTokenId m_paneBackgroundToken = ThemeTokenId::Unset;
    ThemeTokenId m_indicatorColorToken = ThemeTokenId::Unset;
    ThemeTokenId m_dropdownBackgroundToken = ThemeTokenId::Unset;
    ThemeTokenId m_selectedItemBackgroundToken = ThemeTokenId::Unset;
    ThemeTokenId m_fillColorToken = ThemeTokenId::Unset;
    ThemeTokenId m_trackColorToken = ThemeTokenId::Unset;
    ThemeTokenId m_activeTrackColorToken = ThemeTokenId::Unset;
    ThemeTokenId m_thumbColorToken = ThemeTokenId::Unset;
    ThemeTokenId m_onColorToken = ThemeTokenId::Unset;
    ThemeTokenId m_offColorToken = ThemeTokenId::Unset;
    ThemeTokenId m_knobColorToken = ThemeTokenId::Unset;
    ThemeTokenId m_checkedBackgroundToken = ThemeTokenId::Unset;
    ThemeTokenId m_accentColorToken = ThemeTokenId::Unset;
    ThemeTokenId m_activeColorToken = ThemeTokenId::Unset;
    ThemeTokenId m_underlineColorToken = ThemeTokenId::Unset;
    ThemeTokenId m_activeUnderlineColorToken = ThemeTokenId::Unset;
    ThemeTokenId m_activeTabBackgroundToken = ThemeTokenId::Unset;
    ThemeTokenId m_inactiveTabBackgroundToken = ThemeTokenId::Unset;
    ThemeTokenId m_gridLineBrushToken = ThemeTokenId::Unset;
    ThemeTokenId m_titleColorToken = ThemeTokenId::Unset;
    ThemeTokenId m_messageColorToken = ThemeTokenId::Unset;
    ThemeTokenId m_caretColorToken = ThemeTokenId::Unset;

    D2D1_COLOR_F m_backgroundColor{};
    D2D1_COLOR_F m_borderBrushColor{};
    D2D1_COLOR_F m_hoverBackgroundColor{};
    D2D1_COLOR_F m_pressedBackgroundColor{};
    D2D1_COLOR_F m_colorValue{ 1, 1, 1, 1 };
    bool m_hasBackgroundColor = false;
    bool m_hasBorderBrushColor = false;
    bool m_hasHoverBackgroundColor = false;
    bool m_hasPressedBackgroundColor = false;
    bool m_hasColorValue = false;

    UIElement* m_parent = nullptr;
    std::vector<std::shared_ptr<UIElement>> m_children;
    Rect m_bounds{};
    Size m_desiredSize{};
    bool m_isHovered = false;
    bool m_isPressed = false;
    bool m_isFocused = false;
    bool m_animationTicksRegistered = false;
    Point m_lastMousePos{ 0.0f, 0.0f };
    Point m_tooltipAnchorPos{ 0.0f, 0.0f };
    std::chrono::steady_clock::time_point m_lastMouseMoveTime;
    bool m_tooltipVisible = false;
    RenderNode m_renderNode;
    std::shared_ptr<ContextMenu> m_contextMenu;

    Event<UIElement*> m_onClickEvent;
    Event<UIElement*, Point> m_onMouseDownEvent;

    static bool s_animationsEnabled;
    static float s_animationDeltaSeconds;
};

} // namespace CUI
