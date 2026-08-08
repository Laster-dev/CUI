#include "ShowcaseHelpers.h"
#include "framework/style/ThemeManager.h"
#include "framework/style/ThemeTokenId.h"
#include "framework/controls/TextBox.h"
#include "framework/controls/Expander.h"
#include "framework/controls/ScrollViewer.h"
#include <algorithm>
#include <cctype>
#include <iomanip>
#include <sstream>

using namespace CUI;
using namespace CUI::DSL;

namespace {
template <typename T>
std::shared_ptr<T> BindThemeToken(const std::shared_ptr<T>& element, const std::string& tokenProp, const std::string& tokenName) {
    if (element) {
        // Prefer typed token setters when the prop is a known theme.*Token binding.
        ThemeTokenId id = ThemeTokenIdFromName(tokenName);
        if (tokenProp == "theme.backgroundToken") element->SetBackgroundToken(id);
        else if (tokenProp == "theme.hoverBackgroundToken") element->SetHoverBackgroundToken(id);
        else if (tokenProp == "theme.pressedBackgroundToken") element->SetPressedBackgroundToken(id);
        else if (tokenProp == "theme.borderToken") element->SetBorderToken(id);
        else if (tokenProp == "theme.focusedBorderToken") element->SetFocusedBorderToken(id);
        else if (tokenProp == "theme.colorToken") element->SetColorToken(id);
        else if (tokenProp == "theme.placeholderColorToken") element->SetPlaceholderColorToken(id);
        else if (tokenProp == "theme.dropdownBackgroundToken") element->SetDropdownBackgroundToken(id);
        else if (tokenProp == "theme.selectedItemBackgroundToken") element->SetSelectedItemBackgroundToken(id);
        else if (tokenProp == "theme.underlineColorToken") element->SetUnderlineColorToken(id);
        else if (tokenProp == "theme.activeUnderlineColorToken") element->SetActiveUnderlineColorToken(id);
        else if (tokenProp == "theme.caretColorToken") element->SetCaretColorToken(id);
        // Unknown theme prop — ignore (no string SetProperty bag).
    }
    return element;
}

std::string ResolveLegacyColorToken(const std::string& color) {
    if (color == "#569CD6" || color == "#4EC9B0" || color == "#007ACC" || color == "#0E639C") return "accentColor";
    if (color == "#10B981" || color == "#8E44AD" || color == "#D7A400" || color == "#C586C0" || color == "#9CDCFE") return "accentColor";
    if (color == "#6A9955" || color == "#CE9178" || color == "#DCDCAA") return "accentColor";
    if (color == "#D13438" || color == "#D16969") return "dangerColor";
    if (color == "#AAAAAA" || color == "#888888" || color == "#5A5A5A") return "textMuted";
    if (color == "#CCCCCC" || color == "#DBDBDB" || color == "#E5E5E5" || color == "#B5CEA8") return "textSecondary";
    // Direct token names
    if (color == "textPrimary" || color == "textSecondary" || color == "textMuted" ||
        color == "accentColor" || color == "accentForeground" || color == "dangerColor" ||
        color == "titleBarText") {
        return color;
    }
    return "";
}

std::string FormatFloatLiteral(float v) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(3) << v;
    std::string s = oss.str();
    while (s.size() > 1 && s.back() == '0') s.pop_back();
    if (!s.empty() && s.back() == '.') s += "0";
    if (s.find('.') == std::string::npos) s += ".0";
    return s + "f";
}

std::string FormatThicknessLiteral(const Thickness& t) {
    std::ostringstream oss;
    oss << "Thickness(" << FormatFloatLiteral(t.left) << ", "
        << FormatFloatLiteral(t.top) << ", "
        << FormatFloatLiteral(t.right) << ", "
        << FormatFloatLiteral(t.bottom) << ")";
    return oss.str();
}

