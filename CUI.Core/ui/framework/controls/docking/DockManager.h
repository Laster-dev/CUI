#pragma once

#include "../UIElement.h"
#include "DockTypes.h"
#include "DockFloatWindow.h"
#include <memory>
#include <string>
#include <vector>
#include <windows.h>

namespace CUI {

class Window;

// Single self-drawn VS-style docking host.
// Chrome (tabs / splitters / guides / AH) is painted here.
// Only pane *content* elements are UIElement children — no nested dock panels.
class DockManager : public UIElement {
public:
    DockManager();
    ~DockManager() override;

    const char* GetClassName() const override { return "DockManager"; }

    void SetOwnerWindow(Window* window) { m_ownerWindow = window; }
    Window* GetOwnerWindow() const { return m_ownerWindow; }

    int AddToolPane(const std::string& title, std::shared_ptr<UIElement> content, DockSide side);
    int AddDocument(const std::string& title, std::shared_ptr<UIElement> content);
    void ClosePane(int paneIndex);
    void FloatPane(int paneIndex, Point screenDipTopLeft = Point());
    void DockPane(int paneIndex, DockSide side);
    void SetPaneAutoHide(int paneIndex, bool autoHide);

    int FindPaneIndexByTitle(const std::string& title) const;
    const DockPaneData* GetPane(int index) const;
    int GetPaneCount() const { return static_cast<int>(m_panes.size()); }

    void SetSideSize(DockSide side, float size);
    float GetSideSize(DockSide side) const;

    bool SaveLayout(const std::wstring& path) const;
    bool LoadLayout(const std::wstring& path);

    Size Measure(Size availableSize) override;
    void Arrange(Rect finalRect) override;
    void OnRender(GraphicsContext& ctx) override;
    void RenderOverlay(GraphicsContext& ctx) override;
    UIElement* HitTest(float x, float y) override;
    UIElement* HitTestOverlay(float x, float y) override;
    void OnMouseDown(Point pt) override;
    void OnMouseMove(Point pt) override;
    void OnMouseUp(Point pt) override;
    void OnMouseLeave() override;
    HCURSOR GetCursor() const override;
    bool OnAnimationTick() override;

    // Used by DockFloatWindow / serializer.
    void NotifyFloatClosed(DockFloatWindow* wnd);
    Point LocalToScreenDip(Point local) const;
    HWND OwnerHwnd() const;

private:
    friend class DockLayoutSerializer;
    friend class DockFloatWindow;

    enum class HitPart : uint8_t {
        None = 0,
        Tab,
        Pin,
        Close,
        Header,
        Splitter,
        AutoHide,
        Content,
        TabScrollLeft,
        TabScrollRight
    };

    struct HitResult {
        HitPart part = HitPart::None;
        DockSide side = DockSide::None;
        int groupPaneLocal = -1; // index within group
        int paneIndex = -1;
        int splitter = -1; // 0=L,1=R,2=T,3=B
    };

    struct SlotGeom {
        Rect outer;
        Rect header;
        Rect content;
        Rect tabStrip;   // clipped region for tabs (excludes chrome buttons)
        Rect pinBtn;
        Rect closeBtn;
        Rect scrollLeft;
        Rect scrollRight;
        std::vector<Rect> tabs; // absolute positions (may extend past strip; clipped when drawn/hit)
        float totalTabsWidth = 0.0f;
        bool showScroll = false;
        bool visible = false;
    };

    struct LayoutGeom {
        SlotGeom left, right, top, bottom, center;
        Rect splitL, splitR, splitT, splitB;
        float strip = 22.0f;
    };

    static constexpr float kHeaderH = 28.0f;
    static constexpr float kSplitThick = 5.0f;
    static constexpr float kDragThreshold = 6.0f;
    static constexpr float kMinSide = 80.0f;
    static constexpr float kMinCenter = 120.0f;
    static constexpr float kTabMinW = 64.0f;
    static constexpr float kTabMaxW = 160.0f;
    static constexpr float kTabPadX = 10.0f;
    static constexpr float kChromeBtn = 22.0f;
    static constexpr float kScrollBtn = 18.0f;

    std::string MakePaneId();
    DockTabGroup& SlotGroup(DockSide side);
    const DockTabGroup& SlotGroup(DockSide side) const;
    DockSide SideOfPane(int paneIndex) const;
    void RemovePaneFromAllSlots(int paneIndex);
    void AddPaneToSlot(int paneIndex, DockSide side, bool select = true);
    void SyncContentChildren();
    void RelayoutContents();
    void ApplyLayoutNow();
    void EnsureTabVisible(DockSide side);
    float MeasureTabWidth(const std::string& title) const;
    LayoutGeom ComputeGeom(const Rect& bounds);
    void FillSlotGeom(SlotGeom& slot, DockTabGroup& group, DockSide side, Rect outer);
    HitResult HitTestChrome(float x, float y) const;
    Rect GuideHot(DockDropKind kind, const Rect& host) const;
    DockDropKind HitTestDrop(float x, float y) const;
    void BeginDrag(int paneIndex, Point pt);
    void UpdateDrag(Point pt);
    void EndDrag(Point pt);
    void CancelDrag();
    void DrawSlot(GraphicsContext& ctx, DockSide side, const SlotGeom& g) const;
    void DrawGuides(GraphicsContext& ctx) const;
    void DrawDragGhost(GraphicsContext& ctx) const;
    void DrawAutoHideStrips(GraphicsContext& ctx) const;
    int SelectedPaneOf(const DockTabGroup& g) const;
    void SelectInGroup(DockSide side, int localIndex);
    void CloseFloatForPane(int paneIndex);
    void OnMouseWheel(float delta) override;

    Window* m_ownerWindow = nullptr;
    std::vector<DockPaneData> m_panes;
    DockTabGroup m_left, m_right, m_top, m_bottom, m_center;
    std::vector<DockAutoHideItem> m_autoHide;
    std::vector<std::unique_ptr<DockFloatWindow>> m_floats;

    float m_leftSize = 220.0f;
    float m_rightSize = 240.0f;
    float m_topSize = 140.0f;
    float m_bottomSize = 150.0f;

    LayoutGeom m_geom{};
    HitResult m_hover{};

    // Drag
    bool m_dragArmed = false;
    bool m_dragging = false;
    int m_dragPane = -1;
    Point m_dragStart{};
    Point m_dragPt{};
    DockDropKind m_dropHighlight = DockDropKind::None;
    float m_guideOpacity = 0.0f;

    // Splitter drag
    int m_activeSplitter = -1;
    float m_splitStartSize = 0.0f;
    Point m_splitStartPt{};

    unsigned long long m_nextPaneId = 1;
};

} // namespace CUI
