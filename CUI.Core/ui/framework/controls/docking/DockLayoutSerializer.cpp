#include "DockLayoutSerializer.h"
#include "DockManager.h"
#include <fstream>
#include <sstream>

namespace CUI {

namespace {
void WriteGroup(std::ostream& os, const char* name, const DockTabGroup& g, const std::vector<DockPaneData>& panes) {
    os << "\"" << name << "\":[";
    for (size_t i = 0; i < g.paneIndices.size(); ++i) {
        if (i) os << ",";
        const int idx = g.paneIndices[i];
        if (idx >= 0 && idx < static_cast<int>(panes.size())) {
            os << "\"" << panes[idx].id << "\"";
        }
    }
    os << "]";
}
} // namespace

bool DockLayoutSerializer::Save(const DockManager& manager, const std::wstring& path) {
    std::ofstream out(path);
    if (!out) {
        return false;
    }
    out << "{";
    out << "\"leftSize\":" << manager.m_leftSize << ",";
    out << "\"rightSize\":" << manager.m_rightSize << ",";
    out << "\"topSize\":" << manager.m_topSize << ",";
    out << "\"bottomSize\":" << manager.m_bottomSize << ",";
    WriteGroup(out, "left", manager.m_left, manager.m_panes);
    out << ",";
    WriteGroup(out, "right", manager.m_right, manager.m_panes);
    out << ",";
    WriteGroup(out, "top", manager.m_top, manager.m_panes);
    out << ",";
    WriteGroup(out, "bottom", manager.m_bottom, manager.m_panes);
    out << ",";
    WriteGroup(out, "center", manager.m_center, manager.m_panes);
    out << "}";
    return true;
}

bool DockLayoutSerializer::Load(DockManager& manager, const std::wstring& path) {
    std::ifstream in(path);
    if (!in) {
        return false;
    }
    std::stringstream buffer;
    buffer << in.rdbuf();
    const std::string json = buffer.str();

    auto findNum = [&](const char* key, float& outVal) {
        const std::string needle = std::string("\"") + key + "\":";
        const auto pos = json.find(needle);
        if (pos == std::string::npos) return;
        outVal = static_cast<float>(atof(json.c_str() + pos + needle.size()));
    };
    findNum("leftSize", manager.m_leftSize);
    findNum("rightSize", manager.m_rightSize);
    findNum("topSize", manager.m_topSize);
    findNum("bottomSize", manager.m_bottomSize);

    auto loadGroup = [&](const char* key, DockTabGroup& g) {
        g.paneIndices.clear();
        g.selected = 0;
        const std::string needle = std::string("\"") + key + "\":[";
        auto pos = json.find(needle);
        if (pos == std::string::npos) return;
        pos += needle.size();
        while (pos < json.size() && json[pos] != ']') {
            if (json[pos] == '"') {
                const auto end = json.find('"', pos + 1);
                if (end == std::string::npos) break;
                const std::string id = json.substr(pos + 1, end - pos - 1);
                for (int i = 0; i < static_cast<int>(manager.m_panes.size()); ++i) {
                    if (manager.m_panes[i].id == id) {
                        g.paneIndices.push_back(i);
                        break;
                    }
                }
                pos = end + 1;
            } else {
                ++pos;
            }
        }
    };

    // Clear slots then reload membership (panes stay alive).
    manager.m_left = {};
    manager.m_right = {};
    manager.m_top = {};
    manager.m_bottom = {};
    manager.m_center = {};
    loadGroup("left", manager.m_left);
    loadGroup("right", manager.m_right);
    loadGroup("top", manager.m_top);
    loadGroup("bottom", manager.m_bottom);
    loadGroup("center", manager.m_center);
    manager.InvalidateArrange();
    manager.MarkRenderContentDirty();
    return true;
}

} // namespace CUI