std::string FormatTokenLiteral(ThemeTokenId id) {
    switch (id) {
    case ThemeTokenId::WindowBackground: return "ThemeTokenId::WindowBackground";
    case ThemeTokenId::CardBackground: return "ThemeTokenId::CardBackground";
    case ThemeTokenId::CardBorder: return "ThemeTokenId::CardBorder";
    case ThemeTokenId::TextPrimary: return "ThemeTokenId::TextPrimary";
    case ThemeTokenId::TextSecondary: return "ThemeTokenId::TextSecondary";
    case ThemeTokenId::TextMuted: return "ThemeTokenId::TextMuted";
    case ThemeTokenId::TitleBarBackground: return "ThemeTokenId::TitleBarBackground";
    case ThemeTokenId::TitleBarText: return "ThemeTokenId::TitleBarText";
    case ThemeTokenId::AccentColor: return "ThemeTokenId::AccentColor";
    case ThemeTokenId::AccentForeground: return "ThemeTokenId::AccentForeground";
    case ThemeTokenId::DangerColor: return "ThemeTokenId::DangerColor";
    case ThemeTokenId::PaneBackground: return "ThemeTokenId::PaneBackground";
    case ThemeTokenId::InputBackground: return "ThemeTokenId::InputBackground";
    case ThemeTokenId::InputBorder: return "ThemeTokenId::InputBorder";
    case ThemeTokenId::HoverBackground: return "ThemeTokenId::HoverBackground";
    case ThemeTokenId::PressedBackground: return "ThemeTokenId::PressedBackground";
    case ThemeTokenId::SelectedBackground: return "ThemeTokenId::SelectedBackground";
    case ThemeTokenId::FocusedBorder: return "ThemeTokenId::FocusedBorder";
    case ThemeTokenId::ActivityBarBackground: return "ThemeTokenId::ActivityBarBackground";
    case ThemeTokenId::SideBarBackground: return "ThemeTokenId::SideBarBackground";
    case ThemeTokenId::EditorBackground: return "ThemeTokenId::EditorBackground";
    case ThemeTokenId::StatusBarBackground: return "ThemeTokenId::StatusBarBackground";
    case ThemeTokenId::TabBarBackground: return "ThemeTokenId::TabBarBackground";
    default: return {};
    }
}

std::string FormatAlignmentLiteral(Alignment a) {
    switch (a) {
    case Alignment::Start: return "Alignment::Start";
    case Alignment::Center: return "Alignment::Center";
    case Alignment::End: return "Alignment::End";
    default: return "Alignment::Stretch";
    }
}

std::string FormatOrientationLiteral(Orientation o) {
    return o == Orientation::Horizontal ? "Orientation::Horizontal" : "Orientation::Vertical";
}

std::string FormatDockLiteral(Dock d) {
    switch (d) {
    case Dock::Top: return "Dock::Top";
    case Dock::Right: return "Dock::Right";
    case Dock::Bottom: return "Dock::Bottom";
    default: return "Dock::Left";
    }
}

void EmitLine(std::ostringstream& oss, int& emitted, int maxLines, const std::string& line) {
    if (emitted >= maxLines) return;
    oss << line << "\n";
    ++emitted;
}

void EmitTokenSetter(std::ostringstream& oss, int& emitted, int maxLines,
                     const char* setter, ThemeTokenId id) {
    if (id == ThemeTokenId::Unset) return;
    std::string lit = FormatTokenLiteral(id);
    if (lit.empty()) return;
    EmitLine(oss, emitted, maxLines, std::string("target->") + setter + "(" + lit + ");");
}

