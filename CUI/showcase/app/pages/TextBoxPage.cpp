#include "PageRegistry.h"
#include "../ShowcaseHelpers.h"
#include "framework/core/CUIDsl.h"

using namespace CUI::DSL;

ShowcasePage BuildTextBoxPage(const ShowcaseContext& ctx) {
    auto target = TextField("在此输入多行文本...").Width(320).Height(120).Build();
    target->SetProperty("TextWrapping", CUI::Value("Wrap"));
    target->SetProperty("AcceptsReturn", CUI::Value(true));
    return { "TextBox 输入框", CreatePage(
        "TextBox 输入框控件全属性控制台",
        "支持光标颜色/宽度/闪烁周期自定义、占位词与高级排版。",
        CreateDemoSurface({ target }, 0.0f),
        CreatePropertyGrid(ctx, target)) };
}
