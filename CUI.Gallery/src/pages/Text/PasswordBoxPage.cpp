#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"

#include "framework/core/CUIDsl.h"
#include "framework/core/State.h"
#include "framework/controls/PasswordBox.h"

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

std::shared_ptr<UIElement> BuildPasswordBoxPage() {
    auto basic = PasswordBoxWidget("请输入您的安全密码").Width(280).Height(28).Build();
    basic->SetToolTip("点击右侧眼睛图标可切换明文 / 密文");

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

    auto noReveal = PasswordBoxWidget("隐藏明文切换按钮").Width(280).Height(28).Build();
    noReveal->SetShowRevealButton(false);

    auto prefilled = Make<PasswordBox>();
    prefilled->SetPlaceholder("密码框");
    prefilled->SetWidth(280.0f);
    prefilled->SetHeight(28.0f);
    prefilled->SetPassword("P@ssw0rd!123");

    auto disabled = PasswordBoxWidget("不可用").Width(280).Height(28).Build();
    disabled->SetIsEnabled(false);

    SamplePageSpec spec;
    spec.title = "PasswordBox(密码框)";
    spec.subtitle = "用于隐藏输入字符的密码文本框，输入内容自动以 • 掩码显示。";
    spec.sections = {
        {
            "常规用法",
            "输入自动掩码；点击右侧眼睛图标可临时查看明文。",
            Column(12).Children({
                basic,
                status,
            }).Build(),
        },
        {
            "配置项",
            "SetShowRevealButton(false) 隐藏明文切换按钮；SetPassword 可预置初始密码；支持禁用态。",
            Column(12).Children({
                noReveal,
                prefilled,
                disabled,
            }).Build(),
        },
    };
    spec.source =
        "auto pwd = PasswordBoxWidget(\"请输入密码\").Build();\n"
        "pwd->SetShowRevealButton(true);\n"
        "State<std::string> password{ \"\" };\n"
        "pwd->Text.Bind(password, BindingMode::TwoWay);\n"
        "std::string value = pwd->GetPassword();\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
