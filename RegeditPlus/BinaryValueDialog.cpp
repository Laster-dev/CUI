#include "BinaryValueDialog.h"
#include "framework/style/ThemeManager.h"
#include "framework/style/ThemeTokenId.h"

#include <algorithm>
#include <cmath>

using namespace CUI;

namespace RegeditPlus {
namespace {

std::string WideToUtf8(const std::wstring& w) {
    if (w.empty()) return std::string();
    const int cb = WideCharToMultiByte(CP_UTF8, 0, w.data(), static_cast<int>(w.size()), nullptr, 0, nullptr, nullptr);
    if (cb <= 0) return std::string();
    std::string out(static_cast<size_t>(cb), '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.data(), static_cast<int>(w.size()), &out[0], cb, nullptr, nullptr);
    return out;
}

float Clamp01(float v) { return (std::max)(0.0f, (std::min)(1.0f, v)); }
float EaseOut(float t) { t = Clamp01(t); float inv = 1.0f - t; return 1.0f - inv * inv * inv; }
float EaseIn(float t) { t = Clamp01(t); return t * t * t; }

} // namespace

BinaryValueDialog::BinaryValueDialog() {
    SetBackgroundToken(ThemeTokenId::CardBackground);
    SetBorderToken(ThemeTokenId::CardBorder);

    m_title = std::make_shared<TextBlock>("编辑二进制数值");
    m_title->SetFontSize(16.0f);
    m_title->SetFontWeight("Bold");
    m_title->SetFontFamily("微软雅黑");
    m_title->SetColorToken(ThemeTokenId::TextPrimary);

    m_nameLabel = std::make_shared<TextBlock>("数值名称(N):");
    m_nameLabel->SetFontSize(13.0f);
    m_nameLabel->SetFontFamily("微软雅黑");
    m_nameLabel->SetColorToken(ThemeTokenId::TextPrimary);

    m_nameBox = std::make_shared<TextBox>();
    m_nameBox->SetFontFamily("微软雅黑");
    m_nameBox->SetFontSize(13.0f);
    m_nameBox->SetHeight(28.0f);
    m_nameBox->SetIsReadOnly(true);

    m_dataLabel = std::make_shared<TextBlock>("数值数据(V):");
    m_dataLabel->SetFontSize(13.0f);
    m_dataLabel->SetFontFamily("微软雅黑");
    m_dataLabel->SetColorToken(ThemeTokenId::TextPrimary);

    m_hex = std::make_shared<HexEditor>();
    m_hex->SetBytesPerRow(8);

    auto styleSecondary = [](const std::shared_ptr<Button>& btn) {
        btn->SetBackgroundToken(ThemeTokenId::CardBackground);
        btn->SetHoverBackgroundToken(ThemeTokenId::HoverBackground);
        btn->SetPressedBackgroundToken(ThemeTokenId::PressedBackground);
        btn->SetBorderToken(ThemeTokenId::CardBorder);
        btn->SetBorderThickness(1.0f);
        btn->SetColorToken(ThemeTokenId::TextPrimary);
        btn->SetFontFamily("微软雅黑");
        btn->SetPadding(Thickness(16, 6, 16, 6));
    };

    m_ok = std::make_shared<Button>("确定");
    m_ok->SetBackgroundToken(ThemeTokenId::AccentColor);
    m_ok->SetHoverBackgroundToken(ThemeTokenId::AccentColor);
    m_ok->SetPressedBackgroundToken(ThemeTokenId::AccentColor);
    m_ok->SetColorToken(ThemeTokenId::AccentForeground);
    m_ok->SetFontFamily("微软雅黑");
    m_ok->SetPadding(Thickness(16, 6, 16, 6));
    m_ok->OnClick().Connect([this](UIElement*) {
        auto data = m_hex ? m_hex->GetBytes() : std::vector<BYTE>{};
        auto cb = m_callback;
        Hide();
        if (cb) cb(true, std::move(data));
    });

    m_cancel = std::make_shared<Button>("取消");
    styleSecondary(m_cancel);
    m_cancel->OnClick().Connect([this](UIElement*) {
        auto cb = m_callback;
        Hide();
        if (cb) cb(false, {});
    });

    AddChild(m_title);
    AddChild(m_nameLabel);
    AddChild(m_nameBox);
    AddChild(m_dataLabel);
    AddChild(m_hex);
    AddChild(m_ok);
    AddChild(m_cancel);
}

void BinaryValueDialog::InvalidateCard() {
    m_cardCacheValid = false;
    m_cardLayer.Invalidate(RenderLayer::ContentDirty | RenderLayer::SizeDirty);
}

void BinaryValueDialog::Show(UIElement* root, const std::wstring& valueName,
                             std::vector<BYTE> data,
                             std::function<void(bool, std::vector<BYTE>)> callback) {
    if (!root) return;
    m_callback = std::move(callback);
    if (m_nameBox) m_nameBox->SetText(WideToUtf8(valueName));
    if (m_hex) m_hex->SetBytes(std::move(data));

    m_isOpen = true;
    m_animState = 1;
    m_animStart = std::chrono::steady_clock::now();
    m_animProgress = 0.0f;
    InvalidateCard();
    m_cardLayer.SetCacheable(true);

    root->AddChildQuiet(std::static_pointer_cast<UIElement>(shared_from_this()));
    const Rect rootBounds = root->GetBounds();
    if (!rootBounds.IsEmpty()) {
        Arrange(rootBounds);
    }
    if (!UIElement::AreAnimationsEnabled()) {
        m_animState = 2;
        m_animProgress = 1.0f;
    }
    RequestAnimationTicks();
    MarkRenderRectDirty(root->GetBounds());
}

void BinaryValueDialog::Hide() {
    if (m_animState == 3 || m_animState == 0) return;
    m_animState = 3;
    m_animStart = std::chrono::steady_clock::now();
    if (!UIElement::AreAnimationsEnabled()) {
        m_animState = 0;
        m_animProgress = 0.0f;
        m_isOpen = false;
        if (m_parent) {
            UIElement* parent = m_parent;
            parent->RemoveChildQuiet(std::static_pointer_cast<UIElement>(shared_from_this()));
            parent->MarkRenderRectDirty(parent->GetBounds());
        }
        return;
    }
    RequestAnimationTicks();
}

Size BinaryValueDialog::Measure(Size availableSize) { return availableSize; }
void BinaryValueDialog::Arrange(Rect finalRect) { m_bounds = finalRect; }
void BinaryValueDialog::Render(GraphicsContext& ctx) { (void)ctx; }
void BinaryValueDialog::OnRender(GraphicsContext& ctx) { (void)ctx; }

bool BinaryValueDialog::HasSelfAnimation() const {
    return UIElement::AreAnimationsEnabled() && (m_animState == 1 || m_animState == 3);
}

bool BinaryValueDialog::OnAnimationTick() {
    bool child = UIElement::OnAnimationTick();
    if (!m_isOpen && m_animState == 0) return child;

    if (m_hex && m_animState == 2) {
        if (m_hex->OnAnimationTick()) {
            InvalidateCard();
            MarkRenderRectDirty(m_dialogBounds.IsEmpty() ? m_bounds : m_dialogBounds);
            child = true;
        }
    }

    if (!UIElement::AreAnimationsEnabled()) {
        if (m_animState == 1) { m_animState = 2; m_animProgress = 1.0f; return child; }
        if (m_animState == 3) {
            m_animState = 0; m_animProgress = 0.0f; m_isOpen = false;
            if (m_parent) {
                UIElement* parent = m_parent;
                parent->RemoveChildQuiet(std::static_pointer_cast<UIElement>(shared_from_this()));
                parent->MarkRenderRectDirty(parent->GetBounds());
            }
            return false;
        }
        return child;
    }

    auto now = std::chrono::steady_clock::now();
    float elapsed = std::chrono::duration<float>(now - m_animStart).count();
    const float dur = 0.18f;
    if (m_animState == 1) {
        float t = elapsed / dur;
        m_animProgress = EaseOut(t);
        if (t >= 1.0f) { m_animState = 2; m_animProgress = 1.0f; }
        else RequestAnimationTicks();
        InvalidateCard();
        return true;
    }
    if (m_animState == 3) {
        float t = elapsed / dur;
        m_animProgress = 1.0f - EaseIn(t);
        if (t >= 1.0f) {
            m_animState = 0; m_animProgress = 0.0f; m_isOpen = false;
            if (m_parent) {
                UIElement* parent = m_parent;
                parent->RemoveChildQuiet(std::static_pointer_cast<UIElement>(shared_from_this()));
                parent->MarkRenderRectDirty(parent->GetBounds());
            }
            return false;
        }
        RequestAnimationTicks();
        InvalidateCard();
        return true;
    }
    return child;
}

void BinaryValueDialog::LayoutChildren() {
    const float pad = 20.0f;
    const float x = m_dialogBounds.x + pad;
    const float w = m_dialogBounds.width - pad * 2.0f;
    float y = m_dialogBounds.y + 16.0f;

    if (m_title) {
        m_title->Measure(Size(w, 28.0f));
        m_title->Arrange(Rect(x, y, w, 28.0f));
    }
    y += 36.0f;
    if (m_nameLabel) {
        m_nameLabel->Measure(Size(w, 20.0f));
        m_nameLabel->Arrange(Rect(x, y, w, 20.0f));
    }
    y += 22.0f;
    if (m_nameBox) {
        m_nameBox->Measure(Size(w, 28.0f));
        m_nameBox->Arrange(Rect(x, y, w, 28.0f));
    }
    y += 36.0f;
    if (m_dataLabel) {
        m_dataLabel->Measure(Size(w, 20.0f));
        m_dataLabel->Arrange(Rect(x, y, w, 20.0f));
    }
    y += 22.0f;
    const float btnArea = 56.0f;
    const float hexH = (std::max)(120.0f, m_dialogBounds.y + m_dialogBounds.height - btnArea - y);
    if (m_hex) {
        m_hex->SetWidth(w);
        m_hex->SetHeight(hexH);
        m_hex->Measure(Size(w, hexH));
        m_hex->Arrange(Rect(x, y, w, hexH));
    }

    const float btnY = m_dialogBounds.y + m_dialogBounds.height - 44.0f;
    float right = m_dialogBounds.x + m_dialogBounds.width - pad;
    if (m_cancel) {
        Size s = m_cancel->Measure(Size(w, 32.0f));
        float bw = (std::max)(80.0f, s.width);
        right -= bw;
        m_cancel->Arrange(Rect(right, btnY, bw, 32.0f));
        right -= 10.0f;
    }
    if (m_ok) {
        Size s = m_ok->Measure(Size(w, 32.0f));
        float bw = (std::max)(80.0f, s.width);
        right -= bw;
        m_ok->Arrange(Rect(right, btnY, bw, 32.0f));
    }
}

void BinaryValueDialog::OnRenderOverlay(GraphicsContext& ctx) {
    if (!m_isOpen || m_animProgress <= 0.001f) return;

    Rect windowRect = m_bounds;
    UIElement* root = this;
    while (root->GetParent()) root = root->GetParent();
    if (root) windowRect = root->GetBounds();

    ctx.FillRect(windowRect, D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.40f * m_animProgress));

