#include "CheckBox.h"
#include <algorithm>
#include <cmath>

namespace CUI {
namespace {
float EaseOutCubic(float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    return 1.0f - std::pow(1.0f - t, 3.0f);
}

float EaseOutQuad(float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    return 1.0f - (1.0f - t) * (1.0f - t);
}

Point LerpPoint(const Point& a, const Point& b, float t) {
    return Point(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t);
}

}

CheckBox::CheckBox() {
    SetProperty("text", Value("CheckBox"));
    SetProperty("checkState", Value("Unchecked"));
    SetProperty("isThreeState", Value(false));
    SetProperty("background", Value(D2D1::ColorF(0x3C / 255.0f, 0x3C / 255.0f, 0x3C / 255.0f, 1.0f)));
    SetProperty("checkedBackground", Value(D2D1::ColorF(0x00 / 255.0f, 0x7A / 255.0f, 0xCC / 255.0f, 1.0f)));
    SetProperty("color", Value(D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f)));
    SetProperty("fontSize", Value(13.0f));
    SetProperty("fontFamily", Value("Segoe UI"));
    SetProperty("padding", Value(Thickness(4, 4, 4, 4)));
    SetProperty("cornerRadius", Value(3.0f));
}

CheckBox::CheckBox(const std::string& text) : CheckBox() {
    SetProperty("text", Value(text));
}

std::vector<PropertyMeta> CheckBox::GetPropertyMetas() const {
    auto metas = Control::GetPropertyMetas();
    metas.push_back({ "text", "标签文本 (Text)", "基本信息", "string" });
    metas.push_back({ "checkState", "选中状态 (CheckState)", "复选配置", "enum", { "Unchecked", "Checked", "Indeterminate" } });
    metas.push_back({ "isThreeState", "三态模式 (ThreeState)", "复选配置", "bool" });
    metas.push_back({ "checkedBackground", "选中背景 (CheckedBg)", "色彩外观", "color" });
    metas.push_back({ "fontFamily", "字体名称 (FontFamily)", "字体文本", "enum", { "Segoe UI", "Consolas", "微软雅黑", "Times New Roman" } });
    metas.push_back({ "fontSize", "字体大小 (FontSize)", "字体文本", "number" });
    metas.push_back({ "color", "文字颜色 (Color)", "字体文本", "color" });
    return metas;
}

CheckState CheckBox::GetState() const {
    std::string s = GetProperty("checkState").AsString("Unchecked");
    if (s == "Checked") return CheckState::Checked;
    if (s == "Indeterminate") return CheckState::Indeterminate;
    return CheckState::Unchecked;
}

void CheckBox::SetState(CheckState state) {
    std::string s = "Unchecked";
    if (state == CheckState::Checked) s = "Checked";
    else if (state == CheckState::Indeterminate) s = "Indeterminate";

    SetProperty("checkState", Value(s));
    SetProperty("isChecked", Value(state == CheckState::Checked));
    m_onCheckStateChangedEvent.Invoke(this, state);
}

Size CheckBox::Measure(Size availableSize) {
    std::string text = GetProperty("text").AsString("");
    std::string font = GetProperty("fontFamily").AsString("Segoe UI");
    float fontSize = GetProperty("fontSize").AsFloat(13.0f);

    GraphicsContext ctx;
    Size textSize = ctx.MeasureText(text, font, fontSize);

    Thickness margin = GetProperty("margin").AsThickness(Thickness(0));
    Thickness padding = GetProperty("padding").AsThickness(Thickness(0));

    float boxW = 16.0f;
    float gap = 8.0f;

    float w = boxW + gap + textSize.width + margin.left + margin.right + padding.left + padding.right;
    float h = (std::max)(16.0f, textSize.height) + margin.top + margin.bottom + padding.top + padding.bottom;

    float expW = GetProperty("width").AsFloat(-1.0f);
    float expH = GetProperty("height").AsFloat(-1.0f);

    if (expW >= 0.0f) w = expW;
    if (expH >= 0.0f) h = expH;

    m_desiredSize = Size(w, h);
    return m_desiredSize;
}