std::string GenerateAutoCodeExample(const std::shared_ptr<UIElement>& target) {
    if (!target) {
        return std::string("// No target\n");
    }

    std::ostringstream oss;
    oss << "// Auto-generated typed sample (no SetProperty string bag)\n";
    oss << "// Target: " << target->GetClassName() << "\n\n";
    oss << "auto target = std::make_shared<" << target->GetClassName() << ">();\n";

    int emitted = 0;
    constexpr int kMaxLines = 60;

    // --- Hot layout / chrome (typed members) ---
    if (target->GetWidth() >= 0.0f) {
        EmitLine(oss, emitted, kMaxLines, "target->SetWidth(" + FormatFloatLiteral(target->GetWidth()) + ");");
    }
    if (target->GetHeight() >= 0.0f) {
        EmitLine(oss, emitted, kMaxLines, "target->SetHeight(" + FormatFloatLiteral(target->GetHeight()) + ");");
    }
    if (target->GetMinWidth() > 0.0f) {
        EmitLine(oss, emitted, kMaxLines, "target->SetMinWidth(" + FormatFloatLiteral(target->GetMinWidth()) + ");");
    }
    if (target->GetMinHeight() > 0.0f) {
        EmitLine(oss, emitted, kMaxLines, "target->SetMinHeight(" + FormatFloatLiteral(target->GetMinHeight()) + ");");
    }

    Thickness margin = target->GetMargin();
    if (margin.left != 0.f || margin.top != 0.f || margin.right != 0.f || margin.bottom != 0.f) {
        EmitLine(oss, emitted, kMaxLines, "target->SetMargin(" + FormatThicknessLiteral(margin) + ");");
    }
    Thickness padding = target->GetPadding();
    if (padding.left != 0.f || padding.top != 0.f || padding.right != 0.f || padding.bottom != 0.f) {
        EmitLine(oss, emitted, kMaxLines, "target->SetPadding(" + FormatThicknessLiteral(padding) + ");");
    }

    if (target->GetCornerRadius() != 0.0f) {
        EmitLine(oss, emitted, kMaxLines, "target->SetCornerRadius(" + FormatFloatLiteral(target->GetCornerRadius()) + ");");
    }
    if (target->GetBorderThickness() != 0.0f) {
        EmitLine(oss, emitted, kMaxLines, "target->SetBorderThickness(" + FormatFloatLiteral(target->GetBorderThickness()) + ");");
    }
    if (target->GetOpacity() != 1.0f) {
        EmitLine(oss, emitted, kMaxLines, "target->SetOpacity(" + FormatFloatLiteral(target->GetOpacity()) + ");");
    }
    if (target->GetFlexGrow() != 0.0f) {
        EmitLine(oss, emitted, kMaxLines, "target->SetFlexGrow(" + FormatFloatLiteral(target->GetFlexGrow()) + ");");
    }
    if (!target->GetClipToBounds()) {
        EmitLine(oss, emitted, kMaxLines, "target->SetClipToBounds(false);");
    }

    if (target->GetAlign() != Alignment::Stretch) {
        EmitLine(oss, emitted, kMaxLines, "target->SetAlign(" + FormatAlignmentLiteral(target->GetAlign()) + ");");
    }
    if (target->GetAlignHorizontal() != Alignment::Stretch) {
        EmitLine(oss, emitted, kMaxLines, "target->SetAlignHorizontal(" + FormatAlignmentLiteral(target->GetAlignHorizontal()) + ");");
    }
    if (target->GetAlignVertical() != Alignment::Stretch) {
        EmitLine(oss, emitted, kMaxLines, "target->SetAlignVertical(" + FormatAlignmentLiteral(target->GetAlignVertical()) + ");");
    }
    if (target->GetOrientation() != Orientation::Vertical) {
        EmitLine(oss, emitted, kMaxLines, "target->SetOrientation(" + FormatOrientationLiteral(target->GetOrientation()) + ");");
    }
    if (target->GetGap() != 0.0f) {
        EmitLine(oss, emitted, kMaxLines, "target->SetGap(" + FormatFloatLiteral(target->GetGap()) + ");");
    }
    if (target->GetItemWidth() >= 0.0f) {
        EmitLine(oss, emitted, kMaxLines, "target->SetItemWidth(" + FormatFloatLiteral(target->GetItemWidth()) + ");");
    }
    if (target->GetItemHeight() >= 0.0f) {
        EmitLine(oss, emitted, kMaxLines, "target->SetItemHeight(" + FormatFloatLiteral(target->GetItemHeight()) + ");");
    }
    if (!target->GetLastChildFill()) {
        EmitLine(oss, emitted, kMaxLines, "target->SetLastChildFill(false);");
    }
    if (target->GetRows() != 0) {
        EmitLine(oss, emitted, kMaxLines, "target->SetRows(" + std::to_string(target->GetRows()) + ");");
    }
    if (target->GetColumns() != 0) {
        EmitLine(oss, emitted, kMaxLines, "target->SetColumns(" + std::to_string(target->GetColumns()) + ");");
    }

    if (target->GetCanvasLeft() != UIElement::kAttachedUnset) {
        EmitLine(oss, emitted, kMaxLines, "target->SetCanvasLeft(" + FormatFloatLiteral(target->GetCanvasLeft()) + ");");
    }
    if (target->GetCanvasTop() != UIElement::kAttachedUnset) {
        EmitLine(oss, emitted, kMaxLines, "target->SetCanvasTop(" + FormatFloatLiteral(target->GetCanvasTop()) + ");");
    }
    if (target->GetCanvasRight() != UIElement::kAttachedUnset) {
        EmitLine(oss, emitted, kMaxLines, "target->SetCanvasRight(" + FormatFloatLiteral(target->GetCanvasRight()) + ");");
    }
    if (target->GetCanvasBottom() != UIElement::kAttachedUnset) {
        EmitLine(oss, emitted, kMaxLines, "target->SetCanvasBottom(" + FormatFloatLiteral(target->GetCanvasBottom()) + ");");
    }
    if (target->GetGridColumn() != 0) {
        EmitLine(oss, emitted, kMaxLines, "target->SetGridColumn(" + std::to_string(target->GetGridColumn()) + ");");
    }
    if (target->GetGridRow() != 0) {
        EmitLine(oss, emitted, kMaxLines, "target->SetGridRow(" + std::to_string(target->GetGridRow()) + ");");
    }
    if (target->GetGridColumnSpan() != 1) {
        EmitLine(oss, emitted, kMaxLines, "target->SetGridColumnSpan(" + std::to_string(target->GetGridColumnSpan()) + ");");
    }
    if (target->GetGridRowSpan() != 1) {
        EmitLine(oss, emitted, kMaxLines, "target->SetGridRowSpan(" + std::to_string(target->GetGridRowSpan()) + ");");
    }
    if (target->GetDock() != Dock::Left) {
        EmitLine(oss, emitted, kMaxLines, "target->SetDock(" + FormatDockLiteral(target->GetDock()) + ");");
    }

    // --- Content ---
    if (!target->GetText().empty()) {
        EmitLine(oss, emitted, kMaxLines, "target->SetText(\"" + target->GetText() + "\");");
    }
    if (!target->GetPlaceholder().empty()) {
        EmitLine(oss, emitted, kMaxLines, "target->SetPlaceholder(\"" + target->GetPlaceholder() + "\");");
    }
    if (target->GetFontFamily() != "微软雅黑") {
        EmitLine(oss, emitted, kMaxLines, "target->SetFontFamily(\"" + target->GetFontFamily() + "\");");
    }
    if (target->GetFontSize() != 12.0f) {
        EmitLine(oss, emitted, kMaxLines, "target->SetFontSize(" + FormatFloatLiteral(target->GetFontSize()) + ");");
    }
    if (target->GetFontWeight() != "Normal") {
        EmitLine(oss, emitted, kMaxLines, "target->SetFontWeight(\"" + target->GetFontWeight() + "\");");
    }

    // --- Theme tokens (enum, not strings) ---
    EmitTokenSetter(oss, emitted, kMaxLines, "SetBackgroundToken", target->GetBackgroundToken());
    EmitTokenSetter(oss, emitted, kMaxLines, "SetHoverBackgroundToken", target->GetHoverBackgroundToken());
    EmitTokenSetter(oss, emitted, kMaxLines, "SetPressedBackgroundToken", target->GetPressedBackgroundToken());
    EmitTokenSetter(oss, emitted, kMaxLines, "SetDisabledBackgroundToken", target->GetDisabledBackgroundToken());
    EmitTokenSetter(oss, emitted, kMaxLines, "SetBorderToken", target->GetBorderToken());
    EmitTokenSetter(oss, emitted, kMaxLines, "SetFocusedBorderToken", target->GetFocusedBorderToken());
    EmitTokenSetter(oss, emitted, kMaxLines, "SetColorToken", target->GetColorToken());
    EmitTokenSetter(oss, emitted, kMaxLines, "SetPlaceholderColorToken", target->GetPlaceholderColorToken());
    EmitTokenSetter(oss, emitted, kMaxLines, "SetSelectedBackgroundToken", target->GetSelectedBackgroundToken());
    EmitTokenSetter(oss, emitted, kMaxLines, "SetDropdownBackgroundToken", target->GetDropdownBackgroundToken());
    EmitTokenSetter(oss, emitted, kMaxLines, "SetSelectedItemBackgroundToken", target->GetSelectedItemBackgroundToken());
    EmitTokenSetter(oss, emitted, kMaxLines, "SetUnderlineColorToken", target->GetUnderlineColorToken());
    EmitTokenSetter(oss, emitted, kMaxLines, "SetActiveUnderlineColorToken", target->GetActiveUnderlineColorToken());
    EmitTokenSetter(oss, emitted, kMaxLines, "SetCaretColorToken", target->GetCaretColorToken());
    EmitTokenSetter(oss, emitted, kMaxLines, "SetFillColorToken", target->GetFillColorToken());
    EmitTokenSetter(oss, emitted, kMaxLines, "SetTrackColorToken", target->GetTrackColorToken());
    EmitTokenSetter(oss, emitted, kMaxLines, "SetActiveTrackColorToken", target->GetActiveTrackColorToken());
    EmitTokenSetter(oss, emitted, kMaxLines, "SetThumbColorToken", target->GetThumbColorToken());
    EmitTokenSetter(oss, emitted, kMaxLines, "SetOnColorToken", target->GetOnColorToken());
    EmitTokenSetter(oss, emitted, kMaxLines, "SetOffColorToken", target->GetOffColorToken());
    EmitTokenSetter(oss, emitted, kMaxLines, "SetKnobColorToken", target->GetKnobColorToken());
    EmitTokenSetter(oss, emitted, kMaxLines, "SetCheckedBackgroundToken", target->GetCheckedBackgroundToken());
    EmitTokenSetter(oss, emitted, kMaxLines, "SetAccentColorToken", target->GetAccentColorToken());
    EmitTokenSetter(oss, emitted, kMaxLines, "SetActiveColorToken", target->GetActiveColorToken());

    oss << "\n// ... (" << emitted << " typed setters; raw ColorF / SetProperty omitted) ...\n";
    return oss.str();
}
}

