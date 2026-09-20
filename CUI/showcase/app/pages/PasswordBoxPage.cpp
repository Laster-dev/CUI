#ifndef CUI_NO_DSL_SHORTCUTS
#define CUI_NO_DSL_SHORTCUTS   // 关闭「控件名即工厂」宏层，避免与 Widgets:: 句柄同名冲突
#endif
#include "PageRegistry.h"
#include "framework/core/Widgets.h"
#include "../ShowcaseHelpers.h"
#include "framework/core/CUIDsl.h"

using namespace CUI;
using namespace CUI::DSL;

ShowcasePage BuildPasswordBoxPage(const ShowcaseContext& ctx) {
    CUI::Widgets::Ref pwdBox = Widgets::PasswordBox("请输入您的安全密码").Shared();
        pwdBox.ToolTip("悬停提示: 点击右侧眼睛图标可查看明文");

    return { "PasswordBox 密码框", CreatePage(
        "PasswordBox 密码输入框控件",
        "自动对输入内容进行 • 掩码隐藏，内置眼睛按钮切换明文/密文，原生支持 ToolTip。",
        CreateDemoSurface({ pwdBox }, 16.0f)) };
}
