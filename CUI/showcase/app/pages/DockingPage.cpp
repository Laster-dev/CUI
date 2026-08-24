#include "PageRegistry.h"
#include "../ShowcaseHelpers.h"
#include "framework/core/CUIDsl.h"
#include "framework/controls/docking/Docking.h"
#include "framework/controls/Button.h"
#include "framework/controls/TreeView.h"
#include "framework/controls/TextBlock.h"
#include "framework/style/ThemeManager.h"
#include "framework/style/ThemeTokenId.h"

using namespace CUI;
using namespace CUI::DSL;

namespace {

std::shared_ptr<UIElement> MakePaneBody(const std::string& text) {
    auto body = std::make_shared<TextBlock>();
    CUI::DSL::Borrow(body).Text(text);
    CUI::DSL::Borrow(body).FontSize(13.0f);
    CUI::DSL::Borrow(body).ForegroundToken(ThemeTokenId::TextPrimary);
    CUI::DSL::Borrow(body).Padding(Thickness(12.0f));
    CUI::DSL::Borrow(body).BackgroundToken(ThemeTokenId::WindowBackground);
    CUI::DSL::Borrow(body).Background(ThemeManager::Instance().GetColor(ThemeTokenId::WindowBackground));
    return body;
}

} // namespace

ShowcasePage BuildDockingPage(const ShowcaseContext& ctx) {
    auto dock = std::make_shared<DockManager>();
    DSL::Borrow(dock).OwnerWindow(ctx.windowRef);
    CUI::DSL::Borrow(dock).FlexGrow(1.0f);
    CUI::DSL::Borrow(dock).Align(Alignment::Stretch);
    CUI::DSL::Borrow(dock).MinHeight(520.0f);
    CUI::DSL::Borrow(dock).BackgroundToken(ThemeTokenId::WindowBackground);
    CUI::DSL::Borrow(dock).Background(ThemeManager::Instance().GetColor(ThemeTokenId::WindowBackground));

    DSL::Borrow(dock).SideSize(DockSide::Left, 220.0f);
    DSL::Borrow(dock).SideSize(DockSide::Right, 240.0f);
    DSL::Borrow(dock).SideSize(DockSide::Top, 130.0f);
    DSL::Borrow(dock).SideSize(DockSide::Bottom, 140.0f);

    auto tree = std::make_shared<TreeView>();
    auto solution = tree->AddItem("Solution 'CUI'", true);
    auto core = std::make_shared<TreeViewItem>();
    core->header = "CUI.Core";
    core->parent = solution.get();
    solution->children.push_back(core);
    auto neo = std::make_shared<TreeViewItem>();
    neo->header = "EverythingNEO";
    neo->parent = solution.get();
    solution->children.push_back(neo);
    tree->InvalidateVisibleItems();
    dock->AddToolPane("Solution Explorer", tree, DockSide::Left);
    dock->AddToolPane("Git", MakePaneBody("Git Changes\n M DockManager.cpp\n M DockFloatWindow.cpp"), DockSide::Left);

    dock->AddToolPane("Properties", MakePaneBody("Selected object\nDockManager\nLeft: 220\nRight: 240"), DockSide::Right);

    dock->AddToolPane("Toolbox", MakePaneBody("Common controls\n- Button\n- TextBox\n- ListView"), DockSide::Top);
    dock->AddToolPane("Output", MakePaneBody("Build started...\n1>------ Build started: Project: CUI.Core ------"), DockSide::Bottom);

    dock->AddDocument("main.cpp", MakePaneBody("// Document editor\nint main() {\n  return 0;\n}"));
    dock->AddDocument("DockManager.h", MakePaneBody("#pragma once\nclass DockManager;"));
    dock->AddDocument("README.md", MakePaneBody("# Docking\nDrag tab headers to float or redock."));

    auto floatBtn = std::make_shared<Button>("Float Properties");
    CUI::DSL::Borrow(floatBtn).Width(140.0f);
    CUI::DSL::Borrow(floatBtn).Height(28.0f);
    floatBtn->OnClick().Connect([dock](UIElement*) {
        const int idx = dock->FindPaneIndexByTitle("Properties");
        if (idx >= 0) {
            dock->FloatPane(idx);
        }
    });

    auto saveBtn = std::make_shared<Button>("Save Layout");
    CUI::DSL::Borrow(saveBtn).Width(110.0f);
    CUI::DSL::Borrow(saveBtn).Height(28.0f);
    saveBtn->OnClick().Connect([dock](UIElement*) {
        dock->SaveLayout(L"cui-dock-layout.json");
    });

    auto loadBtn = std::make_shared<Button>("Load Layout");
    CUI::DSL::Borrow(loadBtn).Width(110.0f);
    CUI::DSL::Borrow(loadBtn).Height(28.0f);
    loadBtn->OnClick().Connect([dock](UIElement*) {
        dock->LoadLayout(L"cui-dock-layout.json");
    });

    auto title = std::make_shared<TextBlock>();
    CUI::DSL::Borrow(title).Text("Visual Studio 式停靠布局");
    CUI::DSL::Borrow(title).FontSize(20.0f);
    CUI::DSL::Borrow(title).FontWeight(FontWeight::SemiBold);
    CUI::DSL::Borrow(title).ForegroundToken(ThemeTokenId::TextPrimary);

    auto subtitle = std::make_shared<TextBlock>();
    CUI::DSL::Borrow(subtitle).Text("拖标签停靠 · 撕出为 CUI 窗口 · 标题栏拖回引导区还原");
    CUI::DSL::Borrow(subtitle).FontSize(12.0f);
    CUI::DSL::Borrow(subtitle).ForegroundToken(ThemeTokenId::TextSecondary);

    auto toolbar = Row(8).Height(36).Children({ floatBtn, saveBtn, loadBtn }).Build();

    auto header = Column(8).Padding(16, 16, 16, 8).Children({
        title,
        subtitle,
        toolbar
    }).Build();

    auto page = Column(0).FlexGrow(1.0f).Children({
        header,
        dock
    }).Build();
    CUI::DSL::Borrow(page).BackgroundToken(ThemeTokenId::WindowBackground);
    CUI::DSL::Borrow(page).Background(ThemeManager::Instance().GetColor(ThemeTokenId::WindowBackground));
    CUI::DSL::Borrow(page).Align(Alignment::Stretch);

    return { "Docking 停靠布局", page };
}
