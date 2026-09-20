#include "PasswordBox.h"
#include "../core/CUIDsl.h"

namespace CUI {

PasswordBox::PasswordBox() : TextBox("请输入密码"), Password(this) {
    this->ApplyIsPasswordMode(true);
    this->ApplyShowRevealButton(true);
}

PasswordBox::PasswordBox(const std::string& placeholder) : TextBox(placeholder), Password(this) {
    this->ApplyIsPasswordMode(true);
    this->ApplyShowRevealButton(true);
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

void PasswordBox::ApplyProperty(PropertyId id, const Value& val) {
    switch (id) {
    case PropertyId::IsPasswordRevealed: ApplyIsPasswordRevealed(val.AsBool()); return;
    case PropertyId::ShowRevealButton: ApplyShowRevealButton(val.AsBool()); return;
    default: TextBox::ApplyProperty(id, val); return;
    }
}

} // namespace CUI