void CheckBox::OnRender(GraphicsContext& ctx) {
    Thickness padding = GetProperty("padding").AsThickness(Thickness(0));
    float radius = GetProperty("cornerRadius").AsFloat(4.0f);

    CheckState state = GetState();
    float fillTarget = state == CheckState::Unchecked ? 0.0f : 1.0f;
    float checkTarget = state == CheckState::Checked ? 1.0f : 0.0f;
    float indeterminateTarget = state == CheckState::Indeterminate ? 1.0f : 0.0f;
    m_fillAnim.SetTarget(fillTarget);
    m_checkAnim.SetTarget(checkTarget);
    m_indeterminateAnim.SetTarget(indeterminateTarget);

    if (!UIElement::AreAnimationsEnabled()) {
        m_fillAnim.Reset(fillTarget);
        m_checkAnim.Reset(checkTarget);
        m_indeterminateAnim.Reset(indeterminateTarget);
        UpdateVisualStateTarget();
        m_visualStateAnim.Reset(m_visualStateTarget);
    }

    float boxSize = 18.0f;
    float boxY = m_bounds.y + (m_bounds.height - boxSize) / 2.0f;
    Rect boxRect(m_bounds.x + padding.left, boxY, boxSize, boxSize);

    D2D1_COLOR_F accentBase = D2D1::ColorF(0x4C / 255.0f, 0xC2 / 255.0f, 0xFF / 255.0f, 1.0f); // #4CC2FF WinUI Fluent Accent Blue
    float visualState = m_visualStateAnim.Current();
    D2D1_COLOR_F accentBlue = BlendColor(accentBase, D2D1::ColorF(0x78 / 255.0f, 0xD7 / 255.0f, 0xFF / 255.0f, 1.0f), visualState);
    D2D1_COLOR_F checkedIconColor = D2D1::ColorF(0x00 / 255.0f, 0x00 / 255.0f, 0x00 / 255.0f, 1.0f); // Black check mark
    D2D1_COLOR_F bg = GetAnimatedBackground(D2D1::ColorF(0x20 / 255.0f, 0x20 / 255.0f, 0x20 / 255.0f, 1.0f));
    D2D1_COLOR_F border = BlendColor(
        D2D1::ColorF(0x8E / 255.0f, 0x8E / 255.0f, 0x8E / 255.0f, 0.8f),
        D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.9f),
        (std::min)(1.0f, visualState / 0.55f));

    float fillProgress = m_fillAnim.Current();
    float checkProgress = m_checkAnim.Current();
    float indeterminateProgress = m_indeterminateAnim.Current();

    D2D1_COLOR_F fillColor = BlendColor(bg, accentBlue, fillProgress);
    D2D1_COLOR_F borderColor = BlendColor(border, accentBlue, fillProgress * 0.9f);
    ctx.FillRoundedRect(boxRect, radius, fillColor);
    ctx.DrawRoundedRect(boxRect, radius, borderColor, 1.2f);

    float checkFactor = EaseOutCubic(checkProgress);
    if (checkFactor > 0.001f) {
        Point p1(boxRect.x + 4.6f, boxRect.y + 9.4f);
        Point p2(boxRect.x + 7.6f, boxRect.y + 12.3f);
        Point p3(boxRect.x + 13.4f, boxRect.y + 6.0f);
        float len1 = std::sqrt((p2.x - p1.x) * (p2.x - p1.x) + (p2.y - p1.y) * (p2.y - p1.y));
        float len2 = std::sqrt((p3.x - p2.x) * (p3.x - p2.x) + (p3.y - p2.y) * (p3.y - p2.y));
        float total = len1 + len2;
        float drawLen = total * checkFactor;

        if (drawLen <= len1) {
            float segT = len1 > 0.0f ? drawLen / len1 : 1.0f;
            ctx.DrawLine(p1, LerpPoint(p1, p2, segT), checkedIconColor, 1.9f);
        } else {
            ctx.DrawLine(p1, p2, checkedIconColor, 1.9f);
            float remain = drawLen - len1;
            float segT = len2 > 0.0f ? remain / len2 : 1.0f;
            ctx.DrawLine(p2, LerpPoint(p2, p3, segT), checkedIconColor, 1.9f);
        }
    }

    float indeterminateFactor = EaseOutQuad(indeterminateProgress);
    if (indeterminateFactor > 0.001f) {
        float halfWidth = 4.5f * indeterminateFactor;
        float centerX = boxRect.x + boxRect.width * 0.5f;
        float centerY = boxRect.y + boxRect.height * 0.5f;
        Rect barRect(centerX - halfWidth, centerY - 1.5f, halfWidth * 2.0f, 3.0f);
        ctx.FillRoundedRect(barRect, 1.0f, checkedIconColor);
    }

    // Draw Label Text
    std::string text = GetProperty("text").AsString("");
    if (!text.empty()) {
        float textX = boxRect.x + boxSize + 10.0f;
        Rect textRect(textX, m_bounds.y, m_bounds.width - (textX - m_bounds.x), m_bounds.height);

        D2D1_COLOR_F textColor = GetProperty("color").AsColor(D2D1::ColorF(1.0f, 1.0f, 1.0f));
        std::string font = GetProperty("fontFamily").AsString("Segoe UI");
        float fontSize = GetProperty("fontSize").AsFloat(13.0f);

        ctx.DrawText(text, textRect, textColor, font, fontSize, DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    }
}

