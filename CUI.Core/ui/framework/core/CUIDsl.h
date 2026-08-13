#pragma once

#include "../controls/UIElement.h"
#include "../controls/Panel.h"
#include "../controls/Button.h"
#include "../controls/TextBox.h"
#include "../controls/PasswordBox.h"
#include "../controls/TextBlock.h"
#include "../controls/CheckBox.h"
#include "../controls/HyperlinkButton.h"
#include "../controls/ComboBox.h"
#include "../controls/ListBox.h"
#include "../controls/ListView.h"
#include "../controls/Image.h"
#include "../controls/ScrollViewer.h"
#include "../controls/TabView.h"
#include "../controls/MenuBar.h"
#include "../controls/PropertyGrid.h"
#include "../controls/TreeView.h"
#include "../controls/Slider.h"
#include "../controls/NumberBox.h"
#include "../controls/RadioButton.h"
#include "../controls/ToggleSwitch.h"
#include "../controls/DatePicker.h"
#include "../controls/TimePicker.h"
#include "../controls/ColorPicker.h"
#include "../controls/BreadcrumbBar.h"
#include "../controls/ProgressBar.h"
#include "../controls/ProgressRing.h"
#include "../controls/AutoSuggestBox.h"
#include "../controls/FilePicker.h"
#include "../controls/FolderPicker.h"
#include "../controls/PagingControl.h"
#include "../controls/Splitter.h"
#include "../controls/Expander.h"
#include "../controls/MessageBox.h"
#include "../style/ThemeManager.h"

#include <memory>
#include <string>
#include <vector>
#include <initializer_list>
#include <functional>

namespace CUI {
class Window;
namespace DSL {

inline D2D1_COLOR_F Rgb(unsigned int rgb, float alpha = 1.0f) {
    return D2D1::ColorF(
        static_cast<float>((rgb >> 16) & 0xFF) / 255.0f,
        static_cast<float>((rgb >> 8) & 0xFF) / 255.0f,
        static_cast<float>(rgb & 0xFF) / 255.0f,
        alpha
    );
}

template <typename T>
class ElementBuilder {
public:
    ElementBuilder() : m_element(std::make_shared<T>()) {}
    explicit ElementBuilder(std::shared_ptr<T> elem) : m_element(elem) {}

    operator std::shared_ptr<T>() const { return m_element; }
    operator std::shared_ptr<UIElement>() const { return m_element; }
    std::shared_ptr<T> Build() const { return m_element; }
    T* operator->() const { return m_element.get(); }

    ElementBuilder& Id(const std::string& id) {
        m_element->SetId(id);
        return *this;
    }

    ElementBuilder& Width(float w) {
        m_element->SetWidth(w);
        return *this;
    }

    ElementBuilder& Height(float h) {
        m_element->SetHeight(h);
        return *this;
    }

    ElementBuilder& MinWidth(float w) {
        m_element->SetMinWidth(w);
        return *this;
    }

    ElementBuilder& MinHeight(float h) {
        m_element->SetMinHeight(h);
        return *this;
    }

    ElementBuilder& Size(float w, float h) {
        m_element->SetWidth(w);
        m_element->SetHeight(h);
        return *this;
    }

    ElementBuilder& Margin(float all) {
        m_element->SetMargin(Thickness(all));
        return *this;
    }

    ElementBuilder& Margin(float l, float t, float r, float b) {
        m_element->SetMargin(Thickness(l, t, r, b));
        return *this;
    }

    ElementBuilder& Padding(float all) {
        m_element->SetPadding(Thickness(all));
        return *this;
    }

    ElementBuilder& Padding(float l, float t, float r, float b) {
        m_element->SetPadding(Thickness(l, t, r, b));
        return *this;
    }

    ElementBuilder& FlexGrow(float flex) {
        m_element->SetFlexGrow(flex);
        return *this;
    }

    ElementBuilder& BackgroundToken(ThemeTokenId id) {
        m_element->SetBackgroundToken(id);
        return *this;
    }

    ElementBuilder& Background(D2D1_COLOR_F color) {
        m_element->SetBackground(color);
        return *this;
    }

    ElementBuilder& HoverBackgroundToken(ThemeTokenId id) {
        m_element->SetHoverBackgroundToken(id);
        return *this;
    }

    ElementBuilder& HoverBackground(D2D1_COLOR_F color) {
        m_element->SetHoverBackground(color);
        return *this;
    }

    ElementBuilder& PressedBackgroundToken(ThemeTokenId id) {
        m_element->SetPressedBackgroundToken(id);
        return *this;
    }

