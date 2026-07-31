#include "PageRegistry.h"
#include "../ShowcaseHelpers.h"
#include "framework/core/CUIDsl.h"
#include "framework/controls/Toast.h"

using namespace CUI;
using namespace CUI::DSL;

ShowcasePage BuildComboBoxPage(const ShowcaseContext& ctx) {
    auto target = std::make_shared<ComboBox>();
    target->AddItem("VS Code Dark+");
    target->AddItem("WinUI 3 Light");
    target->AddItem("Monokai Pro");
    target->SetSelectedIndex(0);
    target->SetProperty("width", Value(240.0f));

    auto input = TextField("自定义新主题").Width(280).Height(26).Build();
    auto addBtn = ElevatedButton("添加选项到下拉菜单").Background("#007ACC").Padding(12, 6, 12, 6).Build();
    addBtn->OnClick().Connect([window = ctx.windowRef, target, input](UIElement*) {
        std::string text = input->GetProperty("text").AsString("自定义新主题");
        if (text.empty()) text = "未命名主题";
        target->AddItem(text);
        Toast::Show(window->GetRootElement().get(), "ComboBox", "已添加新选项: " + text, ToastCorner::BottomRight, 1800);
    });

    return { "ComboBox 下拉框", CreatePage(
        "ComboBox 下拉选择框全属性控制台",
        "支持弹出式下拉菜单与动态在线添加新选项。",
        CreateDemoSurface({ target }, 0.0f),
        CreateRightScrollPanel({
            CreateShowcaseText("下拉框属性控制表 (ComboBox)", 12.0f, "#569CD6", true),
            CheckboxTile("是否启用 (IsEnabled)").Build(),
            CreateShowcaseText("添加新选项 (AddItem):", 11.0f, "#AAAAAA"),
            input,
            addBtn,
            CreateShowcaseText("显式宽度 (Width):", 11.0f, "#AAAAAA"),
            TextField("240").Width(280).Height(26).Build()
        })) };
}
