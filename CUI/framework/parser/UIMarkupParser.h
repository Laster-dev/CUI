#pragma once
#include "../controls/UIElement.h"
#include "../core/Binding.h"
#include <string>
#include <memory>
#include <vector>
#include <functional>
#include <unordered_map>

namespace CUI {

struct DeferredBinding {
    std::shared_ptr<UIElement> element;
    std::string propertyName;
    std::string bindingPath;
    BindingMode mode;
};

class UIMarkupParser {
public:
    UIMarkupParser();
    ~UIMarkupParser() = default;

    std::shared_ptr<UIElement> ParseXmlString(const std::string& xmlText);
    std::shared_ptr<UIElement> ParseXmlFile(const std::string& filePath);

    void RegisterElementFactory(const std::string& tagName, std::function<std::shared_ptr<UIElement>()> factory);

    std::shared_ptr<UIElement> CreateElement(const std::string& tagName);

    const std::vector<DeferredBinding>& GetDeferredBindings() const { return m_deferredBindings; }
    void ApplyBindings(std::shared_ptr<Object> dataContext);

private:
    std::unordered_map<std::string, std::function<std::shared_ptr<UIElement>()>> m_factories;
    std::vector<DeferredBinding> m_deferredBindings;
};

} // namespace CUI
