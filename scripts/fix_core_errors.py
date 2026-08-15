import os
import re

def update_file(path, transformer):
    with open(path, 'r', encoding='utf-8', errors='ignore') as f:
        content = f.read()
    new_content = transformer(content)
    if new_content != content:
        with open(path, 'w', encoding='utf-8') as f:
            f.write(new_content)
        print(f"Updated {path}")
        return True
    return False

# 1. Fix UIElement.h template parameter Id -> PropId
def fix_ui_element_h():
    path = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\UIElement.h'
    def trans(content):
        # In class declaration
        content = content.replace('template<typename T, PropertyId Id>\n    BindableProperty<T>* GetOrCreatePropertyBinding();', 'template<typename T, PropertyId PropId>\n    BindableProperty<T>* GetOrCreatePropertyBinding();')
        content = content.replace('template<typename T, PropertyId Id>\n    BindableProperty<T>* FindPropertyBinding() const;', 'template<typename T, PropertyId PropId>\n    BindableProperty<T>* FindPropertyBinding() const;')
        
        # In definitions at bottom
        content = re.sub(
            r'template<typename T, PropertyId Id>\s*BindableProperty<T>\*\s*UIElement::GetOrCreatePropertyBinding\(\)\s*\{[\s\S]*?return result;\s*\}',
            '''template<typename T, PropertyId PropId>
BindableProperty<T>* UIElement::GetOrCreatePropertyBinding() {
    if (!m_propertyBindings) {
        m_propertyBindings = std::make_unique<std::unordered_map<PropertyId, std::unique_ptr<PropertyBindingSlotBase>>>();
    }
    auto existing = m_propertyBindings->find(PropId);
    if (existing != m_propertyBindings->end()) {
        auto* slot = dynamic_cast<PropertyBindingSlot<T>*>(existing->second.get());
        return slot ? &slot->value : nullptr;
    }

    auto slot = std::make_unique<PropertyBindingSlot<T>>(
        *this,
        PropId,
        [this] { return PropertyValueTraits<T>::FromValue(GetProperty(PropId)); },
        [this](const T& value) { SetProperty(PropId, PropertyValueTraits<T>::ToValue(value)); });
    auto* result = &slot->value;
    m_propertyBindings->emplace(PropId, std::move(slot));
    return result;
}''',
            content
        )
        content = re.sub(
            r'template<typename T, PropertyId Id>\s*BindableProperty<T>\*\s*UIElement::FindPropertyBinding\(\)\s*const\s*\{[\s\S]*?return slot \? &slot->value : nullptr;\s*\}',
            '''template<typename T, PropertyId PropId>
BindableProperty<T>* UIElement::FindPropertyBinding() const {
    if (!m_propertyBindings) return nullptr;
    const auto existing = m_propertyBindings->find(PropId);
    if (existing == m_propertyBindings->end()) return nullptr;
    auto* slot = dynamic_cast<PropertyBindingSlot<T>*>(existing->second.get());
    return slot ? &slot->value : nullptr;
}''',
            content
        )
        return content
    update_file(path, trans)

# 2. Fix UIElement.cpp
def fix_ui_element_cpp():
    path = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\UIElement.cpp'
    def trans(content):
        content = content.replace('ActualWidth.Initialize([this]() { return GetActualWidth(); });', 'ActualWidth.Initialize([this]() { return GetBounds().width; });')
        content = content.replace('ActualHeight.Initialize([this]() { return GetActualHeight(); });', 'ActualHeight.Initialize([this]() { return GetBounds().height; });')
        content = content.replace('m_onClickEvent.Invoke(this);', 'OnClick.Invoke(this);')
        return content
    update_file(path, trans)

# 3. Fix Toast.cpp
def fix_toast_cpp():
    path = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\Toast.cpp'
    def trans(content):
        content = content.replace('m_onClickEvent.Invoke(this);', 'OnClick.Invoke(this);')
        return content
    update_file(path, trans)

fix_ui_element_h()
fix_ui_element_cpp()
fix_toast_cpp()
print("Fixed core errors.")