    ElementBuilder& PressedBackground(D2D1_COLOR_F color) {
        m_element->SetPressedBackground(color);
        return *this;
    }

    ElementBuilder& ColorToken(ThemeTokenId id) {
        m_element->SetColorToken(id);
        return *this;
    }

    ElementBuilder& Color(D2D1_COLOR_F color) {
        m_element->SetColor(color);
        return *this;
    }

    ElementBuilder& FontSize(float size) {
        m_element->SetFontSize(size);
        return *this;
    }

    ElementBuilder& FontFamily(const std::string& family) {
        m_element->SetFontFamily(family);
        return *this;
    }

    ElementBuilder& FontWeight(const std::string& weight) {
        m_element->SetFontWeight(weight);
        return *this;
    }

    ElementBuilder& CornerRadius(float r) {
        m_element->SetCornerRadius(r);
        return *this;
    }

    ElementBuilder& BorderToken(ThemeTokenId id, float thickness = 1.0f) {
        m_element->SetBorderToken(id);
        m_element->SetBorderThickness(thickness);
        return *this;
    }

    ElementBuilder& Border(D2D1_COLOR_F color, float thickness = 1.0f) {
        m_element->SetBorderBrush(color);
        m_element->SetBorderThickness(thickness);
        return *this;
    }

    ElementBuilder& Enabled(bool enabled) {
        m_element->SetIsEnabled(enabled);
        return *this;
    }

    ElementBuilder& Visibility(const std::string& vis) {
        if (vis == "Hidden") m_element->SetVisibility(CUI::Visibility::Hidden);
        else if (vis == "Collapsed") m_element->SetVisibility(CUI::Visibility::Collapsed);
        else m_element->SetVisibility(CUI::Visibility::Visible);
        return *this;
    }

    // Children Adding for Container Elements
    ElementBuilder& Children(std::initializer_list<std::shared_ptr<UIElement>> list) {
        for (auto& child : list) {
            if (child) m_element->AddChild(child);
        }
        return *this;
    }

    ElementBuilder& Add(std::shared_ptr<UIElement> child) {
        if (child) m_element->AddChild(child);
        return *this;
    }

    // Control Specific Extension Methods
    ElementBuilder& Text(const std::string& text) {
        m_element->SetText(text);
        return *this;
    }

    ElementBuilder& Icon(const std::string& icon) {
        m_element->SetIcon(icon);
        return *this;
    }

    ElementBuilder& Subtitle(const std::string& subtitle) {
        auto expander = std::dynamic_pointer_cast<Expander>(m_element);
        if (expander) {
            expander->SetSubtitle(subtitle);
        }
        return *this;
    }

    ElementBuilder& Orientation(const std::string& orient) {
        if (orient == "Horizontal" || orient == "Row") {
            m_element->SetOrientation(CUI::Orientation::Horizontal);
        } else {
            m_element->SetOrientation(CUI::Orientation::Vertical);
        }
        return *this;
    }

    ElementBuilder& Gap(float gap) {
        m_element->SetGap(gap);
        return *this;
    }

    // Event Wiring fluent extensions
    ElementBuilder& OnClick(std::function<void(UIElement*)> handler) {
        if constexpr (std::is_base_of_v<Control, T> || std::is_same_v<Button, T> || std::is_same_v<HyperlinkButton, T>) {
            m_element->OnClick().Connect(handler);
        }
        return *this;
    }

    ElementBuilder& OnTextChanged(std::function<void(TextBox*, const std::string&)> handler) {
        if constexpr (std::is_same_v<TextBox, T>) {
            m_element->OnTextChanged().Connect(handler);
        }
        return *this;
    }

    ElementBuilder& OnCheckChanged(std::function<void(CheckBox*, CheckState)> handler) {
        if constexpr (std::is_base_of_v<CheckBox, T> || std::is_same_v<CheckBox, T> || std::is_same_v<RadioButton, T>) {
            m_element->OnCheckStateChanged().Connect(handler);
        }
        return *this;
    }

