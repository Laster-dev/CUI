#include "ShowcaseHelpers.h"
#include "framework/style/ThemeManager.h"
#include "framework/controls/TextBox.h"
#include "framework/controls/CollapsePanel.h"
#include "framework/controls/ScrollViewer.h"
#include <algorithm>
#include <iomanip>
#include <sstream>

using namespace CUI;
using namespace CUI::DSL;

namespace {
template <typename T>
std::shared_ptr<T> BindThemeToken(const std::shared_ptr<T>& element, const std::string& tokenProp, const std::string& tokenName) {
    if (element) {
        element->SetProperty(tokenProp, Value(tokenName));
        if (tokenProp == "theme.backgroundToken") {
            element->SetProperty("background", Value(ThemeManager::Instance().GetColor(tokenName)));
        } else if (tokenProp == "theme.borderToken") {
            element->SetProperty("borderBrush", Value(ThemeManager::Instance().GetColor(tokenName)));
        } else if (tokenProp == "theme.colorToken") {
            element->SetProperty("color", Value(ThemeManager::Instance().GetColor(tokenName)));
        }
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

std::string FormatValueForCode(const Value& v) {
    using VT = Value::Type;
    switch (v.GetType()) {
    case VT::Bool:
        return v.AsBool() ? "true" : "false";
    case VT::Int: {
        std::ostringstream oss;
        oss << v.AsInt();
        return oss.str();
    }
    case VT::Float: {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(3) << v.AsFloat();
        // Trim trailing zeros a bit for readability.
        std::string s = oss.str();
        while (s.size() > 1 && s.back() == '0') s.pop_back();
        if (!s.empty() && s.back() == '.') s += "0";
        return s;
    }
    case VT::String:
        return "\"" + v.AsString() + "\"";
    case VT::Thickness: {
        Thickness t = v.AsThickness();
        std::ostringstream oss;
        oss << "Thickness(" << t.left << "," << t.top << "," << t.right << "," << t.bottom << ")";
        return oss.str();
    }
    case VT::Color: {
        D2D1_COLOR_F c = v.AsColor();
        std::ostringstream oss;
        oss << "ColorF(" << std::fixed << std::setprecision(3)
            << c.r << "," << c.g << "," << c.b << "," << c.a << ")";
        return oss.str();
    }
    default:
        return "/*unsupported*/";
    }
}

std::string GenerateAutoCodeExample(const std::shared_ptr<UIElement>& target) {
    if (!target) {
        return std::string("// No target\n");
    }

    std::vector<std::string> keys;
    const auto& all = target->GetAllProperties();
    keys.reserve(all.size());
    for (const auto& kv : all) {
        keys.push_back(kv.first);
    }
    std::sort(keys.begin(), keys.end());

    auto skipKey = [&](const std::string& k) -> bool {
        return k == "visibility" || k == "isEnabled" || k == "focused" || k == "toolTip";
    };

    std::ostringstream oss;
    oss << "// Auto-generated sample (from target properties reflection)\n";
    oss << "// Target: " << target->GetClassName() << "\n\n";
    oss << "// This is not a perfect WinUI XAML mirror, but it's a real code-like\n";
    oss << "// example to show how the sample is configured.\n";
    oss << "auto target = std::make_shared<" << target->GetClassName() << ">();\n";

    int emitted = 0;
    const int kMaxLines = 60;
    for (const auto& k : keys) {
        if (skipKey(k)) continue;
        const auto& v = all.at(k);
        if (v.IsEmpty()) continue;

        // Keep the snippet compact: many theme/*Token properties are useful, but
        // pointer and unsupported types would be noisy.
        if (v.GetType() == Value::Type::Pointer || v.GetType() == Value::Type::None) continue;

        oss << "target->SetProperty(\"" << k << "\", Value(" << FormatValueForCode(v) << "));\n";
        if (++emitted >= kMaxLines) break;
    }
    oss << "\n// ... (" << emitted << " properties) ...\n";
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
    if (bold) text->SetProperty("fontWeight", Value("Bold"));
    if (!fontFamily.empty()) text->SetProperty("fontFamily", Value(fontFamily));
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
    BindThemeToken(panel, "theme.backgroundToken", "cardBackground");
    BindThemeToken(panel, "theme.borderToken", "cardBorder");
    return panel;
}

std::shared_ptr<UIElement> CreateRightScrollPanel(
    std::initializer_list<std::shared_ptr<UIElement>> children) {
    auto container = Column(8).Padding(16).Children(children).Build();
    auto scroll = std::make_shared<ScrollViewer>();
    scroll->SetProperty("width", Value(320.0f));
    scroll->SetProperty("borderThickness", Value(1.0f));
    BindThemeToken(scroll, "theme.backgroundToken", "cardBackground");
    BindThemeToken(scroll, "theme.borderToken", "cardBorder");
    scroll->AddChild(container);
    return scroll;
}

std::shared_ptr<UIElement> CreatePropertyGrid(
    const ShowcaseContext& ctx,
    const std::shared_ptr<UIElement>& target) {
    auto grid = std::make_shared<PropertyGrid>();
    grid->SetProperty("width", Value(320.0f));
    grid->SetTargetElement(target, ctx.windowRef);
    return grid;
}

std::shared_ptr<UIElement> CreateCodeExampleCollapse(
    const std::shared_ptr<UIElement>& target) {
    auto codeBox = std::make_shared<TextBox>();
    codeBox->SetProperty("align", Value("Stretch"));
    codeBox->SetProperty("height", Value(280.0f));
    codeBox->SetProperty("fontFamily", Value("Consolas"));
    codeBox->SetProperty("fontSize", Value(12.0f));
    codeBox->SetProperty("AcceptsReturn", Value(true));
    codeBox->SetProperty("TextWrapping", Value("NoWrap"));
    codeBox->SetIsEnabled(true);
    codeBox->SetIsReadOnly(true);
    codeBox->SetProperty("cornerRadius", Value(4.0f));
    codeBox->SetProperty("borderThickness", Value(1.0f));
    codeBox->SetProperty("padding", Value(Thickness(10, 8, 10, 8)));
    codeBox->SetProperty("text", Value(GenerateAutoCodeExample(target)));
    BindThemeToken(codeBox, "theme.backgroundToken", "cardBackground");
    BindThemeToken(codeBox, "theme.borderToken", "cardBorder");

    auto codeScroll = std::make_shared<ScrollViewer>();
    codeScroll->SetProperty("height", Value(220.0f));
    codeScroll->SetProperty("align", Value("Stretch"));
    BindThemeToken(codeScroll, "theme.backgroundToken", "cardBackground");
    BindThemeToken(codeScroll, "theme.borderToken", "cardBorder");
    codeScroll->AddChild(codeBox);

    auto panel = std::make_shared<CollapsePanel>("示例代码 (Source)");
    panel->SetExpanded(false);
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

    // Main content scrolls; flexGrow keeps PropertyGrid visible on the right.
    auto mainScroll = std::make_shared<ScrollViewer>();
    mainScroll->SetProperty("flexGrow", Value(1.0f));
    mainScroll->SetProperty("minWidth", Value(240.0f));
    mainScroll->SetProperty("align", Value("Stretch"));
    BindThemeToken(mainScroll, "theme.backgroundToken", "windowBackground");
    mainScroll->AddChild(mainColumn);

    std::shared_ptr<UIElement> right = side;
    if (right) {
        right->SetProperty("align", Value("Stretch"));
        if (!right->HasProperty("width")) {
            right->SetProperty("width", Value(320.0f));
        }
    }

    return Row().Children({
        mainScroll,
        right
    }).Build();
}
