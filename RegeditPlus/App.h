#pragma once

#include "framework/window/Window.h"
#include "RegIcons.h"

#include <windows.h>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <utility>
#include <functional>

namespace CUI {
class UIElement;
class TreeView;
struct TreeViewItem;
class ListView;
class TextBlock;
class WindowTitleBar;
class ContextMenu;
class BreadcrumbBar;
} // namespace CUI

namespace RegeditPlus {

struct ValueEntry {
    std::wstring name;
    DWORD type = 0;
    std::vector<BYTE> data;
};

struct Favorite {
    std::string label;   // UTF-8, for menu display
    std::wstring path;   // full registry path
};

class RegeditPlusApp {
public:
    int Run();

private:
    std::shared_ptr<CUI::UIElement> BuildRoot();
    void BuildMenus();

    // --- Tree ------------------------------------------------------------
    void BuildInitialTree();
    std::shared_ptr<CUI::TreeViewItem> MakeNode(const std::wstring& fullPath,
                                                const std::wstring& label,
                                                bool mayHaveChildren = true);
    void EnsureExpanded(const std::shared_ptr<CUI::TreeViewItem>& node);
    void PopulateSubkeys(const std::shared_ptr<CUI::TreeViewItem>& node);
    void RefreshTreeVisual();
    void RevealPath(const std::wstring& fullPath);
    void UpdateBreadcrumb();
    void OnBreadcrumbClicked(int index);
    void OnTreeSelectionChanged(CUI::TreeView* tree, std::shared_ptr<CUI::TreeViewItem> item);
    void OnTreeNodeToggled(CUI::TreeView* tree, std::shared_ptr<CUI::TreeViewItem> item);

    // --- Values -----------------------------------------------------------
    void LoadValues(const std::wstring& path);
    void ReloadCurrent();
    bool HasValidKey() const { return !m_selectedPath.empty(); }
    int SelectedValueRow() const;
    std::string TypeName(DWORD type) const;
    std::string DataText(const ValueEntry& entry) const;

    // --- Context menus -----------------------------------------------------
    std::shared_ptr<CUI::ContextMenu> BuildTreeContextMenu();
    std::shared_ptr<CUI::ContextMenu> BuildListContextMenu();

    // --- Actions -----------------------------------------------------------
    void CreateNewKey();
    void CreateNewValue(DWORD type, const char* defaultName);
    void RenameSelectedKey();
    void DeleteSelectedKey();
    void CopyKeyName();
    void ModifySelectedValue();
    void WriteValueAt(int row, DWORD type, const std::vector<BYTE>& bytes);
    void DeleteSelectedValue();
    void RenameSelectedValue();
    void ToggleStatusBar();
    void ToggleTheme();
    void ApplyChromeColors();
    void ApplyNativeRegeditIcon();
    void RefreshCurrent();
    void ShowFindDialog();
    void FindNext();
    void AddToFavorites();
    void ClearFavorites();

    void ShowMessage(const std::string& title, const std::string& message);
    void PromptText(const std::string& title, const std::string& label,
                    const std::wstring& initial, bool multiline,
                    std::function<void(const std::wstring&)> onOk);

private:
    CUI::Window m_window;
    std::shared_ptr<CUI::UIElement> m_root;
    std::shared_ptr<CUI::WindowTitleBar> m_titleBar;
    std::shared_ptr<CUI::BreadcrumbBar> m_breadcrumb;

    std::shared_ptr<CUI::TreeView> m_tree;
    std::shared_ptr<CUI::ListView> m_list;
    std::shared_ptr<CUI::TextBlock> m_statusPath;
    std::shared_ptr<CUI::UIElement> m_statusBar;

    std::unordered_map<const CUI::TreeViewItem*, std::wstring> m_nodePaths;
    std::unordered_map<const CUI::TreeViewItem*, bool> m_nodeLoaded;
    std::unordered_map<std::wstring, std::weak_ptr<CUI::TreeViewItem>> m_pathNodes;

    std::wstring m_selectedPath;
    int m_selectedRow = -1;
    std::vector<ValueEntry> m_entries;
    std::wstring m_findQuery;
    std::wstring m_findNextStart;
    std::vector<Favorite> m_favorites;
    RegIcons m_icons;
};

} // namespace RegeditPlus