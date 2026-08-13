#include "catalog/Catalog.h"
#include "pages/BasicInput/Pages.h"

#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

namespace Gallery {
namespace {

std::string ToLower(std::string s) {
    for (char& c : s) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return s;
}

} // namespace

const char* CategoryDisplayName(Category category) {
    switch (category) {
    case Category::BasicInput: return "基本输入";
    case Category::Collections: return "集合";
    case Category::DateAndTime: return "日期和时间";
    case Category::DialogsAndFlyouts: return "对话框和浮出层";
    case Category::Layout: return "布局";
    case Category::Media: return "媒体";
    case Category::MenusAndToolbars: return "菜单和工具栏";
    case Category::Motion: return "动效";
    case Category::Navigation: return "导航";
    case Category::Scrolling: return "滚动";
    case Category::StatusAndInfo: return "状态和信息";
    case Category::Styles: return "样式";
    case Category::System: return "系统";
    case Category::Text: return "文本";
    case Category::Windowing: return "窗口";
    }
    return "";
}

const std::vector<Category>& CategoryOrder() {
    static const std::vector<Category> order = {
        Category::BasicInput,
        Category::Collections,
        Category::DateAndTime,
        Category::DialogsAndFlyouts,
        Category::Layout,
        Category::Media,
        Category::MenusAndToolbars,
        Category::Motion,
        Category::Navigation,
        Category::Scrolling,
        Category::StatusAndInfo,
        Category::Styles,
        Category::System,
        Category::Text,
        Category::Windowing,
    };
    return order;
}

const std::vector<Entry>& Entries() {
    static const std::vector<Entry> entries = {
        { "button", "Button(按钮)", "按钮用于触发操作。", Category::BasicInput, BuildButtonPage },
        { "dropdownbutton", "DropDownButton(下拉按钮)", "整个按钮打开菜单。", Category::BasicInput, BuildDropDownButtonPage },
        { "hyperlinkbutton", "HyperlinkButton(超链接按钮)", "用于导航的文本样式按钮。", Category::BasicInput, BuildHyperlinkButtonPage },
        { "splitbutton", "SplitButton(拆分按钮)", "默认操作外加更多命令。", Category::BasicInput, BuildSplitButtonPage },
        { "togglebutton", "ToggleButton(切换按钮)", "保持开或关的按钮。", Category::BasicInput, BuildToggleButtonPage },
        { "checkbox", "CheckBox(复选框)", "打开或关闭某个选项。", Category::BasicInput, BuildCheckBoxPage },
        { "radiobutton", "RadioButton(单选按钮)", "从一组中选择一项。", Category::BasicInput, BuildRadioButtonPage },
        { "combobox", "ComboBox(组合框)", "显示当前值，并打开列表进行更改。", Category::BasicInput, BuildComboBoxPage },
        { "slider", "Slider(滑块)", "从范围内选取一个值。", Category::BasicInput, BuildSliderPage },
        { "rangeslider", "RangeSlider(范围滑块)", "两个滑块分别设置下限和上限。", Category::BasicInput, BuildRangeSliderPage },
        { "rating", "RatingControl(评分)", "用星级表示评分。", Category::BasicInput, BuildRatingPage },
        { "toggleswitch", "ToggleSwitch(开关)", "打开或关闭某项设置。", Category::BasicInput, BuildToggleSwitchPage },
        { "colorpicker", "ColorPicker(颜色选择器)", "从色板或色谱中选取颜色。", Category::BasicInput, BuildColorPickerPage },
        { "segmented", "SegmentedControl(分段控件)", "紧凑的互斥选择。", Category::BasicInput, BuildSegmentedPage },
    };
    return entries;
}

const Entry* FindByTag(const std::string& tag) {
    for (const auto& e : Entries()) {
        if (e.tag == tag) {
            return &e;
        }
    }
    return nullptr;
}

const Entry* FindByTitle(const std::string& title) {
    for (const auto& e : Entries()) {
        if (e.title == title) {
            return &e;
        }
        const std::string full(e.title);
        const auto paren = full.find('(');
        if (paren != std::string::npos && full.compare(0, paren, title) == 0) {
            return &e;
        }
    }
    return nullptr;
}

std::vector<const Entry*> EntriesIn(Category category) {
    std::vector<const Entry*> out;
    for (const auto& e : Entries()) {
        if (e.category == category) {
            out.push_back(&e);
        }
    }
    return out;
}

std::vector<std::string> SearchTitles(const std::string& query) {
    const std::string q = ToLower(query);
    std::vector<std::string> out;
    if (q.empty()) {
        return out;
    }
    for (const auto& e : Entries()) {
        const std::string title = ToLower(e.title);
        const std::string tag = ToLower(e.tag);
        const std::string subtitle = ToLower(e.subtitle);
        const std::string category = ToLower(CategoryDisplayName(e.category));
        if (title.find(q) != std::string::npos
            || tag.find(q) != std::string::npos
            || subtitle.find(q) != std::string::npos
            || category.find(q) != std::string::npos) {
            out.push_back(e.title);
        }
    }
    return out;
}

} // namespace Gallery
