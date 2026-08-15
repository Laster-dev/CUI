#include "Toast.h"
#include "ToastCenter.h"
#include "../animation/AnimationManager.h"
#include "../style/ThemeManager.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace CUI {

static float Clamp01(float value) {
    return (std::max)(0.0f, (std::min)(1.0f, value));
}

static float EaseOutCubic(float value) {
    float inv = 1.0f - Clamp01(value);
    return 1.0f - inv * inv * inv;
}

static std::string ThemeHex(const std::string& tokenName) {
    D2D1_COLOR_F c = ThemeManager::Instance().GetFlatColor(tokenName);
    char buf[16];
    sprintf_s(buf, "#%02X%02X%02X",
        static_cast<int>(Clamp01(c.r) * 255.0f + 0.5f),
        static_cast<int>(Clamp01(c.g) * 255.0f + 0.5f),
        static_cast<int>(Clamp01(c.b) * 255.0f + 0.5f));
    return std::string(buf);
}

static std::string ResolveThemeColorHex(const UIElement* element, PropertyId tokenPropId, const std::string& fallback) {
    if (!element || tokenPropId == PropertyId::None || !element->HasProperty(tokenPropId)) {
        return fallback;
    }
    const std::string tokenName = element->GetProperty(tokenPropId).AsString();
    return tokenName.empty() ? fallback : ThemeHex(tokenName);
}


const char* Toast::CornerToString(ToastCorner corner) {
    switch (corner) {
    case ToastCorner::TopLeft: return "TopLeft";
    case ToastCorner::TopRight: return "TopRight";
    case ToastCorner::BottomLeft: return "BottomLeft";
    default: return "BottomRight";
    }
}

ToastCorner Toast::ParseCorner(const std::string& corner, ToastCorner fallback) {
    if (corner == "TopLeft") return ToastCorner::TopLeft;
    if (corner == "TopRight") return ToastCorner::TopRight;
    if (corner == "BottomLeft") return ToastCorner::BottomLeft;
    if (corner == "BottomRight") return ToastCorner::BottomRight;
    return fallback;
}

const char* Toast::TypeToString(ToastType type) {
    switch (type) {
    case ToastType::Success: return "Success";
    case ToastType::Warning: return "Warning";
    case ToastType::Error: return "Error";
    default: return "Info";
    }
}

ToastType Toast::ParseType(const std::string& typeStr, ToastType fallback) {
    if (typeStr == "Success") return ToastType::Success;
    if (typeStr == "Warning" || typeStr == "Warn") return ToastType::Warning;
    if (typeStr == "Error") return ToastType::Error;
    if (typeStr == "Info") return ToastType::Info;
    return fallback;
}

void Toast::SetType(ToastType type) {
    m_type = type;
    const char* accentToken = "accentColor";
    switch (type) {
    case ToastType::Error:
        accentToken = "dangerColor";
        break;
    case ToastType::Success:
    case ToastType::Warning:
    case ToastType::Info:
    default:
        accentToken = "accentColor";
        break;
    }
    m_accent = ThemeHex(accentToken);
    SetAccentColorToken(ThemeTokenIdFromName(accentToken));
}

Toast::Toast() {
    m_background = ThemeHex("cardBackground");
    m_accent = ThemeHex("accentColor");
    m_titleColor = ThemeHex("textPrimary");
    m_messageColor = ThemeHex("textSecondary");
    SetVisibility(Visibility::Visible);
    SetOpacity(1.0f);
    SetWidth(m_width);
    SetBackgroundToken(ThemeTokenId::CardBackground);
    SetAccentColorToken(ThemeTokenId::AccentColor);
    SetTitleColorToken(ThemeTokenId::TextPrimary);
    SetMessageColorToken(ThemeTokenId::TextSecondary);
    SetBackground(m_background);

    m_txtTitle = std::make_shared<TextBlock>(m_titleText);
    m_txtTitle->SetFontSize(15.0f);
    m_txtTitle->SetFontWeight(CUI::FontWeight::Bold);
    m_txtTitle->SetColorToken(ThemeTokenId::TextPrimary);
    m_txtTitle->SetColor(Value::ParseColor(m_titleColor));

    m_txtMessage = std::make_shared<TextBlock>(m_messageText);
    m_txtMessage->SetFontSize(12.5f);
    m_txtMessage->SetColorToken(ThemeTokenId::TextSecondary);
    m_txtMessage->SetColor(Value::ParseColor(m_messageColor));

    AddChild(m_txtTitle);
    AddChild(m_txtMessage);
}

