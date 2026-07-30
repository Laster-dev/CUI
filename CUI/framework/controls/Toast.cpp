#include "Toast.h"
#include "ToastCenter.h"
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

static std::string ColorValueToHex(const Value& value, const std::string& fallback) {
    if (value.GetType() == Value::Type::String) {
        std::string s = value.AsString(fallback);
        return s.empty() ? fallback : s;
    }
    if (value.GetType() == Value::Type::Color) {
        D2D1_COLOR_F c = value.AsColor();
        char buf[16];
        std::snprintf(buf, sizeof(buf), "#%02X%02X%02X",
            static_cast<int>(Clamp01(c.r) * 255.0f + 0.5f),
            static_cast<int>(Clamp01(c.g) * 255.0f + 0.5f),
            static_cast<int>(Clamp01(c.b) * 255.0f + 0.5f));
        return std::string(buf);
    }
    return fallback;
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

Toast::Toast() {
    SetProperty("visibility", Value("Visible"));
    SetProperty("opacity", Value(1.0f));
    SetProperty("title", Value(m_titleText));
    SetProperty("message", Value(m_messageText));
    SetProperty("corner", Value("BottomRight"));
    SetProperty("durationMs", Value(m_durationMs));
    SetProperty("autoClose", Value(m_autoClose));
    SetProperty("closeable", Value(m_closeable));
    SetProperty("width", Value(m_width));
    SetProperty("offsetX", Value(m_offsetX));
    SetProperty("offsetY", Value(m_offsetY));
    SetProperty("spacing", Value(m_spacing));
    SetProperty("background", Value(m_background));
    SetProperty("accent", Value(m_accent));
    SetProperty("titleColor", Value(m_titleColor));
    SetProperty("messageColor", Value(m_messageColor));

    m_txtTitle = std::make_shared<TextBlock>(m_titleText);
    m_txtTitle->SetProperty("fontSize", Value(15.0f));
    m_txtTitle->SetProperty("fontWeight", Value("Bold"));
    m_txtTitle->SetProperty("color", Value(m_titleColor));

    m_txtMessage = std::make_shared<TextBlock>(m_messageText);
    m_txtMessage->SetProperty("fontSize", Value(12.5f));
    m_txtMessage->SetProperty("color", Value(m_messageColor));

    AddChild(m_txtTitle);
    AddChild(m_txtMessage);
}

std::vector<PropertyMeta> Toast::GetPropertyMetas() const {
    auto metas = UIElement::GetPropertyMetas();
    metas.push_back({ "title", "Title", "Toast", "string" });
    metas.push_back({ "message", "Message", "Toast", "string" });
    metas.push_back({ "corner", "Corner", "Toast", "enum", { "TopLeft", "TopRight", "BottomLeft", "BottomRight" } });
    metas.push_back({ "durationMs", "DurationMs", "Toast", "number" });
    metas.push_back({ "autoClose", "AutoClose", "Toast", "bool" });
    metas.push_back({ "closeable", "Closeable", "Toast", "bool" });
    metas.push_back({ "accent", "Accent", "Toast", "color" });
    metas.push_back({ "titleColor", "TitleColor", "Toast", "color" });
    metas.push_back({ "messageColor", "MessageColor", "Toast", "color" });
    metas.push_back({ "offsetX", "OffsetX", "Toast", "number" });
    metas.push_back({ "offsetY", "OffsetY", "Toast", "number" });
    metas.push_back({ "spacing", "StackSpacing", "Toast", "number" });
    return metas;
}

void Toast::SyncMembersFromProperties() {
    m_titleText = GetProperty("title").AsString(m_titleText);
    m_messageText = GetProperty("message").AsString(m_messageText);
    m_background = ColorValueToHex(GetProperty("background"), m_background);
    m_accent = ColorValueToHex(GetProperty("accent"), m_accent);
    m_titleColor = ColorValueToHex(GetProperty("titleColor"), m_titleColor);
    m_messageColor = ColorValueToHex(GetProperty("messageColor"), m_messageColor);
    m_width = GetProperty("width").AsFloat(m_width);
    m_offsetX = GetProperty("offsetX").AsFloat(m_offsetX);
    m_offsetY = GetProperty("offsetY").AsFloat(m_offsetY);
    m_spacing = GetProperty("spacing").AsFloat(m_spacing);
    m_durationMs = GetProperty("durationMs").AsInt(m_durationMs);
    m_autoClose = GetProperty("autoClose").AsBool(m_autoClose);
    m_closeable = GetProperty("closeable").AsBool(m_closeable);
    m_corner = ParseCorner(GetProperty("corner").AsString(CornerToString(m_corner)), m_corner);
}

void Toast::UpdateTextElements() {
    SyncMembersFromProperties();
    if (m_txtTitle) {
        m_txtTitle->SetProperty("text", Value(m_titleText));
        m_txtTitle->SetProperty("color", Value(m_titleColor));
    }
    if (m_txtMessage) {
        m_txtMessage->SetProperty("text", Value(m_messageText));
        m_txtMessage->SetProperty("color", Value(m_messageColor));
    }
}

void Toast::ApplyFrom(const UIElement* source) {
    if (!source) return;
    static const char* keys[] = {
        "title", "message", "corner", "durationMs", "autoClose", "closeable",
        "width", "offsetX", "offsetY", "spacing",
        "background", "accent", "titleColor", "messageColor"
    };
    for (const char* key : keys) {
        if (source->HasProperty(key)) { SetProperty(key, source->GetProperty(key)); }
    }
    UpdateTextElements();
}

D2D1_COLOR_F Toast::ColorWithAlpha(const std::string& color, float alpha) {
    D2D1_COLOR_F c = Value::ParseColor(color);
    c.a = Clamp01(alpha) * c.a;
    return c;
}

void Toast::SetTitle(const std::string& title) { SetProperty("title", Value(title)); m_titleText = title; UpdateTextElements(); }
void Toast::SetMessage(const std::string& message) { SetProperty("message", Value(message)); m_messageText = message; UpdateTextElements(); }
void Toast::SetCorner(ToastCorner corner) { m_corner = corner; SetProperty("corner", Value(CornerToString(corner))); }
void Toast::SetDurationMs(int durationMs) { m_durationMs = durationMs; SetProperty("durationMs", Value(durationMs)); }
void Toast::SetWidth(float width) { m_width = (std::max)(180.0f, width); SetProperty("width", Value(m_width)); }
void Toast::SetAutoClose(bool enabled) { m_autoClose = enabled; SetProperty("autoClose", Value(enabled)); }
void Toast::SetBackground(const std::string& color) { m_background = color; SetProperty("background", Value(color)); }
void Toast::SetAccent(const std::string& color) { m_accent = color; SetProperty("accent", Value(color)); }
void Toast::SetTitleColor(const std::string& color) { m_titleColor = color; SetProperty("titleColor", Value(color)); UpdateTextElements(); }
void Toast::SetMessageColor(const std::string& color) { m_messageColor = color; SetProperty("messageColor", Value(color)); UpdateTextElements(); }
void Toast::SetOffsetX(float offsetX) { m_offsetX = offsetX; SetProperty("offsetX", Value(offsetX)); }
void Toast::SetOffsetY(float offsetY) { m_offsetY = offsetY; SetProperty("offsetY", Value(offsetY)); }
void Toast::SetSpacing(float spacing) { m_spacing = (std::max)(0.0f, spacing); SetProperty("spacing", Value(m_spacing)); }
void Toast::SetCloseable(bool closeable) { m_closeable = closeable; SetProperty("closeable", Value(closeable)); }

ToastCorner Toast::GetCorner() const {
    return ParseCorner(GetProperty("corner").AsString(CornerToString(m_corner)), m_corner);
}

void Toast::Show() {
    UpdateTextElements();
    m_state = 1;
    m_stateTime = std::chrono::steady_clock::now();
    m_isHovering = false;
    m_currentOpacity = 0.0f;
    m_currentSlideX = 24.0f;
    m_currentSlideY = 10.0f;
}

void Toast::Hide() {
    if (m_state == 3 || m_state == 4) return;
    m_state = 4;
    m_stateTime = std::chrono::steady_clock::now();
}

Size Toast::Measure(Size availableSize) {
    float width = GetProperty("width").AsFloat(m_width);
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
    float width = GetProperty("width").AsFloat(m_width);
    width = (std::min)(width, (std::max)(180.0f, windowRect.width - margin * 2.0f));
    width = (std::max)(180.0f, width);

    float offsetX = GetProperty("offsetX").AsFloat(m_offsetX);
    float offsetY = GetProperty("offsetY").AsFloat(m_offsetY);
    float spacing = GetProperty("spacing").AsFloat(m_spacing);
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

    D2D1_COLOR_F shadow = ColorWithAlpha("#000000", 0.28f * opacity);
    D2D1_COLOR_F bg = ColorWithAlpha(m_background, opacity);
    D2D1_COLOR_F accentClr = ColorWithAlpha(m_accent, opacity);
    D2D1_COLOR_F border = ColorWithAlpha("#FFFFFF", 0.10f * opacity);
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
        m_txtTitle->SetProperty("text", Value(m_titleText));
        m_txtTitle->SetProperty("color", Value(titleClr));
        m_txtTitle->Arrange(Rect(innerX, innerY, innerW, 22.0f));
        m_txtTitle->Render(ctx);
    }
    if (m_txtMessage) {
        m_txtMessage->SetProperty("text", Value(m_messageText));
        m_txtMessage->SetProperty("color", Value(messageClr));
        m_txtMessage->Arrange(Rect(innerX, innerY + 26.0f, innerW, 40.0f));
        m_txtMessage->Render(ctx);
    }

    if (m_closeable) {
        m_closeBtnBounds = GetCloseButtonRect();
        D2D1_COLOR_F closeBg = ColorWithAlpha(m_isHovering ? "#3A3A3D" : "#000000", (m_isHovering ? 0.45f : 0.0f) * opacity);
        if (closeBg.a > 0.01f) {
            ctx.FillRoundedRect(m_closeBtnBounds, 4.0f, closeBg);
        }
        D2D1_COLOR_F xColor = ColorWithAlpha("#CCCCCC", opacity);
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
    Rect windowRect = root ? root->GetBounds() : m_boundsCache;
    Rect bounds = CalculateBounds(windowRect, 0);

    ToastCorner corner = GetCorner();
    float sx = m_currentSlideX;
    float sy = m_currentSlideY;
    if (corner == ToastCorner::TopLeft || corner == ToastCorner::BottomLeft) sx = -sx;
    if (corner == ToastCorner::BottomLeft || corner == ToastCorner::BottomRight) sy = -sy;
    RenderContent(ctx, bounds, m_currentOpacity, sx, sy);
}

UIElement* Toast::OnHitTestOverlay(float x, float y) {
    if (m_state == 3 || m_currentOpacity <= 0.05f) return nullptr;
    return m_renderBounds.Contains(x, y) ? this : nullptr;
}

void Toast::OnMouseEnter() {
    UIElement::OnMouseEnter();
    m_isHovering = true;
}

void Toast::OnMouseLeave() {
    UIElement::OnMouseLeave();
    m_isHovering = false;
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
        m_onClickEvent.Invoke(this);
        return;
    }

    if (m_closeable) {
        Hide();
    }
    m_onClickEvent.Invoke(this);
}

bool Toast::OnAnimationTick() {
    if (m_state == 3) return false;

    auto now = std::chrono::steady_clock::now();
    float elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_stateTime).count() / 1000.0f;

    if (m_state == 1) {
        float t = Clamp01(elapsed / 0.22f);
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
        }
        return true;
    }

    if (m_state == 2) {
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
    }

    if (m_state == 4) {
        float t = Clamp01(elapsed / 0.18f);
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

std::shared_ptr<Toast> Toast::Show(UIElement* root,
    const std::string& title,
    const std::string& message,
    ToastCorner corner,
    int durationMs) {
    return ToastCenter::Show(root, title, message, corner, durationMs);
}

} // namespace CUI

