import os

# 1. MessageBox.h
path_mb = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\MessageBox.h'
with open(path_mb, 'r', encoding='utf-8') as f:
    mb = f.read()

# Add getters before the properties
getters = '''    const std::string& GetPrimaryButtonText() const { return m_primaryText; }
    const std::string& GetSecondaryButtonText() const { return m_secondaryText; }
    const std::string& GetCloseButtonText() const { return m_closeText; }
    bool GetInputEnabled() const { return m_inputEnabled; }

'''
if 'const std::string& GetPrimaryButtonText() const' not in mb:
    mb = mb.replace('    void SetTitle(const std::string& title);', getters + '    void SetTitle(const std::string& title);')
    with open(path_mb, 'w', encoding='utf-8') as f:
        f.write(mb)

# 2. PasswordBox.h
path_pb = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\PasswordBox.h'
with open(path_pb, 'r', encoding='utf-8') as f:
    pb = f.read()

if 'struct PasswordBoxPasswordProperty' not in pb:
    pb_props = '''    struct PasswordBoxPasswordProperty {
        PasswordBox* owner = nullptr;
        PasswordBoxPasswordProperty() = default;
        explicit PasswordBoxPasswordProperty(PasswordBox* o) : owner(o) {}
        PasswordBoxPasswordProperty& operator=(const std::string& p) { if (owner) owner->SetPassword(p); return *this; }
        operator std::string() const { return owner ? owner->GetPassword() : ""; }
        std::string Get() const { return owner ? owner->GetPassword() : ""; }
    } Password;

'''
    pb = pb.replace('    std::string GetPassword() const { return GetText(); }', pb_props + '    std::string GetPassword() const { return GetText(); }')
    with open(path_pb, 'w', encoding='utf-8') as f:
        f.write(pb)

# 3. Flyout.h
path_fl = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\Flyout.h'
with open(path_fl, 'r', encoding='utf-8') as f:
    fl = f.read()

if 'struct FlyoutPlacementProperty' not in fl:
    fl_props = '''    struct FlyoutPlacementProperty {
        Flyout* owner = nullptr;
        FlyoutPlacementProperty() = default;
        explicit FlyoutPlacementProperty(Flyout* o) : owner(o) {}
        FlyoutPlacementProperty& operator=(FlyoutPlacement p) { if (owner) owner->SetPlacement(p); return *this; }
        operator FlyoutPlacement() const { return owner ? owner->GetPlacement() : FlyoutPlacement::Bottom; }
        FlyoutPlacement Get() const { return owner ? owner->GetPlacement() : FlyoutPlacement::Bottom; }
    } Placement;

    struct FlyoutContentProperty {
        Flyout* owner = nullptr;
        FlyoutContentProperty() = default;
        explicit FlyoutContentProperty(Flyout* o) : owner(o) {}
        FlyoutContentProperty& operator=(std::shared_ptr<UIElement> c) { if (owner) owner->SetContent(std::move(c)); return *this; }
        operator std::shared_ptr<UIElement>() const { return owner ? owner->GetContent() : nullptr; }
        std::shared_ptr<UIElement> Get() const { return owner ? owner->GetContent() : nullptr; }
        std::shared_ptr<UIElement> operator->() const { return owner ? owner->GetContent() : nullptr; }
    } Content;

'''
    fl = fl.replace('    void SetContent(std::shared_ptr<UIElement> content);', fl_props + '    void SetContent(std::shared_ptr<UIElement> content);')
    with open(path_fl, 'w', encoding='utf-8') as f:
        f.write(fl)

print("Updated MessageBox.h, PasswordBox.h, Flyout.h")
