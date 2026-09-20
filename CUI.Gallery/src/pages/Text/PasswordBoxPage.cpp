#ifndef CUI_NO_DSL_SHORTCUTS
#define CUI_NO_DSL_SHORTCUTS   // 关闭「控件名即工厂」宏层，避免与 Widgets:: 句柄同名冲突
#endif
#include "Gallery.h"
#include "framework/core/Widgets.h"
using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

std::shared_ptr<UIElement> BuildPasswordBoxPage() {
    CUI::Widgets::Ref basic = Widgets::PasswordBox("请输入您的安全密码").Width(280).Height(28).Shared();
    basic->ToolTip = "点击右侧眼睛图标可切换明文 / 密文";

    State<std::string> passwordState{ "" };
    basic->Text.Bind(passwordState, BindingMode::TwoWay);
    auto statusValue = MakeComputed<std::string>([](const std::string& text) {
        if (text.empty()) {
            return std::string("密码为空。");
        }
        return "已输入 " + std::to_string(text.size()) + " 个字符（显示为 • 掩码）。";
    }, passwordState);
    auto status = MakeStatus("");
    status->Text.Bind(statusValue, BindingMode::OneWay);

    CUI::Widgets::Ref noReveal = Widgets::PasswordBox("隐藏明文切换按钮").Width(280).Height(28).Shared();
        noReveal.ShowRevealButton(false);

    CUI::Widgets::Ref prefilled = Widgets::PasswordBox().Shared();
    prefilled->Placeholder = "密码框";
        prefilled.Width(280.0f);
        prefilled.Height(28.0f);
        prefilled.Password("P@ssw0rd!123");

    CUI::Widgets::Ref disabled = Widgets::PasswordBox("不可用").Width(280).Height(28).Shared();
        disabled.IsEnabled(false);

    SamplePageSpec spec;
    spec.title = "PasswordBox(密码框)";
    spec.subtitle = "用于隐藏输入字符的密码文本框，输入内容自动以 • 掩码显示。";
    spec.sections = {
        {
            "常规用法",
            "输入自动掩码；点击右侧眼睛图标可临时查看明文。",
            Column(12, {
                basic,
                status,
            }),
        },
        {
            "配置项",
            "ApplyShowRevealButton(false) 隐藏明文切换按钮；SetPassword 可预置初始密码；支持禁用态。",
            Column(12, {
                noReveal,
                prefilled,
                disabled,
            }),
        },
    };
    spec.source =
        "auto pwd = Widgets::PasswordBox(\"请输入密码\").Shared();\n"
        "pwd.ShowRevealButton(true);\n"
        "State<std::string> password{ \"\" };\n"
        "pwd->Text.Bind(password, BindingMode::TwoWay);\n"
        "std::string value = pwd->GetPassword();\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery


