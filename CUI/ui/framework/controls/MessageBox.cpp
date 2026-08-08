#include "MessageBox.h"
#include "../style/ThemeManager.h"
#include <algorithm>

namespace CUI {

ContentDialog::ContentDialog() {
    SetBackgroundToken(ThemeTokenId::CardBackground);
    SetBorderToken(ThemeTokenId::CardBorder);

    m_txtTitle = std::make_shared<TextBlock>(m_titleText);
    m_txtTitle->SetFontSize(18.0f);
    m_txtTitle->SetFontWeight("Bold");
    m_txtTitle->SetColorToken(ThemeTokenId::TextPrimary);
    m_txtTitle->SetColor(ThemeManager::Instance().GetColor("textPrimary"));

    m_txtMessage = std::make_shared<TextBlock>(m_messageText);
    m_txtMessage->SetFontSize(13.0f);
    m_txtMessage->SetColorToken(ThemeTokenId::TextSecondary);
    m_txtMessage->SetColor(ThemeManager::Instance().GetColor("textSecondary"));

    m_btnPrimary = std::make_shared<Button>(m_primaryText);
    m_btnPrimary->SetBackgroundToken(ThemeTokenId::AccentColor);
    m_btnPrimary->SetHoverBackgroundToken(ThemeTokenId::AccentColor);
    m_btnPrimary->SetPressedBackgroundToken(ThemeTokenId::AccentColor);
    m_btnPrimary->SetBackground(ThemeManager::Instance().GetColor("accentColor"));
    m_btnPrimary->SetColorToken(ThemeTokenId::AccentForeground);
    m_btnPrimary->SetColor(ThemeManager::Instance().GetColor("accentForeground"));
    m_btnPrimary->SetPadding(Thickness(16, 6, 16, 6));
    m_btnPrimary->OnClick().Connect([this](UIElement*) {
        DialogResult res = DialogResult::Primary;
        Hide();
        if (m_callback) m_callback(res);
    });

    auto styleSecondaryButton = [](const std::shared_ptr<Button>& btn) {
        btn->SetBackgroundToken(ThemeTokenId::CardBackground);
        btn->SetHoverBackgroundToken(ThemeTokenId::HoverBackground);
        btn->SetPressedBackgroundToken(ThemeTokenId::PressedBackground);
        btn->SetBorderToken(ThemeTokenId::CardBorder);
        btn->SetBorderThickness(1.0f);
        btn->SetColorToken(ThemeTokenId::TextPrimary);
        btn->SetColor(ThemeManager::Instance().GetColor("textPrimary"));
        btn->SetBackground(ThemeManager::Instance().GetColor("cardBackground"));
        btn->SetPadding(Thickness(16, 6, 16, 6));
    };

    m_btnSecondary = std::make_shared<Button>(m_secondaryText);
    styleSecondaryButton(m_btnSecondary);
    m_btnSecondary->OnClick().Connect([this](UIElement*) {
        DialogResult res = DialogResult::Secondary;
        Hide();
        if (m_callback) m_callback(res);
    });

    m_btnClose = std::make_shared<Button>(m_closeText);
    styleSecondaryButton(m_btnClose);
    m_btnClose->OnClick().Connect([this](UIElement*) {
        DialogResult res = DialogResult::Cancel;
        Hide();
        if (m_callback) m_callback(res);
    });

    AddChild(m_txtTitle);
    AddChild(m_txtMessage);
    AddChild(m_btnPrimary);
    AddChild(m_btnSecondary);
    AddChild(m_btnClose);
}

void ContentDialog::SetTitle(const std::string& title) {
    m_titleText = title;
    if (m_txtTitle) m_txtTitle->SetText(title);
}

void ContentDialog::SetMessage(const std::string& message) {
    m_messageText = message;
    if (m_txtMessage) m_txtMessage->SetText(message);
}

void ContentDialog::SetPrimaryButtonText(const std::string& text) {
    m_primaryText = text;
    if (m_btnPrimary) m_btnPrimary->SetText(text);
}

void ContentDialog::SetSecondaryButtonText(const std::string& text) {
    m_secondaryText = text;
    if (m_btnSecondary) m_btnSecondary->SetText(text);
}

void ContentDialog::SetCloseButtonText(const std::string& text) {
    m_closeText = text;
    if (m_btnClose) m_btnClose->SetText(text);
}

static float Clamp01(float value) {
    return (std::max)(0.0f, (std::min)(1.0f, value));
}

// Fluent Standard Easing / Fast Out Slow In Cubic-Bezier(0, 0, 0, 1)
static float FluentEaseOut(float t) {
    t = Clamp01(t);
    float inv = 1.0f - t;
    return 1.0f - inv * inv * inv;
}

// Fluent Accelerate / Fast In Slow Out Cubic-Bezier(0.7, 0, 1, 0.5)
static float FluentEaseIn(float t) {
    t = Clamp01(t);
    return t * t * t;
}

void ContentDialog::Show(std::function<void(DialogResult)> callback) {
    m_isOpen = true;
    m_callback = callback;
    m_animState = 1; // Opening
    m_animStartTime = std::chrono::steady_clock::now();
    m_animProgress = 0.0f;
    m_cardCacheValid = false;
    m_cardLayer.SetCacheable(true);
    m_cardLayer.Invalidate(RenderLayer::ContentDirty | RenderLayer::SizeDirty);
    if (!UIElement::AreAnimationsEnabled()) {
        m_animState = 2;
        m_animProgress = 1.0f;
    }
    // CollectAnimationBounds covers the overlay; avoid MarkRenderContentDirty which
    // forces a full-scene rebuild and makes the dialog flash on open.
    RequestAnimationTicks();
}

void ContentDialog::Hide() {
    if (m_animState == 3 || m_animState == 0) return;
    m_animState = 3; // Closing
    m_animStartTime = std::chrono::steady_clock::now();
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

void ContentDialog::OnThemeChanged() {
    UIElement::OnThemeChanged();
    m_cardCacheValid = false;
    m_cardLayer.ResetCache();
    if (m_isOpen || m_animState != 0) {
        RequestAnimationTicks();
    }
}

bool ContentDialog::OnAnimationTick() {
    bool childAnim = UIElement::OnAnimationTick();
    if (!m_isOpen && m_animState == 0) return childAnim;

    if (!UIElement::AreAnimationsEnabled()) {
        if (m_animState == 1) {
            m_animState = 2;
            m_animProgress = 1.0f;
            return childAnim;
        }
        if (m_animState == 3) {
            m_animState = 0;
            m_animProgress = 0.0f;
            m_isOpen = false;
            if (m_parent) {
                // Quiet remove: avoid full-scene dirty flash when dialog finishes closing.
                UIElement* parent = m_parent;
                parent->RemoveChildQuiet(std::static_pointer_cast<UIElement>(shared_from_this()));
                parent->MarkRenderRectDirty(parent->GetBounds());
            }
            return false;
        }
        return childAnim;
    }

    auto now = std::chrono::steady_clock::now();
    float elapsed = std::chrono::duration<float>(now - m_animStartTime).count();

    if (m_animState == 1) { // WinUI 3 Popup Open: 167ms
        float t = Clamp01(elapsed / 0.167f);
        m_animProgress = FluentEaseOut(t);
        if (t >= 1.0f) {
            m_animState = 2; // Opened
            m_animProgress = 1.0f;
            // Refresh scene once after open freeze so any under-scrim ripples
            // that ticked during the animation are not left mid-frame.
            if (m_parent) {
                m_parent->MarkRenderRectDirty(m_parent->GetBounds());
            }
        }
        RequestAnimationTicks();
        return true;
    }

    if (m_animState == 3) { // WinUI 3 Popup Close: 133ms
        float t = Clamp01(elapsed / 0.133f);
        m_animProgress = 1.0f - FluentEaseIn(t);
        if (t >= 1.0f) {
            m_animState = 0; // Closed
            m_animProgress = 0.0f;
            m_isOpen = false;
            if (m_parent) {
                // Quiet remove: avoid full-scene dirty flash when dialog finishes closing.
                UIElement* parent = m_parent;
                parent->RemoveChildQuiet(std::static_pointer_cast<UIElement>(shared_from_this()));
                parent->MarkRenderRectDirty(parent->GetBounds());
            }
            return false;
        }
        RequestAnimationTicks();
        return true;
    }

    return childAnim || (m_animState == 1 || m_animState == 3);
}

bool ContentDialog::HasSelfAnimation() const {
    return UIElement::AreAnimationsEnabled() && (m_animState == 1 || m_animState == 3);
}

void ContentDialog::CollectSelfAnimationBounds(Rect& dirtyRect, bool& hasDirty) const {
    // Do NOT contribute a fullscreen dirty rect — that forced the scene path to
    // either full-repaint or skip all under-scrim patches. Overlay paints are
    // driven by Window::InvalidateAnimatedRegions when IsModalOverlayOpen.
    (void)dirtyRect;
    (void)hasDirty;
}

void ContentDialog::CollectAnimationBounds(Rect& dirtyRect, bool& hasDirty) const {
    CollectSelfAnimationBounds(dirtyRect, hasDirty);
}

Size ContentDialog::Measure(Size availableSize) {
    return availableSize;
}

void ContentDialog::Arrange(Rect finalRect) {
    m_bounds = finalRect;
}

void ContentDialog::Render(GraphicsContext& ctx) {
    (void)ctx;
}

void ContentDialog::OnRender(GraphicsContext& ctx) {
    (void)ctx;
}

void ContentDialog::OnRenderOverlay(GraphicsContext& ctx) {
    if (!m_isOpen || m_animProgress <= 0.001f) return;

    Rect windowRect = m_bounds;
    UIElement* root = this;
    while (root->GetParent()) {
        root = root->GetParent();
    }
    if (root) {
        windowRect = root->GetBounds();
    }

    float backdropAlpha = 0.40f * m_animProgress;
    ctx.FillRect(windowRect, D2D1::ColorF(0.0f, 0.0f, 0.0f, backdropAlpha));

    float baseW = (std::min)(480.0f, windowRect.width - 40.0f);
    float baseH = 220.0f;

    float scale = 1.0f;
    if (m_animState == 3) {
        scale = 1.0f + 0.02f * (1.0f - m_animProgress);
    }

    float cardW = baseW * scale;
    float cardH = baseH * scale;
    float cardX = windowRect.x + (windowRect.width - cardW) * 0.5f;
    float cardY = windowRect.y + (windowRect.height - cardH) * 0.5f;
    m_dialogBounds = Rect(cardX, cardY, cardW, cardH);

    m_cardLayer.SetCacheable(true);
    const bool needRaster = !m_cardCacheValid || m_cardLayer.NeedsContentRaster()
        || !m_cardLayer.GetCacheBitmap()
        || std::abs(m_cardLayer.GetCacheSurfaceSize().width - cardW) > 1.0f
        || std::abs(m_cardLayer.GetCacheSurfaceSize().height - cardH) > 1.0f;

    if (needRaster) {
        if (ctx.PushLayerTarget(
                m_cardLayer,
                Size(cardW, cardH),
                Rect(0, 0, cardW, cardH),
                D2D1::ColorF(0, 0, 0, 0),
                true)) {
            // The card layer is rasterized in local layer space, but child controls
            // keep world-space bounds. Do not cull them against the layer-local
            // paint rect or title/buttons may disappear while hit-testing still works.
            const Rect savedPaintBounds = ctx.GetPaintBounds();
            ctx.SetPaintBounds(Rect());
            ctx.PushTransform(D2D1::Matrix3x2F::Translation(-cardX, -cardY));

            D2D1_COLOR_F cardBg = ResolveThemeColor(GetBackgroundToken(), ThemeTokenId::CardBackground);
            D2D1_COLOR_F cardBorder = ResolveThemeColor(GetBorderToken(), ThemeTokenId::CardBorder);
            ctx.FillRoundedRect(m_dialogBounds, 8.0f, cardBg);
            ctx.DrawRoundedRect(m_dialogBounds, 8.0f, cardBorder, 1.0f);

            float innerX = cardX + 24.0f * scale;
            float innerY = cardY + 20.0f * scale;
            float innerW = cardW - 48.0f * scale;

            if (m_txtTitle) {
                m_txtTitle->Measure(Size(innerW, 30.0f * scale));
                m_txtTitle->Arrange(Rect(innerX, innerY, innerW, 30.0f * scale));
                m_txtTitle->Render(ctx);
            }
            if (m_txtMessage) {
                m_txtMessage->Measure(Size(innerW, 80.0f * scale));
                m_txtMessage->Arrange(Rect(innerX, innerY + 36.0f * scale, innerW, 80.0f * scale));
                m_txtMessage->Render(ctx);
            }

            float btnY = cardY + cardH - 52.0f * scale;
            float btnRight = cardX + cardW - 24.0f * scale;
            if (!m_closeText.empty() && m_btnClose) {
                Size s = m_btnClose->Measure(Size(innerW, 32.0f * scale));
                float w = (std::max)(80.0f * scale, s.width);
                btnRight -= w;
                m_btnClose->Arrange(Rect(btnRight, btnY, w, 32.0f * scale));
                m_btnClose->Render(ctx);
                btnRight -= 12.0f * scale;
            }
            if (!m_secondaryText.empty() && m_btnSecondary) {
                Size s = m_btnSecondary->Measure(Size(innerW, 32.0f * scale));
                float w = (std::max)(80.0f * scale, s.width);
                btnRight -= w;
                m_btnSecondary->Arrange(Rect(btnRight, btnY, w, 32.0f * scale));
                m_btnSecondary->Render(ctx);
                btnRight -= 12.0f * scale;
            }
            if (!m_primaryText.empty() && m_btnPrimary) {
                Size s = m_btnPrimary->Measure(Size(innerW, 32.0f * scale));
                float w = (std::max)(80.0f * scale, s.width);
                btnRight -= w;
                m_btnPrimary->Arrange(Rect(btnRight, btnY, w, 32.0f * scale));
                m_btnPrimary->Render(ctx);
            }

            ctx.PopTransform();
            ctx.SetPaintBounds(savedPaintBounds);
            ctx.PopLayerTarget(m_cardLayer);
            m_cardLayer.Validate();
            m_cardCacheValid = true;
        }
    }

    if (m_cardLayer.GetCacheBitmap()) {
        ctx.DrawLayer(m_cardLayer, m_dialogBounds, nullptr, m_animProgress);
        return;
    }

    // Fallback immediate path if layer alloc failed.
    D2D1_COLOR_F cardBg = ResolveThemeColor(GetBackgroundToken(), ThemeTokenId::CardBackground);
    cardBg.a = m_animProgress;
    D2D1_COLOR_F cardBorder = ResolveThemeColor(GetBorderToken(), ThemeTokenId::CardBorder);
    cardBorder.a = m_animProgress;
    ctx.FillRoundedRect(m_dialogBounds, 8.0f, cardBg);
    ctx.DrawRoundedRect(m_dialogBounds, 8.0f, cardBorder, 1.0f);
}

UIElement* ContentDialog::OnHitTestOverlay(float x, float y) {
    // Don't swallow input until the open animation has actually started painting.
    if (!m_isOpen || m_animProgress <= 0.001f) return nullptr;

    if (!m_closeText.empty() && m_btnClose && m_btnClose->GetBounds().Contains(x, y)) return m_btnClose.get();
    if (!m_secondaryText.empty() && m_btnSecondary && m_btnSecondary->GetBounds().Contains(x, y)) return m_btnSecondary.get();
    if (!m_primaryText.empty() && m_btnPrimary && m_btnPrimary->GetBounds().Contains(x, y)) return m_btnPrimary.get();

    // Intercept all mouse clicks on backdrop over full window area
    Rect windowRect = m_bounds;
    UIElement* root = this;
    while (root->GetParent()) {
        root = root->GetParent();
    }
    if (root) {
        windowRect = root->GetBounds();
    }

    if (windowRect.Contains(x, y)) {
        return this;
    }
    return nullptr;
}

UIElement* ContentDialog::HitTestOverlay(float x, float y) {
    return OnHitTestOverlay(x, y);
}

void ContentDialog::ShowMessageBox(UIElement* root, const std::string& title, const std::string& message, std::function<void(DialogResult)> callback) {
    if (!root) return;

    auto dlg = std::make_shared<ContentDialog>();
    dlg->SetTitle(title);
    dlg->SetMessage(message);
    dlg->SetPrimaryButtonText("OK");
    dlg->SetCloseButtonText("Cancel");
    dlg->Show(callback);

    // Quiet add: AddChild() marks the whole scene dirty and flashes under the overlay.
    root->AddChildQuiet(dlg);
    const Rect rootBounds = root->GetBounds();
    if (!rootBounds.IsEmpty()) {
        dlg->Arrange(rootBounds);
    }
    dlg->RequestAnimationTicks();
}

} // namespace CUI
