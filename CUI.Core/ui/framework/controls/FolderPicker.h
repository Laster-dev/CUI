#pragma once
#include "Control.h"
#include <string>

namespace CUI {

class FolderPicker : public Control {
public:
    FolderPicker();
    virtual ~FolderPicker() = default;

    virtual const char* GetClassName() const override { return "FolderPicker"; }
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

    Event<FolderPicker*, const std::string&>& OnPathChanged() { return m_onPathChangedEvent; }

private:
    enum class HitPart : uint8_t { None, Path, Browse };

    Rect PathRect() const;
    Rect BrowseRect() const;
    HitPart HitTestPart(Point pt) const;
    void OpenDialog();
    void MarkPickerDirty();

    std::string m_dialogTitle{ "选择文件夹" };
    HitPart m_hover = HitPart::None;
    HitPart m_pressed = HitPart::None;
    Event<FolderPicker*, const std::string&> m_onPathChangedEvent;
};

} // namespace CUI
