#pragma once
#include "Control.h"
#include <vector>
#include <string>

namespace CUI {

class BreadcrumbBar : public Control {
public:
    BreadcrumbBar();
    virtual ~BreadcrumbBar() = default;

    virtual const char* GetClassName() const override { return "BreadcrumbBar"; }
    virtual std::vector<PropertyMeta> GetPropertyMetas() const override;

    virtual Size Measure(Size availableSize) override;
    virtual void OnRender(GraphicsContext& ctx) override;
    virtual void OnMouseDown(Point pt) override;

    void SetPath(const std::vector<std::string>& pathNodes);
    void PushNode(const std::string& node);
    void PopNode();
    const std::vector<std::string>& GetPath() const { return m_pathNodes; }

    Event<BreadcrumbBar*, int, const std::string&>& OnItemClicked() { return m_onItemClickedEvent; }

private:
    std::vector<std::string> m_pathNodes;
    Event<BreadcrumbBar*, int, const std::string&> m_onItemClickedEvent;
};

} // namespace CUI