std::shared_ptr<UIElement> CreateShowcaseText(
    const std::string& content,
    float size,
    const std::string& color,
    bool bold,
    const std::string& fontFamily) {
    auto text = Text(content).FontSize(size).Build();
    const std::string mappedToken = ResolveLegacyColorToken(color);
    if (color.empty()) {
        BindThemeToken(text, "theme.colorToken", "textPrimary");
    } else if (!mappedToken.empty()) {
        BindThemeToken(text, "theme.colorToken", mappedToken);
    } else {
        BindThemeToken(text, "theme.colorToken", "textPrimary");
    }
    if (bold) text->SetFontWeight("Bold");
    if (!fontFamily.empty()) text->SetFontFamily(fontFamily);
    return text;
}

std::shared_ptr<UIElement> CreateShowcaseHeader(
    const std::string& title,
    const std::string& subtitle) {
    return Column(4).Children({
        BindThemeToken(std::static_pointer_cast<TextBlock>(CreateShowcaseText(title, 18.0f, "", true)), "theme.colorToken", "textPrimary"),
        BindThemeToken(std::static_pointer_cast<TextBlock>(CreateShowcaseText(subtitle, 12.0f, "", false)), "theme.colorToken", "textMuted")
    }).Build();
}

std::shared_ptr<UIElement> CreateDemoSurface(
    std::initializer_list<std::shared_ptr<UIElement>> children,
    float gap) {
    auto surface = Column(gap).Padding(24).CornerRadius(6).Border(ThemeManager::Instance().GetColorHex("cardBorder"), 1).Children(children).Build();
    BindThemeToken(surface, "theme.backgroundToken", "cardBackground");
    BindThemeToken(surface, "theme.borderToken", "cardBorder");
    return surface;
}

