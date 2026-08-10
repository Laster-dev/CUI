#include "PasswordBox.h"

namespace CUI {

PasswordBox::PasswordBox() : TextBox("请输入密码") {
    SetIsPasswordMode(true);
    SetShowRevealButton(true);
}

PasswordBox::PasswordBox(const std::string& placeholder) : TextBox(placeholder) {
    SetIsPasswordMode(true);
    SetShowRevealButton(true);
}

std::vector<PropertyMeta> PasswordBox::GetPropertyMetas() const {
    auto metas = TextBox::GetPropertyMetas();
    metas.push_back({ "isPasswordRevealed", "显示明文 (IsRevealed)", "密码配置", "bool" });
    metas.push_back({ "showRevealButton", "显示眼睛按钮 (ShowRevealBtn)", "密码配置", "bool" });
    return metas;
}

} // namespace CUI
