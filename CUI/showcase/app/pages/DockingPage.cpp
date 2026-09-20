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
    CUI::Widgets::Ref body = CUI::Widgets::TextBlock().Shared();
        body.Text(text);
        body.FontSize(13.0f);
    
        body.Padding(Thickness(12.0f));
        body.BackgroundToken(ThemeTokenId::WindowBackground);
        body.Background(ThemeManager::Instance().GetColor(ThemeTokenId::WindowBackground));
    return body;
}

} // namespace

ShowcasePage BuildDockingPage(const ShowcaseContext& ctx) {
    CUI::Widgets::Ref dock = CUI::Widgets::DockManager().Shared();
        dock.OwnerWindow(ctx.windowRef);
        dock.FlexGrow(1.0f);
        dock.Align(Alignment::Stretch);
        dock.MinHeight(520.0f);
        dock.BackgroundToken(ThemeTokenId::WindowBackground);
        dock.Background(ThemeManager::Instance().GetColor(ThemeTokenId::WindowBackground));

        dock.SideSize(DockSide::Left, 220.0f);
        dock.SideSize(DockSide::Right, 240.0f);
        dock.SideSize(DockSide::Top, 130.0f);
        dock.SideSize(DockSide::Bottom, 140.0f);

    CUI::Widgets::Ref tree = CUI::Widgets::TreeView().Shared();
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

    CUI::Widgets::Ref floatBtn = CUI::Widgets::Button("Float Properties").Shared();
        floatBtn.Width(140.0f);
        floatBtn.Height(28.0f);
    floatBtn->OnClick().Connect([dock](UIElement*) {
        const int idx = dock->FindPaneIndexByTitle("Properties");
        if (idx >= 0) {
            dock->FloatPane(idx);
        }
    });

    CUI::Widgets::Ref saveBtn = CUI::Widgets::Button("Save Layout").Shared();
        saveBtn.Width(110.0f);
        saveBtn.Height(28.0f);
    saveBtn->OnClick().Connect([dock](UIElement*) {
        dock->SaveLayout(L"cui-dock-layout.json");
    });

    CUI::Widgets::Ref loadBtn = CUI::Widgets::Button("Load Layout").Shared();
        loadBtn.Width(110.0f);
        loadBtn.Height(28.0f);
    loadBtn->OnClick().Connect([dock](UIElement*) {
        dock->LoadLayout(L"cui-dock-layout.json");
    });

    CUI::Widgets::Ref title = CUI::Widgets::TextBlock().Shared();
        title.Text("Visual Studio 式停靠布局");
        title.FontSize(20.0f);
        title.FontWeight(FontWeight::SemiBold);
    

    CUI::Widgets::Ref subtitle = CUI::Widgets::TextBlock().Shared();
        subtitle.Text("拖标签停靠 · 撕出为 CUI 窗口 · 标题栏拖回引导区还原");
        subtitle.FontSize(12.0f);
    

    auto toolbar = Row(8).Height(36).Children({ floatBtn, saveBtn, loadBtn }).Build();

    auto header = Column(8).Padding(16, 16, 16, 8).Children({
        title,
        subtitle,
        toolbar
    }).Build();

    CUI::Widgets::Ref page =Column(0).FlexGrow(1.0f).Children({
        header,
        dock
    }).Build();
        page.BackgroundToken(ThemeTokenId::WindowBackground);
        page.Background(ThemeManager::Instance().GetColor(ThemeTokenId::WindowBackground));
        page.Align(Alignment::Stretch);

    return { "Docking 停靠布局", page };
}
