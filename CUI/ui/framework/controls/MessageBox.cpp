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
    m_txtTitle->SetFontFamily("微软雅黑");
    m_txtTitle->SetColorToken(ThemeTokenId::TextPrimary);
    m_txtTitle->SetColor(ThemeManager::Instance().GetColor("textPrimary"));

    m_txtMessage = std::make_shared<TextBlock>(m_messageText);
    m_txtMessage->SetFontSize(14.0f);
    m_txtMessage->SetFontFamily("微软雅黑");
    m_txtMessage->SetColorToken(ThemeTokenId::TextSecondary);
    m_txtMessage->SetColor(ThemeManager::Instance().GetColor("textSecondary"));

    m_inputBox = std::make_shared<TextBox>();
    m_inputBox->SetFontFamily("微软雅黑");
    m_inputBox->SetFontSize(16.0f);
    m_inputBox->SetHeight(32.0f);
    m_inputBox->SetVisibility(Visibility::Collapsed);

    m_btnPrimary = std::make_shared<Button>(m_primaryText);
    m_btnPrimary->SetBackgroundToken(ThemeTokenId::AccentColor);
    m_btnPrimary->SetHoverBackgroundToken(ThemeTokenId::AccentColor);
    m_btnPrimary->SetPressedBackgroundToken(ThemeTokenId::AccentColor);
    m_btnPrimary->SetBackground(ThemeManager::Instance().GetColor("accentColor"));
    m_btnPrimary->SetColorToken(ThemeTokenId::AccentForeground);
    m_btnPrimary->SetColor(ThemeManager::Instance().GetColor("accentForeground"));
    m_btnPrimary->SetFontFamily("微软雅黑");
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
        btn->SetFontFamily("微软雅黑");
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
    AddChild(m_inputBox);
    AddChild(m_btnPrimary);
    AddChild(m_btnSecondary);
    AddChild(m_btnClose);
}

void ContentDialog::SetTitle(const std::string& title) {
    m_titleText = title;
    if (m_txtTitle) m_txtTitle->SetText(title);
    InvalidateCard();
}

void ContentDialog::SetMessage(const std::string& message) {
    m_messageText = message;
    if (m_txtMessage) m_txtMessage->SetText(message);
    InvalidateCard();
}

void ContentDialog::SetPrimaryButtonText(const std::string& text) {
    m_primaryText = text;
    if (m_btnPrimary) m_btnPrimary->SetText(text);
    InvalidateCard();
}

void ContentDialog::SetSecondaryButtonText(const std::string& text) {
    m_secondaryText = text;
    if (m_btnSecondary) m_btnSecondary->SetText(text);
    InvalidateCard();
}

void ContentDialog::SetCloseButtonText(const std::string& text) {
    m_closeText = text;
    if (m_btnClose) m_btnClose->SetText(text);
    InvalidateCard();
}

void ContentDialog::SetInputEnabled(bool enabled, bool multiline) {
    m_inputEnabled = enabled;
    m_inputMultiline = multiline;
    if (m_inputBox) {
        m_inputBox->SetVisibility(enabled ? Visibility::Visible : Visibility::Collapsed);
        m_inputBox->SetAcceptsReturn(multiline);
        m_inputBox->SetTextWrapping(multiline);
        m_inputBox->SetHeight(multiline ? 120.0f : 32.0f);
    }
    InvalidateCard();
}

void ContentDialog::SetInputText(const std::string& text) {
    if (m_inputBox) {
        m_inputBox->SetText(text);
        m_inputBox->SelectAll();
    }
    InvalidateCard();
}

std::string ContentDialog::GetInputText() const {
    return m_inputBox ? m_inputBox->GetText() : std::string();
}

void ContentDialog::InvalidateCard() {
    m_cardCacheValid = false;
    m_cardLayer.Invalidate(RenderLayer::ContentDirty | RenderLayer::SizeDirty);
}

static float Clamp01(float value) {
    return (std::max)(0.0f, (std::min)(1.0f, value));
}

static float FluentEaseOut(float t) {
    t = Clamp01(t);
    float inv = 1.0f - t;
    return 1.0f - inv * inv * inv;
}

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
    InvalidateCard();
    m_cardLayer.SetCacheable(true);
    if (!UIElement::AreAnimationsEnabled()) {
        m_animState = 2;
        m_animProgress = 1.0f;
    }
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
    InvalidateCard();
    m_cardLayer.ResetCache();
    if (m_isOpen || m_animState != 0) {
        RequestAnimationTicks();
    }
}