    float cardW = (std::min)(520.0f, windowRect.width - 40.0f);
    float cardH = (std::min)(480.0f, windowRect.height - 40.0f);
    float cardX = windowRect.x + (windowRect.width - cardW) * 0.5f;
    float cardY = windowRect.y + (windowRect.height - cardH) * 0.5f;
    m_dialogBounds = Rect(cardX, cardY, cardW, cardH);

    LayoutChildren();

    m_cardLayer.SetCacheable(true);
    const bool needRaster = !m_cardCacheValid
        || m_cardLayer.NeedsContentRaster()
        || !m_cardLayer.GetCacheBitmap()
        || std::abs(m_cardLayer.GetCacheSurfaceSize().width - cardW) > 1.0f
        || std::abs(m_cardLayer.GetCacheSurfaceSize().height - cardH) > 1.0f;

    if (needRaster) {
        if (ctx.PushLayerTarget(m_cardLayer, Size(cardW, cardH), Rect(0, 0, cardW, cardH),
                                D2D1::ColorF(0, 0, 0, 0), true)) {
            const Rect saved = ctx.GetPaintBounds();
            ctx.SetPaintBounds(Rect());
            ctx.PushTransform(D2D1::Matrix3x2F::Translation(-cardX, -cardY));

            D2D1_COLOR_F cardBg = ResolveThemeColor(GetBackgroundToken(), ThemeTokenId::CardBackground);
            D2D1_COLOR_F cardBorder = ResolveThemeColor(GetBorderToken(), ThemeTokenId::CardBorder);
            ctx.FillRoundedRect(m_dialogBounds, 8.0f, cardBg);
            ctx.DrawRoundedRect(m_dialogBounds, 8.0f, cardBorder, 1.0f);

            if (m_title) m_title->Render(ctx);
            if (m_nameLabel) m_nameLabel->Render(ctx);
            if (m_nameBox) m_nameBox->Render(ctx);
            if (m_dataLabel) m_dataLabel->Render(ctx);
            if (m_hex) m_hex->Render(ctx);
            if (m_ok) m_ok->Render(ctx);
            if (m_cancel) m_cancel->Render(ctx);

            ctx.PopTransform();
            ctx.SetPaintBounds(saved);
            ctx.PopLayerTarget(m_cardLayer);
            m_cardLayer.Validate();
            m_cardCacheValid = true;
        }
    }

    if (m_cardLayer.GetCacheBitmap()) {
        ctx.DrawLayer(m_cardLayer, m_dialogBounds, nullptr, m_animProgress);
    }
}

UIElement* BinaryValueDialog::OnHitTestOverlay(float x, float y) {
    if (!m_isOpen || m_animProgress <= 0.001f) return nullptr;
    if (m_hex && m_hex->GetBounds().Contains(x, y)) return m_hex.get();
    if (m_ok && m_ok->GetBounds().Contains(x, y)) return m_ok.get();
    if (m_cancel && m_cancel->GetBounds().Contains(x, y)) return m_cancel.get();
    if (m_nameBox && m_nameBox->GetBounds().Contains(x, y)) return m_nameBox.get();
    if (m_dialogBounds.Contains(x, y)) return this;
    return this; // modal: swallow clicks outside
}

UIElement* BinaryValueDialog::HitTestOverlay(float x, float y) {
    return OnHitTestOverlay(x, y);
}

} // namespace RegeditPlus