Toast::~Toast() {
    if (AnimationManager* mgr = AnimationManager::Current()) {
        mgr->CancelWake(this);
    }
    m_host = nullptr;
}

void Toast::RequestHostTicks() {
    if (m_host) {
        m_host->NotifyToastChanged();
    }
}

int Toast::GetAutoCloseRemainMs() const {
    if (!m_autoClose || m_durationMs <= 0 || m_isHovering || m_state != 2) {
        return -1;
    }
    auto now = std::chrono::steady_clock::now();
    float elapsedMs = static_cast<float>(
        std::chrono::duration_cast<std::chrono::milliseconds>(now - m_stateTime).count());
    float remain = static_cast<float>(m_durationMs) - elapsedMs;
    if (remain <= 0.0f) {
        return 0;
    }
    return static_cast<int>(std::ceil(remain));
}

Value Toast::GetProperty(PropertyId id) const {
    switch (id) {
    case PropertyId::Title: return Value(m_titleText);
    case PropertyId::Message: return Value(m_messageText);
    case PropertyId::Corner: return Value(CornerToString(m_corner));
    case PropertyId::DurationMs: return Value(static_cast<float>(m_durationMs));
    case PropertyId::AutoClose: return Value(m_autoClose);
    case PropertyId::Closeable: return Value(m_closeable);
    case PropertyId::OffsetX: return Value(m_offsetX);
    case PropertyId::OffsetY: return Value(m_offsetY);
    case PropertyId::Spacing: return Value(m_spacing);
    default: return UIElement::GetProperty(id);
    }
}

bool Toast::HasProperty(PropertyId id) const {
    switch (id) {
    case PropertyId::Title:
    case PropertyId::Message:
    case PropertyId::Corner:
    case PropertyId::DurationMs:
    case PropertyId::AutoClose:
    case PropertyId::Closeable:
    case PropertyId::OffsetX:
    case PropertyId::OffsetY:
    case PropertyId::Spacing:
        return true;
    default:
        return UIElement::HasProperty(id);
    }
}

void Toast::SetProperty(PropertyId id, const Value& val) {
    switch (id) {
    case PropertyId::Title: SetTitle(val.AsString()); return;
    case PropertyId::Message: SetMessage(val.AsString()); return;
    case PropertyId::Corner: SetCorner(ParseCorner(val.AsString(), m_corner)); return;
    case PropertyId::DurationMs: SetDurationMs(static_cast<int>(val.AsFloat(static_cast<float>(m_durationMs)))); return;
    case PropertyId::AutoClose: SetAutoClose(val.AsBool()); return;
    case PropertyId::Closeable: SetCloseable(val.AsBool()); return;
    case PropertyId::OffsetX: SetOffsetX(val.AsFloat(m_offsetX)); return;
    case PropertyId::OffsetY: SetOffsetY(val.AsFloat(m_offsetY)); return;
    case PropertyId::Spacing: SetSpacing(val.AsFloat(m_spacing)); return;
    default: UIElement::SetProperty(id, val); return;
    }
}

void Toast::SyncMembersFromProperties() {
    // Colors may follow a theme token; re-resolve from the token if one is set.
    m_background = ResolveThemeColorHex(this, PropertyId::BackgroundToken, m_background);
    m_accent = ResolveThemeColorHex(this, PropertyId::AccentColorToken, m_accent);
    m_titleColor = ResolveThemeColorHex(this, PropertyId::TitleColorToken, m_titleColor);
    m_messageColor = ResolveThemeColorHex(this, PropertyId::MessageColorToken, m_messageColor);
    if (GetWidth() >= 0.0f) {
        m_width = GetWidth();
    }
}

void Toast::UpdateTextElements() {
    SyncMembersFromProperties();
    if (m_txtTitle) {
        m_txtTitle->SetText(m_titleText);
        m_txtTitle->SetColor(Value::ParseColor(m_titleColor));
    }
    if (m_txtMessage) {
        m_txtMessage->SetText(m_messageText);
        m_txtMessage->SetColor(Value::ParseColor(m_messageColor));
    }
}