    ElementBuilder& OnValueChanged(std::function<void(Slider*, float)> handler) {
        if constexpr (std::is_same_v<Slider, T>) {
            m_element->OnValueChanged().Connect(handler);
        }
        return *this;
    }

protected:
    std::shared_ptr<T> m_element;
};

// Flutter-Style Widget Aliases
inline ElementBuilder<StackPanel> Column(float gap = 8.0f) {
    return ElementBuilder<StackPanel>().Orientation("Vertical").Gap(gap);
}

inline ElementBuilder<StackPanel> Row(float gap = 8.0f) {
    return ElementBuilder<StackPanel>().Orientation("Horizontal").Gap(gap);
}

inline ElementBuilder<TextBlock> Text(const std::string& content = "") {
    auto l = ElementBuilder<TextBlock>();
    if (!content.empty()) l.Text(content);
    return l;
}

inline ElementBuilder<Button> ElevatedButton(const std::string& text = "", std::function<void(UIElement*)> onPressed = nullptr) {
    auto b = ElementBuilder<Button>();
    if (!text.empty()) b.Text(text);
    if (onPressed) b.OnClick(onPressed);
    return b;
}

inline ElementBuilder<TextBox> TextField(const std::string& text = "", std::function<void(TextBox*, const std::string&)> onChanged = nullptr) {
    auto t = ElementBuilder<TextBox>();
    if (!text.empty()) t.Text(text);
    if (onChanged) t.OnTextChanged(onChanged);
    return t;
}

inline ElementBuilder<CheckBox> CheckboxTile(const std::string& title = "", std::function<void(CheckBox*, CheckState)> onChanged = nullptr) {
    auto c = ElementBuilder<CheckBox>();
    if (!title.empty()) c.Text(title);
    if (onChanged) c.OnCheckChanged(onChanged);
    return c;
}

inline ElementBuilder<Panel> Container() {
    return ElementBuilder<Panel>();
}

inline ElementBuilder<Canvas> CanvasWidget() {
    return ElementBuilder<Canvas>();
}

inline ElementBuilder<Grid> GridWidget() {
    return ElementBuilder<Grid>();
}

inline ElementBuilder<WrapPanel> WrapPanelWidget(const std::string& orient = "Horizontal") {
    return ElementBuilder<WrapPanel>().Orientation(orient);
}

inline ElementBuilder<DockPanel> DockPanelWidget() {
    return ElementBuilder<DockPanel>();
}

inline ElementBuilder<UniformGrid> UniformGridWidget(int rows = 2, int cols = 2) {
    auto u = ElementBuilder<UniformGrid>();
    u->SetRows(rows);
    u->SetColumns(cols);
    return u;
}

inline ElementBuilder<ScrollViewer> SingleChildScrollView() {
    return ElementBuilder<ScrollViewer>();
}

inline ElementBuilder<Panel> Expanded(std::shared_ptr<UIElement> child, float flex = 1.0f) {
    auto p = ElementBuilder<Panel>();
    p.FlexGrow(flex);
    if (child) {
        // Flutter's Expanded gives its child tight constraints from the expanded
        // slot. The wrapper participates in the parent flex layout, and the
        // child must also flex inside that wrapper; otherwise it keeps its old
        // desired height and can paint past the actual viewport.
        child->SetFlexGrow(1.0f);
        p.Add(child);
    }
    return p;
}

inline ElementBuilder<Slider> SliderWidget(float val = 0.0f, float min = 0.0f, float max = 100.0f, std::function<void(Slider*, float)> onChanged = nullptr) {
    auto s = ElementBuilder<Slider>();
    s->SetMinimum(min);
    s->SetMaximum(max);
    s->SetValue(val);
    if (onChanged) s.OnValueChanged(onChanged);
    return s;
}

inline ElementBuilder<ProgressBar> ProgressBarWidget(float val = 0.0f, bool isIndeterminate = false) {
    auto p = ElementBuilder<ProgressBar>();
    p->SetValue(val);
    p->SetIsIndeterminate(isIndeterminate);
    return p;
}

inline ElementBuilder<ProgressRing> ProgressRingWidget(float val = 0.0f, bool isIndeterminate = true) {
    auto p = ElementBuilder<ProgressRing>();
    p->SetValue(val);
    p->SetIsIndeterminate(isIndeterminate);
    return p;
}

inline ElementBuilder<AutoSuggestBox> AutoSuggestBoxWidget(const std::string& placeholder = "搜索…") {
    auto a = ElementBuilder<AutoSuggestBox>();
    a->SetPlaceholder(placeholder);
    return a;
}

inline ElementBuilder<FilePicker> FilePickerWidget(const std::string& path = "") {
    auto f = ElementBuilder<FilePicker>();
    if (!path.empty()) {
        f->SetPath(path);
    }
    return f;
}

inline ElementBuilder<FolderPicker> FolderPickerWidget(const std::string& path = "") {
    auto f = ElementBuilder<FolderPicker>();
    if (!path.empty()) {
        f->SetPath(path);
    }
    return f;
}

inline ElementBuilder<NumberBox> NumberBoxWidget(double val = 0.0) {
    auto n = ElementBuilder<NumberBox>();
    n->SetValue(static_cast<float>(val));
    return n;
}

inline ElementBuilder<PasswordBox> PasswordBoxWidget(const std::string& placeholder = "请输入密码") {
    auto p = ElementBuilder<PasswordBox>();
    p->SetPlaceholder(placeholder);
    return p;
}

inline ElementBuilder<RadioButton> RadioButtonTile(const std::string& text = "", const std::string& group = "DefaultGroup") {
    auto r = ElementBuilder<RadioButton>();
    if (!text.empty()) r.Text(text);
    r->SetGroupName(group);
    return r;
}

inline ElementBuilder<ToggleSwitch> ToggleSwitchTile(const std::string& header = "", bool isOn = false) {
    auto t = ElementBuilder<ToggleSwitch>();
    if (!header.empty()) t.Text(header);
    t->SetIsOn(isOn);
    return t;
}

inline ElementBuilder<DatePicker> DatePickerWidget() {
    return ElementBuilder<DatePicker>();
}

inline ElementBuilder<TimePicker> TimePickerWidget() {
    return ElementBuilder<TimePicker>();
}

inline ElementBuilder<ColorPicker> ColorPickerWidget() {
    return ElementBuilder<ColorPicker>();
}

inline ElementBuilder<BreadcrumbBar> BreadcrumbBarWidget() {
    return ElementBuilder<BreadcrumbBar>();
}

inline ElementBuilder<PagingControl> PagingControlWidget(int current = 1, int total = 10) {
    auto p = ElementBuilder<PagingControl>();
    p->SetTotalPages(total);
    p->SetCurrentPage(current);
    return p;
}

inline ElementBuilder<Splitter> SplitterWidget(Orientation orientation = Orientation::Horizontal) {
    auto s = ElementBuilder<Splitter>();
    // Cross-axis must stay -1 (auto) so parent Stretch makes a full-length bar,
    // not a thickness×thickness square.
    if (orientation == Orientation::Horizontal) {
        s->SetOrientation("Horizontal");
        s->SetWidth(-1.0f);
        s->SetHeight(10.0f);
    } else {
        s->SetOrientation("Vertical");
        s->SetWidth(10.0f);
        s->SetHeight(-1.0f);
    }
    s->SetAlign(Alignment::Stretch);
    return s;
}

inline ElementBuilder<Expander> ExpanderWidget(const std::string& title = "Expander") {
    auto c = ElementBuilder<Expander>();
    c->SetHeader(title);
    return c;
}

// Compat alias for older showcase pages.
inline ElementBuilder<Expander> CollapsePanelWidget(const std::string& title = "Expander") {
    return ExpanderWidget(title);
}

inline ElementBuilder<ListView> ListViewWidget() {
    return ElementBuilder<ListView>();
}

inline ElementBuilder<HyperlinkButton> HyperlinkButtonWidget(const std::string& text = "", const std::string& uri = "") {
    auto h = ElementBuilder<HyperlinkButton>();
    if (!text.empty()) h.Text(text);
    if (!uri.empty()) h->SetNavigateUri(uri);
    return h;
}

inline ElementBuilder<ContentDialog> ContentDialogWidget(const std::string& title = "Dialog", const std::string& message = "") {
    auto d = ElementBuilder<ContentDialog>();
    d->SetTitle(title);
    if (!message.empty()) d->SetMessage(message);
    return d;
}

struct BuildContext {
    Window* window = nullptr;
};

// Flutter-style Component Base Class (Widget build method)
class Component {
public:
    virtual ~Component() = default;
    virtual std::shared_ptr<UIElement> Build() = 0;
    operator std::shared_ptr<UIElement>() { return Build(); }
};

class Widget {
public:
    virtual ~Widget() = default;
    virtual std::shared_ptr<UIElement> build(BuildContext& context) = 0;
};

class StatelessWidget : public Component, public Widget {
public:
    StatelessWidget() = default;
    explicit StatelessWidget(BuildContext context) : m_context(context) {}
    virtual ~StatelessWidget() = default;

    void SetBuildContext(const BuildContext& context) { m_context = context; }
    BuildContext& GetBuildContext() { return m_context; }
    const BuildContext& GetBuildContext() const { return m_context; }

    std::shared_ptr<UIElement> Build() override { return build(m_context); }

private:
    BuildContext m_context;
};

} // namespace DSL
} // namespace CUI