std::shared_ptr<UIElement> CreateRightPanel(
    std::initializer_list<std::shared_ptr<UIElement>> children) {
    auto panel = Column(10).Width(320).Padding(16).Border(ThemeManager::Instance().GetColorHex("cardBorder"), 1).Children(children).Build();
    BindThemeToken(panel, "theme.backgroundToken", "paneBackground");
    BindThemeToken(panel, "theme.borderToken", "cardBorder");
    return panel;
}

std::shared_ptr<UIElement> CreateRightScrollPanel(
    std::initializer_list<std::shared_ptr<UIElement>> children) {
    auto container = Column(8).Padding(16).Children(children).Build();
    auto scroll = std::make_shared<ScrollViewer>();
    scroll->SetWidth(320.0f);
    scroll->SetBorderThickness(1.0f);
    BindThemeToken(scroll, "theme.backgroundToken", "paneBackground");
    BindThemeToken(scroll, "theme.borderToken", "cardBorder");
    scroll->AddChild(container);
    return scroll;
}

std::shared_ptr<UIElement> CreatePropertyGrid(
    const ShowcaseContext& ctx,
    const std::shared_ptr<UIElement>& target) {
    auto grid = std::make_shared<PropertyGrid>();
    grid->SetWidth(320.0f);
    grid->SetTargetElement(target, ctx.windowRef);
    return grid;
}

