import os

# 1. Flyout.h
path_fl = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\Flyout.h'
with open(path_fl, 'r', encoding='utf-8') as f:
    fl = f.read()

props = '''    struct FlyoutPlacementProperty {
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
target = '    void SetContent(std::shared_ptr<UIElement> content); // 设置弹出框中要呈现的元素'
fl = fl.replace(target, props + target)
with open(path_fl, 'w', encoding='utf-8') as f:
    f.write(fl)

# 2. Flyout.cpp
path_fl_cpp = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\Flyout.cpp'
with open(path_fl_cpp, 'r', encoding='utf-8') as f:
    fl_cpp = f.read()

fl_cpp = fl_cpp.replace('Flyout::Flyout() {\n    m_presenter = std::make_shared<FlyoutPresenter>();\n}',
                        'Flyout::Flyout() : Placement(this), Content(this) {\n    m_presenter = std::make_shared<FlyoutPresenter>();\n}')
fl_cpp = fl_cpp.replace('Flyout::Flyout(std::shared_ptr<UIElement> content) {\n    m_presenter = std::make_shared<FlyoutPresenter>();\n    m_presenter->SetContent(content);\n}',
                        'Flyout::Flyout(std::shared_ptr<UIElement> content) : Placement(this), Content(this) {\n    m_presenter = std::make_shared<FlyoutPresenter>();\n    m_presenter->SetContent(content);\n}')
with open(path_fl_cpp, 'w', encoding='utf-8') as f:
    f.write(fl_cpp)

print("Updated Flyout.h and Flyout.cpp cleanly.")