void Toast::ApplyFrom(const UIElement* source) {
    if (!source) return;
    if (const Toast* srcToast = dynamic_cast<const Toast*>(source)) {
        SetTitle(srcToast->m_titleText);
        SetMessage(srcToast->m_messageText);
        SetCorner(srcToast->m_corner);
        SetDurationMs(srcToast->m_durationMs);
        SetAutoClose(srcToast->m_autoClose);
        SetCloseable(srcToast->m_closeable);
        SetWidth(srcToast->m_width);
        SetOffsetX(srcToast->m_offsetX);
        SetOffsetY(srcToast->m_offsetY);
        SetSpacing(srcToast->m_spacing);
        SetBackground(srcToast->m_background);
        SetAccent(srcToast->m_accent);
        SetTitleColor(srcToast->m_titleColor);
        SetMessageColor(srcToast->m_messageColor);
    }
    UpdateTextElements();
}

D2D1_COLOR_F Toast::ColorWithAlpha(const std::string& color, float alpha) {
    D2D1_COLOR_F c = Value::ParseColor(color);
    c.a = Clamp01(alpha) * c.a;
    return c;
}

void Toast::SetTitle(const std::string& title) {
    m_titleText = title;
    UpdateTextElements();
    if (IsLayerPromoted()) {
        GetRenderNode().GetLayer().Invalidate(RenderLayer::ContentDirty);
    }
}
void Toast::SetMessage(const std::string& message) {
    m_messageText = message;
    UpdateTextElements();
    if (IsLayerPromoted()) {
        GetRenderNode().GetLayer().Invalidate(RenderLayer::ContentDirty);
    }
}
void Toast::SetCorner(ToastCorner corner) { m_corner = corner; }
void Toast::SetDurationMs(int durationMs) { m_durationMs = durationMs; }
void Toast::SetWidth(float width) { m_width = (std::max)(180.0f, width); UIElement::SetWidth(m_width); }
void Toast::SetAutoClose(bool enabled) { m_autoClose = enabled; }
void Toast::SetBackground(const std::string& color) { m_background = color; SetBackgroundToken(ThemeTokenIdFromName("")); UIElement::SetBackground(Value::ParseColor(color)); }
void Toast::SetAccent(const std::string& color) { m_accent = color; SetAccentColorToken(ThemeTokenIdFromName("")); }
void Toast::SetTitleColor(const std::string& color) { m_titleColor = color; SetTitleColorToken(ThemeTokenIdFromName("")); UpdateTextElements(); }
void Toast::SetMessageColor(const std::string& color) { m_messageColor = color; SetMessageColorToken(ThemeTokenIdFromName("")); UpdateTextElements(); }
void Toast::SetOffsetX(float offsetX) { m_offsetX = offsetX; }
void Toast::SetOffsetY(float offsetY) { m_offsetY = offsetY; }
void Toast::SetSpacing(float spacing) { m_spacing = (std::max)(0.0f, spacing); }
void Toast::SetCloseable(bool closeable) { m_closeable = closeable; }

ToastCorner Toast::GetCorner() const {
    return m_corner;
}

void Toast::Show() {
    UpdateTextElements();
    PromoteLayer(true);
    m_state = 1;
    m_stateTime = std::chrono::steady_clock::now();
    m_isHovering = false;
    if (!UIElement::AreAnimationsEnabled()) {
        m_state = 2;
        m_currentOpacity = 1.0f;
        m_currentSlideX = 0.0f;
        m_currentSlideY = 0.0f;
    } else {
        m_currentOpacity = 0.0f;
        m_currentSlideX = 24.0f;
        m_currentSlideY = 10.0f;
    }
    RequestHostTicks();
}

void Toast::Hide() {
    if (m_state == 3 || m_state == 4) return;
    m_state = 4;
    m_stateTime = std::chrono::steady_clock::now();
    if (!UIElement::AreAnimationsEnabled()) {
        m_state = 3;
        m_currentOpacity = 0.0f;
        m_currentSlideX = 0.0f;
        m_currentSlideY = 0.0f;
    }
    // Toast is not in the live visual tree — must wake ToastCenter to run exit anim.
    RequestHostTicks();
}

