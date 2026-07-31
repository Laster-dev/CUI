#include "ToastCenter.h"
#include <algorithm>

namespace CUI {

ToastCenter::ToastCenter() {
    SetProperty("visibility", Value("Visible"));
    SetProperty("isHitTestVisible", Value(true));
}

Size ToastCenter::Measure(Size availableSize) {
    m_desiredSize = Size(0.0f, 0.0f);
    return m_desiredSize;
}

void ToastCenter::Arrange(Rect finalRect) {
    m_bounds = finalRect;
}

Rect ToastCenter::GetWindowRect() const {
    UIElement* root = const_cast<ToastCenter*>(this);
    while (root && root->GetParent()) {
        root = root->GetParent();
    }
    return root ? root->GetBounds() : m_bounds;
}

void ToastCenter::Compact() {
    m_toasts.erase(std::remove_if(m_toasts.begin(), m_toasts.end(), [](const std::shared_ptr<Toast>& toast) {
        return !toast || !toast->IsAlive();
    }), m_toasts.end());
}

std::shared_ptr<Toast> ToastCenter::AddToast(const std::shared_ptr<Toast>& toast) {
    if (!toast) return nullptr;
    toast->Show();
    m_toasts.push_back(toast);
    return toast;
}

std::shared_ptr<Toast> ToastCenter::ShowToast(const std::string& title, const std::string& message, ToastCorner corner, int durationMs) {
    auto toast = std::make_shared<Toast>();
    toast->SetTitle(title);
    toast->SetMessage(message);
    toast->SetCorner(corner);
    toast->SetDurationMs(durationMs);
    return AddToast(toast);
}

std::shared_ptr<Toast> ToastCenter::ShowFromTemplate(const UIElement* toastTemplate,
    const std::string& titleOverride,
    const std::string& messageOverride) {
    auto toast = std::make_shared<Toast>();
    if (toastTemplate) {
        toast->ApplyFrom(toastTemplate);
    }
    if (!titleOverride.empty()) toast->SetTitle(titleOverride);
    if (!messageOverride.empty()) toast->SetMessage(messageOverride);
    return AddToast(toast);
}

void ToastCenter::DismissAll() {
    for (auto& toast : m_toasts) {
        if (toast) toast->Hide();
    }
}

size_t ToastCenter::GetActiveCount() const {
    size_t count = 0;
    for (const auto& toast : m_toasts) {
        if (toast && toast->IsAlive()) ++count;
    }
    return count;
}

std::shared_ptr<ToastCenter> ToastCenter::Ensure(UIElement* root) {
    if (!root) return nullptr;
    auto existing = std::dynamic_pointer_cast<ToastCenter>(root->FindElementById("toastCenter"));
    if (existing) return existing;

    for (const auto& child : root->GetChildren()) {
        if (auto center = std::dynamic_pointer_cast<ToastCenter>(child)) {
            if (center->GetId().empty()) center->SetId("toastCenter");
            return center;
        }
    }

    auto center = std::make_shared<ToastCenter>();
    center->SetId("toastCenter");
    root->AddChild(center);
    return center;
}

std::shared_ptr<Toast> ToastCenter::Show(UIElement* root, const std::string& title, const std::string& message, ToastCorner corner, int durationMs) {
    auto center = Ensure(root);
    if (!center) return nullptr;
    return center->ShowToast(title, message, corner, durationMs);
}

void ToastCenter::OnRenderOverlay(GraphicsContext& ctx) {
    Compact();
    if (m_toasts.empty()) return;

    Rect windowRect = GetWindowRect();
    const float gap = 10.0f;

    float topLeftY = windowRect.y + 18.0f;
    float topRightY = windowRect.y + 18.0f;
    float bottomLeftY = windowRect.y + windowRect.height - 18.0f;
    float bottomRightY = windowRect.y + windowRect.height - 18.0f;

    // Newest toasts render on top of the stack (nearest to the corner origin).
    for (auto it = m_toasts.rbegin(); it != m_toasts.rend(); ++it) {
        auto& toast = *it;
        if (!toast || !toast->IsAlive()) continue;

        Rect bounds = toast->CalculateBounds(windowRect, 0);
        float toastHeight = bounds.height;
        float stackGap = gap + toast->GetSpacing();
        ToastCorner corner = toast->GetCorner();

        switch (corner) {
        case ToastCorner::TopLeft:
            bounds.y = topLeftY;
            topLeftY += toastHeight + stackGap;
            break;
        case ToastCorner::TopRight:
            bounds.y = topRightY;
            topRightY += toastHeight + stackGap;
            break;
        case ToastCorner::BottomLeft:
            bounds.y = bottomLeftY - toastHeight;
            bottomLeftY -= toastHeight + stackGap;
            break;
        case ToastCorner::BottomRight:
            bounds.y = bottomRightY - toastHeight;
            bottomRightY -= toastHeight + stackGap;
            break;
        }

        float sx = toast->GetCurrentSlideX();
        float sy = toast->GetCurrentSlideY();
        if (corner == ToastCorner::TopLeft || corner == ToastCorner::BottomLeft) sx = -sx;
        if (corner == ToastCorner::BottomLeft || corner == ToastCorner::BottomRight) sy = -sy;

        toast->RenderContent(ctx, bounds, toast->GetCurrentOpacity(), sx, sy);
    }
}

UIElement* ToastCenter::OnHitTestOverlay(float x, float y) {
    for (auto it = m_toasts.rbegin(); it != m_toasts.rend(); ++it) {
        auto& toast = *it;
        if (!toast || !toast->IsAlive()) continue;
        if (toast->GetRenderBounds().Contains(x, y)) return toast.get();
    }
    return nullptr;
}

bool ToastCenter::OnAnimationTick() {
    bool active = false;
    for (auto& toast : m_toasts) {
        if (toast && toast->OnAnimationTick()) active = true;
    }
    Compact();
    return active;
}

void ToastCenter::CollectAnimationBounds(Rect& dirtyRect, bool& hasDirty) const {
    for (const auto& toast : m_toasts) {
        if (!toast || !toast->HasSelfAnimation()) {
            continue;
        }

        Rect bounds = toast->GetRenderBounds();
        if (bounds.IsEmpty()) {
            bounds = toast->CalculateBounds(GetWindowRect(), 0);
        }
        bounds = bounds.Inflate(8.0f);
        dirtyRect = hasDirty ? dirtyRect.Union(bounds) : bounds;
        hasDirty = true;
    }
}

} // namespace CUI
