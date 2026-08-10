#pragma once
#include "ScrollViewer.h"
#include "Panel.h"
#include "TextBlock.h"
#include "TextBox.h"
#include "CheckBox.h"
#include "ComboBox.h"
#include <map>

namespace CUI {

class PropertyGrid : public ScrollViewer {
public:
    PropertyGrid();
    virtual ~PropertyGrid() = default;

    virtual const char* GetClassName() const override { return "PropertyGrid"; }

    void SetTargetElement(std::shared_ptr<UIElement> target, void* windowHost = nullptr);
    std::shared_ptr<UIElement> GetTargetElement() const { return m_target.lock(); }

    static const std::vector<PropertyMeta>& GetRegisteredMetasForClass(const std::string& className);

private:
    void RebuildUI();
    void OnTargetPropertyChanged(const std::string& propName, const Value& val);

    std::weak_ptr<UIElement> m_target;
    void* m_windowHost = nullptr;
    std::shared_ptr<StackPanel> m_container;
    std::map<PropertyId, std::shared_ptr<TextBox>> m_inputControls;
    std::map<PropertyId, std::shared_ptr<CheckBox>> m_checkControls;
    std::map<PropertyId, std::shared_ptr<ComboBox>> m_comboControls;
    bool m_updatingFromTarget = false;
};

} // namespace CUI