Size Toast::Measure(Size availableSize) {
    float width = GetWidth(); if (width < 0) width = m_width;
    if (availableSize.width > 0.0f) {
        width = (std::min)(width, availableSize.width);
    }
    width = (std::max)(180.0f, width);
    m_desiredSize = Size(width, 92.0f);
    return m_desiredSize;
}

void Toast::Arrange(Rect finalRect) {
    m_boundsCache = finalRect;
    m_bounds = finalRect;
}

Rect Toast::CalculateBounds(const Rect& windowRect, int stackIndex) const {
    const float margin = 18.0f;
    const float height = 92.0f;
    float width = GetWidth(); if (width < 0) width = m_width;
    width = (std::min)(width, (std::max)(180.0f, windowRect.width - margin * 2.0f));
    width = (std::max)(180.0f, width);

    float offsetX = m_offsetX;
    float offsetY = m_offsetY;
    float spacing = m_spacing;
    float stackOffset = stackIndex * (height + spacing);

    float x = windowRect.x + margin + offsetX;
    float y = windowRect.y + margin + offsetY;
    ToastCorner corner = GetCorner();

    switch (corner) {
    case ToastCorner::TopLeft:
        y += stackOffset;
        break;
    case ToastCorner::TopRight:
        x = windowRect.x + windowRect.width - width - margin - offsetX;
        y += stackOffset;
        break;
    case ToastCorner::BottomLeft:
        y = windowRect.y + windowRect.height - height - margin - offsetY - stackOffset;
        break;
    case ToastCorner::BottomRight:
        x = windowRect.x + windowRect.width - width - margin - offsetX;
        y = windowRect.y + windowRect.height - height - margin - offsetY - stackOffset;
        break;
    }

    return Rect(x, y, width, height);
}

Rect Toast::GetCloseButtonRect() const {
    if (!m_closeable) return Rect();
    return Rect(m_renderBounds.x + m_renderBounds.width - 28.0f, m_renderBounds.y + 8.0f, 20.0f, 20.0f);
}

void Toast::RenderContent(GraphicsContext& ctx, const Rect& bounds, float opacity, float slideX, float slideY) {
    if (opacity <= 0.01f) return;

    Rect renderRect = bounds;
    renderRect.x += slideX;
    renderRect.y += slideY;
    m_renderBounds = renderRect;
    m_bounds = renderRect;

    SyncMembersFromProperties();
    const ThemeMode themeMode = ThemeManager::Instance().GetThemeMode();
    const bool lightTheme = themeMode == ThemeMode::Light;

    D2D1_COLOR_F shadow = D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.28f * opacity);
    D2D1_COLOR_F bg = ColorWithAlpha(m_background, opacity);
    D2D1_COLOR_F accentClr = ColorWithAlpha(m_accent, opacity);
    D2D1_COLOR_F border = ColorWithAlpha(ThemeHex("cardBorder"), opacity);
    D2D1_COLOR_F titleClr = ColorWithAlpha(m_titleColor, opacity);
    D2D1_COLOR_F messageClr = ColorWithAlpha(m_messageColor, opacity);

    ctx.FillRoundedRect(Rect(renderRect.x + 3, renderRect.y + 4, renderRect.width - 4, renderRect.height - 4), 10.0f, shadow);
    ctx.FillRoundedRect(renderRect, 10.0f, bg);
    ctx.DrawRoundedRect(renderRect, 10.0f, border, 1.0f);
    ctx.FillRect(Rect(renderRect.x, renderRect.y + 8.0f, 4.0f, renderRect.height - 16.0f), accentClr);

    float innerX = renderRect.x + 16.0f;
    float innerY = renderRect.y + 14.0f;
    float innerW = renderRect.width - (m_closeable ? 44.0f : 28.0f);

    if (m_txtTitle) {
        m_txtTitle->SetText(m_titleText);
        m_txtTitle->SetColor(titleClr);
        m_txtTitle->Arrange(Rect(innerX, innerY, innerW, 22.0f));
        m_txtTitle->Render(ctx);
    }
    if (m_txtMessage) {
        m_txtMessage->SetText(m_messageText);
        m_txtMessage->SetColor(messageClr);
        m_txtMessage->Arrange(Rect(innerX, innerY + 26.0f, innerW, 40.0f));
        m_txtMessage->Render(ctx);
    }

    if (m_closeable) {
        m_closeBtnBounds = GetCloseButtonRect();
        const float closeAlpha = (m_isHovering ? (lightTheme ? 0.08f : 0.12f) : 0.0f) * opacity;
        D2D1_COLOR_F closeBg = lightTheme
            ? ThemeManager::Instance().GetFlatColor(ThemeTokenId::TextPrimary)
            : ThemeManager::Instance().GetFlatColor(ThemeTokenId::AccentForeground);
        closeBg.a = closeAlpha;
        if (closeBg.a > 0.01f) {
            ctx.FillRoundedRect(m_closeBtnBounds, 4.0f, closeBg);
        }
        D2D1_COLOR_F xColor = ColorWithAlpha(ThemeHex("textSecondary"), opacity);
        float cx = m_closeBtnBounds.x + m_closeBtnBounds.width * 0.5f;
        float cy = m_closeBtnBounds.y + m_closeBtnBounds.height * 0.5f;
        ctx.DrawLine(Point(cx - 4.0f, cy - 4.0f), Point(cx + 4.0f, cy + 4.0f), xColor, 1.4f);
        ctx.DrawLine(Point(cx + 4.0f, cy - 4.0f), Point(cx - 4.0f, cy + 4.0f), xColor, 1.4f);
    } else {
        m_closeBtnBounds = Rect();
    }
}

