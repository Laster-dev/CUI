#pragma once
#include "TextBox.h"

namespace CUI {

class PasswordBox : public TextBox {
public:
    PasswordBox();
    explicit PasswordBox(const std::string& placeholder);
    virtual ~PasswordBox() = default;

    virtual const char* GetClassName() const override { return "PasswordBox"; }
    virtual Value GetProperty(PropertyId id) const override;
    virtual bool HasProperty(PropertyId id) const override;
    void SetProperty(PropertyId id, const Value& val) override;

    /**
     * @brief 密码输入框真实明文密码属性代理。
     */
    struct PasswordBoxPasswordProperty {
        PasswordBox* owner = nullptr;
        PasswordBoxPasswordProperty() = default;
        explicit PasswordBoxPasswordProperty(PasswordBox* o) : owner(o) {}
        PasswordBoxPasswordProperty& operator=(const std::string& p) { if (owner) owner->SetPassword(p); return *this; }
        operator std::string() const { return owner ? owner->GetPassword() : ""; }
        std::string Get() const { return owner ? owner->GetPassword() : ""; }
    } Password;

    std::string GetPassword() const { return GetText(); }
    void SetPassword(const std::string& pwd) { SetText(pwd); }
};

} // namespace CUI
