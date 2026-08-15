#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"

#include "framework/core/CUIDsl.h"
#include "framework/core/State.h"
#include "framework/controls/TreeView.h"

#include <memory>
#include <string>
#include <vector>

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {
namespace {

std::shared_ptr<TreeViewItem> TreeItem(const std::string& header, const std::string& icon = "", bool expanded = false) {
    auto item = std::make_shared<TreeViewItem>();
    item->header = header;
    item->icon = icon;
    item->isExpanded = expanded;
    item->expandAnim.Reset(expanded ? 1.0f : 0.0f);
    return item;
}

std::vector<std::shared_ptr<TreeViewItem>> BuildProjectTree() {
    auto workspace = TreeItem("CUI 工作区", "📁", true);
    auto core = TreeItem("CUI.Core", "📦", true);
    core->children = {
        TreeItem("ui", "📁", true),
        TreeItem("framework", "📁", true),
        TreeItem("CUI.Core.vcxproj", "⚙"),
    };
    core->children[0]->children = {
        TreeItem("framework", "📁"),
        TreeItem("render", "🎨"),
        TreeItem("window", "🪟"),
    };
    core->children[1]->children = {
        TreeItem("controls", "📁"),
        TreeItem("core", "📁"),
        TreeItem("style", "📁"),
    };

    auto gallery = TreeItem("CUI.Gallery", "📦", true);
    gallery->children = {
        TreeItem("src", "📁", true),
        TreeItem("assets", "🖼"),
        TreeItem("README.md", "📄"),
    };
    gallery->children[0]->children = {
        TreeItem("pages", "📁"),
        TreeItem("chrome", "📁"),
        TreeItem("main.cpp", "⚙"),
    };

    auto docs = TreeItem("docs", "📚", false);
    docs->children = { TreeItem("architecture.md", "📄"), TreeItem("binding.md", "📄") };
    workspace->children = { core, gallery, docs };
    return { workspace };
}

void SetExpandedRecursively(TreeView* tree, const std::vector<std::shared_ptr<TreeViewItem>>& items, bool expanded) {
    for (const auto& item : items) {
        if (!item) continue;
        tree->SetItemExpanded(item, expanded);
        SetExpandedRecursively(tree, item->children, expanded);
    }
}

} // namespace

Element BuildTreeViewPage() {
    auto tree = TreeViewWidget()
        .Height(330.0f)
        .Width(520.0f);
    tree->SetItems(BuildProjectTree());

    State<std::string> treeStatusText{ "选择节点，单击箭头展开或折叠；双击节点可作为打开命令。" };
    auto treeStatus = MakeStatus("");
    treeStatus->Text->Bind(treeStatusText, BindingMode::OneWay);
    tree->OnSelectionChanged().Connect([treeStatusText](TreeView*, std::shared_ptr<TreeViewItem> item) {
        treeStatusText = item ? "已选择：" + item->header + "。" : "已取消选择。";
    });
    tree->OnItemToggled().Connect([treeStatusText](TreeView*, std::shared_ptr<TreeViewItem> item) {
        treeStatusText = item->header + (item->isExpanded ? " 已展开。" : " 已折叠。");
    });
    tree->OnItemDoubleClicked().Connect([treeStatusText](TreeView*, std::shared_ptr<TreeViewItem> item) {
        treeStatusText = "双击：" + item->header + "；业务层可在此打开文件或页面。";
    });

    auto expandAll = Button("全部展开")
        .OnClick([tree, treeStatusText](UIElement*) {
            SetExpandedRecursively(tree.get(), tree->GetItems(), true);
            treeStatusText = "已展开所有包含子项的节点。";
            });
    auto collapseAll = Button("全部折叠")
        .OnClick([tree, treeStatusText](UIElement*) {
            SetExpandedRecursively(tree.get(), tree->GetItems(), false);
            treeStatusText = "已折叠所有分支。";
            });
    auto selectRoot = Button("选择根节点")
        .OnClick([tree](UIElement*) {
            if (!tree->GetItems().empty()) tree->SetSelectedItem(tree->GetItems().front());
            });
    auto clearSelection = Button("清除选择")
        .OnClick([tree](UIElement*) { tree->SetSelectedItem(nullptr); });
    auto indentCompact = Button("紧凑缩进")
        .OnClick([tree](UIElement*) { tree->SetIndentWidth(14.0f); });
    auto indentWide = Button("宽松缩进")
        .OnClick([tree](UIElement*) { tree->SetIndentWidth(28.0f); });

    State<int> newNodeSerial{ 1 };
    auto addRoot = Button("添加根节点")
        .OnClick([tree, newNodeSerial, treeStatusText](UIElement*) {
            const int serial = newNodeSerial.Get();
            newNodeSerial = serial + 1;
            auto item = tree->AddItem("动态根节点 " + std::to_string(serial), true);
            item->icon = "✨";
            tree->SetSelectedItem(item);
            treeStatusText = "已添加并选中动态根节点。";
            });
    auto addChild = Button("向选中项加载子项")
        .OnClick([tree, newNodeSerial, treeStatusText](UIElement*) {
            auto parent = tree->GetSelectedItem();
            if (!parent) {
            treeStatusText = "请先选择一个节点，再加载子项。";
            return;
            }
            const int serial = newNodeSerial.Get();
            newNodeSerial = serial + 1;
            auto child = TreeItem("延迟加载子项 " + std::to_string(serial), "📄");
            child->parent = parent.get();
            parent->children.push_back(child);
            tree->SetItemExpanded(parent, true);
            tree->InvalidateVisibleItems();
            treeStatusText = "已为 " + parent->header + " 加载一个子项。";
            });
    auto clearTree = Button("清空树")
        .OnClick([tree, treeStatusText](UIElement*) {
            tree->ClearItems();
            treeStatusText = "已清空全部根节点。";
            });
    auto resetTree = Button("替换整个数据源")
        .OnClick([tree, treeStatusText](UIElement*) {
            tree->SetItems(BuildProjectTree());
            treeStatusText = "已通过 SetItems 替换整个树形集合。";
            });

    auto lazyTree = TreeViewWidget()
        .Height(190.0f)
        .Width(520.0f);
    auto lazyRoot = TreeItem("按需加载目录", "📁", false);
    lazyTree->AddItem(lazyRoot);
    State<std::string> lazyStatusText{ "点击“加载子项”，模拟文件系统或网络目录的延迟返回。" };
    auto lazyStatus = MakeStatus("");
    lazyStatus->Text->Bind(lazyStatusText, BindingMode::OneWay);
    auto loadLazy = Button("加载子项")
        .OnClick([lazyTree, lazyRoot, lazyStatusText](UIElement*) {
            if (lazyRoot->children.empty()) {
            lazyRoot->children = {
            TreeItem("assets", "📁"),
            TreeItem("src", "📁"),
            TreeItem("README.md", "📄"),
            };
            for (const auto& child : lazyRoot->children) child->parent = lazyRoot.get();
            lazyTree->InvalidateVisibleItems();
            }
            lazyTree->SetItemExpanded(lazyRoot, true);
            lazyStatusText = "延迟子项已加载；再次点击不会重复创建。";
            });

    SamplePageSpec spec;
    spec.title = "TreeView(树形视图)";
    spec.subtitle = "面向层级集合：支持节点选择、展开折叠、键盘导航、双击、动态加载、数据源替换和缩进控制。";
    spec.sections = {
        {
            "层级集合与节点操作",
            "行点击选择；箭头切换展开；左/右键可导航父子关系；双击通常用于打开节点。",
            Column(10, {
                tree,
                treeStatus,
                Row(8, {expandAll, collapseAll, selectRoot, clearSelection }),
                Row(8, {indentCompact, indentWide, addRoot, addChild }),
                Row(8, {clearTree, resetTree }),
            }),
        },
        {
            "延迟加载子节点",
            "直接修改 TreeViewItem::children 后调用 InvalidateVisibleItems()，适合目录、远程资源或按需展开的数据。",
            Column(8, { lazyTree, lazyStatus, loadLazy }),
        },
    };
    spec.source =
        "auto root = TreeItem(\"CUI 工作区\", \"📁\", true);\n"
        "root->children.push_back(TreeItem(\"CUI.Core\", \"📦\"));\n"
        "tree->SetItems({ root });\n"
        "tree->SetItemExpanded(root, true);\n"
        "tree->OnSelectionChanged().Connect(...);\n"
        "// 修改 children 后：tree->InvalidateVisibleItems();\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
