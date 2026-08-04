#include "PageRegistry.h"
#include "../ShowcaseHelpers.h"

#include <memory>

ShowcasePage BuildTreeViewPage(const ShowcaseContext& ctx) {
    auto rootDocs = std::make_shared<CUI::TreeViewItem>();
    rootDocs->header = "项目文档";
    rootDocs->isExpanded = true;
    auto childReadme = std::make_shared<CUI::TreeViewItem>(); childReadme->header = "README";
    auto childGuide = std::make_shared<CUI::TreeViewItem>(); childGuide->header = "开发指南";
    rootDocs->children = { childReadme, childGuide };

    auto rootFeatures = std::make_shared<CUI::TreeViewItem>();
    rootFeatures->header = "控件能力";
    rootFeatures->isExpanded = true;
    auto childDsl = std::make_shared<CUI::TreeViewItem>(); childDsl->header = "声明式 DSL";
    auto childRender = std::make_shared<CUI::TreeViewItem>(); childRender->header = "Direct2D 渲染";
    auto childInput = std::make_shared<CUI::TreeViewItem>(); childInput->header = "输入与焦点系统";
    rootFeatures->children = { childDsl, childRender, childInput };

    auto tree = std::make_shared<CUI::TreeView>();
    tree->SetItems({ rootDocs, rootFeatures });
    tree->SetProperty("width", CUI::Value(300.0f));
    tree->SetProperty("height", CUI::Value(360.0f));

    return { "TreeView 树形图", CreatePage(
        "TreeView 树形视图控件 (支持层级折叠/展开与多项交互)",
        "高效层级树结构控件，支持展开/折叠箭头、图标、键盘上下左右导航与属性表实时响应。",
        CreateDemoSurface({ tree }, 0.0f),
        CreatePropertyGrid(ctx, tree), tree) };
}
