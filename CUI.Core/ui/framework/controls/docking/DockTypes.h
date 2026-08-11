#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <memory>

namespace CUI {

class UIElement;

enum class DockSide : uint8_t {
    None = 0,
    Left,
    Right,
    Top,
    Bottom,
    Center
};

enum class DockPaneKind : uint8_t {
    Tool = 0,
    Document
};

enum class DockDropKind : uint8_t {
    None = 0,
    EdgeLeft,
    EdgeRight,
    EdgeTop,
    EdgeBottom,
    IntoCenter,
    Float
};

// Pure data — not a UIElement.
struct DockPaneData {
    std::string id;
    std::string title;
    DockPaneKind kind = DockPaneKind::Tool;
    bool canClose = true;
    bool canFloat = true;
    bool autoHide = false;
    std::shared_ptr<UIElement> content; // only real child of DockManager
};

// Tab stack on one dock slot (data only).
struct DockTabGroup {
    std::vector<int> paneIndices; // indices into DockManager::m_panes
    int selected = 0;
    float tabScroll = 0.0f; // VS-style horizontal overflow scroll
};

struct DockAutoHideItem {
    int paneIndex = -1;
    DockSide side = DockSide::Left;
};

inline const char* DockSideToString(DockSide side) {
    switch (side) {
    case DockSide::Left: return "Left";
    case DockSide::Right: return "Right";
    case DockSide::Top: return "Top";
    case DockSide::Bottom: return "Bottom";
    case DockSide::Center: return "Center";
    default: return "None";
    }
}

inline DockSide DockSideFromString(const std::string& s) {
    if (s == "Left") return DockSide::Left;
    if (s == "Right") return DockSide::Right;
    if (s == "Top") return DockSide::Top;
    if (s == "Bottom") return DockSide::Bottom;
    if (s == "Center") return DockSide::Center;
    return DockSide::None;
}

} // namespace CUI
