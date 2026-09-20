#pragma once
#include "DataPackage.h"
#include "../core/Value.h"

namespace CUI {

class GraphicsContext;
class UIElement;
class IDropTarget;

class IDragSource {
public:
    // 析构时自动中止仍在进行中的拖拽，避免 DragDropService 持有已销毁的源对象（悬垂指针）。
    virtual ~IDragSource();
    virtual DataPackage BeginDrag(Point pt) = 0;
    virtual DragDropEffects AllowedEffects() const {
        return DragDropEffects::Copy | DragDropEffects::Move;
    }
    virtual void OnDragCompleted(DragDropEffects effect, IDropTarget* target) {
        (void)effect;
        (void)target;
    }
};

class IDropTarget {
public:
    // 析构时自动中止仍在进行中的拖拽，避免 DragDropService 持有已销毁的放置目标（悬垂指针）。
    virtual ~IDropTarget();
    virtual DragDropEffects OnDragEnter(Point pt, const DataPackage& data, DragDropEffects allowed) {
        return OnDragOver(pt, data, allowed);
    }
    virtual DragDropEffects OnDragOver(Point pt, const DataPackage& data, DragDropEffects allowed) = 0;
    virtual void OnDragLeave() {}
    virtual bool OnDrop(Point pt, DataPackage& data, DragDropEffects effect) = 0;
    virtual Rect DropHighlightRect() const { return Rect(); }
};

class DragDropService {
public:
    static DragDropService* Current();
    static void ApplyCurrent(DragDropService* service);

    void BeginDrag(IDragSource* source, Point start, DataPackage package, DragDropEffects allowed);
    bool IsDragging() const { return m_dragging; }
    IDragSource* GetSource() const { return m_source; }
    DragDropEffects GetCurrentEffect() const { return m_effect; }
    const DataPackage& GetPackage() const { return m_package; }

    void Update(Point pt, UIElement* root);
    void CompleteDrop(Point pt, UIElement* root);
    void Cancel();
    // Explorer → app: hover (OLE DragOver) then drop.
    void BeginExternal(DataPackage package);
    void EndExternal();
    void DeliverExternal(Point pt, DataPackage package, UIElement* root);
    void AbortIfParticipant(IDragSource* source, IDropTarget* target);

    void RenderOverlay(GraphicsContext& ctx);
    void CollectDirty(Rect& dirtyRect, bool& hasDirty) const;
    static void PaintDropAccept(
        GraphicsContext& ctx,
        const Rect& bounds,
        float radius,
        DragDropEffects effect,
        bool drawBadge = true);

private:
    IDropTarget* FindTarget(Point pt, UIElement* root) const;
    DragDropEffects ProposeEffect(DragDropEffects allowed) const;
    void ApplyTarget(IDropTarget* target, Point pt);
    void DirtyGhost();
    Rect GhostRect() const;

    IDragSource* m_source = nullptr;
    IDropTarget* m_target = nullptr;
    DataPackage m_package;
    DragDropEffects m_allowed = DragDropEffects::None;
    DragDropEffects m_effect = DragDropEffects::None;
    Point m_pointer{};
    Rect m_lastGhost{};
    Rect m_lastHighlight{};
    float m_ghostWidth = 180.0f;
    bool m_dragging = false;
    bool m_external = false;

    static DragDropService* s_current;
};

// 定义在 DragDropService 声明之后：接口析构时解绑自身，杜绝拖拽进行中销毁元素导致的 UAF。
inline IDragSource::~IDragSource() {
    if (DragDropService* service = DragDropService::Current()) {
        service->AbortIfParticipant(this, nullptr);
    }
}

inline IDropTarget::~IDropTarget() {
    if (DragDropService* service = DragDropService::Current()) {
        service->AbortIfParticipant(nullptr, this);
    }
}

} // namespace CUI
