#include "ToastCenter.h"
#include "../animation/AnimationManager.h"
#include "../window/IWindowChrome.h"
#include <algorithm>
#include <cmath>

namespace CUI {

ToastCenter::ToastCenter() {
    SetVisibility(Visibility::Visible);
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

Rect ComputeToastAreaRect(UIElement* root) {
    Rect area;
    if (root) {
        area = root->GetBounds();
    }
    if (area.IsEmpty()) {
        return area;
    }
    // 自绘标题栏（WindowTitleBar）占据客户区顶部，Toast 绝不能覆盖标题文本
    // 或右上角的最小化/最大化/关闭按钮，因此可堆叠区域从标题栏下缘开始。
    for (const auto& child : root->GetChildren()) {
        if (!child) {
            continue;
        }
        if (auto* chrome = dynamic_cast<IWindowChrome*>(child.get())) {
            if (const UIElement* el = chrome->GetChromeElement()) {
                const Rect chromeBounds = el->GetBounds();
                if (chromeBounds.height > 0.0f
                    && chromeBounds.y >= area.y
                    && chromeBounds.y + chromeBounds.height <= area.y + area.height) {
                    const float skip = chromeBounds.y + chromeBounds.height - area.y;
                    area.y += skip;
                    area.height -= skip;
                }
            }
            break;
        }
    }
    return area;
}

Rect ToastCenter::GetToastAreaRect() const {
    UIElement* root = const_cast<ToastCenter*>(this);
    while (root && root->GetParent()) {
        root = root->GetParent();
    }
    return root ? ComputeToastAreaRect(root) : m_bounds;
}

void ToastCenter::Compact() {
    m_toasts.erase(std::remove_if(m_toasts.begin(), m_toasts.end(), [](const std::shared_ptr<Toast>& toast) {
        return !toast || !toast->IsAlive();
    }), m_toasts.end());
}

void ToastCenter::NotifyToastChanged() {
    RequestAnimationTicks();
    ScheduleAutoCloseWake();
}

void ToastCenter::ScheduleAutoCloseWake() {
    int earliestMs = -1;
    for (const auto& toast : m_toasts) {
        if (!toast) {
            continue;
        }
        const int remain = toast->GetAutoCloseRemainMs();
        if (remain < 0) {
            continue;
        }
        if (earliestMs < 0 || remain < earliestMs) {
            earliestMs = remain;
        }
    }

    AnimationManager* mgr = AnimationManager::Current();
    if (!mgr) {
        return;
    }
    if (earliestMs < 0) {
        mgr->CancelWake(this);
        return;
    }
    mgr->RequestWake(
        this,
        AnimationManager::clock::now() + std::chrono::milliseconds(earliestMs));
}

std::shared_ptr<Toast> ToastCenter::AddToast(const std::shared_ptr<Toast>& toast) {
    if (!toast) return nullptr;
    toast->SetHost(this);
    toast->Show();
    m_toasts.push_back(toast);
    NotifyToastChanged();
    return toast;
}

namespace {
ToastType AutoDetectTypeFromTitle(const std::string& title, ToastType defaultType) {
    if (defaultType != ToastType::Info) return defaultType;
    std::string t = title;
    std::transform(t.begin(), t.end(), t.begin(), ::tolower);
    if (t.find("success") != std::string::npos || title.find("成功") != std::string::npos || title.find("完成") != std::string::npos) {
        return ToastType::Success;
    }
    if (t.find("warning") != std::string::npos || t.find("warn") != std::string::npos || title.find("警告") != std::string::npos || title.find("提醒") != std::string::npos) {
        return ToastType::Warning;
    }
    if (t.find("error") != std::string::npos || title.find("错误") != std::string::npos || title.find("失败") != std::string::npos || title.find("异常") != std::string::npos) {
        return ToastType::Error;
    }
    return defaultType;
}
}

std::shared_ptr<Toast> ToastCenter::ShowToast(const std::string& title, const std::string& message, ToastType type, ToastCorner corner, int durationMs) {
    auto toast = std::make_shared<Toast>();
    toast->SetTitle(title);
    toast->SetMessage(message);
    toast->SetType(AutoDetectTypeFromTitle(title, type));
    toast->SetCorner(corner);
    toast->SetDurationMs(durationMs);
    return AddToast(toast);
}

std::shared_ptr<Toast> ToastCenter::ShowToast(const std::string& title, const std::string& message, ToastCorner corner, int durationMs) {
    return ShowToast(title, message, ToastType::Info, corner, durationMs);
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
    NotifyToastChanged();
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

std::shared_ptr<Toast> ToastCenter::Show(UIElement* root, const std::string& title, const std::string& message, ToastType type, ToastCorner corner, int durationMs) {
    auto center = Ensure(root);
    if (!center) return nullptr;
    return center->ShowToast(title, message, type, corner, durationMs);
}

std::shared_ptr<Toast> ToastCenter::Show(UIElement* root, const std::string& title, const std::string& message, ToastCorner corner, int durationMs) {
    return Show(root, title, message, ToastType::Info, corner, durationMs);
}

void ToastCenter::OnRenderOverlay(GraphicsContext& ctx) {
    Compact();
    if (m_toasts.empty()) return;

    Rect windowRect = GetToastAreaRect();
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
        if (toast && toast->OnAnimationTick()) {
            active = true;
        }
    }
    Compact();
    ScheduleAutoCloseWake();
    if (active) {
        RequestAnimationTicks();
    }
    return active;
}

void ToastCenter::CollectSelfAnimationBounds(Rect& dirtyRect, bool& hasDirty) const {
    for (const auto& toast : m_toasts) {
        if (!toast || !toast->HasSelfAnimation()) {
            continue;
        }

        Rect bounds = toast->GetRenderBounds();
        if (bounds.IsEmpty()) {
            bounds = toast->CalculateBounds(GetToastAreaRect(), 0);
        }
        bounds = bounds.Inflate(8.0f);
        dirtyRect = hasDirty ? dirtyRect.Union(bounds) : bounds;
        hasDirty = true;
    }
}

void ToastCenter::CollectAnimationBounds(Rect& dirtyRect, bool& hasDirty) const {
    CollectSelfAnimationBounds(dirtyRect, hasDirty);
}

} // namespace CUI
