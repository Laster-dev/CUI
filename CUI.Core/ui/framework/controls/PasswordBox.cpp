#include "PasswordBox.h"

namespace CUI {

PasswordBox::PasswordBox() : TextBox("请输入密码"), Password(this) {
    SetIsPasswordMode(true);
    SetShowRevealButton(true);
}

PasswordBox::PasswordBox(const std::string& placeholder) : TextBox(placeholder), Password(this) {
    SetIsPasswordMode(true);
    SetShowRevealButton(true);
}

Value PasswordBox::GetProperty(PropertyId id) const {
    switch (id) {
    case PropertyId::IsPasswordRevealed: return Value(GetIsPasswordRevealed());
    case PropertyId::ShowRevealButton: return Value(GetShowRevealButton());
    default: return TextBox::GetProperty(id);
    }
}

bool PasswordBox::HasProperty(PropertyId id) const {
    return id == PropertyId::IsPasswordRevealed || id == PropertyId::ShowRevealButton
        || TextBox::HasProperty(id);
}

void PasswordBox::SetProperty(PropertyId id, const Value& val) {
    switch (id) {
    case PropertyId::IsPasswordRevealed: SetIsPasswordRevealed(val.AsBool()); return;
    case PropertyId::ShowRevealButton: SetShowRevealButton(val.AsBool()); return;
    default: TextBox::SetProperty(id, val); return;
    }
}

} // namespace CUI
