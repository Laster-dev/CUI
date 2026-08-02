#pragma once
#include "TextBox.h"

namespace CUI {

class PasswordBox : public TextBox {
public:
    PasswordBox();
    explicit PasswordBox(const std::string& placeholder);
    virtual ~PasswordBox() = default;

    virtual const char* GetClassName() const override { return "PasswordBox"; }
    virtual std::vector<PropertyMeta> GetPropertyMetas() const override;

    std::string GetPassword() const { return GetText(); }
    void SetPassword(const std::string& pwd) { SetText(pwd); }
};

} // namespace CUI
