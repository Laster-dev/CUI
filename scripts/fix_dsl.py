import os

path = r'E:\C++project\CUI\CUI.Core\ui\framework\core\CUIDsl.h'
with open(path, 'r', encoding='utf-8') as f:
    content = f.read()

# remove duplicate constructor
content = content.replace(
'''    ElementBuilder() : ElementRef<T>(std::make_shared<T>()) {}
    explicit ElementBuilder(std::shared_ptr<T> elem) : ElementRef<T>(elem) {}
    template<typename U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>> ElementBuilder(const ElementRef<U>& ref) : ElementRef<T>(ref.Shared()) {} // 默认初始化建造者
    explicit ElementBuilder(std::shared_ptr<T> elem) : m_ptr(elem) {} // 用已有的控件指针初始化''',
'''    ElementBuilder() : ElementRef<T>(std::make_shared<T>()) {}
    explicit ElementBuilder(std::shared_ptr<T> elem) : ElementRef<T>(std::move(elem)) {}
    template<typename U, typename = std::enable_if_t<!std::is_same_v<U, T> && std::is_convertible_v<U*, T*>>>
    ElementBuilder(const ElementRef<U>& ref) : ElementRef<T>(ref.Shared()) {}'''
)

# remove protected std::shared_ptr<T> m_ptr;
content = content.replace(
'''protected:
    std::shared_ptr<T> m_ptr; // 底层生成的控件共享引用实例对象自身
};''',
'''};'''
)

# update ChildArgument
content = content.replace(
'''struct ChildArgument {
    Element element;
    ChildArgument(Element value) : element(std::move(value)) {}
    template<typename T> ChildArgument(std::shared_ptr<T> value) : element(std::move(value)) {}
    template<typename T> ChildArgument(const ElementBuilder<T>& builder) : element(builder.Build()) {}
};''',
'''struct ChildArgument {
    Element element;
    ChildArgument() = default;
    ChildArgument(Element value) : element(std::move(value)) {}
    template<typename T> ChildArgument(std::shared_ptr<T> value) : element(std::move(value)) {}
    template<typename T> ChildArgument(const ElementRef<T>& ref) : element(ref.Shared()) {}
    template<typename T> ChildArgument(const ElementBuilder<T>& builder) : element(builder.Shared()) {}
};'''
)

with open(path, 'w', encoding='utf-8') as f:
    f.write(content)

print("Cleaned up CUIDsl.h")