std::shared_ptr<UIElement> CreateCodeExampleCollapse(
    const std::shared_ptr<UIElement>& target) {
    auto codeBox = std::make_shared<TextBox>();
    codeBox->SetAlign(Alignment::Stretch);
    codeBox->SetHeight(280.0f);
    codeBox->SetFontFamily("Consolas");
    codeBox->SetFontSize(12.0f);
    codeBox->SetAcceptsReturn(true);
    codeBox->SetTextWrapping(false);
    codeBox->SetIsEnabled(true);
    codeBox->SetIsReadOnly(true);
    codeBox->SetCornerRadius(4.0f);
    codeBox->SetBorderThickness(1.0f);
    codeBox->SetPadding(Thickness(10, 8, 10, 8));
    codeBox->SetText(GenerateAutoCodeExample(target));
    codeBox->SetBackgroundToken(ThemeTokenId::InputBackground);
    codeBox->SetBorderToken(ThemeTokenId::CardBorder);

    auto codeScroll = std::make_shared<ScrollViewer>();
    codeScroll->SetHeight(220.0f);
    codeScroll->SetAlign(Alignment::Stretch);
    codeScroll->AddChild(codeBox);

    auto panel = std::make_shared<Expander>("示例代码 (Source)");
    panel->SetIsExpanded(false);
    panel->SetContent(codeScroll);
    return panel;
}

std::shared_ptr<UIElement> CreatePage(
    const std::string& title,
    const std::string& subtitle,
    const std::shared_ptr<UIElement>& demo,
    const std::shared_ptr<UIElement>& side,
    const std::shared_ptr<UIElement>& sampleTarget) {
    auto mainColumnBuilder = Column(16).FlexGrow(1.0f).Padding(20);
    mainColumnBuilder.Add(CreateShowcaseHeader(title, subtitle));
    if (demo) {
        mainColumnBuilder.Add(demo);
    }
    if (sampleTarget) {
        mainColumnBuilder.Add(CreateCodeExampleCollapse(sampleTarget));
    }
    auto mainColumn = mainColumnBuilder.Build();
    BindThemeToken(mainColumn, "theme.backgroundToken", "windowBackground");

    auto mainScroll = std::make_shared<ScrollViewer>();
    mainScroll->SetFlexGrow(1.0f);
    mainScroll->SetMinWidth(240.0f);
    mainScroll->SetAlign(Alignment::Stretch);
    BindThemeToken(mainScroll, "theme.backgroundToken", "windowBackground");
    mainScroll->AddChild(mainColumn);

    std::shared_ptr<UIElement> right = side;
    if (right) {
        right->SetAlign(Alignment::Stretch);
        if (right->GetWidth() < 0.0f) {
            right->SetWidth(320.0f);
        }
    }

    auto pageRow = Row().Children({
        mainScroll,
        right
    }).Build();
    BindThemeToken(pageRow, "theme.backgroundToken", "windowBackground");
    return pageRow;
}