void CheckBox::OnMouseDown(Point pt) {
    Control::OnMouseDown(pt);

    CheckState currentState = GetState();
    bool threeState = IsThreeState();

    CheckState newState = CheckState::Unchecked;
    if (currentState == CheckState::Unchecked) {
        newState = CheckState::Checked;
    } else if (currentState == CheckState::Checked) {
        newState = threeState ? CheckState::Indeterminate : CheckState::Unchecked;
    } else {
        newState = CheckState::Unchecked;
    }

    SetState(newState);
}

bool CheckBox::OnAnimationTick() {
    bool base = Control::OnAnimationTick();
    bool animating = base;

    CheckState state = GetState();
    float fillTarget = state == CheckState::Unchecked ? 0.0f : 1.0f;
    float checkTarget = state == CheckState::Checked ? 1.0f : 0.0f;
    float indeterminateTarget = state == CheckState::Indeterminate ? 1.0f : 0.0f;

    m_fillAnim.SetTarget(fillTarget);
    m_checkAnim.SetTarget(checkTarget);
    m_indeterminateAnim.SetTarget(indeterminateTarget);
    animating = m_fillAnim.Tick(UIElement::GetAnimationDeltaSeconds(), AnimationSpec{ 0.24f, 0.01f }) || animating;
    animating = m_checkAnim.Tick(UIElement::GetAnimationDeltaSeconds(), AnimationSpec{ 0.20f, 0.01f }) || animating;
    animating = m_indeterminateAnim.Tick(UIElement::GetAnimationDeltaSeconds(), AnimationSpec{ 0.20f, 0.01f }) || animating;

    return animating;
}

bool CheckBox::HasSelfAnimation() const {
    float fillTarget = GetState() == CheckState::Unchecked ? 0.0f : 1.0f;
    float checkTarget = GetState() == CheckState::Checked ? 1.0f : 0.0f;
    float indeterminateTarget = GetState() == CheckState::Indeterminate ? 1.0f : 0.0f;
    return Control::HasSelfAnimation()
        || std::abs(fillTarget - m_fillAnim.Current()) > 0.01f
        || std::abs(checkTarget - m_checkAnim.Current()) > 0.01f
        || std::abs(indeterminateTarget - m_indeterminateAnim.Current()) > 0.01f;
}

} // namespace CUI
