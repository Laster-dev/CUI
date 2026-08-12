#pragma once
#include "Control.h"
#include <string>
#include <vector>

namespace CUI {

class FilePicker : public Control {
public:
    FilePicker();
    virtual ~FilePicker() = default;

    virtual const char* GetClassName() const override { return "FilePicker"; }
    virtual std::vector<PropertyMeta> GetPropertyMetas() const override;
    virtual HCURSOR GetCursor() const override;
    bool AcceptsTabFocus() const override { return true; }

    virtual Size Measure(Size availableSize) override;
    virtual void OnRender(GraphicsContext& ctx) override;
    virtual void OnMouseDown(Point pt) override;
    virtual void OnMouseUp(Point pt) override;
    virtual void OnMouseMove(Point pt) override;
    virtual void OnMouseLeave() override;
    virtual void OnKeyDown(int vkCode) override;

    const std::string& GetPath() const { return GetText(); }
    void SetPath(const std::string& path);

    const std::string& GetDialogTitle() const { return m_dialogTitle; }
    void SetDialogTitle(const std::string& title) { m_dialogTitle = title; }

    void SetFilter(const std::string& name, const std::string& spec);
    void AddFilter(const std::string& name, const std::string& spec);
    void ClearFilters();

    Event<FilePicker*, const std::string&>& OnPathChanged() { return m_onPathChangedEvent; }

private:
    enum class HitPart : uint8_t { None, Path, Browse };

    Rect PathRect() const;
    Rect BrowseRect() const;
    HitPart HitTestPart(Point pt) const;
    void OpenDialog();
    void MarkPickerDirty();

    std::string m_dialogTitle{ "选择文件" };
    std::vector<std::pair<std::string, std::string>> m_filters;
    HitPart m_hover = HitPart::None;
    HitPart m_pressed = HitPart::None;
    Event<FilePicker*, const std::string&> m_onPathChangedEvent;
};

} // namespace CUI
