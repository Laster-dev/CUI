#include "DockLayoutSerializer.h"
#include "DockManager.h"
#include "DockFloatWindow.h"
#include <algorithm>
#include <cstdlib>
#include <filesystem>
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
    // std::ofstream 的 wstring 构造是 MSVC 扩展；用 filesystem::path 保证可移植。
    // 必须用具名变量：直接写 out(std::filesystem::path(path)) 会被解析成函数声明。
    const std::filesystem::path outPath(path);
    std::ofstream out(outPath);
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
    // 自动隐藏（钉住）状态：不保存的话，Load 之后这些 pane 会失去钉住状态，
    // 或被残留的 m_autoHide 项留在“既不显示也不在侧边条”的隐形状态。
    out << ",\"autoHide\":[";
    bool firstHide = true;
    for (const DockAutoHideItem& ah : manager.m_autoHide) {
        if (ah.paneIndex < 0 || ah.paneIndex >= static_cast<int>(manager.m_panes.size())) {
            continue;
        }
        if (!firstHide) {
            out << ",";
        }
        firstHide = false;
        out << "{\"id\":\"" << manager.m_panes[ah.paneIndex].id
            << "\",\"side\":" << static_cast<int>(ah.side) << "}";
    }
    out << "]";
    out << "}";
    return true;
}

bool DockLayoutSerializer::Load(DockManager& manager, const std::wstring& path) {
    const std::filesystem::path inPath(path);
    std::ifstream in(inPath);
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

    auto loadAutoHide = [&]() {
        const std::string needle = "\"autoHide\":[";
        auto pos = json.find(needle);
        if (pos == std::string::npos) {
            return;
        }
        pos += needle.size();
        while (pos < json.size() && json[pos] != ']') {
            const auto objStart = json.find('{', pos);
            if (objStart == std::string::npos) {
                break;
            }
            const auto objEnd = json.find('}', objStart);
            if (objEnd == std::string::npos) {
                break;
            }
            const std::string obj = json.substr(objStart + 1, objEnd - objStart - 1);

            std::string id;
            int side = static_cast<int>(DockSide::Left);
            const auto idPos = obj.find("\"id\":\"");
            if (idPos != std::string::npos) {
                const auto s = idPos + 6;
                const auto e = obj.find('"', s);
                if (e != std::string::npos) {
                    id = obj.substr(s, e - s);
                }
            }
            const auto sidePos = obj.find("\"side\":");
            if (sidePos != std::string::npos) {
                side = std::atoi(obj.c_str() + sidePos + 7);
            }

            for (int i = 0; i < static_cast<int>(manager.m_panes.size()); ++i) {
                if (manager.m_panes[i].id != id) {
                    continue;
                }
                manager.m_panes[i].autoHide = true;
                DockAutoHideItem item;
                item.paneIndex = i;
                item.side = static_cast<DockSide>(side);
                manager.m_autoHide.push_back(item);
                break;
            }
            pos = objEnd + 1;
        }
    };

    // Clear slots then reload membership (panes stay alive).
    manager.m_left = {};
    manager.m_right = {};
    manager.m_top = {};
    manager.m_bottom = {};
    manager.m_center = {};
    // 旧布局里被钉住的 pane 必须一并复位：否则残留的 m_autoHide 项会让这些 pane
    // 既不显示内容、也不出现在侧边条上，等于从界面消失。
    manager.m_autoHide.clear();
    manager.m_peekPane = -1;
    for (DockPaneData& pane : manager.m_panes) {
        pane.autoHide = false;
    }
    loadGroup("left", manager.m_left);
    loadGroup("right", manager.m_right);
    loadGroup("top", manager.m_top);
    loadGroup("bottom", manager.m_bottom);
    loadGroup("center", manager.m_center);
    loadAutoHide();

    auto removeFromSlots = [&](int idx) {
        for (auto* g : { &manager.m_left, &manager.m_right, &manager.m_top,
                         &manager.m_bottom, &manager.m_center }) {
            g->paneIndices.erase(
                std::remove(g->paneIndices.begin(), g->paneIndices.end(), idx),
                g->paneIndices.end());
        }
    };
    for (const DockAutoHideItem& ah : manager.m_autoHide) {
        removeFromSlots(ah.paneIndex);
    }

    // 没有被任何 slot 认领、也没有悬浮的 pane 一律回收到 Center，
    // 否则它们在 Load 之后会从界面上彻底消失。
    const auto isClaimed = [&](int idx) {
        for (const auto* g : { &manager.m_left, &manager.m_right, &manager.m_top,
                               &manager.m_bottom, &manager.m_center }) {
            for (int i : g->paneIndices) {
                if (i == idx) {
                    return true;
                }
            }
        }
        return false;
    };
    const auto isFloated = [&](int idx) {
        for (const auto& f : manager.m_floats) {
            if (f && f->GetPaneIndex() == idx) {
                return true;
            }
        }
        return false;
    };
    for (int i = 0; i < static_cast<int>(manager.m_panes.size()); ++i) {
        if (isClaimed(i) || isFloated(i)) {
            continue;
        }
        manager.m_center.paneIndices.push_back(i);
    }

    manager.InvalidateArrange();
    manager.MarkRenderContentDirty();
    return true;
}

} // namespace CUI
