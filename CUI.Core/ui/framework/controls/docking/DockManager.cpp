#include "DockManager.h"
#include "DockLayoutSerializer.h"
#include "../../window/Window.h"
#include "../../window/Dpi.h"
#include "../../style/ThemeManager.h"
#include "../../animation/AnimationManager.h"
#include "../../animation/FrameScheduler.h"
#include <algorithm>
#include <cmath>
#include <sstream>

namespace CUI {

namespace {
AnimationSpec HoverSpec() {
    AnimationSpec s;
    s.responseAt60Hz = 0.28f;
    s.epsilon = 0.01f;
    return s;
}
AnimationSpec TabSpec() {
    AnimationSpec s;
    s.responseAt60Hz = 0.22f;
    s.epsilon = 0.5f;
    return s;
}
AnimationSpec FadeSpec() {
    AnimationSpec s;
    s.maxDurationSeconds = 0.14f;
    s.epsilon = 0.01f;
    return s;
}
AnimationSpec GuideSpec() {
    AnimationSpec s;
    s.maxDurationSeconds = 0.12f;
    s.epsilon = 0.01f;
    return s;
}
} // namespace

DockManager::DockManager() {
    SetBackgroundToken(ThemeTokenId::WindowBackground);
    SetClipToBounds(true);
    SetMinWidth(320.0f);
    SetMinHeight(240.0f);
}

DockManager::~DockManager() {
    m_floats.clear();
}

HWND DockManager::OwnerHwnd() const {
    return m_ownerWindow ? m_ownerWindow->GetHWND() : nullptr;
}

Point DockManager::LocalToScreenDip(Point local) const {
    HWND hwnd = OwnerHwnd();
    if (!hwnd) {
        return local;
    }
    const float scale = GetDpiScaleForWindow(hwnd);
    POINT pt{
        static_cast<LONG>(std::lround(local.x * scale)),
        static_cast<LONG>(std::lround(local.y * scale))
    };
    ClientToScreen(hwnd, &pt);
    return Point(static_cast<float>(pt.x) / scale, static_cast<float>(pt.y) / scale);
}

std::string DockManager::MakePaneId() {
    std::ostringstream oss;
    oss << "pane-" << (m_nextPaneId++);
    return oss.str();
}

DockTabGroup& DockManager::SlotGroup(DockSide side) {
    switch (side) {
    case DockSide::Left: return m_left;
    case DockSide::Right: return m_right;
    case DockSide::Top: return m_top;
    case DockSide::Bottom: return m_bottom;
    default: return m_center;
    }
}

const DockTabGroup& DockManager::SlotGroup(DockSide side) const {
    switch (side) {
    case DockSide::Left: return m_left;
    case DockSide::Right: return m_right;
    case DockSide::Top: return m_top;
    case DockSide::Bottom: return m_bottom;
    default: return m_center;
    }
}

DockManager::SideChromeAnim& DockManager::SideAnim(DockSide side) {
    switch (side) {
    case DockSide::Left: return m_animLeft;
    case DockSide::Right: return m_animRight;
    case DockSide::Top: return m_animTop;
    case DockSide::Bottom: return m_animBottom;
    default: return m_animCenter;
    }
}

const DockManager::SideChromeAnim& DockManager::SideAnim(DockSide side) const {
    switch (side) {
    case DockSide::Left: return m_animLeft;
    case DockSide::Right: return m_animRight;
    case DockSide::Top: return m_animTop;
    case DockSide::Bottom: return m_animBottom;
    default: return m_animCenter;
    }
}

const DockManager::SlotGeom* DockManager::SlotGeomFor(DockSide side) const {
    switch (side) {
    case DockSide::Left: return &m_geom.left;
    case DockSide::Right: return &m_geom.right;
    case DockSide::Top: return &m_geom.top;
    case DockSide::Bottom: return &m_geom.bottom;
    default: return &m_geom.center;
    }
}

void DockManager::SetSideSize(DockSide side, float size) {
    size = (std::max)(kMinSide, size);
    switch (side) {
    case DockSide::Left: m_leftSize = size; break;
    case DockSide::Right: m_rightSize = size; break;
    case DockSide::Top: m_topSize = size; break;
    case DockSide::Bottom: m_bottomSize = size; break;
    default: break;
    }
    InvalidateArrange();
    MarkRenderContentDirty();
}

float DockManager::GetSideSize(DockSide side) const {
    switch (side) {
    case DockSide::Left: return m_leftSize;
    case DockSide::Right: return m_rightSize;
    case DockSide::Top: return m_topSize;
    case DockSide::Bottom: return m_bottomSize;
    default: return 0.0f;
    }
}

int DockManager::SelectedPaneOf(const DockTabGroup& g) const {
    if (g.paneIndices.empty()) {
        return -1;
    }
    const int i = std::clamp(g.selected, 0, static_cast<int>(g.paneIndices.size()) - 1);
    return g.paneIndices[i];
}

void DockManager::SelectInGroup(DockSide side, int localIndex) {
    auto& g = SlotGroup(side);
    if (localIndex < 0 || localIndex >= static_cast<int>(g.paneIndices.size())) {
        return;
    }
    if (g.selected == localIndex) {
        EnsureTabVisible(side);
        SyncSideUnderline(side, false);
        ApplyLayoutNow();
        return;
    }
    g.selected = localIndex;
    EnsureTabVisible(side);
    BeginContentFade(side);
    SyncSideUnderline(side, false);
    ApplyLayoutNow();
}

void DockManager::BeginContentFade(DockSide side) {
    auto& anim = SideAnim(side);
    if (AreAnimationsEnabled()) {
        anim.contentFade.Reset(0.15f);
        anim.contentFade.SetTarget(1.0f);
        RequestAnimationTicks();
    } else {
        anim.contentFade.Reset(1.0f);
    }
}

void DockManager::ApplyContentFadeOpacities() {
    auto apply = [&](DockSide side) {
        const auto& group = SlotGroup(side);
        const int sel = SelectedPaneOf(group);
        const float t = SideAnim(side).contentFade.Current();
        for (int idx : group.paneIndices) {
            if (idx < 0 || idx >= static_cast<int>(m_panes.size()) || !m_panes[idx].content) {
                continue;
            }
            if (idx == sel) {
                m_panes[idx].content->PromoteLayer(true);
                m_panes[idx].content->SetComposeOpacity(std::clamp(t, 0.0f, 1.0f));
            }
        }
    };
    apply(DockSide::Left);
    apply(DockSide::Right);
    apply(DockSide::Top);
    apply(DockSide::Bottom);
    apply(DockSide::Center);
}

void DockManager::SyncSideUnderline(DockSide side, bool jump) {
    auto& anim = SideAnim(side);
    const auto* slot = SlotGeomFor(side);
    const auto& group = SlotGroup(side);
    if (!slot || !slot->visible || group.paneIndices.empty()
        || group.selected < 0 || group.selected >= static_cast<int>(slot->tabs.size())) {
        return;
    }
    const Rect& tr = slot->tabs[group.selected];
    const float x = tr.x;
    const float w = tr.width;
    if (jump || !anim.underlineInited || !AreAnimationsEnabled()) {
        anim.underlineX.Reset(x);
        anim.underlineW.Reset(w);
        anim.underlineInited = true;
    } else {
        anim.underlineX.SetTarget(x);
        anim.underlineW.SetTarget(w);
        anim.underlineInited = true;
        RequestAnimationTicks();
    }
}

void DockManager::ApplyLayoutNow() {
    RelayoutContents();
    for (DockSide s : { DockSide::Left, DockSide::Right, DockSide::Top, DockSide::Bottom, DockSide::Center }) {
        SyncSideUnderline(s, !SideAnim(s).underlineInited);
    }
    ApplyContentFadeOpacities();
    MarkRenderRectDirty(m_bounds);
    RequestAnimationTicks();
}

float DockManager::MeasureTabWidth(const std::string& title) const {
    GraphicsContext probe;
    const float textW = probe.MeasureText(title, "Segoe UI", 12.0f, DWRITE_FONT_WEIGHT_NORMAL).width;
    return std::clamp(textW + kTabPadX * 2.0f, kTabMinW, kTabMaxW);
}

void DockManager::EnsureTabVisible(DockSide side) {
    auto& group = SlotGroup(side);
    if (group.paneIndices.empty()) {
        return;
    }
    const int sel = std::clamp(group.selected, 0, static_cast<int>(group.paneIndices.size()) - 1);
    // Approximate with current geom if valid; otherwise scroll to start of selected via widths.
    float x = 0.0f;
    for (int i = 0; i < sel; ++i) {
        const int idx = group.paneIndices[i];
        const std::string& title = (idx >= 0 && idx < static_cast<int>(m_panes.size()))
            ? m_panes[idx].title
            : "?";
        x += MeasureTabWidth(title);
    }
    const int selIdx = group.paneIndices[sel];
    const float selW = MeasureTabWidth(
        (selIdx >= 0 && selIdx < static_cast<int>(m_panes.size())) ? m_panes[selIdx].title : "?");

    const SlotGeom* slot = nullptr;
    switch (side) {
    case DockSide::Left: slot = &m_geom.left; break;
    case DockSide::Right: slot = &m_geom.right; break;
    case DockSide::Top: slot = &m_geom.top; break;
    case DockSide::Bottom: slot = &m_geom.bottom; break;
    default: slot = &m_geom.center; break;
    }
    const float viewW = (slot && slot->tabStrip.width > 1.0f) ? slot->tabStrip.width : 200.0f;
    if (x < group.tabScroll) {
        group.tabScroll = x;
    } else if (x + selW > group.tabScroll + viewW) {
        group.tabScroll = x + selW - viewW;
    }
    group.tabScroll = (std::max)(0.0f, group.tabScroll);
}

void DockManager::FillSlotGeom(SlotGeom& slot, DockTabGroup& group, DockSide side, Rect outer) {
    slot = {};
    slot.outer = outer;
    slot.visible = !group.paneIndices.empty() && outer.width > 2.0f && outer.height > 2.0f;
    if (!slot.visible) {
        return;
    }
    slot.header = Rect(outer.x, outer.y, outer.width, kHeaderH);
    slot.content = Rect(outer.x, outer.y + kHeaderH, outer.width, (std::max)(0.0f, outer.height - kHeaderH));

    const bool hasPin = (side != DockSide::Center);
    const float chromeW = kChromeBtn + (hasPin ? kChromeBtn : 0.0f) + 8.0f;
    slot.closeBtn = Rect(outer.x + outer.width - kChromeBtn - 4.0f, outer.y + 5.0f, kChromeBtn, 18.0f);
    if (hasPin) {
        slot.pinBtn = Rect(slot.closeBtn.x - kChromeBtn, outer.y + 5.0f, kChromeBtn, 18.0f);
    }

    // Measure natural tab widths (VS: size to title, never squash into overlap).
    std::vector<float> widths;
    widths.reserve(group.paneIndices.size());
    float total = 0.0f;
    for (int idx : group.paneIndices) {
        const std::string& title = (idx >= 0 && idx < static_cast<int>(m_panes.size()))
            ? m_panes[idx].title
            : "?";
        const float w = MeasureTabWidth(title);
        widths.push_back(w);
        total += w;
    }
    slot.totalTabsWidth = total;

    float stripLeft = outer.x + 4.0f;
    float stripRight = outer.x + outer.width - chromeW;
    slot.showScroll = total > (stripRight - stripLeft) + 0.5f;
    if (slot.showScroll) {
        slot.scrollLeft = Rect(stripLeft, outer.y + 4.0f, kScrollBtn, kHeaderH - 8.0f);
        slot.scrollRight = Rect(stripRight - kScrollBtn, outer.y + 4.0f, kScrollBtn, kHeaderH - 8.0f);
        stripLeft = slot.scrollLeft.x + slot.scrollLeft.width;
        stripRight = slot.scrollRight.x;
    }
    slot.tabStrip = Rect(stripLeft, outer.y + 2.0f, (std::max)(0.0f, stripRight - stripLeft), kHeaderH - 4.0f);

    const float maxScroll = (std::max)(0.0f, total - slot.tabStrip.width);
    group.tabScroll = std::clamp(group.tabScroll, 0.0f, maxScroll);

    float x = slot.tabStrip.x - group.tabScroll;
    slot.tabs.clear();
    for (float w : widths) {
        slot.tabs.push_back(Rect(x, slot.tabStrip.y, w, slot.tabStrip.height));
        x += w;
    }
}

void DockManager::RemovePaneFromAllSlots(int paneIndex) {
    auto strip = [paneIndex](DockTabGroup& g) {
        auto& v = g.paneIndices;
        v.erase(std::remove(v.begin(), v.end(), paneIndex), v.end());
        if (g.selected >= static_cast<int>(v.size())) {
            g.selected = (std::max)(0, static_cast<int>(v.size()) - 1);
        }
    };
    strip(m_left);
    strip(m_right);
    strip(m_top);
    strip(m_bottom);
    strip(m_center);
    m_autoHide.erase(
        std::remove_if(m_autoHide.begin(), m_autoHide.end(),
                       [paneIndex](const DockAutoHideItem& it) { return it.paneIndex == paneIndex; }),
        m_autoHide.end());
}

void DockManager::AddPaneToSlot(int paneIndex, DockSide side, bool select) {
    if (paneIndex < 0 || paneIndex >= static_cast<int>(m_panes.size())) {
        return;
    }
    RemovePaneFromAllSlots(paneIndex);
    CloseFloatForPane(paneIndex);
    m_panes[paneIndex].autoHide = false;
    if (side == DockSide::None) {
        side = DockSide::Center;
    }
    auto& g = SlotGroup(side);
    g.paneIndices.push_back(paneIndex);
    if (select || g.paneIndices.size() == 1) {
        g.selected = static_cast<int>(g.paneIndices.size()) - 1;
    }
}

DockSide DockManager::SideOfPane(int paneIndex) const {
    auto has = [paneIndex](const DockTabGroup& g) {
        return std::find(g.paneIndices.begin(), g.paneIndices.end(), paneIndex) != g.paneIndices.end();
    };
    if (has(m_left)) return DockSide::Left;
    if (has(m_right)) return DockSide::Right;
    if (has(m_top)) return DockSide::Top;
    if (has(m_bottom)) return DockSide::Bottom;
    if (has(m_center)) return DockSide::Center;
    for (const auto& ah : m_autoHide) {
        if (ah.paneIndex == paneIndex) {
            return ah.side;
        }
    }
    return DockSide::None;
}

void DockManager::SyncContentChildren() {
    auto isFloated = [&](int index) {
        for (const auto& f : m_floats) {
            if (f && f->GetPaneIndex() == index) {
                return true;
            }
        }
        return false;
    };
    for (int i = 0; i < static_cast<int>(m_panes.size()); ++i) {
        auto& pane = m_panes[i];
        if (!pane.content || isFloated(i)) {
            continue;
        }
        bool found = false;
        for (const auto& ch : GetChildren()) {
            if (ch.get() == pane.content.get()) {
                found = true;
                break;
            }
        }
        if (!found) {
            AddChildQuiet(pane.content);
        }
    }
}

void DockManager::RelayoutContents() {
    SyncContentChildren();

    // Collapse every content first so a just-moved pane cannot keep painting
    // in its old slot until the next focus/arrange pass.
    for (int i = 0; i < static_cast<int>(m_panes.size()); ++i) {
        if (!m_panes[i].content) {
            continue;
        }
        m_panes[i].content->SetVisibility(Visibility::Collapsed);
    }

    m_geom = ComputeGeom(m_bounds);

    auto place = [&](DockSide side, const SlotGeom& g) {
        auto& group = SlotGroup(side);
        const int sel = SelectedPaneOf(group);
        for (int idx : group.paneIndices) {
            if (idx < 0 || idx >= static_cast<int>(m_panes.size())) {
                continue;
            }
            auto& content = m_panes[idx].content;
            if (!content) {
                continue;
            }
            bool floated = false;
            for (const auto& f : m_floats) {
                if (f && f->GetPaneIndex() == idx) {
                    floated = true;
                    break;
                }
            }
            if (floated || m_panes[idx].autoHide) {
                content->SetVisibility(Visibility::Collapsed);
                continue;
            }
            if (idx == sel && g.visible && g.content.height > 1.0f && g.content.width > 1.0f) {
                content->SetVisibility(Visibility::Visible);
                content->Measure(Size(g.content.width, g.content.height));
                content->Arrange(g.content);
            }
        }
    };

    place(DockSide::Left, m_geom.left);
    place(DockSide::Right, m_geom.right);
    place(DockSide::Top, m_geom.top);
    place(DockSide::Bottom, m_geom.bottom);
    place(DockSide::Center, m_geom.center);
}

int DockManager::AddToolPane(const std::string& title, std::shared_ptr<UIElement> content, DockSide side) {
    if (side == DockSide::None || side == DockSide::Center) {
        side = DockSide::Left;
    }
    DockPaneData pane;
    pane.id = MakePaneId();
    pane.title = title;
    pane.kind = DockPaneKind::Tool;
    pane.content = std::move(content);
    const int index = static_cast<int>(m_panes.size());
    m_panes.push_back(std::move(pane));
    AddPaneToSlot(index, side, true);
    EnsureTabVisible(side);
    ApplyLayoutNow();
    return index;
}

int DockManager::AddDocument(const std::string& title, std::shared_ptr<UIElement> content) {
    DockPaneData pane;
    pane.id = MakePaneId();
    pane.title = title;
    pane.kind = DockPaneKind::Document;
    pane.content = std::move(content);
    const int index = static_cast<int>(m_panes.size());
    m_panes.push_back(std::move(pane));
    AddPaneToSlot(index, DockSide::Center, true);
    EnsureTabVisible(DockSide::Center);
    ApplyLayoutNow();
    return index;
}

const DockPaneData* DockManager::GetPane(int index) const {
    if (index < 0 || index >= static_cast<int>(m_panes.size())) {
        return nullptr;
    }
    return &m_panes[index];
}

int DockManager::FindPaneIndexByTitle(const std::string& title) const {
    for (int i = 0; i < static_cast<int>(m_panes.size()); ++i) {
        if (m_panes[i].title == title) {
            return i;
        }
    }
    return -1;
}

void DockManager::CloseFloatForPane(int paneIndex) {
    for (auto it = m_floats.begin(); it != m_floats.end(); ++it) {
        if (*it && (*it)->GetPaneIndex() == paneIndex) {
            (*it)->Destroy();
            m_floats.erase(it);
            return;
        }
    }
}

void DockManager::ClosePane(int paneIndex) {
    if (paneIndex < 0 || paneIndex >= static_cast<int>(m_panes.size())) {
        return;
    }
    CloseFloatForPane(paneIndex);
    if (m_panes[paneIndex].content) {
        RemoveChildQuiet(m_panes[paneIndex].content);
    }
    RemovePaneFromAllSlots(paneIndex);
    // Compact indices: erase pane and remap all slot indices > paneIndex
    m_panes.erase(m_panes.begin() + paneIndex);
    auto remap = [paneIndex](DockTabGroup& g) {
        for (int& idx : g.paneIndices) {
            if (idx > paneIndex) {
                --idx;
            }
        }
        if (g.selected >= static_cast<int>(g.paneIndices.size())) {
            g.selected = (std::max)(0, static_cast<int>(g.paneIndices.size()) - 1);
        }
    };
    remap(m_left);
    remap(m_right);
    remap(m_top);
    remap(m_bottom);
    remap(m_center);
    for (auto& ah : m_autoHide) {
        if (ah.paneIndex > paneIndex) {
            --ah.paneIndex;
        }
    }
    for (auto& f : m_floats) {
        if (f) {
            f->RemapPaneIndexAfterClose(paneIndex);
        }
    }
    ApplyLayoutNow();
}

void DockManager::DockPane(int paneIndex, DockSide side) {
    if (paneIndex < 0 || paneIndex >= static_cast<int>(m_panes.size())) {
        return;
    }
    AddPaneToSlot(paneIndex, side, true);
    EnsureTabVisible(side);
    ApplyLayoutNow();
}

void DockManager::SetPaneAutoHide(int paneIndex, bool autoHide) {
    if (paneIndex < 0 || paneIndex >= static_cast<int>(m_panes.size())) {
        return;
    }
    DockSide side = SideOfPane(paneIndex);
    if (side == DockSide::None || side == DockSide::Center) {
        side = DockSide::Left;
    }
    if (autoHide) {
        RemovePaneFromAllSlots(paneIndex);
        m_panes[paneIndex].autoHide = true;
        DockAutoHideItem item;
        item.paneIndex = paneIndex;
        item.side = side;
        m_autoHide.push_back(item);
    } else {
        m_panes[paneIndex].autoHide = false;
        m_autoHide.erase(
            std::remove_if(m_autoHide.begin(), m_autoHide.end(),
                           [paneIndex](const DockAutoHideItem& it) { return it.paneIndex == paneIndex; }),
            m_autoHide.end());
        AddPaneToSlot(paneIndex, side, true);
        EnsureTabVisible(side);
    }
    ApplyLayoutNow();
}

void DockManager::FloatPane(int paneIndex, Point screenDipTopLeft) {
    if (paneIndex < 0 || paneIndex >= static_cast<int>(m_panes.size())) {
        return;
    }
    if (!m_panes[paneIndex].canFloat) {
        return;
    }
    HWND owner = OwnerHwnd();
    if (!owner) {
        return;
    }
    CloseFloatForPane(paneIndex);
    RemovePaneFromAllSlots(paneIndex);
    m_panes[paneIndex].autoHide = false;

    if (screenDipTopLeft.x == 0.0f && screenDipTopLeft.y == 0.0f) {
        screenDipTopLeft = LocalToScreenDip(Point(m_bounds.x + 64.0f, m_bounds.y + 64.0f));
    }

    auto flt = std::make_unique<DockFloatWindow>();
    flt->SetCloseCallback([this](DockFloatWindow* wnd) { NotifyFloatClosed(wnd); });
    if (!flt->Show(this, paneIndex, owner, screenDipTopLeft, Size(380.0f, 280.0f))) {
        AddPaneToSlot(paneIndex, DockSide::Left, true);
        return;
    }
    m_floats.push_back(std::move(flt));
    ApplyLayoutNow();
}

void DockManager::NotifyFloatClosed(DockFloatWindow* wnd) {
    if (!wnd) {
        return;
    }
    const int pane = wnd->GetPaneIndex();
    for (auto it = m_floats.begin(); it != m_floats.end(); ++it) {
        if (it->get() == wnd) {
            m_floats.erase(it);
            break;
        }
    }
    if (pane >= 0 && pane < static_cast<int>(m_panes.size())) {
        AddPaneToSlot(pane, DockSide::Left, true);
        EnsureTabVisible(DockSide::Left);
        ApplyLayoutNow();
    }
}

DockManager::LayoutGeom DockManager::ComputeGeom(const Rect& bounds) {
    LayoutGeom g;
    if (bounds.width < 2.0f || bounds.height < 2.0f) {
        return g;
    }

    float x = bounds.x;
    float y = bounds.y;
    float w = bounds.width;
    float h = bounds.height;

    const bool hasL = !m_left.paneIndices.empty();
    const bool hasR = !m_right.paneIndices.empty();
    const bool hasT = !m_top.paneIndices.empty();
    const bool hasB = !m_bottom.paneIndices.empty();
    const bool hasC = !m_center.paneIndices.empty();

    // Auto-hide strips reserve edge when slot empty but AH items exist.
    auto hasAh = [&](DockSide s) {
        for (const auto& it : m_autoHide) {
            if (it.side == s) return true;
        }
        return false;
    };
    if (!hasL && hasAh(DockSide::Left)) {
        x += g.strip;
        w -= g.strip;
    }
    if (!hasR && hasAh(DockSide::Right)) {
        w -= g.strip;
    }
    if (!hasT && hasAh(DockSide::Top)) {
        y += g.strip;
        h -= g.strip;
    }
    if (!hasB && hasAh(DockSide::Bottom)) {
        h -= g.strip;
    }

    float leftW = hasL ? (std::min)(m_leftSize, (std::max)(kMinSide, w - kMinCenter - (hasR ? m_rightSize : 0.0f))) : 0.0f;
    float rightW = hasR ? (std::min)(m_rightSize, (std::max)(kMinSide, w - kMinCenter - leftW)) : 0.0f;
    float midW = w - leftW - rightW - (hasL ? kSplitHit : 0.0f) - (hasR ? kSplitHit : 0.0f);
    midW = (std::max)(kMinCenter, midW);

    // Recalc if mid squeezed.
    if (leftW + rightW + midW + (hasL ? kSplitHit : 0) + (hasR ? kSplitHit : 0) > w) {
        midW = (std::max)(kMinCenter, w - (hasL ? kMinSide + kSplitHit : 0) - (hasR ? kMinSide + kSplitHit : 0));
        float rem = w - midW - (hasL ? kSplitHit : 0) - (hasR ? kSplitHit : 0);
        if (hasL && hasR) {
            const float sum = m_leftSize + m_rightSize;
            leftW = rem * (m_leftSize / (std::max)(1.0f, sum));
            rightW = rem - leftW;
        } else if (hasL) {
            leftW = rem;
        } else if (hasR) {
            rightW = rem;
        }
    }

    float midX = x + leftW + (hasL ? kSplitHit : 0.0f);
    float topH = hasT ? (std::min)(m_topSize, (std::max)(kMinSide, h - kMinCenter - (hasB ? m_bottomSize : 0.0f))) : 0.0f;
    float botH = hasB ? (std::min)(m_bottomSize, (std::max)(kMinSide, h - kMinCenter - topH)) : 0.0f;
    float midH = h - topH - botH - (hasT ? kSplitHit : 0.0f) - (hasB ? kSplitHit : 0.0f);
    midH = (std::max)(40.0f, midH);

    auto fillSlot = [&](SlotGeom& slot, DockTabGroup& group, DockSide side, Rect outer) {
        FillSlotGeom(slot, group, side, outer);
    };

    if (hasL) {
        fillSlot(g.left, m_left, DockSide::Left, Rect(x, y, leftW, h));
        g.splitL = Rect(x + leftW, y, kSplitHit, h);
        g.visSplitL = Rect(x + leftW + (kSplitHit - 1.0f) * 0.5f, y, 1.0f, h);
    }
    if (hasR) {
        fillSlot(g.right, m_right, DockSide::Right, Rect(x + w - rightW, y, rightW, h));
        g.splitR = Rect(x + w - rightW - kSplitHit, y, kSplitHit, h);
        g.visSplitR = Rect(x + w - rightW - kSplitHit + (kSplitHit - 1.0f) * 0.5f, y, 1.0f, h);
    }

    float colY = y;
    if (hasT) {
        fillSlot(g.top, m_top, DockSide::Top, Rect(midX, colY, midW, topH));
        g.splitT = Rect(midX, colY + topH, midW, kSplitHit);
        g.visSplitT = Rect(midX, colY + topH + (kSplitHit - 1.0f) * 0.5f, midW, 1.0f);
        colY += topH + kSplitHit;
    }
    float centerH = midH;
    if (hasB) {
        fillSlot(g.bottom, m_bottom, DockSide::Bottom, Rect(midX, y + h - botH, midW, botH));
        g.splitB = Rect(midX, y + h - botH - kSplitHit, midW, kSplitHit);
        g.visSplitB = Rect(midX, y + h - botH - kSplitHit + (kSplitHit - 1.0f) * 0.5f, midW, 1.0f);
    }
    if (hasC || (!hasT && !hasB)) {
        fillSlot(g.center, m_center, DockSide::Center, Rect(midX, colY, midW, centerH));
        if (m_center.paneIndices.empty()) {
            g.center.visible = true;
            g.center.outer = Rect(midX, colY, midW, centerH);
            g.center.header = Rect(midX, colY, midW, kHeaderH);
            g.center.content = Rect(midX, colY + kHeaderH, midW, (std::max)(0.0f, centerH - kHeaderH));
            g.center.tabs.clear();
        }
    } else {
        fillSlot(g.center, m_center, DockSide::Center, Rect(midX, colY, midW, centerH));
    }

    return g;
}

Size DockManager::Measure(Size availableSize) {
    float w = availableSize.width;
    float h = availableSize.height;
    if (!(w > 1.0f) || !std::isfinite(w) || w > 100000.0f) {
        w = 960.0f;
    }
    if (!(h > 1.0f) || !std::isfinite(h) || h > 100000.0f) {
        h = 640.0f;
    }
    // Flex basis — parent FlexGrow expands us.
    m_desiredSize = Size(w, (std::max)(GetMinHeight(), 200.0f));
    m_measureDirty = false;
    return m_desiredSize;
}

void DockManager::Arrange(Rect finalRect) {
    const Thickness margin = GetMargin();
    Rect arranged(
        finalRect.x + margin.left,
        finalRect.y + margin.top,
        (std::max)(0.0f, finalRect.width - margin.left - margin.right),
        (std::max)(0.0f, finalRect.height - margin.top - margin.bottom));
    SetBounds(arranged);
    RelayoutContents();
    m_arrangeDirty = false;
}

DockManager::HitResult DockManager::HitTestChrome(float x, float y) const {
    HitResult hr;
    auto testSlot = [&](DockSide side, const SlotGeom& g, const DockTabGroup& group) {
        if (!g.visible || !g.outer.Contains(x, y)) {
            return false;
        }
        if (g.closeBtn.Contains(x, y)) {
            hr.part = HitPart::Close;
            hr.side = side;
            hr.paneIndex = SelectedPaneOf(group);
            return true;
        }
        if (side != DockSide::Center && !g.pinBtn.IsEmpty() && g.pinBtn.Contains(x, y)) {
            hr.part = HitPart::Pin;
            hr.side = side;
            hr.paneIndex = SelectedPaneOf(group);
            return true;
        }
        if (g.showScroll && g.scrollLeft.Contains(x, y)) {
            hr.part = HitPart::TabScrollLeft;
            hr.side = side;
            return true;
        }
        if (g.showScroll && g.scrollRight.Contains(x, y)) {
            hr.part = HitPart::TabScrollRight;
            hr.side = side;
            return true;
        }
        if (g.tabStrip.Contains(x, y)) {
            for (int i = 0; i < static_cast<int>(g.tabs.size()); ++i) {
                const Rect& tr = g.tabs[i];
                if (tr.x + tr.width <= g.tabStrip.x || tr.x >= g.tabStrip.x + g.tabStrip.width) {
                    continue;
                }
                if (tr.Contains(x, y)) {
                    hr.part = HitPart::Tab;
                    hr.side = side;
                    hr.groupPaneLocal = i;
                    hr.paneIndex = (i >= 0 && i < static_cast<int>(group.paneIndices.size()))
                        ? group.paneIndices[i]
                        : -1;
                    return true;
                }
            }
        }
        if (g.header.Contains(x, y)) {
            hr.part = HitPart::Header;
            hr.side = side;
            hr.paneIndex = SelectedPaneOf(group);
            return true;
        }
        if (g.content.Contains(x, y)) {
            hr.part = HitPart::Content;
            hr.side = side;
            hr.paneIndex = SelectedPaneOf(group);
            return true;
        }
        return true;
    };

    if (m_geom.splitL.Contains(x, y)) {
        hr.part = HitPart::Splitter;
        hr.splitter = 0;
        return hr;
    }
    if (m_geom.splitR.Contains(x, y)) {
        hr.part = HitPart::Splitter;
        hr.splitter = 1;
        return hr;
    }
    if (m_geom.splitT.Contains(x, y)) {
        hr.part = HitPart::Splitter;
        hr.splitter = 2;
        return hr;
    }
    if (m_geom.splitB.Contains(x, y)) {
        hr.part = HitPart::Splitter;
        hr.splitter = 3;
        return hr;
    }

    // AH buttons
    int ahIndex = 0;
    for (const auto& it : m_autoHide) {
        Rect strip;
        Rect btn;
        const float t = m_geom.strip;
        switch (it.side) {
        case DockSide::Left:
            strip = Rect(m_bounds.x, m_bounds.y, t, m_bounds.height);
            btn = Rect(strip.x, strip.y + 4.0f + ahIndex * 72.0f, t, 68.0f);
            break;
        case DockSide::Right:
            strip = Rect(m_bounds.x + m_bounds.width - t, m_bounds.y, t, m_bounds.height);
            btn = Rect(strip.x, strip.y + 4.0f + ahIndex * 72.0f, t, 68.0f);
            break;
        case DockSide::Top:
            strip = Rect(m_bounds.x, m_bounds.y, m_bounds.width, t);
            btn = Rect(strip.x + 4.0f + ahIndex * 100.0f, strip.y, 96.0f, t);
            break;
        case DockSide::Bottom:
            strip = Rect(m_bounds.x, m_bounds.y + m_bounds.height - t, m_bounds.width, t);
            btn = Rect(strip.x + 4.0f + ahIndex * 100.0f, strip.y, 96.0f, t);
            break;
        default:
            break;
        }
        if (btn.Contains(x, y)) {
            hr.part = HitPart::AutoHide;
            hr.paneIndex = it.paneIndex;
            hr.side = it.side;
            return hr;
        }
        ++ahIndex;
    }

    if (testSlot(DockSide::Left, m_geom.left, m_left)) return hr;
    if (testSlot(DockSide::Right, m_geom.right, m_right)) return hr;
    if (testSlot(DockSide::Top, m_geom.top, m_top)) return hr;
    if (testSlot(DockSide::Bottom, m_geom.bottom, m_bottom)) return hr;
    if (testSlot(DockSide::Center, m_geom.center, m_center)) return hr;
    return hr;
}

UIElement* DockManager::HitTest(float x, float y) {
    if (!m_bounds.Contains(x, y) || GetVisibility() != Visibility::Visible) {
        return nullptr;
    }
    if (m_dragging || m_activeSplitter >= 0) {
        return this;
    }
    const HitResult hr = HitTestChrome(x, y);
    if (hr.part == HitPart::Content && hr.paneIndex >= 0 && hr.paneIndex < static_cast<int>(m_panes.size())) {
        if (auto& c = m_panes[hr.paneIndex].content) {
            if (UIElement* hit = c->HitTest(x, y)) {
                return hit;
            }
        }
    }
    if (hr.part != HitPart::None) {
        return this;
    }
    return this;
}

UIElement* DockManager::HitTestOverlay(float x, float y) {
    if (m_dragging && m_bounds.Contains(x, y)) {
        return this;
    }
    return UIElement::HitTestOverlay(x, y);
}

void DockManager::OnMouseDown(Point pt) {
    UIElement::OnMouseDown(pt);
    const HitResult hr = HitTestChrome(pt.x, pt.y);
    m_hover = hr;

    if (hr.part == HitPart::Splitter) {
        m_activeSplitter = hr.splitter;
        m_splitStartPt = pt;
        switch (hr.splitter) {
        case 0: m_splitStartSize = m_leftSize; break;
        case 1: m_splitStartSize = m_rightSize; break;
        case 2: m_splitStartSize = m_topSize; break;
        case 3: m_splitStartSize = m_bottomSize; break;
        default: break;
        }
        return;
    }
    if (hr.part == HitPart::Close && hr.paneIndex >= 0) {
        ClosePane(hr.paneIndex);
        return;
    }
    if (hr.part == HitPart::Pin && hr.paneIndex >= 0) {
        SetPaneAutoHide(hr.paneIndex, !m_panes[hr.paneIndex].autoHide);
        return;
    }
    if (hr.part == HitPart::AutoHide && hr.paneIndex >= 0) {
        SetPaneAutoHide(hr.paneIndex, false);
        return;
    }
    if (hr.part == HitPart::TabScrollLeft && hr.side != DockSide::None) {
        auto& g = SlotGroup(hr.side);
        g.tabScroll = (std::max)(0.0f, g.tabScroll - 80.0f);
        ApplyLayoutNow();
        return;
    }
    if (hr.part == HitPart::TabScrollRight && hr.side != DockSide::None) {
        auto& g = SlotGroup(hr.side);
        g.tabScroll += 80.0f;
        ApplyLayoutNow();
        return;
    }
    if (hr.part == HitPart::Tab && hr.side != DockSide::None) {
        SelectInGroup(hr.side, hr.groupPaneLocal);
        m_dragArmed = true;
        m_dragging = false;
        m_dragPane = hr.paneIndex;
        m_dragStart = pt;
        m_dragPt = pt;
        return;
    }
    if (hr.part == HitPart::Header && hr.paneIndex >= 0) {
        m_dragArmed = true;
        m_dragging = false;
        m_dragPane = hr.paneIndex;
        m_dragStart = pt;
        m_dragPt = pt;
    }
}

void DockManager::OnMouseWheel(float delta) {
    // VS: wheel over tab strip scrolls overflow tabs.
    if (m_hover.side != DockSide::None
        && (m_hover.part == HitPart::Tab || m_hover.part == HitPart::Header
            || m_hover.part == HitPart::TabScrollLeft || m_hover.part == HitPart::TabScrollRight)) {
        auto& g = SlotGroup(m_hover.side);
        g.tabScroll = (std::max)(0.0f, g.tabScroll - delta * 0.4f);
        ApplyLayoutNow();
        return;
    }
    UIElement::OnMouseWheel(delta);
}

void DockManager::BeginDrag(int paneIndex, Point pt) {
    m_dragging = true;
    m_dragPane = paneIndex;
    m_dragPt = pt;
    m_guideOpacity.Reset(0.0f);
    m_guideOpacity.SetTarget(1.0f);
    m_dropHighlight = HitTestDrop(pt.x, pt.y);
    RequestAnimationTicks();
    MarkRenderRectDirty(m_bounds);
}

void DockManager::UpdateDrag(Point pt) {
    m_dragPt = pt;
    m_dropHighlight = HitTestDrop(pt.x, pt.y);
    MarkRenderRectDirty(m_bounds);
    RequestAnimationTicks();
}

void DockManager::CancelDrag() {
    m_dragging = false;
    m_dragArmed = false;
    m_dragPane = -1;
    m_dropHighlight = DockDropKind::None;
    m_guideOpacity.SetTarget(0.0f);
    RequestAnimationTicks();
    MarkRenderRectDirty(m_bounds);
}

void DockManager::EndDrag(Point pt) {
    if (!m_dragging || m_dragPane < 0) {
        CancelDrag();
        return;
    }
    const int pane = m_dragPane;
    const DockDropKind drop = HitTestDrop(pt.x, pt.y);
    CancelDrag();

    DockSide landed = DockSide::None;
    switch (drop) {
    case DockDropKind::EdgeLeft: landed = DockSide::Left; DockPane(pane, landed); break;
    case DockDropKind::EdgeRight: landed = DockSide::Right; DockPane(pane, landed); break;
    case DockDropKind::EdgeTop: landed = DockSide::Top; DockPane(pane, landed); break;
    case DockDropKind::EdgeBottom: landed = DockSide::Bottom; DockPane(pane, landed); break;
    case DockDropKind::IntoCenter: landed = DockSide::Center; DockPane(pane, landed); break;
    case DockDropKind::None:
        if (!m_bounds.Contains(pt.x, pt.y)) {
            FloatPane(pane, LocalToScreenDip(pt));
        }
        break;
    default:
        break;
    }
    if (landed != DockSide::None) {
        BeginContentFade(landed);
        m_dropPulse.Reset(1.0f);
        m_dropPulse.SetTarget(0.0f);
        RequestAnimationTicks();
        ApplyContentFadeOpacities();
        MarkRenderRectDirty(m_bounds);
    }
}

Rect DockManager::GuideHot(DockDropKind kind, const Rect& host) const {
    constexpr float band = 36.0f;
    constexpr float d = 40.0f;
    const float cx = host.x + host.width * 0.5f;
    const float cy = host.y + host.height * 0.5f;
    switch (kind) {
    case DockDropKind::EdgeLeft: return Rect(host.x, host.y, band, host.height);
    case DockDropKind::EdgeRight: return Rect(host.x + host.width - band, host.y, band, host.height);
    case DockDropKind::EdgeTop: return Rect(host.x, host.y, host.width, band);
    case DockDropKind::EdgeBottom: return Rect(host.x, host.y + host.height - band, host.width, band);
    case DockDropKind::IntoCenter: return Rect(cx - d * 0.5f, cy - d * 0.5f, d, d);
    default: return Rect();
    }
}

DockDropKind DockManager::HitTestDrop(float x, float y) const {
    if (!m_dragging) {
        return DockDropKind::None;
    }
    for (DockDropKind k : {
             DockDropKind::EdgeLeft, DockDropKind::EdgeRight, DockDropKind::EdgeTop,
             DockDropKind::EdgeBottom, DockDropKind::IntoCenter }) {
        if (GuideHot(k, m_bounds).Contains(x, y)) {
            return k;
        }
    }
    return DockDropKind::None;
}

void DockManager::OnMouseMove(Point pt) {
    UIElement::OnMouseMove(pt);

    if (m_activeSplitter >= 0) {
        const float dx = pt.x - m_splitStartPt.x;
        const float dy = pt.y - m_splitStartPt.y;
        switch (m_activeSplitter) {
        case 0: m_leftSize = (std::max)(kMinSide, m_splitStartSize + dx); break;
        case 1: m_rightSize = (std::max)(kMinSide, m_splitStartSize - dx); break;
        case 2: m_topSize = (std::max)(kMinSide, m_splitStartSize + dy); break;
        case 3: m_bottomSize = (std::max)(kMinSide, m_splitStartSize - dy); break;
        default: break;
        }
        RelayoutContents();
        MarkRenderContentDirty();
        return;
    }

    if (m_dragArmed && !m_dragging && m_dragPane >= 0) {
        if (std::hypot(pt.x - m_dragStart.x, pt.y - m_dragStart.y) >= kDragThreshold) {
            BeginDrag(m_dragPane, pt);
        }
    }
    if (m_dragging) {
        UpdateDrag(pt);
        return;
    }

    const HitResult prev = m_hover;
    m_hover = HitTestChrome(pt.x, pt.y);
    if (prev.part != m_hover.part || prev.paneIndex != m_hover.paneIndex
        || prev.groupPaneLocal != m_hover.groupPaneLocal || prev.side != m_hover.side) {
        MarkRenderContentDirty();
    }

    // VS-style chrome hover fade
    m_hoverBtnSide = m_hover.side;
    const bool pinHot = (m_hover.part == HitPart::Pin);
    const bool closeHot = (m_hover.part == HitPart::Close);
    const bool scrollL = (m_hover.part == HitPart::TabScrollLeft);
    const bool scrollR = (m_hover.part == HitPart::TabScrollRight);
    m_hoverPin.SetTarget(pinHot ? 1.0f : 0.0f);
    m_hoverClose.SetTarget(closeHot ? 1.0f : 0.0f);
    m_hoverScrollL.SetTarget(scrollL ? 1.0f : 0.0f);
    m_hoverScrollR.SetTarget(scrollR ? 1.0f : 0.0f);
    if (!AreAnimationsEnabled()) {
        m_hoverPin.Reset(pinHot ? 1.0f : 0.0f);
        m_hoverClose.Reset(closeHot ? 1.0f : 0.0f);
        m_hoverScrollL.Reset(scrollL ? 1.0f : 0.0f);
        m_hoverScrollR.Reset(scrollR ? 1.0f : 0.0f);
    } else {
        RequestAnimationTicks();
    }
}

void DockManager::OnMouseUp(Point pt) {
    UIElement::OnMouseUp(pt);
    if (m_activeSplitter >= 0) {
        m_activeSplitter = -1;
        return;
    }
    if (m_dragging) {
        EndDrag(pt);
        return;
    }
    m_dragArmed = false;
    m_dragPane = -1;
}

void DockManager::OnMouseLeave() {
    UIElement::OnMouseLeave();
    if (m_hover.part != HitPart::None) {
        m_hover = {};
        MarkRenderContentDirty();
    }
    m_hoverPin.SetTarget(0.0f);
    m_hoverClose.SetTarget(0.0f);
    m_hoverScrollL.SetTarget(0.0f);
    m_hoverScrollR.SetTarget(0.0f);
    RequestAnimationTicks();
}

HCURSOR DockManager::GetCursor() const {
    if (m_dragging) {
        return LoadCursor(nullptr, IDC_SIZEALL);
    }
    if (m_activeSplitter == 0 || m_activeSplitter == 1 || m_hover.part == HitPart::Splitter) {
        if (m_activeSplitter == 0 || m_activeSplitter == 1 || m_hover.splitter == 0 || m_hover.splitter == 1) {
            return LoadCursor(nullptr, IDC_SIZEWE);
        }
        return LoadCursor(nullptr, IDC_SIZENS);
    }
    return nullptr;
}

bool DockManager::HasSelfAnimation() const {
    if (m_dragging || m_guideOpacity.Current() > 0.01f || m_dropPulse.Current() > 0.01f) {
        return true;
    }
    auto busy = [](const SideChromeAnim& a) {
        return std::abs(a.underlineX.Current() - a.underlineX.Target()) > 0.2f
            || std::abs(a.underlineW.Current() - a.underlineW.Target()) > 0.2f
            || std::abs(a.contentFade.Current() - a.contentFade.Target()) > 0.01f;
    };
    return busy(m_animLeft) || busy(m_animRight) || busy(m_animTop) || busy(m_animBottom) || busy(m_animCenter)
        || std::abs(m_hoverPin.Current() - m_hoverPin.Target()) > 0.01f
        || std::abs(m_hoverClose.Current() - m_hoverClose.Target()) > 0.01f
        || std::abs(m_hoverScrollL.Current() - m_hoverScrollL.Target()) > 0.01f
        || std::abs(m_hoverScrollR.Current() - m_hoverScrollR.Target()) > 0.01f;
}

bool DockManager::OnAnimationTick() {
    bool any = UIElement::OnAnimationTick();
    const float dt = GetAnimationDeltaSeconds();

    m_guideOpacity.SetTarget(m_dragging ? 1.0f : 0.0f);
    any = m_guideOpacity.Tick(dt, GuideSpec()) || any;
    any = m_dropPulse.Tick(dt, FadeSpec()) || any;
    any = m_hoverPin.Tick(dt, HoverSpec()) || any;
    any = m_hoverClose.Tick(dt, HoverSpec()) || any;
    any = m_hoverScrollL.Tick(dt, HoverSpec()) || any;
    any = m_hoverScrollR.Tick(dt, HoverSpec()) || any;

    auto tickSide = [&](SideChromeAnim& a) {
        any = a.underlineX.Tick(dt, TabSpec()) || any;
        any = a.underlineW.Tick(dt, TabSpec()) || any;
        any = a.contentFade.Tick(dt, FadeSpec()) || any;
    };
    tickSide(m_animLeft);
    tickSide(m_animRight);
    tickSide(m_animTop);
    tickSide(m_animBottom);
    tickSide(m_animCenter);

    ApplyContentFadeOpacities();

    if (any || m_dragging || HasSelfAnimation()) {
        MarkRenderRectDirty(m_bounds);
        any = true;
    }
    return any;
}

void DockManager::DrawChromeButtonBg(GraphicsContext& ctx, const Rect& r, float hoverT, bool danger) const {
    if (r.IsEmpty() || hoverT <= 0.01f) {
        return;
    }
    const auto& tokens = ThemeManager::Instance().GetTokens();
    if (danger) {
        D2D1_COLOR_F bg = tokens.dangerColor;
        bg.a = 0.85f * hoverT;
        ctx.FillRect(r, bg);
    } else {
        const bool light = ThemeManager::Instance().GetThemeMode() == ThemeMode::Light;
        const float peak = light ? 0.10f : 0.18f;
        ctx.FillRect(r, D2D1::ColorF(
            tokens.textPrimary.r, tokens.textPrimary.g, tokens.textPrimary.b, peak * hoverT));
    }
}

void DockManager::DrawCloseGlyph(GraphicsContext& ctx, const Rect& r, D2D1_COLOR_F color) const {
    const float cx = r.x + r.width * 0.5f;
    const float cy = r.y + r.height * 0.5f;
    const float arm = 4.2f;
    ctx.DrawLine(Point(cx - arm, cy - arm), Point(cx + arm, cy + arm), color, 1.15f);
    ctx.DrawLine(Point(cx + arm, cy - arm), Point(cx - arm, cy + arm), color, 1.15f);
}

void DockManager::DrawPinGlyph(GraphicsContext& ctx, const Rect& r, D2D1_COLOR_F color, bool autoHide) const {
    const float cx = r.x + r.width * 0.5f;
    const float cy = r.y + r.height * 0.5f;
    if (!autoHide) {
        ctx.DrawLine(Point(cx, cy + 0.5f), Point(cx, cy + 5.5f), color, 1.2f);
        ctx.DrawLine(Point(cx - 3.5f, cy - 4.0f), Point(cx + 3.5f, cy - 4.0f), color, 1.15f);
        ctx.DrawLine(Point(cx - 3.5f, cy - 4.0f), Point(cx - 4.5f, cy - 0.5f), color, 1.1f);
        ctx.DrawLine(Point(cx + 3.5f, cy - 4.0f), Point(cx + 4.5f, cy - 0.5f), color, 1.1f);
        ctx.DrawLine(Point(cx - 4.5f, cy - 0.5f), Point(cx + 4.5f, cy - 0.5f), color, 1.15f);
    } else {
        ctx.DrawLine(Point(cx - 0.5f, cy), Point(cx - 5.5f, cy), color, 1.2f);
        ctx.DrawLine(Point(cx + 4.0f, cy - 3.5f), Point(cx + 4.0f, cy + 3.5f), color, 1.15f);
        ctx.DrawLine(Point(cx + 4.0f, cy - 3.5f), Point(cx + 0.5f, cy - 4.5f), color, 1.1f);
        ctx.DrawLine(Point(cx + 4.0f, cy + 3.5f), Point(cx + 0.5f, cy + 4.5f), color, 1.1f);
        ctx.DrawLine(Point(cx + 0.5f, cy - 4.5f), Point(cx + 0.5f, cy + 4.5f), color, 1.15f);
    }
}

void DockManager::DrawSlot(GraphicsContext& ctx, DockSide side, const SlotGeom& g) const {
    if (!g.visible) {
        return;
    }
    const auto& tokens = ThemeManager::Instance().GetTokens();
    const DockTabGroup& group = SlotGroup(side);
    const auto& anim = SideAnim(side);

    ctx.FillRect(g.outer, tokens.paneBackground);
    ctx.DrawRect(g.outer, tokens.cardBorder, 1.0f);
    ctx.FillRect(g.header, tokens.cardBackground);
    ctx.DrawLine(
        Point(g.header.x, g.header.y + g.header.height - 0.5f),
        Point(g.header.x + g.header.width, g.header.y + g.header.height - 0.5f),
        tokens.cardBorder,
        1.0f);

    const bool sideHover = (m_hoverBtnSide == side);
    const float scrollLT = sideHover ? m_hoverScrollL.Current() : 0.0f;
    const float scrollRT = sideHover ? m_hoverScrollR.Current() : 0.0f;
    const float pinT = sideHover ? m_hoverPin.Current() : 0.0f;
    const float closeT = sideHover ? m_hoverClose.Current() : 0.0f;
    if (g.showScroll) {
        DrawChromeButtonBg(ctx, g.scrollLeft, scrollLT, false);
        DrawChromeButtonBg(ctx, g.scrollRight, scrollRT, false);
        D2D1_COLOR_F cL = (scrollLT > 0.01f) ? tokens.textPrimary : tokens.textSecondary;
        D2D1_COLOR_F cR = (scrollRT > 0.01f) ? tokens.textPrimary : tokens.textSecondary;
        ctx.DrawChevron(g.scrollLeft, cL, GraphicsContext::ChevronDirection::Left, 1.4f);
        ctx.DrawChevron(g.scrollRight, cR, GraphicsContext::ChevronDirection::Right, 1.4f);
    }

    // Clip tabs to strip so overflow never paints over pin/close (VS behavior).
    if (!g.tabStrip.IsEmpty()) {
        ctx.PushClip(g.tabStrip);
        for (int i = 0; i < static_cast<int>(g.tabs.size()); ++i) {
            const Rect& tr = g.tabs[i];
            if (tr.x + tr.width <= g.tabStrip.x || tr.x >= g.tabStrip.x + g.tabStrip.width) {
                continue;
            }
            const bool sel = (i == group.selected);
            const bool hot = (m_hover.part == HitPart::Tab && m_hover.side == side && m_hover.groupPaneLocal == i);
            if (sel) {
                ctx.FillRect(tr, tokens.windowBackground);
            } else if (hot) {
                ctx.FillRect(tr, D2D1::ColorF(
                    tokens.textPrimary.r, tokens.textPrimary.g, tokens.textPrimary.b, 0.06f));
            }
            if (i + 1 < static_cast<int>(g.tabs.size())) {
                ctx.DrawLine(
                    Point(tr.x + tr.width - 0.5f, tr.y + 7.0f),
                    Point(tr.x + tr.width - 0.5f, tr.y + tr.height - 7.0f),
                    tokens.cardBorder,
                    1.0f);
            }

            const int paneIdx = group.paneIndices[i];
            const std::string& title = (paneIdx >= 0 && paneIdx < static_cast<int>(m_panes.size()))
                ? m_panes[paneIdx].title
                : "?";
            ctx.DrawText(
                title,
                Rect(tr.x + kTabPadX, tr.y, tr.width - kTabPadX * 2.0f, tr.height),
                sel ? tokens.textPrimary : tokens.textSecondary,
                "Segoe UI",
                12.0f,
                DWRITE_TEXT_ALIGNMENT_LEADING,
                DWRITE_PARAGRAPH_ALIGNMENT_CENTER,
                sel ? DWRITE_FONT_WEIGHT_SEMI_BOLD : DWRITE_FONT_WEIGHT_NORMAL,
                true);
        }
        if (!group.paneIndices.empty() && anim.underlineInited) {
            const float ux = anim.underlineX.Current();
            const float uw = (std::max)(8.0f, anim.underlineW.Current());
            ctx.FillRect(Rect(ux, g.tabStrip.y + g.tabStrip.height - 2.0f, uw, 2.0f), tokens.accentColor);
        }
        ctx.PopClip();
    }

    if (m_dropPulse.Current() > 0.02f && anim.contentFade.Current() < 0.99f) {
        D2D1_COLOR_F flash = tokens.accentColor;
        flash.a = 0.12f * m_dropPulse.Current();
        ctx.FillRect(g.outer, flash);
    }

    if (side != DockSide::Center && !group.paneIndices.empty() && !g.pinBtn.IsEmpty()) {
        DrawChromeButtonBg(ctx, g.pinBtn, pinT, false);
        const int sel = SelectedPaneOf(group);
        const bool ah = (sel >= 0 && sel < static_cast<int>(m_panes.size()) && m_panes[sel].autoHide);
        D2D1_COLOR_F icon = (pinT > 0.4f) ? tokens.textPrimary : tokens.textSecondary;
        DrawPinGlyph(ctx, g.pinBtn, icon, ah);
    }
    if (!group.paneIndices.empty() && !g.closeBtn.IsEmpty()) {
        DrawChromeButtonBg(ctx, g.closeBtn, closeT, true);
        D2D1_COLOR_F icon = (closeT > 0.4f) ? tokens.accentForeground : tokens.textSecondary;
        DrawCloseGlyph(ctx, g.closeBtn, icon);
    }

    if (side == DockSide::Center && group.paneIndices.empty()) {
        ctx.DrawText(
            "Documents",
            g.content,
            tokens.textSecondary,
            "Segoe UI",
            13.0f,
            DWRITE_TEXT_ALIGNMENT_CENTER,
            DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    }
}

void DockManager::DrawAutoHideStrips(GraphicsContext& ctx) const {
    if (m_autoHide.empty()) {
        return;
    }
    const auto& tokens = ThemeManager::Instance().GetTokens();
    int i = 0;
    for (const auto& it : m_autoHide) {
        if (it.paneIndex < 0 || it.paneIndex >= static_cast<int>(m_panes.size())) {
            ++i;
            continue;
        }
        Rect btn;
        const float t = m_geom.strip;
        switch (it.side) {
        case DockSide::Left:
            btn = Rect(m_bounds.x, m_bounds.y + 4.0f + i * 72.0f, t, 68.0f);
            break;
        case DockSide::Right:
            btn = Rect(m_bounds.x + m_bounds.width - t, m_bounds.y + 4.0f + i * 72.0f, t, 68.0f);
            break;
        case DockSide::Top:
            btn = Rect(m_bounds.x + 4.0f + i * 100.0f, m_bounds.y, 96.0f, t);
            break;
        case DockSide::Bottom:
            btn = Rect(m_bounds.x + 4.0f + i * 100.0f, m_bounds.y + m_bounds.height - t, 96.0f, t);
            break;
        default:
            ++i;
            continue;
        }
        ctx.FillRect(btn, tokens.cardBackground);
        ctx.DrawRect(btn, tokens.cardBorder, 1.0f);
        ctx.DrawText(
            m_panes[it.paneIndex].title,
            btn,
            tokens.textSecondary,
            "Segoe UI",
            10.0f,
            DWRITE_TEXT_ALIGNMENT_CENTER,
            DWRITE_PARAGRAPH_ALIGNMENT_CENTER,
            DWRITE_FONT_WEIGHT_NORMAL,
            true);
        ++i;
    }
}

void DockManager::DrawGuides(GraphicsContext& ctx) const {
    const float guideOp = m_guideOpacity.Current();
    if (guideOp <= 0.01f) {
        return;
    }
    const auto& tokens = ThemeManager::Instance().GetTokens();
    for (DockDropKind k : {
             DockDropKind::EdgeLeft, DockDropKind::EdgeRight, DockDropKind::EdgeTop,
             DockDropKind::EdgeBottom, DockDropKind::IntoCenter }) {
        const Rect r = GuideHot(k, m_bounds);
        const bool hot = (k == m_dropHighlight);
        D2D1_COLOR_F fill = tokens.accentColor;
        fill.a = (hot ? 0.55f : 0.28f) * guideOp;
        ctx.FillRoundedRect(r, 3.0f, fill);
        D2D1_COLOR_F stroke = tokens.accentColor;
        stroke.a *= guideOp;
        ctx.DrawRoundedRect(r, 3.0f, stroke, 1.0f);
    }
    if (m_dropHighlight != DockDropKind::None) {
        D2D1_COLOR_F preview = tokens.accentColor;
        preview.a = 0.16f * guideOp;
        Rect band = m_bounds;
        switch (m_dropHighlight) {
        case DockDropKind::EdgeLeft: band.width *= 0.28f; break;
        case DockDropKind::EdgeRight: band.x += band.width * 0.72f; band.width *= 0.28f; break;
        case DockDropKind::EdgeTop: band.height *= 0.28f; break;
        case DockDropKind::EdgeBottom: band.y += band.height * 0.72f; band.height *= 0.28f; break;
        case DockDropKind::IntoCenter:
            band = GuideHot(DockDropKind::IntoCenter, m_bounds);
            band = band.Inflate(80.0f);
            break;
        default: band = Rect(); break;
        }
        if (!band.IsEmpty()) {
            ctx.FillRect(band, preview);
        }
    }
}

void DockManager::DrawDragGhost(GraphicsContext& ctx) const {
    if (!m_dragging || m_dragPane < 0 || m_dragPane >= static_cast<int>(m_panes.size())) {
        return;
    }
    const auto& tokens = ThemeManager::Instance().GetTokens();
    Rect ghost(m_dragPt.x + 14.0f, m_dragPt.y + 14.0f, 168.0f, 30.0f);
    D2D1_COLOR_F fill = tokens.cardBackground;
    fill.a = 0.94f;
    ctx.FillRoundedRect(ghost, 4.0f, fill);
    ctx.DrawRoundedRect(ghost, 4.0f, tokens.accentColor, 1.5f);
    ctx.DrawText(
        m_panes[m_dragPane].title,
        Rect(ghost.x + 10.0f, ghost.y, ghost.width - 16.0f, ghost.height),
        tokens.textPrimary,
        "Segoe UI",
        12.0f,
        DWRITE_TEXT_ALIGNMENT_LEADING,
        DWRITE_PARAGRAPH_ALIGNMENT_CENTER,
        DWRITE_FONT_WEIGHT_SEMI_BOLD,
        true);
}

void DockManager::OnRender(GraphicsContext& ctx) {
    const auto& tokens = ThemeManager::Instance().GetTokens();
    ctx.FillRect(m_bounds, tokens.windowBackground);

    // VS: 1px visual splitter, larger invisible hit target.
    auto drawSplit = [&](const Rect& r) {
        if (r.IsEmpty()) return;
        ctx.FillRect(r, tokens.cardBorder);
    };
    drawSplit(m_geom.visSplitL);
    drawSplit(m_geom.visSplitR);
    drawSplit(m_geom.visSplitT);
    drawSplit(m_geom.visSplitB);

    DrawSlot(ctx, DockSide::Left, m_geom.left);
    DrawSlot(ctx, DockSide::Right, m_geom.right);
    DrawSlot(ctx, DockSide::Top, m_geom.top);
    DrawSlot(ctx, DockSide::Bottom, m_geom.bottom);
    DrawSlot(ctx, DockSide::Center, m_geom.center);
    DrawAutoHideStrips(ctx);

    ctx.DrawRect(m_bounds, tokens.cardBorder, 1.0f);
}

void DockManager::RenderOverlay(GraphicsContext& ctx) {
    DrawGuides(ctx);
    DrawDragGhost(ctx);
    UIElement::RenderOverlay(ctx);
}

bool DockManager::SaveLayout(const std::wstring& path) const {
    return DockLayoutSerializer::Save(*this, path);
}

bool DockManager::LoadLayout(const std::wstring& path) {
    return DockLayoutSerializer::Load(*this, path);
}

} // namespace CUI
