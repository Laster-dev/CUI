#pragma once
#include "Control.h"
#include <string>

namespace CUI {

enum class ImageType {
    Avatar,
    FileIcon,
    StatusBadge,
    Custom
};

class Image : public Control {
public:
    Image();
    Image(ImageType type, const std::string& text = "", D2D1_COLOR_F color = D2D1::ColorF(0x00 / 255.0f, 0x7A / 255.0f, 0xCC / 255.0f, 1.0f));
    virtual ~Image() = default;

    virtual const char* GetClassName() const override { return "Image"; }

    virtual Size Measure(Size availableSize) override;
    virtual void OnRender(GraphicsContext& ctx) override;

    void SetImageType(ImageType type) { m_imageType = type; }
    void SetBadgeText(const std::string& text) { m_badgeText = text; }
    void SetBadgeColor(D2D1_COLOR_F color) { m_badgeColor = color; }

private:
    ImageType m_imageType = ImageType::Avatar;
    std::string m_badgeText = "UI";
    D2D1_COLOR_F m_badgeColor = D2D1::ColorF(0x00 / 255.0f, 0x7A / 255.0f, 0xCC / 255.0f, 1.0f);
};

} // namespace CUI
