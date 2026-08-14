#include "catalog/Catalog.h"
#include "catalog/CategoryRegistrations.h"

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
    static std::vector<Entry> entries;
    if (entries.empty()) {
        BasicInputCatalog::Register(entries);
        CollectionsCatalog::Register(entries);
        DateAndTimeCatalog::Register(entries);
        DialogsAndFlyoutsCatalog::Register(entries);
        LayoutCatalog::Register(entries);
        MediaCatalog::Register(entries);
        MenusAndToolbarsCatalog::Register(entries);
        MotionCatalog::Register(entries);
        NavigationCatalog::Register(entries);
        ScrollingCatalog::Register(entries);
        StatusAndInfoCatalog::Register(entries);
        StylesCatalog::Register(entries);
        SystemCatalog::Register(entries);
        TextCatalog::Register(entries);
        WindowingCatalog::Register(entries);
    }
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
