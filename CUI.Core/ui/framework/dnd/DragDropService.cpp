#include "DragDropService.h"
#include "../controls/UIElement.h"
#include "../render/GraphicsContext.h"
#include "../style/ThemeManager.h"
#include "../window/Window.h"
#include <windows.h>
#include <algorithm>
#include <cmath>

namespace CUI {

DragDropService* DragDropService::s_current = nullptr;

DragDropService* DragDropService::Current() {
    return s_current;
}

void DragDropService::SetCurrent(DragDropService* service) {
    s_current = service;
}

static const char* EffectLabel(DragDropEffects effect) {
    if (effect == DragDropEffects::Copy) return "复制";
    if (effect == DragDropEffects::Move) return "移动";
    if (effect == DragDropEffects::Link) return "链接";
    return "不可放置";
}

DragDropEffects DragDropService::ProposeEffect(DragDropEffects allowed) const {
    const bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
    if (ctrl && HasEffect(allowed, DragDropEffects::Copy)) {
        return DragDropEffects::Copy;
    }
    if (HasEffect(allowed, DragDropEffects::Move)) {
        return DragDropEffects::Move;
    }
    if (HasEffect(allowed, DragDropEffects::Copy)) {
        return DragDropEffects::Copy;
    }
    if (HasEffect(allowed, DragDropEffects::Link)) {
        return DragDropEffects::Link;
    }
    return DragDropEffects::None;
}

static CUI::IDropTarget* AsDropTarget(UIElement* e) {
    return dynamic_cast<CUI::IDropTarget*>(e);
}

static CUI::IDropTarget* FindDeepestDropTarget(UIElement* e, Point pt, int depth, int& bestDepth) {
    if (!e || e->GetVisibility() != Visibility::Visible) {
        return nullptr;
    }
    CUI::IDropTarget* best = nullptr;
    if (CUI::IDropTarget* target = AsDropTarget(e)) {
        Rect hl = target->DropHighlightRect();
        if (hl.IsEmpty()) {
            hl = e->GetBounds();
        }
        if (hl.Contains(pt.x, pt.y)) {
            best = target;
            bestDepth = depth;
        }
    }
    for (const auto& child : e->GetChildren()) {
        int childBest = bestDepth;
        if (CUI::IDropTarget* nested = FindDeepestDropTarget(child.get(), pt, depth + 1, childBest)) {
            if (childBest >= bestDepth) {
                best = nested;
                bestDepth = childBest;
            }
        }
    }
    return best;
}

IDropTarget* DragDropService::FindTarget(Point pt, UIElement* root) const {
    if (!root) {
        return nullptr;
    }
    UIElement* hit = root->HitTest(pt.x, pt.y);
    if (!hit) {
        hit = root->HitTestOverlay(pt.x, pt.y);
    }
    for (UIElement* e = hit; e; e = e->GetParent()) {
        if (CUI::IDropTarget* target = AsDropTarget(e)) {
            return target;
        }
    }
    int bestDepth = -1;
    return FindDeepestDropTarget(root, pt, 0, bestDepth);
}

void DragDropService::SetTarget(IDropTarget* target, Point pt) {
    if (m_target == target) {
        if (m_target) {
            m_effect = m_target->OnDragOver(pt, m_package, ProposeEffect(m_allowed));
            if (!HasEffect(m_allowed, m_effect)) {
                m_effect = DragDropEffects::None;
            }
        }
        return;
    }
    if (m_target) {
        m_target->OnDragLeave();
    }
    m_target = target;
    if (m_target) {
        m_effect = m_target->OnDragEnter(pt, m_package, ProposeEffect(m_allowed));
        if (!HasEffect(m_allowed, m_effect)) {
            m_effect = DragDropEffects::None;
        }
    } else {
        m_effect = DragDropEffects::None;
    }
}

void DragDropService::DirtyGhost() {
    if (Window* w = Window::Current()) {
        w->InvalidateDragFeedback();
    }
}

Rect DragDropService::GhostRect() const {
    const float w = m_ghostWidth > 40.0f ? m_ghostWidth : 180.0f;
    return Rect(m_pointer.x + 14.0f, m_pointer.y + 16.0f, w, 32.0f);
}

void DragDropService::AbortIfParticipant(IDragSource* source, IDropTarget* target) {
    if (!m_dragging) {
        return;
    }
    const bool involved = (source && m_source == source) || (target && m_target == target);
    if (!involved) {
        return;
    }
    if (source && m_source == source) {
        m_source = nullptr;
    }
    if (target && m_target == target) {
        m_target = nullptr;
    }
    Cancel();
}

void DragDropService::BeginDrag(IDragSource* source, Point start, DataPackage package, DragDropEffects allowed) {
    Cancel();
    m_source = source;
    m_package = std::move(package);
    m_allowed = allowed;
    m_pointer = start;
    m_dragging = true;
    m_external = false;
    m_effect = ProposeEffect(m_allowed);
    m_lastGhost = GhostRect();
    DirtyGhost();
}

void DragDropService::BeginExternal(DataPackage package) {
    if (m_dragging && !m_external) {
        Cancel();
    }
    m_source = nullptr;
    m_package = std::move(package);
    m_allowed = DragDropEffects::Copy;
    m_dragging = true;
    m_external = true;
    m_effect = DragDropEffects::Copy;
}

void DragDropService::EndExternal() {
    if (!m_external) {
        return;
    }
    if (m_target) {
        m_target->OnDragLeave();
    }
    m_target = nullptr;
    m_source = nullptr;
    m_dragging = false;
    m_external = false;
    m_effect = DragDropEffects::None;
    DirtyGhost();
    m_lastGhost = Rect();
    m_lastHighlight = Rect();
}

void DragDropService::Update(Point pt, UIElement* root) {
    if (!m_dragging) {
        return;
    }
    DirtyGhost();
    m_pointer = pt;
    SetTarget(FindTarget(pt, root), pt);
    if (!m_external) {
        SetCursor(LoadCursor(nullptr, m_effect == DragDropEffects::None ? IDC_NO : IDC_ARROW));
    }
    DirtyGhost();
}

void DragDropService::CompleteDrop(Point pt, UIElement* root) {
    if (!m_dragging) {
        return;
    }
    m_pointer = pt;
    SetTarget(FindTarget(pt, root), pt);
    IDropTarget* droppedOn = m_target;
    DragDropEffects effect = m_effect;
    bool accepted = false;
    if (droppedOn && effect != DragDropEffects::None) {
        accepted = droppedOn->OnDrop(pt, m_package, effect);
        droppedOn->OnDragLeave();
    } else if (droppedOn) {
        droppedOn->OnDragLeave();
    }
    IDragSource* source = m_source;
    m_target = nullptr;
    m_dragging = false;
    m_external = false;
    m_source = nullptr;
    DirtyGhost();
    m_lastGhost = Rect();
    m_lastHighlight = Rect();
    if (source) {
        source->OnDragCompleted(accepted ? effect : DragDropEffects::None, accepted ? droppedOn : nullptr);
    }
    m_effect = DragDropEffects::None;
}

void DragDropService::Cancel() {
    if (!m_dragging) {
        m_source = nullptr;
        m_target = nullptr;
        m_effect = DragDropEffects::None;
        return;
    }
    if (m_target) {
        m_target->OnDragLeave();
    }
    IDragSource* source = m_source;
    m_target = nullptr;
    m_dragging = false;
    m_external = false;
    m_source = nullptr;
    DirtyGhost();
    m_lastGhost = Rect();
    m_lastHighlight = Rect();
    if (source) {
        source->OnDragCompleted(DragDropEffects::None, nullptr);
    }
    m_effect = DragDropEffects::None;
}

void DragDropService::DeliverExternal(Point pt, DataPackage package, UIElement* root) {
    if (m_dragging) {
        Cancel();
    }
    m_package = std::move(package);
    m_allowed = DragDropEffects::Copy;
    IDropTarget* target = FindTarget(pt, root);
    if (!target) {
        return;
    }
    const DragDropEffects proposed = ProposeEffect(m_allowed);
    const DragDropEffects effect = target->OnDragEnter(pt, m_package, proposed);
    if (effect != DragDropEffects::None && HasEffect(m_allowed, effect)) {
        target->OnDrop(pt, m_package, effect);
    }
    target->OnDragLeave();
}

void DragDropService::CollectDirty(Rect& dirtyRect, bool& hasDirty) const {
    if (!m_dragging) {
        return;
    }
    Rect r = GhostRect();
    if (m_lastGhost.width > 0.0f) {
        r = r.Union(m_lastGhost);
    }
    if (m_target) {
        const Rect hl = m_target->DropHighlightRect();
        if (hl.width > 0.0f) {
            r = r.Union(hl);
        }
    }
    if (m_lastHighlight.width > 0.0f) {
        r = r.Union(m_lastHighlight);
    }
    if (!hasDirty) {
        dirtyRect = r;
        hasDirty = true;
    } else {
        dirtyRect = dirtyRect.Union(r);
    }
}

void DragDropService::PaintDropAccept(
    GraphicsContext& ctx,
    const Rect& bounds,
    float radius,
    DragDropEffects effect,
    bool drawBadge) {
    if (bounds.IsEmpty()) {
        return;
    }
    auto& theme = ThemeManager::Instance();
    const auto accent = theme.GetColor(ThemeTokenId::AccentColor);
    const auto onAccent = theme.GetColor(ThemeTokenId::AccentForeground);
    const float r = radius > 0.0f ? radius : 6.0f;
    ctx.FillRoundedRect(bounds, r, D2D1::ColorF(accent.r, accent.g, accent.b, 0.22f));
    ctx.DrawRoundedRect(bounds, r, D2D1::ColorF(accent.r, accent.g, accent.b, 1.0f), 2.5f);
    const Rect inner = bounds.Inflate(-3.5f);
    if (inner.width > 10.0f && inner.height > 10.0f) {
        ctx.DrawRoundedRect(
            inner,
            (std::max)(1.0f, r - 2.0f),
            D2D1::ColorF(accent.r, accent.g, accent.b, 0.55f),
            1.0f);
    }
    if (!drawBadge) {
        return;
    }
    const char* badge = EffectLabel(effect);
    const float badgeW = effect == DragDropEffects::None ? 68.0f : 44.0f;
    const Rect badgeRect(
        bounds.x + 8.0f,
        bounds.y + 6.0f,
        badgeW,
        20.0f);
    ctx.FillRoundedRect(badgeRect, 4.0f, D2D1::ColorF(accent.r, accent.g, accent.b, 0.98f));
    ctx.DrawText(
        badge,
        badgeRect,
        onAccent,
        "微软雅黑",
        10.0f,
        DWRITE_TEXT_ALIGNMENT_CENTER);
}

void DragDropService::RenderOverlay(GraphicsContext& ctx) {
    if (!m_dragging) {
        return;
    }

    auto& theme = ThemeManager::Instance();
    if (m_target && m_effect != DragDropEffects::None) {
        const Rect hl = m_target->DropHighlightRect().Inflate(2.0f);
        if (hl.width > 0.0f && hl.height > 0.0f) {
            PaintDropAccept(ctx, hl, 8.0f, m_effect, true);
            m_lastHighlight = hl;
        }
    } else {
        m_lastHighlight = Rect();
    }

    const auto card = theme.GetColor(ThemeTokenId::CardBackground);
    const auto border = theme.GetColor(ThemeTokenId::CardBorder);
    const auto text = theme.GetColor(ThemeTokenId::TextPrimary);
    const auto accent = theme.GetColor(ThemeTokenId::AccentColor);
    const auto onAccent = theme.GetColor(ThemeTokenId::AccentForeground);
    const float alpha = m_effect == DragDropEffects::None ? 0.55f : 0.94f;
    const std::string label = m_package.PreviewLabel();
    const char* badge = EffectLabel(m_effect);
    const float badgeW = m_effect == DragDropEffects::None ? 68.0f : 40.0f;
    const Size labelSize = ctx.MeasureText(label, "微软雅黑", 12.0f);
    m_ghostWidth = (std::min)(300.0f, (std::max)(120.0f, labelSize.width + badgeW + 28.0f));
    const Rect ghost = GhostRect();

    ctx.FillRoundedRect(ghost, 6.0f, D2D1::ColorF(card.r, card.g, card.b, alpha));
    ctx.DrawRoundedRect(ghost, 6.0f, D2D1::ColorF(border.r, border.g, border.b, alpha), 1.0f);

    ctx.DrawText(
        label,
        Rect(ghost.x + 10.0f, ghost.y, ghost.width - badgeW - 16.0f, ghost.height),
        text,
        "微软雅黑",
        12.0f);

    const Rect badgeRect(ghost.x + ghost.width - badgeW - 6.0f, ghost.y + 6.0f, badgeW, 20.0f);
    const auto badgeFill = m_effect == DragDropEffects::None
        ? D2D1::ColorF(0.45f, 0.18f, 0.18f, 0.9f)
        : D2D1::ColorF(accent.r, accent.g, accent.b, 0.95f);
    ctx.FillRoundedRect(badgeRect, 4.0f, badgeFill);
    ctx.DrawText(
        badge,
        badgeRect,
        onAccent,
        "微软雅黑",
        10.0f,
        DWRITE_TEXT_ALIGNMENT_CENTER);
    m_lastGhost = ghost;
}

} // namespace CUI
