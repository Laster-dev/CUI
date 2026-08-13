#pragma once

#include "framework/controls/UIElement.h"
#include <functional>
#include <string>
#include <vector>

namespace Gallery {

enum class Category {
    BasicInput,
    Collections,
    DateAndTime,
    DialogsAndFlyouts,
    Layout,
    Media,
    MenusAndToolbars,
    Motion,
    Navigation,
    Scrolling,
    StatusAndInfo,
    Styles,
    System,
    Text,
    Windowing,
};

struct Entry {
    const char* tag = "";
    const char* title = "";
    const char* subtitle = "";
    Category category = Category::BasicInput;
    std::function<std::shared_ptr<CUI::UIElement>()> build;
};

const char* CategoryDisplayName(Category category);
const std::vector<Category>& CategoryOrder();

// Only entries with a page factory. Sidebar, Home, and search all use this.
const std::vector<Entry>& Entries();
const Entry* FindByTag(const std::string& tag);
const Entry* FindByTitle(const std::string& title);
std::vector<const Entry*> EntriesIn(Category category);
std::vector<std::string> SearchTitles(const std::string& query);

} // namespace Gallery
