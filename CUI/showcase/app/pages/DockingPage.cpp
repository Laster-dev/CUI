#include "PageRegistry.h"
#include "../ShowcaseHelpers.h"
#include "framework/core/CUIDsl.h"
#include "framework/controls/docking/Docking.h"
#include "framework/controls/Button.h"
#include "framework/controls/TreeView.h"
#include "framework/controls/PropertyGrid.h"
#include "framework/controls/TextBlock.h"
#include "framework/style/ThemeManager.h"
#include "framework/style/ThemeTokenId.h"

using namespace CUI;
using namespace CUI::DSL;

namespace {

std::shared_ptr<UIElement> MakePaneBody(const std::string& text) {
    auto body = std::make_shared<TextBlock>();
    body->SetText(text);
    body->SetFontSize(13.0f);
    body->SetColorToken(ThemeTokenId::TextPrimary);
    body->SetPadding(Thickness(12.0f));
    body->SetBackgroundToken(ThemeTokenId::WindowBackground);
    body->SetBackground(ThemeManager::Instance().GetColor(ThemeTokenId::WindowBackground));
    return body;
}

} // namespace

ShowcasePage BuildDockingPage(const ShowcaseContext& ctx) {
    auto dock = std::make_shared<DockManager>();
    dock->SetOwnerWindow(ctx.windowRef);
    dock->SetFlexGrow(1.0f);
    dock->SetAlign(Alignment::Stretch);
    dock->SetMinHeight(520.0f);
    dock->SetBackgroundToken(ThemeTokenId::WindowBackground);
    dock->SetBackground(ThemeManager::Instance().GetColor(ThemeTokenId::WindowBackground));

    dock->SetSideSize(DockSide::Left, 220.0f);
    dock->SetSideSize(DockSide::Right, 240.0f);
    dock->SetSideSize(DockSide::Top, 130.0f);
    dock->SetSideSize(DockSide::Bottom, 140.0f);

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

    auto props = std::make_shared<PropertyGrid>();
    dock->AddToolPane("Properties", props, DockSide::Right);

    dock->AddToolPane("Toolbox", MakePaneBody("Common controls\n- Button\n- TextBox\n- ListView"), DockSide::Top);
    dock->AddToolPane("Output", MakePaneBody("Build started...\n1>------ Build started: Project: CUI.Core ------"), DockSide::Bottom);

    dock->AddDocument("main.cpp", MakePaneBody("// Document editor\nint main() {\n  return 0;\n}"));
    dock->AddDocument("DockManager.h", MakePaneBody("#pragma once\nclass DockManager;"));
    dock->AddDocument("README.md", MakePaneBody("# Docking\nDrag tab headers to float or redock."));

    auto floatBtn = std::make_shared<Button>("Float Properties");
    floatBtn->SetWidth(140.0f);
    floatBtn->SetHeight(28.0f);
    floatBtn->OnClick().Connect([dock](UIElement*) {
        const int idx = dock->FindPaneIndexByTitle("Properties");
        if (idx >= 0) {
            dock->FloatPane(idx);
        }
    });

    auto saveBtn = std::make_shared<Button>("Save Layout");
    saveBtn->SetWidth(110.0f);
    saveBtn->SetHeight(28.0f);
    saveBtn->OnClick().Connect([dock](UIElement*) {
        dock->SaveLayout(L"cui-dock-layout.json");
    });

    auto loadBtn = std::make_shared<Button>("Load Layout");
    loadBtn->SetWidth(110.0f);
    loadBtn->SetHeight(28.0f);
    loadBtn->OnClick().Connect([dock](UIElement*) {
        dock->LoadLayout(L"cui-dock-layout.json");
    });

    auto title = std::make_shared<TextBlock>();
    title->SetText("Visual Studio 式停靠布局");
    title->SetFontSize(20.0f);
    title->SetFontWeight("SemiBold");
    title->SetColorToken(ThemeTokenId::TextPrimary);

    auto subtitle = std::make_shared<TextBlock>();
    subtitle->SetText("单一自绘宿主 · 拖标签出引导 · 拖出窗口撕成 HWND · AH 自动隐藏");
    subtitle->SetFontSize(12.0f);
    subtitle->SetColorToken(ThemeTokenId::TextSecondary);

    auto toolbar = Row(8).Height(36).Children({ floatBtn, saveBtn, loadBtn }).Build();

    auto page = Column(10).Padding(16).FlexGrow(1.0f).Children({
        title,
        subtitle,
        toolbar,
        dock
    }).Build();
    page->SetBackgroundToken(ThemeTokenId::WindowBackground);
    page->SetBackground(ThemeManager::Instance().GetColor(ThemeTokenId::WindowBackground));
    page->SetAlign(Alignment::Stretch);

    return { "Docking 停靠布局", page };
}
