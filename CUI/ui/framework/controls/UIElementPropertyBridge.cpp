#include "UIElement.h"
#include "../core/PropertyId.h"
#include "../style/ThemeManager.h"
#include "../style/ThemeTokenId.h"
#include <algorithm>

namespace CUI {

namespace {

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
void DescGetFontWeight(const UIElement* self, Value& out) { out = Value(self->GetFontWeight()); }
void DescSetFontWeight(UIElement* self, const Value& in) { self->SetFontWeight(in.AsString()); }

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
static const char* const kFontFamilyOptions[] = { "Segoe UI", "Consolas", "微软雅黑", "Times New Roman", nullptr };
static const char* const kFontWeightOptions[] = { "Normal", "Bold", "Light", nullptr };

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
    { PropertyId::Rows, "行数 (Rows)", "面板布局", PropertyKind::Int, nullptr, &DescGetRows, &DescSetRows },
    { PropertyId::Columns, "列数 (Columns)", "面板布局", PropertyKind::Int, nullptr, &DescGetColumns, &DescSetColumns },

    { PropertyId::CanvasLeft, "Canvas.Left", "Canvas", PropertyKind::Float, nullptr, &DescGetCanvasLeft, &DescSetCanvasLeft },
    { PropertyId::CanvasTop, "Canvas.Top", "Canvas", PropertyKind::Float, nullptr, &DescGetCanvasTop, &DescSetCanvasTop },
    { PropertyId::CanvasRight, "Canvas.Right", "Canvas", PropertyKind::Float, nullptr, &DescGetCanvasRight, &DescSetCanvasRight },
    { PropertyId::CanvasBottom, "Canvas.Bottom", "Canvas", PropertyKind::Float, nullptr, &DescGetCanvasBottom, &DescSetCanvasBottom },
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

void UIElement::SetWidth(float v) {
    m_width = v;
    NotifyFieldChanged(PropertyId::Width, Value(v));
}

void UIElement::SetHeight(float v) {
    m_height = v;
    NotifyFieldChanged(PropertyId::Height, Value(v));
}

void UIElement::SetMinWidth(float v) {
    m_minWidth = v;
    NotifyFieldChanged(PropertyId::MinWidth, Value(v));
}

void UIElement::SetMinHeight(float v) {
    m_minHeight = v;
    NotifyFieldChanged(PropertyId::MinHeight, Value(v));
}

void UIElement::SetMargin(const Thickness& margin) {
    m_margin = margin;
    NotifyFieldChanged(PropertyId::Margin, Value(margin));
}

void UIElement::SetPadding(const Thickness& padding) {
    m_padding = padding;
    NotifyFieldChanged(PropertyId::Padding, Value(padding));
}

void UIElement::SetVisibility(Visibility v) {
    m_visibility = v;
    NotifyFieldChanged(PropertyId::Visibility, Value(VisibilityToString(v)));
}

void UIElement::SetIsEnabled(bool enabled) {
    m_isEnabled = enabled;
    NotifyFieldChanged(PropertyId::IsEnabled, Value(enabled));
}

void UIElement::SetOpacity(float v) {
    m_opacity = v;
    NotifyFieldChanged(PropertyId::Opacity, Value(v));
}

void UIElement::SetCornerRadius(float v) {
    m_cornerRadius = v;
    NotifyFieldChanged(PropertyId::CornerRadius, Value(v));
}

void UIElement::SetBorderThickness(float v) {
    m_borderThickness = v;
    NotifyFieldChanged(PropertyId::BorderThickness, Value(v));
}

void UIElement::SetFlexGrow(float v) {
    m_flexGrow = v;
    NotifyFieldChanged(PropertyId::FlexGrow, Value(v));
}

void UIElement::SetAlign(Alignment a) {
    m_align = a;
    NotifyFieldChanged(PropertyId::Align, Value(AlignmentToString(a)));
}

void UIElement::SetAlignHorizontal(Alignment a) {
    m_alignHorizontal = a;
    NotifyFieldChanged(PropertyId::AlignHorizontal, Value(AlignmentToString(a)));
}

void UIElement::SetAlignVertical(Alignment a) {
    m_alignVertical = a;
    NotifyFieldChanged(PropertyId::AlignVertical, Value(AlignmentToString(a)));
}

void UIElement::SetOrientation(Orientation o) {
    m_orientation = o;
    NotifyFieldChanged(PropertyId::Orientation, Value(OrientationToString(o)));
}

void UIElement::SetGap(float v) {
    m_gap = v;
    NotifyFieldChanged(PropertyId::Gap, Value(v));
}

void UIElement::SetItemWidth(float v) {
    m_itemWidth = v;
    NotifyFieldChanged(PropertyId::ItemWidth, Value(v));
}

void UIElement::SetItemHeight(float v) {
    m_itemHeight = v;
    NotifyFieldChanged(PropertyId::ItemHeight, Value(v));
}

void UIElement::SetLastChildFill(bool v) {
    m_lastChildFill = v;
    NotifyFieldChanged(PropertyId::LastChildFill, Value(v));
}

void UIElement::SetRows(int v) {
    m_rows = v;
    NotifyFieldChanged(PropertyId::Rows, Value(v));
}

void UIElement::SetColumns(int v) {
    m_columns = v;
    NotifyFieldChanged(PropertyId::Columns, Value(v));
}

void UIElement::SetClipToBounds(bool v) {
    m_clipToBounds = v;
    NotifyFieldChanged(PropertyId::ClipToBounds, Value(v));
}

void UIElement::SetCanvasLeft(float v) {
    m_canvasLeft = v;
    NotifyFieldChanged(PropertyId::CanvasLeft, Value(v));
}

void UIElement::SetCanvasTop(float v) {
    m_canvasTop = v;
    NotifyFieldChanged(PropertyId::CanvasTop, Value(v));
}

void UIElement::SetCanvasRight(float v) {
    m_canvasRight = v;
    NotifyFieldChanged(PropertyId::CanvasRight, Value(v));
}

void UIElement::SetCanvasBottom(float v) {
    m_canvasBottom = v;
    NotifyFieldChanged(PropertyId::CanvasBottom, Value(v));
}

void UIElement::SetGridColumn(int v) {
    m_gridColumn = v;
    NotifyFieldChanged(PropertyId::GridColumn, Value(v));
}

void UIElement::SetGridRow(int v) {
    m_gridRow = v;
    NotifyFieldChanged(PropertyId::GridRow, Value(v));
}

void UIElement::SetGridColumnSpan(int v) {
    m_gridColumnSpan = v;
    NotifyFieldChanged(PropertyId::GridColumnSpan, Value(v));
}

void UIElement::SetGridRowSpan(int v) {
    m_gridRowSpan = v;
    NotifyFieldChanged(PropertyId::GridRowSpan, Value(v));
}

void UIElement::SetDock(Dock d) {
    m_dock = d;
    NotifyFieldChanged(PropertyId::Dock, Value(DockToString(d)));
}

// --- Theme token setters ---

void UIElement::SetBackgroundToken(ThemeTokenId id) {
    m_backgroundToken = id;
    NotifyFieldChanged(PropertyId::BackgroundToken, TokenValue(id));
}

void UIElement::SetHoverBackgroundToken(ThemeTokenId id) {
    m_hoverBackgroundToken = id;
    NotifyFieldChanged(PropertyId::HoverBackgroundToken, TokenValue(id));
}

void UIElement::SetPressedBackgroundToken(ThemeTokenId id) {
    m_pressedBackgroundToken = id;
    NotifyFieldChanged(PropertyId::PressedBackgroundToken, TokenValue(id));
}

void UIElement::SetDisabledBackgroundToken(ThemeTokenId id) {
    m_disabledBackgroundToken = id;
    NotifyFieldChanged(PropertyId::DisabledBackgroundToken, TokenValue(id));
}

void UIElement::SetBorderToken(ThemeTokenId id) {
    m_borderToken = id;
    NotifyFieldChanged(PropertyId::BorderToken, TokenValue(id));
}

void UIElement::SetFocusedBorderToken(ThemeTokenId id) {
    m_focusedBorderToken = id;
    NotifyFieldChanged(PropertyId::FocusedBorderToken, TokenValue(id));
}

void UIElement::SetColorToken(ThemeTokenId id) {
    m_colorToken = id;
    NotifyFieldChanged(PropertyId::ColorToken, TokenValue(id));
}

void UIElement::SetSecondaryColorToken(ThemeTokenId id) {
    m_secondaryColorToken = id;
    NotifyFieldChanged(PropertyId::SecondaryColorToken, TokenValue(id));
}

void UIElement::SetPlaceholderColorToken(ThemeTokenId id) {
    m_placeholderColorToken = id;
    NotifyFieldChanged(PropertyId::PlaceholderColorToken, TokenValue(id));
}

void UIElement::SetSelectedBackgroundToken(ThemeTokenId id) {
    m_selectedBackgroundToken = id;
    NotifyFieldChanged(PropertyId::SelectedBackgroundToken, TokenValue(id));
}

void UIElement::SetHeaderBackgroundToken(ThemeTokenId id) {
    m_headerBackgroundToken = id;
    NotifyFieldChanged(PropertyId::HeaderBackgroundToken, TokenValue(id));
}

void UIElement::SetPaneBackgroundToken(ThemeTokenId id) {
    m_paneBackgroundToken = id;
    NotifyFieldChanged(PropertyId::PaneBackgroundToken, TokenValue(id));
}

void UIElement::SetIndicatorColorToken(ThemeTokenId id) {
    m_indicatorColorToken = id;
    NotifyFieldChanged(PropertyId::IndicatorColorToken, TokenValue(id));
}

void UIElement::SetDropdownBackgroundToken(ThemeTokenId id) {
    m_dropdownBackgroundToken = id;
    NotifyFieldChanged(PropertyId::DropdownBackgroundToken, TokenValue(id));
}

void UIElement::SetSelectedItemBackgroundToken(ThemeTokenId id) {
    m_selectedItemBackgroundToken = id;
    NotifyFieldChanged(PropertyId::SelectedItemBackgroundToken, TokenValue(id));
}

void UIElement::SetFillColorToken(ThemeTokenId id) {
    m_fillColorToken = id;
    NotifyFieldChanged(PropertyId::FillColorToken, TokenValue(id));
}

void UIElement::SetTrackColorToken(ThemeTokenId id) {
    m_trackColorToken = id;
    NotifyFieldChanged(PropertyId::TrackColorToken, TokenValue(id));
}

void UIElement::SetActiveTrackColorToken(ThemeTokenId id) {
    m_activeTrackColorToken = id;
    NotifyFieldChanged(PropertyId::ActiveTrackColorToken, TokenValue(id));
}

void UIElement::SetThumbColorToken(ThemeTokenId id) {
    m_thumbColorToken = id;
    NotifyFieldChanged(PropertyId::ThumbColorToken, TokenValue(id));
}

void UIElement::SetOnColorToken(ThemeTokenId id) {
    m_onColorToken = id;
    NotifyFieldChanged(PropertyId::OnColorToken, TokenValue(id));
}

void UIElement::SetOffColorToken(ThemeTokenId id) {
    m_offColorToken = id;
    NotifyFieldChanged(PropertyId::OffColorToken, TokenValue(id));
}

void UIElement::SetKnobColorToken(ThemeTokenId id) {
    m_knobColorToken = id;
    NotifyFieldChanged(PropertyId::KnobColorToken, TokenValue(id));
}

void UIElement::SetCheckedBackgroundToken(ThemeTokenId id) {
    m_checkedBackgroundToken = id;
    NotifyFieldChanged(PropertyId::CheckedBackgroundToken, TokenValue(id));
}

void UIElement::SetAccentColorToken(ThemeTokenId id) {
    m_accentColorToken = id;
    NotifyFieldChanged(PropertyId::AccentColorToken, TokenValue(id));
}

void UIElement::SetActiveColorToken(ThemeTokenId id) {
    m_activeColorToken = id;
    NotifyFieldChanged(PropertyId::ActiveColorToken, TokenValue(id));
}

void UIElement::SetUnderlineColorToken(ThemeTokenId id) {
    m_underlineColorToken = id;
    NotifyFieldChanged(PropertyId::UnderlineColorToken, TokenValue(id));
}

void UIElement::SetActiveUnderlineColorToken(ThemeTokenId id) {
    m_activeUnderlineColorToken = id;
    NotifyFieldChanged(PropertyId::ActiveUnderlineColorToken, TokenValue(id));
}

void UIElement::SetActiveTabBackgroundToken(ThemeTokenId id) {
    m_activeTabBackgroundToken = id;
    NotifyFieldChanged(PropertyId::ActiveTabBackgroundToken, TokenValue(id));
}

void UIElement::SetInactiveTabBackgroundToken(ThemeTokenId id) {
    m_inactiveTabBackgroundToken = id;
    NotifyFieldChanged(PropertyId::InactiveTabBackgroundToken, TokenValue(id));
}

void UIElement::SetGridLineBrushToken(ThemeTokenId id) {
    m_gridLineBrushToken = id;
    NotifyFieldChanged(PropertyId::GridLineBrushToken, TokenValue(id));
}

void UIElement::SetTitleColorToken(ThemeTokenId id) {
    m_titleColorToken = id;
    NotifyFieldChanged(PropertyId::TitleColorToken, TokenValue(id));
}

void UIElement::SetMessageColorToken(ThemeTokenId id) {
    m_messageColorToken = id;
    NotifyFieldChanged(PropertyId::MessageColorToken, TokenValue(id));
}

void UIElement::SetCaretColorToken(ThemeTokenId id) {
    m_caretColorToken = id;
    NotifyFieldChanged(PropertyId::CaretColorToken, TokenValue(id));
}

// --- Content setters ---

void UIElement::SetText(const std::string& text) {
    m_text = text;
    NotifyFieldChanged(PropertyId::Text, Value(text));
}

void UIElement::SetPlaceholder(const std::string& placeholder) {
    m_placeholder = placeholder;
    NotifyFieldChanged(PropertyId::Placeholder, Value(placeholder));
}

void UIElement::SetFontFamily(const std::string& font) {
    m_fontFamily = font;
    NotifyFieldChanged(PropertyId::FontFamily, Value(font));
}

void UIElement::SetFontSize(float size) {
    m_fontSize = size;
    NotifyFieldChanged(PropertyId::FontSize, Value(size));
}

void UIElement::SetFontWeight(const std::string& weight) {
    m_fontWeight = weight;
    NotifyFieldChanged(PropertyId::FontWeight, Value(weight));
}

void UIElement::SetToolTip(const std::string& tip) {
    m_toolTip = tip;
    NotifyFieldChanged(PropertyId::ToolTip, Value(tip));
}

void UIElement::SetIcon(const std::string& icon) {
    m_icon = icon;
    NotifyFieldChanged(PropertyId::Icon, Value(icon));
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
    m_backgroundColor = c;
    m_hasBackgroundColor = true;
    NotifyFieldChanged(PropertyId::Background, Value(c));
}

void UIElement::SetHoverBackground(D2D1_COLOR_F c) {
    m_hoverBackgroundColor = c;
    m_hasHoverBackgroundColor = true;
    NotifyFieldChanged(PropertyId::HoverBackground, Value(c));
}

void UIElement::SetPressedBackground(D2D1_COLOR_F c) {
    m_pressedBackgroundColor = c;
    m_hasPressedBackgroundColor = true;
    NotifyFieldChanged(PropertyId::PressedBackground, Value(c));
}

void UIElement::SetBorderBrush(D2D1_COLOR_F c) {
    m_borderBrushColor = c;
    m_hasBorderBrushColor = true;
    NotifyFieldChanged(PropertyId::BorderBrush, Value(c));
}

void UIElement::SetColor(D2D1_COLOR_F c) {
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

namespace {
const char* PropertyKindToMetaType(PropertyKind kind) {
    switch (kind) {
    case PropertyKind::Bool: return "bool";
    case PropertyKind::Int: return "int";
    case PropertyKind::Float: return "float";
    case PropertyKind::String: return "string";
    case PropertyKind::Color: return "color";
    case PropertyKind::Thickness: return "thickness";
    case PropertyKind::Enum: return "enum";
    case PropertyKind::ThemeToken: return "themeToken";
    default: return "string";
    }
}
} // namespace

std::vector<PropertyMeta> UIElement::GetPropertyMetas() const {
    PropertyDescSpan span = GetPropertyDescs();
    std::vector<PropertyMeta> metas;
    if (!span.data || span.count == 0) {
        return metas;
    }
    metas.reserve(span.count);
    for (size_t i = 0; i < span.count; ++i) {
        const PropertyDesc& desc = span.data[i];
        PropertyMeta meta;
        meta.id = desc.id;
        meta.displayName = desc.displayName ? desc.displayName : PropertyIdToName(desc.id);
        meta.category = desc.category ? desc.category : "";
        meta.type = PropertyKindToMetaType(desc.kind);
        if (desc.enumOptions) {
            for (const char* const* p = desc.enumOptions; *p; ++p) {
                meta.options.emplace_back(*p);
            }
        }
        metas.push_back(std::move(meta));
    }
    return metas;
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
