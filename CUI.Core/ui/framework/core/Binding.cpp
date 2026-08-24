#include "Binding.h"
#include "CUIDsl.h"
#include "../controls/UIElement.h"
#include "../style/ThemeTokenId.h"

namespace CUI {

namespace {

const char* VisibilityToString(Visibility v) {
    switch (v) {
    case Visibility::Hidden: return "Hidden";
    case Visibility::Collapsed: return "Collapsed";
    default: return "Visible";
    }
}

Visibility ParseVisibility(const std::string& s) {
    if (s == "Hidden") return Visibility::Hidden;
    if (s == "Collapsed") return Visibility::Collapsed;
    return Visibility::Visible;
}

const char* AlignmentToString(Alignment a) {
    switch (a) {
    case Alignment::Start: return "Start";
    case Alignment::Center: return "Center";
    case Alignment::End: return "End";
    default: return "Stretch";
    }
}

Alignment ParseAlignment(const std::string& s) {
    if (s == "Start") return Alignment::Start;
    if (s == "Center") return Alignment::Center;
    if (s == "End") return Alignment::End;
    return Alignment::Stretch;
}

const char* OrientationToString(Orientation o) {
    return (o == Orientation::Horizontal) ? "Horizontal" : "Vertical";
}

Orientation ParseOrientation(const std::string& s) {
    return (s == "Horizontal") ? Orientation::Horizontal : Orientation::Vertical;
}

const char* DockToString(Dock d) {
    switch (d) {
    case Dock::Top: return "Top";
    case Dock::Right: return "Right";
    case Dock::Bottom: return "Bottom";
    default: return "Left";
    }
}

Dock ParseDock(const std::string& s) {
    if (s == "Top") return Dock::Top;
    if (s == "Right") return Dock::Right;
    if (s == "Bottom") return Dock::Bottom;
    return Dock::Left;
}

Thickness ThicknessFromValue(const Value& val) {
    if (val.GetType() == Value::Type::String) {
        return Thickness::Parse(val.AsString());
    }
    return val.AsThickness();
}

ThemeTokenId TokenFromValue(const Value& val) {
    if (val.GetType() == Value::Type::String) {
        return ThemeTokenIdFromName(val.AsString());
    }
    return ThemeTokenId::Unset;
}

Value TokenValue(ThemeTokenId id) {
    return Value(ThemeTokenIdToName(id));
}

Value GetElementProperty(UIElement* e, PropertyId id) {
    if (!e || id == PropertyId::None) return Value();
    switch (id) {
    case PropertyId::Width: return (e->GetWidth() >= 0.0f) ? Value(e->GetWidth()) : Value();
    case PropertyId::Height: return (e->GetHeight() >= 0.0f) ? Value(e->GetHeight()) : Value();
    case PropertyId::MinWidth: return Value(e->GetMinWidth());
    case PropertyId::MinHeight: return Value(e->GetMinHeight());
    case PropertyId::MaxWidth: return (e->GetMaxWidth() >= 0.0f) ? Value(e->GetMaxWidth()) : Value();
    case PropertyId::MaxHeight: return (e->GetMaxHeight() >= 0.0f) ? Value(e->GetMaxHeight()) : Value();
    case PropertyId::Margin: return Value(e->GetMargin());
    case PropertyId::Padding: return Value(e->GetPadding());
    case PropertyId::Visibility: return Value(VisibilityToString(e->GetVisibility()));
    case PropertyId::IsEnabled: return Value(e->IsEnabled());
    case PropertyId::Opacity: return Value(e->GetOpacity());
    case PropertyId::CornerRadius: return Value(e->GetCornerRadius());
    case PropertyId::BorderThickness: return Value(e->GetBorderThickness());
    case PropertyId::FlexGrow: return Value(e->GetFlexGrow());
    case PropertyId::Align: return Value(AlignmentToString(e->GetAlign()));
    case PropertyId::AlignHorizontal: return Value(AlignmentToString(e->GetAlignHorizontal()));
    case PropertyId::AlignVertical: return Value(AlignmentToString(e->GetAlignVertical()));
    case PropertyId::Orientation: return Value(OrientationToString(e->GetOrientation()));
    case PropertyId::Gap: return Value(e->GetGap());
    case PropertyId::ItemWidth: return (e->GetItemWidth() >= 0.0f) ? Value(e->GetItemWidth()) : Value();
    case PropertyId::ItemHeight: return (e->GetItemHeight() >= 0.0f) ? Value(e->GetItemHeight()) : Value();
    case PropertyId::LastChildFill: return Value(e->GetLastChildFill());
    case PropertyId::JustifyLines: return Value(e->GetJustifyLines());
    case PropertyId::FillLastLine: return Value(e->GetFillLastLine());
    case PropertyId::Rows: return Value(e->GetRows());
    case PropertyId::Columns: return Value(e->GetColumns());
    case PropertyId::ClipToBounds: return Value(e->GetClipToBounds());
    case PropertyId::CanvasLeft: return Value(e->GetCanvasLeft());
    case PropertyId::CanvasTop: return Value(e->GetCanvasTop());
    case PropertyId::CanvasRight: return Value(e->GetCanvasRight());
    case PropertyId::CanvasBottom: return Value(e->GetCanvasBottom());
    case PropertyId::ZIndex: return Value(e->GetZIndex());
    case PropertyId::GridColumn: return Value(e->GetGridColumn());
    case PropertyId::GridRow: return Value(e->GetGridRow());
    case PropertyId::GridColumnSpan: return Value(e->GetGridColumnSpan());
    case PropertyId::GridRowSpan: return Value(e->GetGridRowSpan());
    case PropertyId::Dock: return Value(DockToString(e->GetDock()));
    case PropertyId::Text: return Value(e->GetText());
    case PropertyId::Placeholder: return Value(e->GetPlaceholder());
    case PropertyId::FontFamily: return Value(e->GetFontFamily());
    case PropertyId::FontSize: return Value(e->GetFontSize());
    case PropertyId::FontWeight: return Value(FontWeightToString(e->GetFontWeight()));
    case PropertyId::FontStyle: return Value(FontStyleToString(e->GetFontStyle()));
    case PropertyId::FontStretch: return Value(FontStretchToString(e->GetFontStretch()));
    case PropertyId::IsUnderline: return Value(e->IsUnderline());
    case PropertyId::IsStrikethrough: return Value(e->IsStrikethrough());
    case PropertyId::ToolTip: return Value(e->GetToolTip());
    case PropertyId::BackgroundToken: return TokenValue(e->GetBackgroundToken());
    case PropertyId::HoverBackgroundToken: return TokenValue(e->GetHoverBackgroundToken());
    case PropertyId::PressedBackgroundToken: return TokenValue(e->GetPressedBackgroundToken());
    case PropertyId::DisabledBackgroundToken: return TokenValue(e->GetDisabledBackgroundToken());
    case PropertyId::BorderToken: return TokenValue(e->GetBorderToken());
    case PropertyId::FocusedBorderToken: return TokenValue(e->GetFocusedBorderToken());
    case PropertyId::ColorToken: return TokenValue(e->GetColorToken());
    default:
        return e->GetProperty(id);
    }
}

void SetElementProperty(UIElement* e, PropertyId id, const Value& val) {
    if (!e || id == PropertyId::None) return;
    switch (id) {
    case PropertyId::Width: DSL::Borrow(e).Width(val.AsFloat()); break;
    case PropertyId::Height: DSL::Borrow(e).Height(val.AsFloat()); break;
    case PropertyId::MinWidth: DSL::Borrow(e).MinWidth(val.AsFloat()); break;
    case PropertyId::MinHeight: DSL::Borrow(e).MinHeight(val.AsFloat()); break;
    case PropertyId::MaxWidth: DSL::Borrow(e).MaxWidth(val.AsFloat()); break;
    case PropertyId::MaxHeight: DSL::Borrow(e).MaxHeight(val.AsFloat()); break;
    case PropertyId::Margin: DSL::Borrow(e).Margin(ThicknessFromValue(val)); break;
    case PropertyId::Padding: DSL::Borrow(e).Padding(ThicknessFromValue(val)); break;
    case PropertyId::Visibility: DSL::Borrow(e).Visibility(ParseVisibility(val.AsString("Visible"))); break;
    case PropertyId::IsEnabled: DSL::Borrow(e).IsEnabled(val.AsBool()); break;
    case PropertyId::Opacity: DSL::Borrow(e).Opacity(val.AsFloat()); break;
    case PropertyId::CornerRadius: DSL::Borrow(e).CornerRadius(val.AsFloat()); break;
    case PropertyId::BorderThickness: DSL::Borrow(e).BorderThickness(val.AsFloat()); break;
    case PropertyId::FlexGrow: DSL::Borrow(e).FlexGrow(val.AsFloat()); break;
    case PropertyId::Align: DSL::Borrow(e).Align(ParseAlignment(val.AsString("Stretch"))); break;
    case PropertyId::AlignHorizontal: e->SetAlignHorizontal(ParseAlignment(val.AsString("Stretch"))); break;
    case PropertyId::AlignVertical: e->SetAlignVertical(ParseAlignment(val.AsString("Stretch"))); break;
    case PropertyId::Orientation: DSL::Borrow(e).Orientation(ParseOrientation(val.AsString("Vertical"))); break;
    case PropertyId::Gap: DSL::Borrow(e).Gap(val.AsFloat()); break;
    case PropertyId::ItemWidth: DSL::Borrow(e).ItemWidth(val.AsFloat()); break;
    case PropertyId::ItemHeight: DSL::Borrow(e).ItemHeight(val.AsFloat()); break;
    case PropertyId::LastChildFill: DSL::Borrow(e).LastChildFill(val.AsBool()); break;
    case PropertyId::JustifyLines: DSL::Borrow(e).JustifyLines(val.AsBool()); break;
    case PropertyId::FillLastLine: DSL::Borrow(e).FillLastLine(val.AsBool()); break;
    case PropertyId::Rows: DSL::Borrow(e).Rows(val.AsInt()); break;
    case PropertyId::Columns: DSL::Borrow(e).Columns(val.AsInt()); break;
    case PropertyId::ClipToBounds: DSL::Borrow(e).ClipToBounds(val.AsBool()); break;
    case PropertyId::CanvasLeft: DSL::Borrow(e).CanvasLeft(val.AsFloat()); break;
    case PropertyId::CanvasTop: DSL::Borrow(e).CanvasTop(val.AsFloat()); break;
    case PropertyId::CanvasRight: DSL::Borrow(e).CanvasRight(val.AsFloat()); break;
    case PropertyId::CanvasBottom: DSL::Borrow(e).CanvasBottom(val.AsFloat()); break;
    case PropertyId::ZIndex: DSL::Borrow(e).ZIndex(val.AsInt()); break;
    case PropertyId::GridColumn: DSL::Borrow(e).GridColumn(val.AsInt()); break;
    case PropertyId::GridRow: DSL::Borrow(e).GridRow(val.AsInt()); break;
    case PropertyId::GridColumnSpan: DSL::Borrow(e).GridColumnSpan(val.AsInt()); break;
    case PropertyId::GridRowSpan: DSL::Borrow(e).GridRowSpan(val.AsInt()); break;
    case PropertyId::Dock: DSL::Borrow(e).Dock(ParseDock(val.AsString("Left"))); break;
    case PropertyId::Text: DSL::Borrow(e).Text(val.AsString()); break;
    case PropertyId::Placeholder: DSL::Borrow(e).Placeholder(val.AsString()); break;
    case PropertyId::FontFamily: DSL::Borrow(e).FontFamily(val.AsString()); break;
    case PropertyId::FontSize: DSL::Borrow(e).FontSize(val.AsFloat()); break;
    case PropertyId::FontWeight: DSL::Borrow(e).FontWeight(FontWeightFromString(val.AsString())); break;
    case PropertyId::FontStyle: DSL::Borrow(e).FontStyle(FontStyleFromString(val.AsString())); break;
    case PropertyId::FontStretch: DSL::Borrow(e).FontStretch(FontStretchFromString(val.AsString())); break;
    case PropertyId::IsUnderline: e->SetIsUnderline(val.AsBool()); break;
    case PropertyId::IsStrikethrough: e->SetIsStrikethrough(val.AsBool()); break;
    case PropertyId::ToolTip: e->SetToolTip(val.AsString()); break;
    case PropertyId::BackgroundToken: DSL::Borrow(e).BackgroundToken(TokenFromValue(val)); break;
    case PropertyId::HoverBackgroundToken: e->SetHoverBackgroundToken(TokenFromValue(val)); break;
    case PropertyId::PressedBackgroundToken: e->SetPressedBackgroundToken(TokenFromValue(val)); break;
    case PropertyId::DisabledBackgroundToken: e->SetDisabledBackgroundToken(TokenFromValue(val)); break;
    case PropertyId::BorderToken: e->SetBorderToken(TokenFromValue(val)); break;
    case PropertyId::FocusedBorderToken: e->SetFocusedBorderToken(TokenFromValue(val)); break;
    case PropertyId::ColorToken: e->SetColorToken(TokenFromValue(val)); break;
    default:
        e->SetProperty(id, val);
        break;
    }
}

} // namespace

Binding::Binding(std::shared_ptr<Object> target, PropertyId targetProperty,
                 std::shared_ptr<Object> source, PropertyId sourceProperty,
                 BindingMode mode)
    : m_target(target), m_targetProperty(targetProperty),
      m_source(source), m_sourceProperty(sourceProperty), m_mode(mode)
{
    UpdateTarget();

    if (mode == BindingMode::OneWay || mode == BindingMode::TwoWay) {
        if (auto src = m_source.lock()) {
            m_sourceConnId = src->OnPropertyIdChanged().Connect(
                [this](PropertyId propId, const Value&) {
                    if (!m_isUpdating && propId == m_sourceProperty) {
                        UpdateTarget();
                    }
                });
        }
    }

    if (mode == BindingMode::TwoWay) {
        if (auto tgt = m_target.lock()) {
            m_targetConnId = tgt->OnPropertyIdChanged().Connect(
                [this](PropertyId propId, const Value&) {
                    if (!m_isUpdating && propId == m_targetProperty) {
                        UpdateSource();
                    }
                });
        }
    }
}

Binding::~Binding() {
    if (m_sourceConnId != 0) {
        if (auto src = m_source.lock()) {
            src->OnPropertyIdChanged().Disconnect(m_sourceConnId);
        }
    }
    if (m_targetConnId != 0) {
        if (auto tgt = m_target.lock()) {
            tgt->OnPropertyIdChanged().Disconnect(m_targetConnId);
        }
    }
}

void Binding::UpdateTarget() {
    auto tgt = m_target.lock();
    auto src = m_source.lock();
    if (!tgt || !src) return;

    auto* srcElem = dynamic_cast<UIElement*>(src.get());
    auto* tgtElem = dynamic_cast<UIElement*>(tgt.get());
    if (!srcElem || !tgtElem) return;

    m_isUpdating = true;
    SetElementProperty(tgtElem, m_targetProperty, GetElementProperty(srcElem, m_sourceProperty));
    m_isUpdating = false;
}

void Binding::UpdateSource() {
    auto tgt = m_target.lock();
    auto src = m_source.lock();
    if (!tgt || !src) return;

    auto* srcElem = dynamic_cast<UIElement*>(src.get());
    auto* tgtElem = dynamic_cast<UIElement*>(tgt.get());
    if (!srcElem || !tgtElem) return;

    m_isUpdating = true;
    SetElementProperty(srcElem, m_sourceProperty, GetElementProperty(tgtElem, m_targetProperty));
    m_isUpdating = false;
}

} // namespace CUI

