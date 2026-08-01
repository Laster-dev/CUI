#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "NumberBox.h"
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace CUI {

NumberBox::NumberBox() {
    SetProperty("value", Value(0.0f));
    SetProperty("minimum", Value(-100000.0f));
    SetProperty("maximum", Value(100000.0f));
    SetProperty("step", Value(1.0f));
    SetProperty("width", Value(120.0f));
    SetProperty("height", Value(28.0f));

    SetProperty("background", Value(D2D1::ColorF(0x25 / 255.0f, 0x25 / 255.0f, 0x26 / 255.0f, 1.0f)));
    SetProperty("borderBrush", Value(D2D1::ColorF(0x3C / 255.0f, 0x3C / 255.0f, 0x3C / 255.0f, 1.0f)));
    SetProperty("borderThickness", Value(1.0f));

    m_inputBox = std::make_shared<TextBox>();
    m_inputBox->SetText("0");
    m_inputBox->SetProperty("background", Value(D2D1::ColorF(0x1E / 255.0f, 0x1E / 255.0f, 0x1E / 255.0f, 1.0f)));
    m_inputBox->SetProperty("borderBrush", Value(D2D1::ColorF(0x3C / 255.0f, 0x3C / 255.0f, 0x3C / 255.0f, 1.0f)));
    m_inputBox->SetProperty("color", Value(D2D1::ColorF(0xCC / 255.0f, 0xCC / 255.0f, 0xCC / 255.0f, 1.0f)));

    m_btnUp = std::make_shared<Button>("▲");
    m_btnUp->SetProperty("fontSize", Value(8.0f));
    m_btnUp->SetProperty("background", Value(D2D1::ColorF(0x2D / 255.0f, 0x2D / 255.0f, 0x2D / 255.0f, 1.0f)));
    m_btnUp->SetProperty("hoverBackground", Value(D2D1::ColorF(0x3E / 255.0f, 0x3E / 255.0f, 0x42 / 255.0f, 1.0f)));
    m_btnUp->SetProperty("borderBrush", Value(D2D1::ColorF(0x3C / 255.0f, 0x3C / 255.0f, 0x3C / 255.0f, 1.0f)));
    m_btnUp->SetProperty("color", Value(D2D1::ColorF(0xAA / 255.0f, 0xAA / 255.0f, 0xAA / 255.0f, 1.0f)));
    m_btnUp->SetProperty("padding", Value(Thickness(0)));

    m_btnDown = std::make_shared<Button>("▼");
    m_btnDown->SetProperty("fontSize", Value(8.0f));
    m_btnDown->SetProperty("background", Value(D2D1::ColorF(0x2D / 255.0f, 0x2D / 255.0f, 0x2D / 255.0f, 1.0f)));
    m_btnDown->SetProperty("hoverBackground", Value(D2D1::ColorF(0x3E / 255.0f, 0x3E / 255.0f, 0x42 / 255.0f, 1.0f)));
    m_btnDown->SetProperty("borderBrush", Value(D2D1::ColorF(0x3C / 255.0f, 0x3C / 255.0f, 0x3C / 255.0f, 1.0f)));
    m_btnDown->SetProperty("color", Value(D2D1::ColorF(0xAA / 255.0f, 0xAA / 255.0f, 0xAA / 255.0f, 1.0f)));
    m_btnDown->SetProperty("padding", Value(Thickness(0)));

    AddChild(m_inputBox);
    AddChild(m_btnUp);
    AddChild(m_btnDown);

    m_btnUp->OnClick().Connect([this](UIElement*) {
        SetValue(GetValue() + GetStep());
    });

    m_btnDown->OnClick().Connect([this](UIElement*) {
        SetValue(GetValue() - GetStep());
    });

    m_inputBox->OnTextChanged().Connect([this](TextBox*, const std::string& txt) {
        if (txt.empty() || txt == "-") return;
        float parsed = static_cast<float>(atof(txt.c_str()));
        SetValue(parsed);
    });
}

std::vector<PropertyMeta> NumberBox::GetPropertyMetas() const {
    auto metas = UIElement::GetPropertyMetas();
    metas.push_back({ "value", "当前数值 (Value)", "微调配置", "number" });
    metas.push_back({ "minimum", "最小值 (Minimum)", "微调配置", "number" });
    metas.push_back({ "maximum", "最大值 (Maximum)", "微调配置", "number" });
    metas.push_back({ "step", "步进 (Step)", "微调配置", "number" });
    return metas;
}

Size NumberBox::Measure(Size availableSize) {
    float expW = GetProperty("width").AsFloat(120.0f);
    float expH = GetProperty("height").AsFloat(28.0f);
    m_desiredSize = Size(expW, expH);
    return m_desiredSize;
}

void NumberBox::Arrange(Rect finalRect) {
    m_bounds = finalRect;
    float btnW = 20.0f;
    float btnH = finalRect.height * 0.5f;

    Rect inputRect(finalRect.x, finalRect.y, (std::max)(0.0f, finalRect.width - btnW), finalRect.height);
    m_inputBox->Arrange(inputRect);

    m_btnUp->Arrange(Rect(finalRect.x + finalRect.width - btnW, finalRect.y, btnW, btnH));
    m_btnDown->Arrange(Rect(finalRect.x + finalRect.width - btnW, finalRect.y + btnH, btnW, btnH));
}

void NumberBox::SetValue(float val) {
    float minVal = GetMinimum();
    float maxVal = GetMaximum();
    val = std::clamp(val, minVal, maxVal);

    // Round value to 4 decimal places to prevent floating point precision artifacts (e.g., -0.00000001, 0.0999999)
    val = std::round(val * 10000.0f) / 10000.0f;
    if (val == -0.0f) val = 0.0f;

    if (std::abs(GetValue() - val) > 0.0001f) {
        SetProperty("value", Value(val));

        std::stringstream ss;
        ss << std::setprecision(6) << val;
        m_inputBox->SetText(ss.str());

        m_onValueChangedEvent.Invoke(this, val);
    }
}

void NumberBox::OnKeyDown(int vkCode) {
    if (vkCode == VK_UP) {
        SetValue(GetValue() + GetStep());
    } else if (vkCode == VK_DOWN) {
        SetValue(GetValue() - GetStep());
    } else {
        Control::OnKeyDown(vkCode);
    }
}

} // namespace CUI
