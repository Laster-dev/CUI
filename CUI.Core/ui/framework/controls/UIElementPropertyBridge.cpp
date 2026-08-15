#include "UIElement.h"
#include "../core/PropertyId.h"
#include "../style/ThemeManager.h"
#include "../style/ThemeTokenId.h"
#include <algorithm>
#include <cmath>

namespace CUI {

namespace {

bool ColorUnchanged(bool has, const D2D1_COLOR_F& cur, const D2D1_COLOR_F& next) {
    return has && cur.r == next.r && cur.g == next.g && cur.b == next.b && cur.a == next.a;
}

const char* VisibilityToString(Visibility v) {
    switch (v) {
    case Visibility::Hidden: return "Hidden";
    case Visibility::Collapsed: return "Collapsed";
    default: return "Visible";
    }
}

Visibility ParseVisibility(const std::string& s, Visibility fallback = Visibility::Visible) {
    if (s == "Hidden") return Visibility::Hidden;
    if (s == "Collapsed") return Visibility::Collapsed;
    if (s == "Visible") return Visibility::Visible;
    return fallback;
}

const char* AlignmentToString(Alignment a) {
    switch (a) {
    case Alignment::Start: return "Start";
    case Alignment::Center: return "Center";
    case Alignment::End: return "End";
    default: return "Stretch";
    }
}

Alignment ParseAlignment(const std::string& s, Alignment fallback = Alignment::Stretch) {
    if (s == "Start") return Alignment::Start;
    if (s == "Center") return Alignment::Center;
    if (s == "End") return Alignment::End;
    if (s == "Stretch") return Alignment::Stretch;
    return fallback;
}

const char* OrientationToString(Orientation o) {
    return (o == Orientation::Horizontal) ? "Horizontal" : "Vertical";
}

Orientation ParseOrientation(const std::string& s, Orientation fallback = Orientation::Vertical) {
    if (s == "Horizontal") return Orientation::Horizontal;
    if (s == "Vertical") return Orientation::Vertical;
    return fallback;
}

const char* DockToString(Dock d) {
    switch (d) {
    case Dock::Top: return "Top";
    case Dock::Right: return "Right";
    case Dock::Bottom: return "Bottom";
    default: return "Left";
    }
}

Dock ParseDock(const std::string& s, Dock fallback = Dock::Left) {
    if (s == "Top") return Dock::Top;
    if (s == "Right") return Dock::Right;
    if (s == "Bottom") return Dock::Bottom;
    if (s == "Left") return Dock::Left;
    return fallback;
}

Value TokenValue(ThemeTokenId id) {
    return Value(ThemeTokenIdToName(id));
}

ThemeTokenId TokenFromValue(const Value& val) {
    if (val.GetType() == Value::Type::String) {
        return ThemeTokenIdFromName(val.AsString());
    }
    return ThemeTokenId::Unset;
}

Thickness ThicknessFromValue(const Value& val) {
    if (val.GetType() == Value::Type::String) {
        return Thickness::Parse(val.AsString());
    }
    return val.AsThickness();
}

// --- PropertyDesc accessors (core layout) ---

void DescGetWidth(const UIElement* self, Value& out) {
    const float w = self->GetWidth();
    out = (w >= 0.0f) ? Value(w) : Value();
}
void DescSetWidth(UIElement* self, const Value& in) { self->SetWidth(in.AsFloat()); }
void DescGetHeight(const UIElement* self, Value& out) {
    const float h = self->GetHeight();
    out = (h >= 0.0f) ? Value(h) : Value();
}
void DescSetHeight(UIElement* self, const Value& in) { self->SetHeight(in.AsFloat()); }
void DescGetMinWidth(const UIElement* self, Value& out) { out = Value(self->GetMinWidth()); }
void DescSetMinWidth(UIElement* self, const Value& in) { self->SetMinWidth(in.AsFloat()); }
void DescGetMinHeight(const UIElement* self, Value& out) { out = Value(self->GetMinHeight()); }
void DescSetMinHeight(UIElement* self, const Value& in) { self->SetMinHeight(in.AsFloat()); }
void DescGetMaxWidth(const UIElement* self, Value& out) { out = Value(self->GetMaxWidth()); }
void DescSetMaxWidth(UIElement* self, const Value& in) { self->SetMaxWidth(in.AsFloat()); }
void DescGetMaxHeight(const UIElement* self, Value& out) { out = Value(self->GetMaxHeight()); }
void DescSetMaxHeight(UIElement* self, const Value& in) { self->SetMaxHeight(in.AsFloat()); }
void DescGetMargin(const UIElement* self, Value& out) { out = Value(self->GetMargin()); }
void DescSetMargin(UIElement* self, const Value& in) { self->SetMargin(ThicknessFromValue(in)); }
void DescGetPadding(const UIElement* self, Value& out) { out = Value(self->GetPadding()); }
void DescSetPadding(UIElement* self, const Value& in) { self->SetPadding(ThicknessFromValue(in)); }
void DescGetOpacity(const UIElement* self, Value& out) { out = Value(self->GetOpacity()); }
void DescSetOpacity(UIElement* self, const Value& in) { self->SetOpacity(in.AsFloat()); }
void DescGetIsEnabled(const UIElement* self, Value& out) { out = Value(self->IsEnabled()); }
void DescSetIsEnabled(UIElement* self, const Value& in) { self->SetIsEnabled(in.AsBool()); }
void DescGetVisibility(const UIElement* self, Value& out) {
    out = Value(VisibilityToString(self->GetVisibility()));
}
void DescSetVisibility(UIElement* self, const Value& in) {
    self->SetVisibility(ParseVisibility(in.AsString("Visible")));
}
void DescGetBorderThickness(const UIElement* self, Value& out) { out = Value(self->GetBorderThickness()); }
void DescSetBorderThickness(UIElement* self, const Value& in) { self->SetBorderThickness(in.AsFloat()); }
void DescGetCornerRadius(const UIElement* self, Value& out) { out = Value(self->GetCornerRadius()); }
void DescSetCornerRadius(UIElement* self, const Value& in) { self->SetCornerRadius(in.AsFloat()); }
void DescGetAlignH(const UIElement* self, Value& out) {
    out = Value(AlignmentToString(self->GetAlignHorizontal()));
}
void DescSetAlignH(UIElement* self, const Value& in) {
    self->SetAlignHorizontal(ParseAlignment(in.AsString("Stretch")));
}
void DescGetAlignV(const UIElement* self, Value& out) {
    out = Value(AlignmentToString(self->GetAlignVertical()));
}
void DescSetAlignV(UIElement* self, const Value& in) {
    self->SetAlignVertical(ParseAlignment(in.AsString("Stretch")));
}
void DescGetText(const UIElement* self, Value& out) { out = Value(self->GetText()); }
void DescSetText(UIElement* self, const Value& in) { self->SetText(in.AsString()); }
void DescGetToolTip(const UIElement* self, Value& out) { out = Value(self->GetToolTip()); }
void DescSetToolTip(UIElement* self, const Value& in) { self->SetToolTip(in.AsString()); }
void DescGetFontFamily(const UIElement* self, Value& out) { out = Value(self->GetFontFamily()); }
void DescSetFontFamily(UIElement* self, const Value& in) { self->SetFontFamily(in.AsString()); }
void DescGetFontSize(const UIElement* self, Value& out) { out = Value(self->GetFontSize()); }
void DescSetFontSize(UIElement* self, const Value& in) { self->SetFontSize(in.AsFloat()); }
void DescGetFontWeight(const UIElement* self, Value& out) { out = Value(FontWeightToString(self->GetFontWeight())); }
void DescSetFontWeight(UIElement* self, const Value& in) { self->SetFontWeight(FontWeightFromString(in.AsString())); }
void DescGetFontStyle(const UIElement* self, Value& out) { out = Value(FontStyleToString(self->GetFontStyle())); }
void DescSetFontStyle(UIElement* self, const Value& in) { self->SetFontStyle(FontStyleFromString(in.AsString())); }
void DescGetFontStretch(const UIElement* self, Value& out) { out = Value(FontStretchToString(self->GetFontStretch())); }
void DescSetFontStretch(UIElement* self, const Value& in) { self->SetFontStretch(FontStretchFromString(in.AsString())); }
void DescGetIsUnderline(const UIElement* self, Value& out) { out = Value(self->IsUnderline()); }
void DescSetIsUnderline(UIElement* self, const Value& in) { self->SetIsUnderline(in.AsBool()); }
void DescGetIsStrikethrough(const UIElement* self, Value& out) { out = Value(self->IsStrikethrough()); }
void DescSetIsStrikethrough(UIElement* self, const Value& in) { self->SetIsStrikethrough(in.AsBool()); }

void DescGetFlexGrow(const UIElement* self, Value& out) { out = Value(self->GetFlexGrow()); }
void DescSetFlexGrow(UIElement* self, const Value& in) { self->SetFlexGrow(in.AsFloat()); }
void DescGetAlign(const UIElement* self, Value& out) { out = Value(AlignmentToString(self->GetAlign())); }
void DescSetAlign(UIElement* self, const Value& in) { self->SetAlign(ParseAlignment(in.AsString("Stretch"))); }
void DescGetOrientation(const UIElement* self, Value& out) { out = Value(OrientationToString(self->GetOrientation())); }
void DescSetOrientation(UIElement* self, const Value& in) { self->SetOrientation(ParseOrientation(in.AsString("Vertical"))); }
void DescGetGap(const UIElement* self, Value& out) { out = Value(self->GetGap()); }
void DescSetGap(UIElement* self, const Value& in) { self->SetGap(in.AsFloat()); }
void DescGetItemWidth(const UIElement* self, Value& out) {
    const float w = self->GetItemWidth();
    out = (w >= 0.0f) ? Value(w) : Value();
}
void DescSetItemWidth(UIElement* self, const Value& in) { self->SetItemWidth(in.AsFloat()); }
void DescGetItemHeight(const UIElement* self, Value& out) {
    const float h = self->GetItemHeight();
    out = (h >= 0.0f) ? Value(h) : Value();
}
void DescSetItemHeight(UIElement* self, const Value& in) { self->SetItemHeight(in.AsFloat()); }
void DescGetLastChildFill(const UIElement* self, Value& out) { out = Value(self->GetLastChildFill()); }
void DescSetLastChildFill(UIElement* self, const Value& in) { self->SetLastChildFill(in.AsBool()); }
void DescGetJustifyLines(const UIElement* self, Value& out) { out = Value(self->GetJustifyLines()); }
void DescSetJustifyLines(UIElement* self, const Value& in) { self->SetJustifyLines(in.AsBool()); }
void DescGetFillLastLine(const UIElement* self, Value& out) { out = Value(self->GetFillLastLine()); }
void DescSetFillLastLine(UIElement* self, const Value& in) { self->SetFillLastLine(in.AsBool()); }
void DescGetRows(const UIElement* self, Value& out) { out = Value(self->GetRows()); }
void DescSetRows(UIElement* self, const Value& in) { self->SetRows(in.AsInt()); }
void DescGetColumns(const UIElement* self, Value& out) { out = Value(self->GetColumns()); }
void DescSetColumns(UIElement* self, const Value& in) { self->SetColumns(in.AsInt()); }
void DescGetClipToBounds(const UIElement* self, Value& out) { out = Value(self->GetClipToBounds()); }
void DescSetClipToBounds(UIElement* self, const Value& in) { self->SetClipToBounds(in.AsBool()); }
void DescGetPlaceholder(const UIElement* self, Value& out) { out = Value(self->GetPlaceholder()); }
void DescSetPlaceholder(UIElement* self, const Value& in) { self->SetPlaceholder(in.AsString()); }
void DescGetIcon(const UIElement* self, Value& out) { out = Value(self->GetIcon()); }
void DescSetIcon(UIElement* self, const Value& in) { self->SetIcon(in.AsString()); }
void DescGetFocused(const UIElement* self, Value& out) { out = Value(self->IsFocused()); }
void DescSetFocused(UIElement* self, const Value& in) {
    if (in.AsBool()) self->OnFocus();
    else self->OnBlur();
}
void DescGetCanvasLeft(const UIElement* self, Value& out) { out = Value(self->GetCanvasLeft()); }
void DescSetCanvasLeft(UIElement* self, const Value& in) { self->SetCanvasLeft(in.AsFloat()); }
void DescGetCanvasTop(const UIElement* self, Value& out) { out = Value(self->GetCanvasTop()); }
void DescSetCanvasTop(UIElement* self, const Value& in) { self->SetCanvasTop(in.AsFloat()); }
void DescGetCanvasRight(const UIElement* self, Value& out) { out = Value(self->GetCanvasRight()); }
void DescSetCanvasRight(UIElement* self, const Value& in) { self->SetCanvasRight(in.AsFloat()); }
void DescGetCanvasBottom(const UIElement* self, Value& out) { out = Value(self->GetCanvasBottom()); }
void DescSetCanvasBottom(UIElement* self, const Value& in) { self->SetCanvasBottom(in.AsFloat()); }
void DescGetZIndex(const UIElement* self, Value& out) { out = Value(self->GetZIndex()); }
void DescSetZIndex(UIElement* self, const Value& in) { self->SetZIndex(in.AsInt()); }
void DescGetGridColumn(const UIElement* self, Value& out) { out = Value(self->GetGridColumn()); }
void DescSetGridColumn(UIElement* self, const Value& in) { self->SetGridColumn(in.AsInt()); }
void DescGetGridRow(const UIElement* self, Value& out) { out = Value(self->GetGridRow()); }
void DescSetGridRow(UIElement* self, const Value& in) { self->SetGridRow(in.AsInt()); }
void DescGetGridColumnSpan(const UIElement* self, Value& out) { out = Value(self->GetGridColumnSpan()); }
void DescSetGridColumnSpan(UIElement* self, const Value& in) { self->SetGridColumnSpan(in.AsInt()); }
void DescGetGridRowSpan(const UIElement* self, Value& out) { out = Value(self->GetGridRowSpan()); }
void DescSetGridRowSpan(UIElement* self, const Value& in) { self->SetGridRowSpan(in.AsInt()); }
void DescGetDock(const UIElement* self, Value& out) { out = Value(DockToString(self->GetDock())); }
void DescSetDock(UIElement* self, const Value& in) { self->SetDock(ParseDock(in.AsString("Left"))); }
void DescGetBackground(const UIElement* self, Value& out) {
    out = self->HasBackgroundColor()
        ? Value(self->GetBackgroundColor())
        : Value(D2D1::ColorF(0, 0, 0, 0));
}
void DescSetBackground(UIElement* self, const Value& in) { self->SetBackground(in.AsColor()); }
void DescGetBorderBrush(const UIElement* self, Value& out) {
    out = self->HasBorderBrushColor()
        ? Value(self->GetBorderBrushColor())
        : Value(D2D1::ColorF(0, 0, 0, 0));
}
void DescSetBorderBrush(UIElement* self, const Value& in) { self->SetBorderBrush(in.AsColor()); }
void DescGetHoverBackground(const UIElement* self, Value& out) {
    out = self->HasHoverBackgroundColor() ? Value(self->GetHoverBackgroundColor()) : Value();
}
void DescSetHoverBackground(UIElement* self, const Value& in) { self->SetHoverBackground(in.AsColor()); }
void DescGetPressedBackground(const UIElement* self, Value& out) {
    out = self->HasPressedBackgroundColor() ? Value(self->GetPressedBackgroundColor()) : Value();
}
void DescSetPressedBackground(UIElement* self, const Value& in) { self->SetPressedBackground(in.AsColor()); }
void DescGetColor(const UIElement* self, Value& out) {
    out = self->HasColorValue()
        ? Value(self->GetColorValue())
        : Value(D2D1::ColorF(1, 1, 1, 1));
}
void DescSetColor(UIElement* self, const Value& in) { self->SetColor(in.AsColor()); }

#define CUI_TOKEN_DESC(PropName, Getter, Setter) \
    void DescGet_##PropName(const UIElement* self, Value& out) { out = TokenValue(self->Getter()); } \
    void DescSet_##PropName(UIElement* self, const Value& in) { self->Setter(TokenFromValue(in)); }

CUI_TOKEN_DESC(BackgroundToken, GetBackgroundToken, SetBackgroundToken)
CUI_TOKEN_DESC(HoverBackgroundToken, GetHoverBackgroundToken, SetHoverBackgroundToken)
CUI_TOKEN_DESC(PressedBackgroundToken, GetPressedBackgroundToken, SetPressedBackgroundToken)
CUI_TOKEN_DESC(DisabledBackgroundToken, GetDisabledBackgroundToken, SetDisabledBackgroundToken)
CUI_TOKEN_DESC(BorderToken, GetBorderToken, SetBorderToken)
CUI_TOKEN_DESC(FocusedBorderToken, GetFocusedBorderToken, SetFocusedBorderToken)
CUI_TOKEN_DESC(ColorToken, GetColorToken, SetColorToken)
CUI_TOKEN_DESC(SecondaryColorToken, GetSecondaryColorToken, SetSecondaryColorToken)
CUI_TOKEN_DESC(PlaceholderColorToken, GetPlaceholderColorToken, SetPlaceholderColorToken)
CUI_TOKEN_DESC(SelectedBackgroundToken, GetSelectedBackgroundToken, SetSelectedBackgroundToken)
CUI_TOKEN_DESC(HeaderBackgroundToken, GetHeaderBackgroundToken, SetHeaderBackgroundToken)
CUI_TOKEN_DESC(PaneBackgroundToken, GetPaneBackgroundToken, SetPaneBackgroundToken)
CUI_TOKEN_DESC(IndicatorColorToken, GetIndicatorColorToken, SetIndicatorColorToken)
CUI_TOKEN_DESC(DropdownBackgroundToken, GetDropdownBackgroundToken, SetDropdownBackgroundToken)
CUI_TOKEN_DESC(SelectedItemBackgroundToken, GetSelectedItemBackgroundToken, SetSelectedItemBackgroundToken)
CUI_TOKEN_DESC(FillColorToken, GetFillColorToken, SetFillColorToken)
CUI_TOKEN_DESC(TrackColorToken, GetTrackColorToken, SetTrackColorToken)
CUI_TOKEN_DESC(ActiveTrackColorToken, GetActiveTrackColorToken, SetActiveTrackColorToken)
CUI_TOKEN_DESC(ThumbColorToken, GetThumbColorToken, SetThumbColorToken)
CUI_TOKEN_DESC(OnColorToken, GetOnColorToken, SetOnColorToken)
CUI_TOKEN_DESC(OffColorToken, GetOffColorToken, SetOffColorToken)
CUI_TOKEN_DESC(KnobColorToken, GetKnobColorToken, SetKnobColorToken)
CUI_TOKEN_DESC(CheckedBackgroundToken, GetCheckedBackgroundToken, SetCheckedBackgroundToken)
CUI_TOKEN_DESC(AccentColorToken, GetAccentColorToken, SetAccentColorToken)
CUI_TOKEN_DESC(ActiveColorToken, GetActiveColorToken, SetActiveColorToken)
CUI_TOKEN_DESC(UnderlineColorToken, GetUnderlineColorToken, SetUnderlineColorToken)
CUI_TOKEN_DESC(ActiveUnderlineColorToken, GetActiveUnderlineColorToken, SetActiveUnderlineColorToken)
CUI_TOKEN_DESC(ActiveTabBackgroundToken, GetActiveTabBackgroundToken, SetActiveTabBackgroundToken)
CUI_TOKEN_DESC(InactiveTabBackgroundToken, GetInactiveTabBackgroundToken, SetInactiveTabBackgroundToken)
CUI_TOKEN_DESC(GridLineBrushToken, GetGridLineBrushToken, SetGridLineBrushToken)
CUI_TOKEN_DESC(TitleColorToken, GetTitleColorToken, SetTitleColorToken)
CUI_TOKEN_DESC(MessageColorToken, GetMessageColorToken, SetMessageColorToken)
CUI_TOKEN_DESC(CaretColorToken, GetCaretColorToken, SetCaretColorToken)

#undef CUI_TOKEN_DESC

static const char* const kVisibilityOptions[] = { "Visible", "Hidden", "Collapsed", nullptr };
static const char* const kAlignmentOptions[] = { "Stretch", "Start", "Center", "End", nullptr };
static const char* const kOrientationOptions[] = { "Vertical", "Horizontal", nullptr };
static const char* const kDockOptions[] = { "Left", "Top", "Right", "Bottom", nullptr };
static const char* const kFontFamilyOptions[] = { "微软雅黑", "Segoe UI", "Consolas", "Times New Roman", nullptr };
static const char* const kFontWeightOptions[] = { "Thin", "ExtraLight", "Light", "Normal", "Medium", "SemiBold", "Bold", "ExtraBold", "Black", nullptr };
static const char* const kFontStyleOptions[] = { "Normal", "Italic", "Oblique", nullptr };
static const char* const kFontStretchOptions[] = { "UltraCondensed", "ExtraCondensed", "Condensed", "SemiCondensed", "Normal", "SemiExpanded", "Expanded", "ExtraExpanded", "UltraExpanded", nullptr };

static const PropertyDesc kUIElementDescs[] = {
    { PropertyId::Text, "文本内容 (Text)", "基本信息", PropertyKind::String, nullptr, &DescGetText, &DescSetText },
    { PropertyId::ToolTip, "提示信息 (ToolTip)", "基本信息", PropertyKind::String, nullptr, &DescGetToolTip, &DescSetToolTip },

    { PropertyId::BackgroundToken, "背景 Token", "主题色彩", PropertyKind::ThemeToken, nullptr, &DescGet_BackgroundToken, &DescSet_BackgroundToken },
    { PropertyId::HoverBackgroundToken, "悬停背景 Token", "主题色彩", PropertyKind::ThemeToken, nullptr, &DescGet_HoverBackgroundToken, &DescSet_HoverBackgroundToken },
    { PropertyId::PressedBackgroundToken, "按下背景 Token", "主题色彩", PropertyKind::ThemeToken, nullptr, &DescGet_PressedBackgroundToken, &DescSet_PressedBackgroundToken },
    { PropertyId::DisabledBackgroundToken, "禁用背景 Token", "主题色彩", PropertyKind::ThemeToken, nullptr, &DescGet_DisabledBackgroundToken, &DescSet_DisabledBackgroundToken },
    { PropertyId::BorderToken, "边框 Token", "主题色彩", PropertyKind::ThemeToken, nullptr, &DescGet_BorderToken, &DescSet_BorderToken },
    { PropertyId::FocusedBorderToken, "焦点边框 Token", "主题色彩", PropertyKind::ThemeToken, nullptr, &DescGet_FocusedBorderToken, &DescSet_FocusedBorderToken },
    { PropertyId::ColorToken, "文字 Token", "主题色彩", PropertyKind::ThemeToken, nullptr, &DescGet_ColorToken, &DescSet_ColorToken },
    { PropertyId::SecondaryColorToken, "次要文字 Token", "主题色彩", PropertyKind::ThemeToken, nullptr, &DescGet_SecondaryColorToken, &DescSet_SecondaryColorToken },
    { PropertyId::PlaceholderColorToken, "占位文字 Token", "主题色彩", PropertyKind::ThemeToken, nullptr, &DescGet_PlaceholderColorToken, &DescSet_PlaceholderColorToken },
    { PropertyId::SelectedBackgroundToken, "选中背景 Token", "主题色彩", PropertyKind::ThemeToken, nullptr, &DescGet_SelectedBackgroundToken, &DescSet_SelectedBackgroundToken },
    { PropertyId::HeaderBackgroundToken, "表头背景 Token", "主题色彩", PropertyKind::ThemeToken, nullptr, &DescGet_HeaderBackgroundToken, &DescSet_HeaderBackgroundToken },
    { PropertyId::PaneBackgroundToken, "面板背景 Token", "主题色彩", PropertyKind::ThemeToken, nullptr, &DescGet_PaneBackgroundToken, &DescSet_PaneBackgroundToken },
    { PropertyId::IndicatorColorToken, "指示器 Token", "主题色彩", PropertyKind::ThemeToken, nullptr, &DescGet_IndicatorColorToken, &DescSet_IndicatorColorToken },
    { PropertyId::DropdownBackgroundToken, "下拉背景 Token", "主题色彩", PropertyKind::ThemeToken, nullptr, &DescGet_DropdownBackgroundToken, &DescSet_DropdownBackgroundToken },
    { PropertyId::SelectedItemBackgroundToken, "选中项背景 Token", "主题色彩", PropertyKind::ThemeToken, nullptr, &DescGet_SelectedItemBackgroundToken, &DescSet_SelectedItemBackgroundToken },
    { PropertyId::FillColorToken, "填充 Token", "主题色彩", PropertyKind::ThemeToken, nullptr, &DescGet_FillColorToken, &DescSet_FillColorToken },
    { PropertyId::TrackColorToken, "轨道 Token", "主题色彩", PropertyKind::ThemeToken, nullptr, &DescGet_TrackColorToken, &DescSet_TrackColorToken },
    { PropertyId::ActiveTrackColorToken, "激活轨 Token", "主题色彩", PropertyKind::ThemeToken, nullptr, &DescGet_ActiveTrackColorToken, &DescSet_ActiveTrackColorToken },
    { PropertyId::ThumbColorToken, "滑块 Token", "主题色彩", PropertyKind::ThemeToken, nullptr, &DescGet_ThumbColorToken, &DescSet_ThumbColorToken },
    { PropertyId::OnColorToken, "开启 Token", "主题色彩", PropertyKind::ThemeToken, nullptr, &DescGet_OnColorToken, &DescSet_OnColorToken },
    { PropertyId::OffColorToken, "关闭 Token", "主题色彩", PropertyKind::ThemeToken, nullptr, &DescGet_OffColorToken, &DescSet_OffColorToken },
    { PropertyId::KnobColorToken, "旋钮 Token", "主题色彩", PropertyKind::ThemeToken, nullptr, &DescGet_KnobColorToken, &DescSet_KnobColorToken },
    { PropertyId::CheckedBackgroundToken, "选中背景 Token", "主题色彩", PropertyKind::ThemeToken, nullptr, &DescGet_CheckedBackgroundToken, &DescSet_CheckedBackgroundToken },
    { PropertyId::AccentColorToken, "强调色 Token", "主题色彩", PropertyKind::ThemeToken, nullptr, &DescGet_AccentColorToken, &DescSet_AccentColorToken },
    { PropertyId::ActiveColorToken, "激活 Token", "主题色彩", PropertyKind::ThemeToken, nullptr, &DescGet_ActiveColorToken, &DescSet_ActiveColorToken },
    { PropertyId::UnderlineColorToken, "下划线 Token", "主题色彩", PropertyKind::ThemeToken, nullptr, &DescGet_UnderlineColorToken, &DescSet_UnderlineColorToken },
    { PropertyId::ActiveUnderlineColorToken, "激活下划线 Token", "主题色彩", PropertyKind::ThemeToken, nullptr, &DescGet_ActiveUnderlineColorToken, &DescSet_ActiveUnderlineColorToken },
    { PropertyId::ActiveTabBackgroundToken, "活动标签 Token", "主题色彩", PropertyKind::ThemeToken, nullptr, &DescGet_ActiveTabBackgroundToken, &DescSet_ActiveTabBackgroundToken },
    { PropertyId::InactiveTabBackgroundToken, "非活动标签 Token", "主题色彩", PropertyKind::ThemeToken, nullptr, &DescGet_InactiveTabBackgroundToken, &DescSet_InactiveTabBackgroundToken },
    { PropertyId::GridLineBrushToken, "网格线 Token", "主题色彩", PropertyKind::ThemeToken, nullptr, &DescGet_GridLineBrushToken, &DescSet_GridLineBrushToken },
    { PropertyId::TitleColorToken, "标题 Token", "主题色彩", PropertyKind::ThemeToken, nullptr, &DescGet_TitleColorToken, &DescSet_TitleColorToken },
    { PropertyId::MessageColorToken, "正文 Token", "主题色彩", PropertyKind::ThemeToken, nullptr, &DescGet_MessageColorToken, &DescSet_MessageColorToken },
    { PropertyId::CaretColorToken, "光标 Token", "主题色彩", PropertyKind::ThemeToken, nullptr, &DescGet_CaretColorToken, &DescSet_CaretColorToken },

    { PropertyId::Width, "宽度 (Width) [-1自适应]", "尺寸布局", PropertyKind::Float, nullptr, &DescGetWidth, &DescSetWidth },
    { PropertyId::Height, "高度 (Height) [-1自适应]", "尺寸布局", PropertyKind::Float, nullptr, &DescGetHeight, &DescSetHeight },
    { PropertyId::MinWidth, "最小宽度 (MinWidth)", "尺寸布局", PropertyKind::Float, nullptr, &DescGetMinWidth, &DescSetMinWidth },
    { PropertyId::MinHeight, "最小高度 (MinHeight)", "尺寸布局", PropertyKind::Float, nullptr, &DescGetMinHeight, &DescSetMinHeight },
    { PropertyId::MaxWidth, "最大宽度 (MaxWidth)", "尺寸布局", PropertyKind::Float, nullptr, &DescGetMaxWidth, &DescSetMaxWidth },
    { PropertyId::MaxHeight, "最大高度 (MaxHeight)", "尺寸布局", PropertyKind::Float, nullptr, &DescGetMaxHeight, &DescSetMaxHeight },
    { PropertyId::Margin, "外边距 (Margin)", "尺寸布局", PropertyKind::Thickness, nullptr, &DescGetMargin, &DescSetMargin },
    { PropertyId::Padding, "内边距 (Padding)", "尺寸布局", PropertyKind::Thickness, nullptr, &DescGetPadding, &DescSetPadding },
    { PropertyId::AlignHorizontal, "水平对齐 (AlignH)", "尺寸布局", PropertyKind::Enum, kAlignmentOptions, &DescGetAlignH, &DescSetAlignH },
    { PropertyId::AlignVertical, "垂直对齐 (AlignV)", "尺寸布局", PropertyKind::Enum, kAlignmentOptions, &DescGetAlignV, &DescSetAlignV },
    { PropertyId::FlexGrow, "Flex 伸展", "面板布局", PropertyKind::Float, nullptr, &DescGetFlexGrow, &DescSetFlexGrow },
    { PropertyId::Align, "子项对齐 (Align)", "面板布局", PropertyKind::Enum, kAlignmentOptions, &DescGetAlign, &DescSetAlign },
    { PropertyId::Orientation, "方向 (Orientation)", "面板布局", PropertyKind::Enum, kOrientationOptions, &DescGetOrientation, &DescSetOrientation },
    { PropertyId::Gap, "间距 (Gap)", "面板布局", PropertyKind::Float, nullptr, &DescGetGap, &DescSetGap },
    { PropertyId::ItemWidth, "子项宽度 (ItemWidth)", "面板布局", PropertyKind::Float, nullptr, &DescGetItemWidth, &DescSetItemWidth },
    { PropertyId::ItemHeight, "子项高度 (ItemHeight)", "面板布局", PropertyKind::Float, nullptr, &DescGetItemHeight, &DescSetItemHeight },
    { PropertyId::LastChildFill, "末子填充 (LastChildFill)", "面板布局", PropertyKind::Bool, nullptr, &DescGetLastChildFill, &DescSetLastChildFill },
    { PropertyId::JustifyLines, "行填满 (JustifyLines)", "面板布局", PropertyKind::Bool, nullptr, &DescGetJustifyLines, &DescSetJustifyLines },
    { PropertyId::FillLastLine, "最后一行填满 (FillLastLine)", "面板布局", PropertyKind::Bool, nullptr, &DescGetFillLastLine, &DescSetFillLastLine },
    { PropertyId::Rows, "行数 (Rows)", "面板布局", PropertyKind::Int, nullptr, &DescGetRows, &DescSetRows },
    { PropertyId::Columns, "列数 (Columns)", "面板布局", PropertyKind::Int, nullptr, &DescGetColumns, &DescSetColumns },

    { PropertyId::CanvasLeft, "Canvas.Left", "Canvas", PropertyKind::Float, nullptr, &DescGetCanvasLeft, &DescSetCanvasLeft },
    { PropertyId::CanvasTop, "Canvas.Top", "Canvas", PropertyKind::Float, nullptr, &DescGetCanvasTop, &DescSetCanvasTop },
    { PropertyId::CanvasRight, "Canvas.Right", "Canvas", PropertyKind::Float, nullptr, &DescGetCanvasRight, &DescSetCanvasRight },
    { PropertyId::CanvasBottom, "Canvas.Bottom", "Canvas", PropertyKind::Float, nullptr, &DescGetCanvasBottom, &DescSetCanvasBottom },
    { PropertyId::ZIndex, "Canvas.ZIndex", "Canvas", PropertyKind::Int, nullptr, &DescGetZIndex, &DescSetZIndex },
    { PropertyId::GridColumn, "Grid.Column", "Grid", PropertyKind::Int, nullptr, &DescGetGridColumn, &DescSetGridColumn },
    { PropertyId::GridRow, "Grid.Row", "Grid", PropertyKind::Int, nullptr, &DescGetGridRow, &DescSetGridRow },
    { PropertyId::GridColumnSpan, "Grid.ColumnSpan", "Grid", PropertyKind::Int, nullptr, &DescGetGridColumnSpan, &DescSetGridColumnSpan },
    { PropertyId::GridRowSpan, "Grid.RowSpan", "Grid", PropertyKind::Int, nullptr, &DescGetGridRowSpan, &DescSetGridRowSpan },
    { PropertyId::Dock, "DockPanel.Dock", "Dock", PropertyKind::Enum, kDockOptions, &DescGetDock, &DescSetDock },

    { PropertyId::BorderThickness, "边框粗细 (BorderThickness)", "外观", PropertyKind::Float, nullptr, &DescGetBorderThickness, &DescSetBorderThickness },
    { PropertyId::CornerRadius, "圆角半径 (CornerRadius)", "外观", PropertyKind::Float, nullptr, &DescGetCornerRadius, &DescSetCornerRadius },
    { PropertyId::Opacity, "不透明度 (Opacity) [0-1]", "外观", PropertyKind::Float, nullptr, &DescGetOpacity, &DescSetOpacity },
    { PropertyId::ClipToBounds, "裁剪到边界 (ClipToBounds)", "外观", PropertyKind::Bool, nullptr, &DescGetClipToBounds, &DescSetClipToBounds },
    { PropertyId::Background, "背景色 (Background)", "外观", PropertyKind::Color, nullptr, &DescGetBackground, &DescSetBackground },
    { PropertyId::BorderBrush, "边框色 (BorderBrush)", "外观", PropertyKind::Color, nullptr, &DescGetBorderBrush, &DescSetBorderBrush },
    { PropertyId::HoverBackground, "悬停背景 (HoverBackground)", "外观", PropertyKind::Color, nullptr, &DescGetHoverBackground, &DescSetHoverBackground },
    { PropertyId::PressedBackground, "按下背景 (PressedBackground)", "外观", PropertyKind::Color, nullptr, &DescGetPressedBackground, &DescSetPressedBackground },
    { PropertyId::Color, "前景色 (Color)", "外观", PropertyKind::Color, nullptr, &DescGetColor, &DescSetColor },

    { PropertyId::FontFamily, "字体名称 (FontFamily)", "字体文本", PropertyKind::Enum, kFontFamilyOptions, &DescGetFontFamily, &DescSetFontFamily },
    { PropertyId::FontSize, "字体大小 (FontSize)", "字体文本", PropertyKind::Float, nullptr, &DescGetFontSize, &DescSetFontSize },
    { PropertyId::FontWeight, "字体粗细 (FontWeight)", "字体文本", PropertyKind::Enum, kFontWeightOptions, &DescGetFontWeight, &DescSetFontWeight },
    { PropertyId::FontStyle, "字体样式 (FontStyle)", "字体文本", PropertyKind::Enum, kFontStyleOptions, &DescGetFontStyle, &DescSetFontStyle },
    { PropertyId::FontStretch, "字体宽度 (FontStretch)", "字体文本", PropertyKind::Enum, kFontStretchOptions, &DescGetFontStretch, &DescSetFontStretch },
    { PropertyId::IsUnderline, "下划线 (IsUnderline)", "字体文本", PropertyKind::Bool, nullptr, &DescGetIsUnderline, &DescSetIsUnderline },
    { PropertyId::IsStrikethrough, "删除线 (IsStrikethrough)", "字体文本", PropertyKind::Bool, nullptr, &DescGetIsStrikethrough, &DescSetIsStrikethrough },
    { PropertyId::Placeholder, "占位符 (Placeholder)", "字体文本", PropertyKind::String, nullptr, &DescGetPlaceholder, &DescSetPlaceholder },
    { PropertyId::Icon, "图标 (Icon)", "基本信息", PropertyKind::String, nullptr, &DescGetIcon, &DescSetIcon },

    { PropertyId::IsEnabled, "是否启用 (IsEnabled)", "交互状态", PropertyKind::Bool, nullptr, &DescGetIsEnabled, &DescSetIsEnabled },
    { PropertyId::Visibility, "显示状态 (Visibility)", "交互状态", PropertyKind::Enum, kVisibilityOptions, &DescGetVisibility, &DescSetVisibility },
    { PropertyId::Focused, "焦点 (Focused)", "交互状态", PropertyKind::Bool, nullptr, &DescGetFocused, &DescSetFocused },
};

static constexpr size_t kUIElementDescCount = sizeof(kUIElementDescs) / sizeof(kUIElementDescs[0]);

const PropertyDesc* gUIElementDescById[static_cast<size_t>(PropertyId::Count)] = {};

struct UIElementDescLookupInit {
    UIElementDescLookupInit() {
        for (size_t i = 0; i < kUIElementDescCount; ++i) {
            const PropertyId id = kUIElementDescs[i].id;
            const auto idx = static_cast<size_t>(id);
            if (id != PropertyId::None && idx < static_cast<size_t>(PropertyId::Count)) {
                gUIElementDescById[idx] = &kUIElementDescs[i];
            }
        }
    }
};

static UIElementDescLookupInit gUIElementDescLookupInit;

} // namespace

void UIElement::NotifyFieldChanged(PropertyId id, const Value& val) {
    NotifyPropertyIdChanged(id, val);
    MarkRenderContentDirty();
}

// --- Typed layout / chrome setters ---
// Same-value writes must not InvalidateMeasure / MarkRenderContentDirty.
// FileBrowser / RelayoutChildren / theme walks call these every pass.

void UIElement::SetWidth(float v) {
    if (m_width == v) return;
    m_width = v;
    NotifyFieldChanged(PropertyId::Width, Value(v));
    InvalidateMeasure();
}

void UIElement::SetHeight(float v) {
    if (m_height == v) return;
    m_height = v;
    NotifyFieldChanged(PropertyId::Height, Value(v));
    InvalidateMeasure();
}

void UIElement::SetMinWidth(float v) {
    if (m_minWidth == v) return;
    m_minWidth = v;
    NotifyFieldChanged(PropertyId::MinWidth, Value(v));
    InvalidateMeasure();
}

void UIElement::SetMinHeight(float v) {
    if (m_minHeight == v) return;
    m_minHeight = v;
    NotifyFieldChanged(PropertyId::MinHeight, Value(v));
    InvalidateMeasure();
}

void UIElement::SetMaxWidth(float v) {
    if (m_maxWidth == v) return;
    m_maxWidth = v;
    NotifyFieldChanged(PropertyId::MaxWidth, Value(v));
    InvalidateMeasure();
}

void UIElement::SetMaxHeight(float v) {
    if (m_maxHeight == v) return;
    m_maxHeight = v;
    NotifyFieldChanged(PropertyId::MaxHeight, Value(v));
    InvalidateMeasure();
}

void UIElement::SetMargin(const Thickness& margin) {
    if (m_margin == margin) return;
    m_margin = margin;
    NotifyFieldChanged(PropertyId::Margin, Value(margin));
    InvalidateMeasure();
}

void UIElement::SetPadding(const Thickness& padding) {
    if (m_padding == padding) return;
    m_padding = padding;
    NotifyFieldChanged(PropertyId::Padding, Value(padding));
    InvalidateMeasure();
}

void UIElement::SetVisibility(Visibility v) {
    if (m_visibility == v) {
        return;
    }
    m_visibility = v;
    NotifyFieldChanged(PropertyId::Visibility, Value(VisibilityToString(v)));
    InvalidateMeasure();
}

void UIElement::SetIsEnabled(bool enabled) {
    if (m_isEnabled == enabled) return;
    m_isEnabled = enabled;
    NotifyFieldChanged(PropertyId::IsEnabled, Value(enabled));
}

void UIElement::SetOpacity(float v) {
    if (m_layerPromoted) {
        if (std::abs(v - m_composeOpacity) < 0.0005f) {
            return;
        }
        SetComposeOpacity(v);
        NotifyFieldChanged(PropertyId::Opacity, Value(v));
        return;
    }
    if (m_opacity == v) return;
    m_opacity = v;
    NotifyFieldChanged(PropertyId::Opacity, Value(v));
}

void UIElement::SetCornerRadius(float v) {
    if (m_cornerRadius == v) return;
    m_cornerRadius = v;
    NotifyFieldChanged(PropertyId::CornerRadius, Value(v));
}

void UIElement::SetBorderThickness(float v) {
    if (m_borderThickness == v) return;
    m_borderThickness = v;
    NotifyFieldChanged(PropertyId::BorderThickness, Value(v));
}

void UIElement::SetFlexGrow(float v) {
    if (m_flexGrow == v) return;
    m_flexGrow = v;
    NotifyFieldChanged(PropertyId::FlexGrow, Value(v));
    InvalidateMeasure();
}

void UIElement::SetAlign(Alignment a) {
    if (m_align == a) return;
    m_align = a;
    NotifyFieldChanged(PropertyId::Align, Value(AlignmentToString(a)));
    InvalidateArrange();
}

void UIElement::SetAlignHorizontal(Alignment a) {
    if (m_alignHorizontal == a) return;
    m_alignHorizontal = a;
    NotifyFieldChanged(PropertyId::AlignHorizontal, Value(AlignmentToString(a)));
    InvalidateArrange();
}

void UIElement::SetAlignVertical(Alignment a) {
    if (m_alignVertical == a) return;
    m_alignVertical = a;
    NotifyFieldChanged(PropertyId::AlignVertical, Value(AlignmentToString(a)));
    InvalidateArrange();
}

void UIElement::SetOrientation(CUI::Orientation o) {
    if (m_orientation == o) return;
    m_orientation = o;
    NotifyFieldChanged(PropertyId::Orientation, Value(OrientationToString(o)));
    InvalidateMeasure();
}

void UIElement::SetGap(float v) {
    if (m_gap == v) return;
    m_gap = v;
    NotifyFieldChanged(PropertyId::Gap, Value(v));
    InvalidateMeasure();
}

void UIElement::SetItemWidth(float v) {
    if (m_itemWidth == v) return;
    m_itemWidth = v;
    NotifyFieldChanged(PropertyId::ItemWidth, Value(v));
    InvalidateMeasure();
}

void UIElement::SetItemHeight(float v) {
    if (m_itemHeight == v) return;
    m_itemHeight = v;
    NotifyFieldChanged(PropertyId::ItemHeight, Value(v));
    InvalidateMeasure();
}

void UIElement::SetJustifyLines(bool v) {
    if (m_justifyLines == v) return;
    m_justifyLines = v;
    NotifyFieldChanged(PropertyId::JustifyLines, Value(v));
    InvalidateMeasure();
}

void UIElement::SetFillLastLine(bool v) {
    if (m_fillLastLine == v) return;
    m_fillLastLine = v;
    NotifyFieldChanged(PropertyId::FillLastLine, Value(v));
    InvalidateMeasure();
}

void UIElement::SetLastChildFill(bool v) {
    if (m_lastChildFill == v) return;
    m_lastChildFill = v;
    NotifyFieldChanged(PropertyId::LastChildFill, Value(v));
    InvalidateMeasure();
}

void UIElement::SetRows(int v) {
    if (m_rows == v) return;
    m_rows = v;
    NotifyFieldChanged(PropertyId::Rows, Value(v));
    InvalidateMeasure();
}

void UIElement::SetColumns(int v) {
    if (m_columns == v) return;
    m_columns = v;
    NotifyFieldChanged(PropertyId::Columns, Value(v));
    InvalidateMeasure();
}

void UIElement::SetClipToBounds(bool v) {
    if (m_clipToBounds == v) return;
    m_clipToBounds = v;
    NotifyFieldChanged(PropertyId::ClipToBounds, Value(v));
}

void UIElement::SetCanvasLeft(float v) {
    if (m_canvasLeft == v) return;
    m_canvasLeft = v;
    NotifyFieldChanged(PropertyId::CanvasLeft, Value(v));
    // 附加坐标影响排列位置，必须失效布局，否则位置变更不会重新 Arrange。
    InvalidateArrange();
}

void UIElement::SetCanvasTop(float v) {
    if (m_canvasTop == v) return;
    m_canvasTop = v;
    NotifyFieldChanged(PropertyId::CanvasTop, Value(v));
    InvalidateArrange();
}

void UIElement::SetCanvasRight(float v) {
    if (m_canvasRight == v) return;
    m_canvasRight = v;
    NotifyFieldChanged(PropertyId::CanvasRight, Value(v));
    InvalidateArrange();
}

void UIElement::SetCanvasBottom(float v) {
    if (m_canvasBottom == v) return;
    m_canvasBottom = v;
    NotifyFieldChanged(PropertyId::CanvasBottom, Value(v));
    InvalidateArrange();
}

void UIElement::SetZIndex(int v) {
    if (m_zIndex == v) return;
    m_zIndex = v;
    NotifyFieldChanged(PropertyId::ZIndex, Value(v));
    if (m_parent) {
        m_parent->MarkRenderContentDirty();
    }
}

void UIElement::SetGridColumn(int v) {
    if (m_gridColumn == v) return;
    m_gridColumn = v;
    NotifyFieldChanged(PropertyId::GridColumn, Value(v));
    InvalidateArrange();
}

void UIElement::SetGridRow(int v) {
    if (m_gridRow == v) return;
    m_gridRow = v;
    NotifyFieldChanged(PropertyId::GridRow, Value(v));
    InvalidateArrange();
}

void UIElement::SetGridColumnSpan(int v) {
    if (m_gridColumnSpan == v) return;
    m_gridColumnSpan = v;
    NotifyFieldChanged(PropertyId::GridColumnSpan, Value(v));
    InvalidateArrange();
}

void UIElement::SetGridRowSpan(int v) {
    if (m_gridRowSpan == v) return;
    m_gridRowSpan = v;
    NotifyFieldChanged(PropertyId::GridRowSpan, Value(v));
    InvalidateArrange();
}

void UIElement::SetDock(Dock d) {
    if (m_dock == d) return;
    m_dock = d;
    NotifyFieldChanged(PropertyId::Dock, Value(DockToString(d)));
    // 停靠方位变化会改变 DockPanel 的切割顺序，必须让父容器重新测量/排列。
    InvalidateMeasure();
}

// --- Theme token setters ---

void UIElement::SetBackgroundToken(ThemeTokenId id) {
    if (m_backgroundToken == id && !m_hasBackgroundColor) return;
    m_backgroundToken = id;
    m_hasBackgroundColor = false;
    NotifyFieldChanged(PropertyId::BackgroundToken, TokenValue(id));
}

void UIElement::SetHoverBackgroundToken(ThemeTokenId id) {
    if (m_hoverBackgroundToken == id && !m_hasHoverBackgroundColor) return;
    m_hoverBackgroundToken = id;
    // 显式指定的 Token 应覆盖基类/构造器预设的硬编码悬浮色（如 Button 的默认蓝色强调色），
    // 否则点击聚焦后视觉状态会一直沿用旧硬编码色（表现为“点击后变蓝、失焦才消失”）。
    m_hasHoverBackgroundColor = false;
    NotifyFieldChanged(PropertyId::HoverBackgroundToken, TokenValue(id));
}

void UIElement::SetPressedBackgroundToken(ThemeTokenId id) {
    if (m_pressedBackgroundToken == id && !m_hasPressedBackgroundColor) return;
    m_pressedBackgroundToken = id;
    m_hasPressedBackgroundColor = false;
    NotifyFieldChanged(PropertyId::PressedBackgroundToken, TokenValue(id));
}

void UIElement::SetDisabledBackgroundToken(ThemeTokenId id) {
    if (m_disabledBackgroundToken == id) return;
    m_disabledBackgroundToken = id;
    NotifyFieldChanged(PropertyId::DisabledBackgroundToken, TokenValue(id));
}

void UIElement::SetBorderToken(ThemeTokenId id) {
    if (m_borderToken == id && !m_hasBorderBrushColor) return;
    m_borderToken = id;
    m_hasBorderBrushColor = false;
    NotifyFieldChanged(PropertyId::BorderToken, TokenValue(id));
}

void UIElement::SetFocusedBorderToken(ThemeTokenId id) {
    if (m_focusedBorderToken == id) return;
    m_focusedBorderToken = id;
    NotifyFieldChanged(PropertyId::FocusedBorderToken, TokenValue(id));
}

void UIElement::SetColorToken(ThemeTokenId id) {
    if (m_colorToken == id && !m_hasColorValue) return;
    m_colorToken = id;
    m_hasColorValue = false;
    NotifyFieldChanged(PropertyId::ColorToken, TokenValue(id));
}

void UIElement::SetSecondaryColorToken(ThemeTokenId id) {
    if (m_secondaryColorToken == id) return;
    m_secondaryColorToken = id;
    NotifyFieldChanged(PropertyId::SecondaryColorToken, TokenValue(id));
}

void UIElement::SetPlaceholderColorToken(ThemeTokenId id) {
    if (m_placeholderColorToken == id) return;
    m_placeholderColorToken = id;
    NotifyFieldChanged(PropertyId::PlaceholderColorToken, TokenValue(id));
}

void UIElement::SetSelectedBackgroundToken(ThemeTokenId id) {
    if (m_selectedBackgroundToken == id) return;
    m_selectedBackgroundToken = id;
    NotifyFieldChanged(PropertyId::SelectedBackgroundToken, TokenValue(id));
}

void UIElement::SetHeaderBackgroundToken(ThemeTokenId id) {
    if (m_headerBackgroundToken == id) return;
    m_headerBackgroundToken = id;
    NotifyFieldChanged(PropertyId::HeaderBackgroundToken, TokenValue(id));
}

void UIElement::SetPaneBackgroundToken(ThemeTokenId id) {
    if (m_paneBackgroundToken == id) return;
    m_paneBackgroundToken = id;
    NotifyFieldChanged(PropertyId::PaneBackgroundToken, TokenValue(id));
}

void UIElement::SetIndicatorColorToken(ThemeTokenId id) {
    if (m_indicatorColorToken == id) return;
    m_indicatorColorToken = id;
    NotifyFieldChanged(PropertyId::IndicatorColorToken, TokenValue(id));
}

void UIElement::SetDropdownBackgroundToken(ThemeTokenId id) {
    if (m_dropdownBackgroundToken == id) return;
    m_dropdownBackgroundToken = id;
    NotifyFieldChanged(PropertyId::DropdownBackgroundToken, TokenValue(id));
}

void UIElement::SetSelectedItemBackgroundToken(ThemeTokenId id) {
    if (m_selectedItemBackgroundToken == id) return;
    m_selectedItemBackgroundToken = id;
    NotifyFieldChanged(PropertyId::SelectedItemBackgroundToken, TokenValue(id));
}

void UIElement::SetFillColorToken(ThemeTokenId id) {
    if (m_fillColorToken == id) return;
    m_fillColorToken = id;
    NotifyFieldChanged(PropertyId::FillColorToken, TokenValue(id));
}

void UIElement::SetTrackColorToken(ThemeTokenId id) {
    if (m_trackColorToken == id) return;
    m_trackColorToken = id;
    NotifyFieldChanged(PropertyId::TrackColorToken, TokenValue(id));
}

void UIElement::SetActiveTrackColorToken(ThemeTokenId id) {
    if (m_activeTrackColorToken == id) return;
    m_activeTrackColorToken = id;
    NotifyFieldChanged(PropertyId::ActiveTrackColorToken, TokenValue(id));
}

void UIElement::SetThumbColorToken(ThemeTokenId id) {
    if (m_thumbColorToken == id) return;
    m_thumbColorToken = id;
    NotifyFieldChanged(PropertyId::ThumbColorToken, TokenValue(id));
}

void UIElement::SetOnColorToken(ThemeTokenId id) {
    if (m_onColorToken == id) return;
    m_onColorToken = id;
    NotifyFieldChanged(PropertyId::OnColorToken, TokenValue(id));
}

void UIElement::SetOffColorToken(ThemeTokenId id) {
    if (m_offColorToken == id) return;
    m_offColorToken = id;
    NotifyFieldChanged(PropertyId::OffColorToken, TokenValue(id));
}

void UIElement::SetKnobColorToken(ThemeTokenId id) {
    if (m_knobColorToken == id) return;
    m_knobColorToken = id;
    NotifyFieldChanged(PropertyId::KnobColorToken, TokenValue(id));
}

void UIElement::SetCheckedBackgroundToken(ThemeTokenId id) {
    if (m_checkedBackgroundToken == id) return;
    m_checkedBackgroundToken = id;
    NotifyFieldChanged(PropertyId::CheckedBackgroundToken, TokenValue(id));
}

void UIElement::SetAccentColorToken(ThemeTokenId id) {
    if (m_accentColorToken == id) return;
    m_accentColorToken = id;
    NotifyFieldChanged(PropertyId::AccentColorToken, TokenValue(id));
}

void UIElement::SetActiveColorToken(ThemeTokenId id) {
    if (m_activeColorToken == id) return;
    m_activeColorToken = id;
    NotifyFieldChanged(PropertyId::ActiveColorToken, TokenValue(id));
}

void UIElement::SetUnderlineColorToken(ThemeTokenId id) {
    if (m_underlineColorToken == id) return;
    m_underlineColorToken = id;
    NotifyFieldChanged(PropertyId::UnderlineColorToken, TokenValue(id));
}

void UIElement::SetActiveUnderlineColorToken(ThemeTokenId id) {
    if (m_activeUnderlineColorToken == id) return;
    m_activeUnderlineColorToken = id;
    NotifyFieldChanged(PropertyId::ActiveUnderlineColorToken, TokenValue(id));
}

void UIElement::SetActiveTabBackgroundToken(ThemeTokenId id) {
    if (m_activeTabBackgroundToken == id) return;
    m_activeTabBackgroundToken = id;
    NotifyFieldChanged(PropertyId::ActiveTabBackgroundToken, TokenValue(id));
}

void UIElement::SetInactiveTabBackgroundToken(ThemeTokenId id) {
    if (m_inactiveTabBackgroundToken == id) return;
    m_inactiveTabBackgroundToken = id;
    NotifyFieldChanged(PropertyId::InactiveTabBackgroundToken, TokenValue(id));
}

void UIElement::SetGridLineBrushToken(ThemeTokenId id) {
    if (m_gridLineBrushToken == id) return;
    m_gridLineBrushToken = id;
    NotifyFieldChanged(PropertyId::GridLineBrushToken, TokenValue(id));
}

void UIElement::SetTitleColorToken(ThemeTokenId id) {
    if (m_titleColorToken == id) return;
    m_titleColorToken = id;
    NotifyFieldChanged(PropertyId::TitleColorToken, TokenValue(id));
}

void UIElement::SetMessageColorToken(ThemeTokenId id) {
    if (m_messageColorToken == id) return;
    m_messageColorToken = id;
    NotifyFieldChanged(PropertyId::MessageColorToken, TokenValue(id));
}

void UIElement::SetCaretColorToken(ThemeTokenId id) {
    if (m_caretColorToken == id) return;
    m_caretColorToken = id;
    NotifyFieldChanged(PropertyId::CaretColorToken, TokenValue(id));
}

// --- Content setters ---

void UIElement::SetText(const std::string& text) {
    if (m_text == text) return;
    m_text = text;
    NotifyFieldChanged(PropertyId::Text, Value(text));
}

void UIElement::BindText(const std::shared_ptr<Observable<std::string>>& value) {
    Text->Bind(value);
}

void UIElement::UnbindText() {
    Text->Unbind();
}

void UIElement::SetPlaceholder(const std::string& placeholder) {
    if (m_placeholder == placeholder) return;
    m_placeholder = placeholder;
    NotifyFieldChanged(PropertyId::Placeholder, Value(placeholder));
}

void UIElement::SetFontFamily(const std::string& font) {
    if (m_fontFamily == font) return;
    m_fontFamily = font;
    NotifyFieldChanged(PropertyId::FontFamily, Value(font));
}

void UIElement::SetFontSize(float size) {
    if (m_fontSize == size) return;
    m_fontSize = size;
    NotifyFieldChanged(PropertyId::FontSize, Value(size));
}

const char* FontWeightToString(FontWeight value) {
    switch (value) {
    case CUI::FontWeight::Thin: return "Thin";
    case CUI::FontWeight::ExtraLight: return "ExtraLight";
    case CUI::FontWeight::Light: return "Light";
    case CUI::FontWeight::Medium: return "Medium";
    case CUI::FontWeight::SemiBold: return "SemiBold";
    case CUI::FontWeight::Bold: return "Bold";
    case CUI::FontWeight::ExtraBold: return "ExtraBold";
    case CUI::FontWeight::Black: return "Black";
    default: return "Normal";
    }
}

FontWeight FontWeightFromString(const std::string& value) {
    if (value == "Thin") return CUI::FontWeight::Thin;
    if (value == "ExtraLight") return CUI::FontWeight::ExtraLight;
    if (value == "Light") return CUI::FontWeight::Light;
    if (value == "Medium") return CUI::FontWeight::Medium;
    if (value == "SemiBold") return CUI::FontWeight::SemiBold;
    if (value == "Bold") return CUI::FontWeight::Bold;
    if (value == "ExtraBold") return CUI::FontWeight::ExtraBold;
    if (value == "Black") return CUI::FontWeight::Black;
    return CUI::FontWeight::Normal;
}

const char* FontStyleToString(FontStyle value) {
    switch (value) {
    case CUI::FontStyle::Italic: return "Italic";
    case CUI::FontStyle::Oblique: return "Oblique";
    default: return "Normal";
    }
}

FontStyle FontStyleFromString(const std::string& value) {
    if (value == "Italic") return CUI::FontStyle::Italic;
    if (value == "Oblique") return CUI::FontStyle::Oblique;
    return CUI::FontStyle::Normal;
}

const char* FontStretchToString(FontStretch value) {
    switch (value) {
    case CUI::FontStretch::UltraCondensed: return "UltraCondensed";
    case CUI::FontStretch::ExtraCondensed: return "ExtraCondensed";
    case CUI::FontStretch::Condensed: return "Condensed";
    case CUI::FontStretch::SemiCondensed: return "SemiCondensed";
    case CUI::FontStretch::SemiExpanded: return "SemiExpanded";
    case CUI::FontStretch::Expanded: return "Expanded";
    case CUI::FontStretch::ExtraExpanded: return "ExtraExpanded";
    case CUI::FontStretch::UltraExpanded: return "UltraExpanded";
    default: return "Normal";
    }
}

FontStretch FontStretchFromString(const std::string& value) {
    if (value == "UltraCondensed") return CUI::FontStretch::UltraCondensed;
    if (value == "ExtraCondensed") return CUI::FontStretch::ExtraCondensed;
    if (value == "Condensed") return CUI::FontStretch::Condensed;
    if (value == "SemiCondensed") return CUI::FontStretch::SemiCondensed;
    if (value == "SemiExpanded") return CUI::FontStretch::SemiExpanded;
    if (value == "Expanded") return CUI::FontStretch::Expanded;
    if (value == "ExtraExpanded") return CUI::FontStretch::ExtraExpanded;
    if (value == "UltraExpanded") return CUI::FontStretch::UltraExpanded;
    return CUI::FontStretch::Normal;
}

void UIElement::SetFontWeight(CUI::FontWeight weight) {
    if (m_fontWeight == weight) return;
    m_fontWeight = weight;
    NotifyFieldChanged(PropertyId::FontWeight, Value(FontWeightToString(weight)));
}

DWRITE_FONT_WEIGHT UIElement::ResolveFontWeight() const {
    return static_cast<DWRITE_FONT_WEIGHT>(m_fontWeight);
}

void UIElement::SetFontStyle(CUI::FontStyle style) {
    if (m_fontStyle == style) return;
    m_fontStyle = style;
    NotifyFieldChanged(PropertyId::FontStyle, Value(FontStyleToString(style)));
}

DWRITE_FONT_STYLE UIElement::ResolveFontStyle() const {
    return static_cast<DWRITE_FONT_STYLE>(m_fontStyle);
}

void UIElement::SetFontStretch(CUI::FontStretch stretch) {
    if (m_fontStretch == stretch) return;
    m_fontStretch = stretch;
    NotifyFieldChanged(PropertyId::FontStretch, Value(FontStretchToString(stretch)));
}

DWRITE_FONT_STRETCH UIElement::ResolveFontStretch() const {
    return static_cast<DWRITE_FONT_STRETCH>(m_fontStretch);
}

void UIElement::SetIsUnderline(bool underline) {
    if (m_isUnderline == underline) return;
    m_isUnderline = underline;
    NotifyFieldChanged(PropertyId::IsUnderline, Value(underline));
}

void UIElement::SetIsStrikethrough(bool strikethrough) {
    if (m_isStrikethrough == strikethrough) return;
    m_isStrikethrough = strikethrough;
    NotifyFieldChanged(PropertyId::IsStrikethrough, Value(strikethrough));
}

void UIElement::SetToolTip(const std::string& tip) {
    if (m_toolTip == tip) return;
    m_toolTip = tip;
    NotifyFieldChanged(PropertyId::ToolTip, Value(tip));
    if (tip.empty()) {
        HideToolTipNow();
    }
}

void UIElement::SetIcon(const std::string& icon) {
    if (m_icon == icon) return;
    m_icon = icon;
    NotifyFieldChanged(PropertyId::Icon, Value(icon));
    InvalidateMeasure();
    MarkRenderContentDirty();
}

// --- Theme resolve ---

D2D1_COLOR_F UIElement::ResolveThemeColor(ThemeTokenId token, ThemeTokenId fallback) const {
    ThemeTokenId id = (token != ThemeTokenId::Unset) ? token : fallback;
    if (id == ThemeTokenId::Unset) {
        id = ThemeTokenId::TextPrimary;
    }
    return ThemeManager::Instance().GetColor(id);
}

D2D1_COLOR_F UIElement::ResolveThemeColor(PropertyId tokenId, ThemeTokenId fallback) const {
    ThemeTokenId id = ThemeTokenId::Unset;
    switch (tokenId) {
    case PropertyId::BackgroundToken: id = m_backgroundToken; break;
    case PropertyId::HoverBackgroundToken: id = m_hoverBackgroundToken; break;
    case PropertyId::PressedBackgroundToken: id = m_pressedBackgroundToken; break;
    case PropertyId::DisabledBackgroundToken: id = m_disabledBackgroundToken; break;
    case PropertyId::BorderToken: id = m_borderToken; break;
    case PropertyId::FocusedBorderToken: id = m_focusedBorderToken; break;
    case PropertyId::ColorToken: id = m_colorToken; break;
    case PropertyId::SecondaryColorToken: id = m_secondaryColorToken; break;
    case PropertyId::PlaceholderColorToken: id = m_placeholderColorToken; break;
    case PropertyId::SelectedBackgroundToken: id = m_selectedBackgroundToken; break;
    case PropertyId::HeaderBackgroundToken: id = m_headerBackgroundToken; break;
    case PropertyId::PaneBackgroundToken: id = m_paneBackgroundToken; break;
    case PropertyId::IndicatorColorToken: id = m_indicatorColorToken; break;
    case PropertyId::DropdownBackgroundToken: id = m_dropdownBackgroundToken; break;
    case PropertyId::SelectedItemBackgroundToken: id = m_selectedItemBackgroundToken; break;
    case PropertyId::FillColorToken: id = m_fillColorToken; break;
    case PropertyId::TrackColorToken: id = m_trackColorToken; break;
    case PropertyId::ActiveTrackColorToken: id = m_activeTrackColorToken; break;
    case PropertyId::ThumbColorToken: id = m_thumbColorToken; break;
    case PropertyId::OnColorToken: id = m_onColorToken; break;
    case PropertyId::OffColorToken: id = m_offColorToken; break;
    case PropertyId::KnobColorToken: id = m_knobColorToken; break;
    case PropertyId::CheckedBackgroundToken: id = m_checkedBackgroundToken; break;
    case PropertyId::AccentColorToken: id = m_accentColorToken; break;
    case PropertyId::ActiveColorToken: id = m_activeColorToken; break;
    case PropertyId::UnderlineColorToken: id = m_underlineColorToken; break;
    case PropertyId::ActiveUnderlineColorToken: id = m_activeUnderlineColorToken; break;
    case PropertyId::ActiveTabBackgroundToken: id = m_activeTabBackgroundToken; break;
    case PropertyId::InactiveTabBackgroundToken: id = m_inactiveTabBackgroundToken; break;
    case PropertyId::GridLineBrushToken: id = m_gridLineBrushToken; break;
    case PropertyId::TitleColorToken: id = m_titleColorToken; break;
    case PropertyId::MessageColorToken: id = m_messageColorToken; break;
    case PropertyId::CaretColorToken: id = m_caretColorToken; break;
    default: break;
    }
    if (id == ThemeTokenId::Unset) {
        id = fallback;
    }
    return ResolveThemeColor(id, ThemeTokenId::Unset);
}

// --- PropertyId property bridge (PropertyDesc table cold path) ---

bool UIElement::DescHasOptionalProperty(const PropertyDesc& desc) const {
    if (desc.id == PropertyId::None) return false;
    if (desc.kind == PropertyKind::ThemeToken) {
        if (!desc.get) return false;
        Value out;
        desc.get(this, out);
        return !out.AsString().empty();
    }
    if (desc.kind == PropertyKind::Color) {
        switch (desc.id) {
        case PropertyId::Background: return m_hasBackgroundColor;
        case PropertyId::BorderBrush: return m_hasBorderBrushColor;
        case PropertyId::HoverBackground: return m_hasHoverBackgroundColor;
        case PropertyId::PressedBackground: return m_hasPressedBackgroundColor;
        case PropertyId::Color: return m_hasColorValue;
        default: return false;
        }
    }
    if (desc.kind == PropertyKind::Float) {
        switch (desc.id) {
        case PropertyId::Width: return m_width >= 0.0f;
        case PropertyId::Height: return m_height >= 0.0f;
        case PropertyId::ItemWidth: return m_itemWidth >= 0.0f;
        case PropertyId::ItemHeight: return m_itemHeight >= 0.0f;
        default: break;
        }
    }
    return true;
}

void UIElement::SetProperty(PropertyId id, const Value& val) {
    if (const PropertyDesc* desc = FindPropertyDescForElement(this, id)) {
        if (desc->set) {
            desc->set(this, val);
        }
    }
}

void UIElement::SetBackground(D2D1_COLOR_F c) {
    if (ColorUnchanged(m_hasBackgroundColor, m_backgroundColor, c)) return;
    m_backgroundColor = c;
    m_hasBackgroundColor = true;
    NotifyFieldChanged(PropertyId::Background, Value(c));
}

void UIElement::SetHoverBackground(D2D1_COLOR_F c) {
    if (ColorUnchanged(m_hasHoverBackgroundColor, m_hoverBackgroundColor, c)) return;
    m_hoverBackgroundColor = c;
    m_hasHoverBackgroundColor = true;
    NotifyFieldChanged(PropertyId::HoverBackground, Value(c));
}

void UIElement::SetPressedBackground(D2D1_COLOR_F c) {
    if (ColorUnchanged(m_hasPressedBackgroundColor, m_pressedBackgroundColor, c)) return;
    m_pressedBackgroundColor = c;
    m_hasPressedBackgroundColor = true;
    NotifyFieldChanged(PropertyId::PressedBackground, Value(c));
}

void UIElement::SetBorderBrush(D2D1_COLOR_F c) {
    if (ColorUnchanged(m_hasBorderBrushColor, m_borderBrushColor, c)) return;
    m_borderBrushColor = c;
    m_hasBorderBrushColor = true;
    NotifyFieldChanged(PropertyId::BorderBrush, Value(c));
}

void UIElement::SetColor(D2D1_COLOR_F c) {
    if (ColorUnchanged(m_hasColorValue, m_colorValue, c)) return;
    m_colorValue = c;
    m_hasColorValue = true;
    NotifyFieldChanged(PropertyId::Color, Value(c));
}

Value UIElement::GetProperty(PropertyId id) const {
    if (const PropertyDesc* desc = FindPropertyDescForElement(this, id)) {
        if (desc->get) {
            Value out;
            desc->get(this, out);
            return out;
        }
    }
    return Value();
}

bool UIElement::HasProperty(PropertyId id) const {
    if (const PropertyDesc* desc = FindPropertyDescForElement(this, id)) {
        return DescHasOptionalProperty(*desc);
    }
    return false;
}

std::vector<std::pair<PropertyId, Value>> UIElement::SnapshotProperties() const {
    std::vector<std::pair<PropertyId, Value>> out;
    out.reserve(64);

    PropertyDescSpan span = GetPropertyDescs();
    if (span.data) {
        for (size_t i = 0; i < span.count; ++i) {
            const PropertyDesc& desc = span.data[i];
            if (desc.id == PropertyId::None) continue;
            if (!DescHasOptionalProperty(desc)) continue;
            out.emplace_back(desc.id, GetProperty(desc.id));
        }
    }

    return out;
}

PropertyDescSpan UIElement::GetPropertyDescs() const {
    return PropertyDescSpan{ kUIElementDescs, kUIElementDescCount };
}

const PropertyDesc* FindPropertyDescById(PropertyId id) {
    const auto idx = static_cast<size_t>(id);
    if (id == PropertyId::None || idx >= static_cast<size_t>(PropertyId::Count)) {
        return nullptr;
    }
    return gUIElementDescById[idx];
}

const PropertyDesc* FindPropertyDescForElement(const UIElement* element, PropertyId id) {
    if (element) {
        PropertyDescSpan span = element->GetPropertyDescs();
        for (size_t i = 0; i < span.count; ++i) {
            if (span.data[i].id == id) {
                return &span.data[i];
            }
        }
    }
    return FindPropertyDescById(id);
}

} // namespace CUI
