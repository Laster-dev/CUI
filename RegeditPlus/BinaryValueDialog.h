#pragma once

#include "framework/controls/UIElement.h"
#include "framework/controls/Button.h"
#include "framework/controls/TextBlock.h"
#include "framework/controls/TextBox.h"
#include "framework/render/RenderLayer.h"
#include "HexEditor.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <chrono>

namespace RegeditPlus {

// Modal overlay matching regedit "编辑二进制数值" layout.
class BinaryValueDialog : public CUI::UIElement {
public:
    BinaryValueDialog();
    const char* GetClassName() const override { return "BinaryValueDialog"; }

    void Show(CUI::UIElement* root,
              const std::wstring& valueName,
              std::vector<BYTE> data,
              std::function<void(bool ok, std::vector<BYTE> data)> callback);
    void Hide();
    void InvalidateCard();

    bool IsOpen() const { return m_isOpen; }
    bool IsModalOverlayOpen() const override { return m_isOpen; }

    CUI::Size Measure(CUI::Size availableSize) override;
    void Arrange(CUI::Rect finalRect) override;
    void Render(CUI::GraphicsContext& ctx) override;
    void OnRender(CUI::GraphicsContext& ctx) override;
    void OnRenderOverlay(CUI::GraphicsContext& ctx) override;
    CUI::UIElement* HitTestOverlay(float x, float y) override;
    CUI::UIElement* OnHitTestOverlay(float x, float y) override;
    bool OnAnimationTick() override;
    bool HasSelfAnimation() const override;

private:
    void LayoutChildren();

    bool m_isOpen = false;
    std::function<void(bool, std::vector<BYTE>)> m_callback;

    std::shared_ptr<CUI::TextBlock> m_title;
    std::shared_ptr<CUI::TextBlock> m_nameLabel;
    std::shared_ptr<CUI::TextBox> m_nameBox;
    std::shared_ptr<CUI::TextBlock> m_dataLabel;
    std::shared_ptr<HexEditor> m_hex;
    std::shared_ptr<CUI::Button> m_ok;
    std::shared_ptr<CUI::Button> m_cancel;

    CUI::Rect m_dialogBounds;
    CUI::RenderLayer m_cardLayer;
    bool m_cardCacheValid = false;

    int m_animState = 0; // 0 closed, 1 opening, 2 open, 3 closing
    std::chrono::steady_clock::time_point m_animStart;
    float m_animProgress = 0.0f;
};

} // namespace RegeditPlus