void Toast::OnRenderOverlay(GraphicsContext& ctx) {
    if (m_state == 3) return;
    UIElement* root = this;
    while (root->GetParent()) root = root->GetParent();
    // 跳过自绘标题栏带，避免 TopLeft/TopRight 的 Toast 覆盖标题或窗口按钮。
    Rect windowRect = root ? ComputeToastAreaRect(root) : m_boundsCache;
    Rect bounds = CalculateBounds(windowRect, 0);

    ToastCorner corner = GetCorner();
    float sx = m_currentSlideX;
    float sy = m_currentSlideY;
    if (corner == ToastCorner::TopLeft || corner == ToastCorner::BottomLeft) sx = -sx;
    if (corner == ToastCorner::BottomLeft || corner == ToastCorner::BottomRight) sy = -sy;

    PromoteLayer(true);
    SetBounds(bounds);
    m_renderNode.SetBounds(bounds);
    auto& layer = m_renderNode.GetLayer();
    layer.SetBounds(bounds);

    // Raster once at full opacity into the layer; animate via DrawLayer opacity/offset.
    if (layer.NeedsContentRaster() || !layer.GetCacheBitmap()) {
        const float w = (std::max)(1.0f, std::ceil(bounds.width));
        const float h = (std::max)(1.0f, std::ceil(bounds.height));
        if (ctx.PushLayerTarget(
                layer,
                Size(w, h),
                Rect(0, 0, w, h),
                D2D1::ColorF(0, 0, 0, 0),
                true)) {
            ctx.PushTransform(D2D1::Matrix3x2F::Translation(-bounds.x, -bounds.y));
            RenderContent(ctx, bounds, 1.0f, 0.0f, 0.0f);
            ctx.PopTransform();
            ctx.PopLayerTarget(layer);
            layer.Validate();
        }
    }

    if (layer.GetCacheBitmap()) {
        const Rect dest(bounds.x + sx, bounds.y + sy, bounds.width, bounds.height);
        ctx.DrawLayer(layer, dest, nullptr, m_currentOpacity);
        m_renderBounds = dest;
        layer.ClearDirtyFlags(RenderLayer::OpacityDirty | RenderLayer::TransformDirty);
        return;
    }

    RenderContent(ctx, bounds, m_currentOpacity, sx, sy);
}

UIElement* Toast::OnHitTestOverlay(float x, float y) {
    if (m_state == 3 || m_currentOpacity <= 0.05f) return nullptr;
    return m_renderBounds.Contains(x, y) ? this : nullptr;
}

void Toast::OnMouseEnter() {
    UIElement::OnMouseEnter();
    m_isHovering = true;
    RequestHostTicks();
}