bool ContentDialog::OnAnimationTick() {
    bool childAnim = UIElement::OnAnimationTick();
    if (!m_isOpen && m_animState == 0) return childAnim;

    // Input TextBox registers itself with AnimationManager for focus-line / caret.
    // Do not tick it here (double-speed) and do not keep the dialog self-animating
    // for the whole input lifetime (that forced full-window invalidates).
    // Visual updates go through TextBox::NotifyHostOverlayDirty → InvalidateCard.

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

    if (m_animState == 1) {
        float t = Clamp01(elapsed / 0.167f);
        m_animProgress = FluentEaseOut(t);
        if (t >= 1.0f) {
            m_animState = 2;
            m_animProgress = 1.0f;
            if (m_parent) {
                m_parent->MarkRenderRectDirty(m_parent->GetBounds());
            }
        }
        RequestAnimationTicks();
        return true;
    }

    if (m_animState == 3) {
        float t = Clamp01(elapsed / 0.133f);
        m_animProgress = 1.0f - FluentEaseIn(t);
        if (t >= 1.0f) {
            m_animState = 0;
            m_animProgress = 0.0f;
            m_isOpen = false;
            if (m_parent) {
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
    // Open/close scrim only — an open input dialog must not report self-animation
    // or Window invalidates the entire client every frame (TextBox drag becomes unusable).
    if (UIElement::AreAnimationsEnabled() && (m_animState == 1 || m_animState == 3)) return true;
    return false;
}

void ContentDialog::CollectSelfAnimationBounds(Rect& dirtyRect, bool& hasDirty) const {
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

void ContentDialog::LayoutCardChildren(float scale) {
    float cardX = m_dialogBounds.x;
    float cardY = m_dialogBounds.y;
    float cardW = m_dialogBounds.width;
    float cardH = m_dialogBounds.height;

    float innerX = cardX + 24.0f * scale;
    float innerY = cardY + 20.0f * scale;
    float innerW = cardW - 48.0f * scale;

    if (m_txtTitle) {
        m_txtTitle->Measure(Size(innerW, 30.0f * scale));
        m_txtTitle->Arrange(Rect(innerX, innerY, innerW, 30.0f * scale));
    }

    float msgH = m_inputEnabled ? 36.0f * scale : 80.0f * scale;
    if (m_txtMessage) {
        m_txtMessage->Measure(Size(innerW, msgH));
        m_txtMessage->Arrange(Rect(innerX, innerY + 36.0f * scale, innerW, msgH));
    }

    if (m_inputEnabled && m_inputBox) {
        float inputY = innerY + 36.0f * scale + msgH + 8.0f * scale;
        float inputH = (m_inputMultiline ? 120.0f : 32.0f) * scale;
        m_inputBox->Measure(Size(innerW, inputH));
        m_inputBox->Arrange(Rect(innerX, inputY, innerW, inputH));
    }

    float btnY = cardY + cardH - 52.0f * scale;
    float btnRight = cardX + cardW - 24.0f * scale;
    if (!m_closeText.empty() && m_btnClose) {
        Size s = m_btnClose->Measure(Size(innerW, 32.0f * scale));
        float w = (std::max)(80.0f * scale, s.width);
        btnRight -= w;
        m_btnClose->Arrange(Rect(btnRight, btnY, w, 32.0f * scale));
        btnRight -= 12.0f * scale;
    }
    if (!m_secondaryText.empty() && m_btnSecondary) {
        Size s = m_btnSecondary->Measure(Size(innerW, 32.0f * scale));
        float w = (std::max)(80.0f * scale, s.width);
        btnRight -= w;
        m_btnSecondary->Arrange(Rect(btnRight, btnY, w, 32.0f * scale));
        btnRight -= 12.0f * scale;
    }
    if (!m_primaryText.empty() && m_btnPrimary) {
        Size s = m_btnPrimary->Measure(Size(innerW, 32.0f * scale));
        float w = (std::max)(80.0f * scale, s.width);
        btnRight -= w;
        m_btnPrimary->Arrange(Rect(btnRight, btnY, w, 32.0f * scale));
    }
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

    float baseW = (std::min)(520.0f, windowRect.width - 40.0f);
    float baseH = m_inputEnabled ? (m_inputMultiline ? 340.0f : 260.0f) : 220.0f;

    float scale = 1.0f;
    if (m_animState == 3) {
        scale = 1.0f + 0.02f * (1.0f - m_animProgress);
    }

    float cardW = baseW * scale;
    float cardH = baseH * scale;
    float cardX = windowRect.x + (windowRect.width - cardW) * 0.5f;
    float cardY = windowRect.y + (windowRect.height - cardH) * 0.5f;
    m_dialogBounds = Rect(cardX, cardY, cardW, cardH);

    LayoutCardChildren(scale);

    // Cache chrome; for input dialogs still allow cache but invalidate on caret /
    // selection / text changes via InvalidateCard() — never every vsync.
    m_cardLayer.SetCacheable(true);
    const bool needRaster = !m_cardCacheValid
        || m_cardLayer.NeedsContentRaster()
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
            const Rect savedPaintBounds = ctx.GetPaintBounds();
            ctx.SetPaintBounds(Rect());
            ctx.PushTransform(D2D1::Matrix3x2F::Translation(-cardX, -cardY));

            D2D1_COLOR_F cardBg = ResolveThemeColor(GetBackgroundToken(), ThemeTokenId::CardBackground);
            D2D1_COLOR_F cardBorder = ResolveThemeColor(GetBorderToken(), ThemeTokenId::CardBorder);
            ctx.FillRoundedRect(m_dialogBounds, 8.0f, cardBg);
            ctx.DrawRoundedRect(m_dialogBounds, 8.0f, cardBorder, 1.0f);

            if (m_txtTitle) m_txtTitle->Render(ctx);
            if (m_txtMessage) m_txtMessage->Render(ctx);
            if (m_inputEnabled && m_inputBox) m_inputBox->Render(ctx);
            if (!m_closeText.empty() && m_btnClose) m_btnClose->Render(ctx);
            if (!m_secondaryText.empty() && m_btnSecondary) m_btnSecondary->Render(ctx);
            if (!m_primaryText.empty() && m_btnPrimary) m_btnPrimary->Render(ctx);

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

    D2D1_COLOR_F cardBg = ResolveThemeColor(GetBackgroundToken(), ThemeTokenId::CardBackground);
    cardBg.a = m_animProgress;
    D2D1_COLOR_F cardBorder = ResolveThemeColor(GetBorderToken(), ThemeTokenId::CardBorder);
    cardBorder.a = m_animProgress;
    ctx.FillRoundedRect(m_dialogBounds, 8.0f, cardBg);
    ctx.DrawRoundedRect(m_dialogBounds, 8.0f, cardBorder, 1.0f);
}

UIElement* ContentDialog::OnHitTestOverlay(float x, float y) {
    if (!m_isOpen || m_animProgress <= 0.001f) return nullptr;

    if (m_inputEnabled && m_inputBox && m_inputBox->GetVisibility() == Visibility::Visible
        && m_inputBox->GetBounds().Contains(x, y)) {
        return m_inputBox.get();
    }
    if (!m_closeText.empty() && m_btnClose && m_btnClose->GetBounds().Contains(x, y)) return m_btnClose.get();
    if (!m_secondaryText.empty() && m_btnSecondary && m_btnSecondary->GetBounds().Contains(x, y)) return m_btnSecondary.get();
    if (!m_primaryText.empty() && m_btnPrimary && m_btnPrimary->GetBounds().Contains(x, y)) return m_btnPrimary.get();

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

void ContentDialog::ShowMessageBox(UIElement* root, const std::string& title, const std::string& message,
                                   std::function<void(DialogResult)> callback) {
    if (!root) return;

    auto dlg = std::make_shared<ContentDialog>();
    dlg->SetTitle(title);
    dlg->SetMessage(message);
    dlg->SetPrimaryButtonText("确定");
    dlg->SetCloseButtonText("取消");
    dlg->Show(callback);

    root->AddChildQuiet(dlg);
    const Rect rootBounds = root->GetBounds();
    if (!rootBounds.IsEmpty()) {
        dlg->Arrange(rootBounds);
    }
    dlg->RequestAnimationTicks();
}

void ContentDialog::ShowInputBox(
    UIElement* root,
    const std::string& title,
    const std::string& message,
    const std::string& initialText,
    bool multiline,
    std::function<void(DialogResult, const std::string&)> callback) {
    if (!root) return;

    auto dlg = std::make_shared<ContentDialog>();
    dlg->SetTitle(title);
    dlg->SetMessage(message);
    dlg->SetInputEnabled(true, multiline);
    dlg->SetInputText(initialText);
    dlg->SetPrimaryButtonText("确定");
    dlg->SetCloseButtonText("取消");
    dlg->Show([dlg, callback](DialogResult r) {
        if (callback) {
            callback(r, dlg->GetInputText());
        }
    });

    root->AddChildQuiet(dlg);
    const Rect rootBounds = root->GetBounds();
    if (!rootBounds.IsEmpty()) {
        dlg->Arrange(rootBounds);
    }
    dlg->RequestAnimationTicks();
}

} // namespace CUI
