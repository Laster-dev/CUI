#pragma once

#include "../UIElement.h"
#include "../../animation/AnimationSystem.h"
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
    bool HasSelfAnimation() const override;
    void OnMouseWheel(float delta) override;

    void NotifyFloatClosed(DockFloatWindow* wnd);
    Point LocalToScreenDip(Point local) const;
    Point ScreenDipToLocal(Point screenDip) const;
    void BeginFloatRedock(int paneIndex);
    void UpdateFloatRedock(Point screenDip);
    bool CompleteFloatRedock(Point screenDip);
    void CancelFloatRedock();
    HWND OwnerHwnd() const;
    void InvalidateOwner();

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
        int groupPaneLocal = -1;
        int paneIndex = -1;
        int splitter = -1;
    };

    struct SlotGeom {
        Rect outer;
        Rect header;
        Rect content;
        Rect tabStrip;
        Rect pinBtn;
        Rect closeBtn;
        Rect scrollLeft;
        Rect scrollRight;
        std::vector<Rect> tabs;
        float totalTabsWidth = 0.0f;
        bool showScroll = false;
        bool visible = false;
    };

    struct LayoutGeom {
        SlotGeom left, right, top, bottom, center;
        Rect splitL, splitR, splitT, splitB;
        Rect visSplitL, visSplitR, visSplitT, visSplitB;
        float strip = 24.0f;
    };

    struct SideChromeAnim {
        AnimatedScalar underlineX{ 0.0f };
        AnimatedScalar underlineW{ 0.0f };
        AnimatedScalar contentFade{ 1.0f };
        bool underlineInited = false;
    };

    static constexpr float kHeaderH = 28.0f;
    static constexpr float kSplitHit = 5.0f;
    static constexpr float kDragThreshold = 6.0f;
    static constexpr float kMinSide = 80.0f;
    static constexpr float kMinCenter = 120.0f;
    static constexpr float kTabMinW = 64.0f;
    static constexpr float kTabMaxW = 160.0f;
    static constexpr float kTabPadX = 10.0f;
    static constexpr float kChromeBtn = 20.0f;
    static constexpr float kScrollBtn = 18.0f;
    static constexpr float kAutoHideStrip = 24.0f;

    std::string MakePaneId();
    DockTabGroup& SlotGroup(DockSide side);
    const DockTabGroup& SlotGroup(DockSide side) const;
    SideChromeAnim& SideAnim(DockSide side);
    const SideChromeAnim& SideAnim(DockSide side) const;
    DockSide SideOfPane(int paneIndex) const;
    void RemovePaneFromAllSlots(int paneIndex);
    void AddPaneToSlot(int paneIndex, DockSide side, bool select = true);
    void SyncContentChildren();
    void RelayoutContents();
    void ApplyLayoutNow();
    void EnsureTabVisible(DockSide side);
    void SyncSideUnderline(DockSide side, bool jump);
    void JumpAllUnderlines();
    void BeginContentFade(DockSide side);
    void ApplyContentFadeOpacities();
    float MeasureTabWidth(const std::string& title) const;
    bool HasAutoHideOn(DockSide side) const;
    void AutoHideStripInsets(float& left, float& top, float& right, float& bottom) const;
    float MeasureAutoHideTabExtent(const std::string& title) const;
    float AutoHideTabOrigin(DockSide side, int indexOnSide) const;
    Rect AutoHideTabRect(DockSide side, int indexOnSide, const std::string& title) const;
    void DrawAutoHideLabel(GraphicsContext& ctx, const Rect& btn, const std::string& title,
                           DockSide side, D2D1_COLOR_F color) const;
    DockSide PeekSide() const;
    Rect PeekOuterRect() const;
    SlotGeom MakePeekGeom() const;
    void ShowPeek(int paneIndex);
    void HidePeek();
    void DrawPeek(GraphicsContext& ctx) const;
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
    void DrawChromeButtonBg(GraphicsContext& ctx, const Rect& r, float hoverT, bool danger) const;
    void DrawCloseGlyph(GraphicsContext& ctx, const Rect& r, D2D1_COLOR_F color) const;
    void DrawPinGlyph(GraphicsContext& ctx, const Rect& r, D2D1_COLOR_F color, bool autoHide) const;
    int SelectedPaneOf(const DockTabGroup& g) const;
    void SelectInGroup(DockSide side, int localIndex);
    void CloseFloatForPane(int paneIndex);
    const SlotGeom* SlotGeomFor(DockSide side) const;

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

    bool m_dragArmed = false;
    bool m_dragging = false;
    int m_dragPane = -1;
    Point m_dragStart{};
    Point m_dragPt{};
    DockDropKind m_dropHighlight = DockDropKind::None;
    AnimatedScalar m_guideOpacity{ 0.0f };
    AnimatedScalar m_dropPulse{ 0.0f };

    SideChromeAnim m_animLeft, m_animRight, m_animTop, m_animBottom, m_animCenter;
    AnimatedScalar m_hoverPin{ 0.0f };
    AnimatedScalar m_hoverClose{ 0.0f };
    AnimatedScalar m_hoverScrollL{ 0.0f };
    AnimatedScalar m_hoverScrollR{ 0.0f };
    DockSide m_hoverBtnSide = DockSide::None;

    int m_activeSplitter = -1;
    float m_splitStartSize = 0.0f;
    Point m_splitStartPt{};
    int m_peekPane = -1;

    unsigned long long m_nextPaneId = 1;
};

} // namespace CUI
