#include "PageRegistry.h"
#include "../ShowcaseHelpers.h"
#include "framework/core/CUIDsl.h"
#include "framework/controls/Toast.h"
#include "framework/style/ThemeManager.h"

using namespace CUI;
using namespace CUI::DSL;

ShowcasePage BuildComboBoxPage(const ShowcaseContext& ctx) {
    auto target = std::make_shared<ComboBox>();
    target->AddItem("VS Code Dark+");
    target->AddItem("WinUI 3 Light");
    target->AddItem("Monokai Pro");
    target->SetSelectedIndex(0);
    target->SetWidth(240.0f);

    auto input = TextField("自定义新主题").Width(280).Height(48).Build();
    input->SetColorToken(ThemeTokenId::TextPrimary);
    input->SetPlaceholderColorToken(ThemeTokenId::TextMuted);
    input->SetColor(ThemeManager::Instance().GetColor("textPrimary"));

    auto addBtn = ElevatedButton("添加选项到下拉菜单").Padding(12, 6, 12, 6).Build();
    addBtn->OnClick().Connect([window = ctx.windowRef, target, input](UIElement*) {
        std::string text = input->GetText();
        if (text.empty()) text = "自定义新主题";
        if (text.empty()) text = "未命名主题";
        target->AddItem(text);
        Toast::Show(window->GetRootElement().get(), "ComboBox", "已添加新选项: " + text, ToastCorner::BottomRight, 1800);
    });

    auto demo = Column(12).Children({
        CreateDemoSurface({ target }, 0.0f),
        CreateDemoSurface({
            CreateShowcaseText("动态添加选项 (走 ThemeManager)", 12.0f, "accentColor", true),
            input,
            addBtn
        }, 8.0f)
    }).Build();

    return { "ComboBox 下拉框", CreatePage(
        "ComboBox 下拉选择框全属性控制台",
        "ComboBox 下拉选择。",
        demo) };
}