void Toast::OnMouseLeave() {
    UIElement::OnMouseLeave();
    m_isHovering = false;
    RequestHostTicks();
}

void Toast::OnMouseDown(Point pt) {
    m_isPressed = true;
    m_onMouseDownEvent.Invoke(this, pt);
}

void Toast::OnMouseUp(Point pt) {
    if (!m_isPressed) return;
    m_isPressed = false;
    if (!m_renderBounds.Contains(pt.x, pt.y)) return;

    if (m_closeable && m_closeBtnBounds.Contains(pt.x, pt.y)) {
        Hide();
        OnClick.Invoke(this);
        return;
    }

    if (m_closeable) {
        Hide();
    }
    OnClick.Invoke(this);
}

bool Toast::OnAnimationTick() {
    if (m_state == 3) {
        return false;
    }

    auto now = std::chrono::steady_clock::now();
    float elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_stateTime).count() / 1000.0f;

    if (!UIElement::AreAnimationsEnabled()) {
        if (m_state == 1) {
            m_state = 2;
            m_stateTime = now;
            m_currentOpacity = 1.0f;
            m_currentSlideX = 0.0f;
            m_currentSlideY = 0.0f;
            return false;
        } else if (m_state == 2) {
            m_currentOpacity = 1.0f;
            m_currentSlideX = 0.0f;
            m_currentSlideY = 0.0f;
            if (m_autoClose && m_durationMs > 0 && !m_isHovering) {
                if (elapsed * 1000.0f >= static_cast<float>(m_durationMs)) {
                    Hide();
                    return true;
                }
            }
            return false;
        } else if (m_state == 4) {
            m_state = 3;
            m_currentOpacity = 0.0f;
            m_currentSlideX = 0.0f;
            m_currentSlideY = 0.0f;
            return false;
        }
        return false;
    }

    if (m_state == 1) {
        // Finite CSS-like duration (not asymptotic) — browser compositor style.
        constexpr float kEnterSeconds = 0.22f;
        float t = Clamp01(elapsed / kEnterSeconds);
        float e = EaseOutCubic(t);
        m_currentOpacity = e;
        m_currentSlideX = (1.0f - e) * 28.0f;
        m_currentSlideY = (1.0f - e) * 12.0f;
        if (t >= 1.0f) {
            m_state = 2;
            m_stateTime = now;
            m_currentOpacity = 1.0f;
            m_currentSlideX = 0.0f;
            m_currentSlideY = 0.0f;
            return false;
        }
        return true;
    }

    if (m_state == 2) {
        m_currentOpacity = 1.0f;
        m_currentSlideX = 0.0f;
        m_currentSlideY = 0.0f;
        if (m_autoClose && m_durationMs > 0 && !m_isHovering) {
            if (elapsed * 1000.0f >= static_cast<float>(m_durationMs)) {
                // Stay in host's tick for exit — don't rely on wake-to-Toast (not in live tree).
                m_state = 4;
                m_stateTime = now;
                return true;
            }
        }
        return false;
    }

    if (m_state == 4) {
        constexpr float kExitSeconds = 0.18f;
        float t = Clamp01(elapsed / kExitSeconds);
        float e = EaseOutCubic(t);
        m_currentOpacity = 1.0f - e;
        m_currentSlideX = e * 18.0f;
        m_currentSlideY = e * 8.0f;
        if (t >= 1.0f) {
            m_state = 3;
            m_currentOpacity = 0.0f;
            m_currentSlideX = 0.0f;
            m_currentSlideY = 0.0f;
            return false;
        }
        return true;
    }

    return false;
}

bool Toast::HasSelfAnimation() const {
    return m_state == 1 || m_state == 4;
}

std::shared_ptr<Toast> Toast::Show(UIElement* root,
    const std::string& title,
    const std::string& message,
    ToastType type,
    ToastCorner corner,
    int durationMs) {
    return ToastCenter::Show(root, title, message, type, corner, durationMs);
}

std::shared_ptr<Toast> Toast::Show(UIElement* root,
    const std::string& title,
    const std::string& message,
    ToastCorner corner,
    int durationMs) {
    return ToastCenter::Show(root, title, message, corner, durationMs);
}

} // namespace CUI
