#include "MessageBox.h"
#include "../style/ThemeManager.h"
#include <algorithm>

namespace CUI {

ContentDialog::ContentDialog() {
    SetProperty("theme.backgroundToken", Value("cardBackground"));
    SetProperty("theme.borderToken", Value("cardBorder"));

    m_txtTitle = std::make_shared<TextBlock>(m_titleText);
    m_txtTitle->SetProperty("fontSize", Value(18.0f));
    m_txtTitle->SetProperty("fontWeight", Value("Bold"));
    m_txtTitle->SetProperty("theme.colorToken", Value("textPrimary"));
    m_txtTitle->SetProperty("color", Value(ThemeManager::Instance().GetColor("textPrimary")));

    m_txtMessage = std::make_shared<TextBlock>(m_messageText);
    m_txtMessage->SetProperty("fontSize", Value(13.0f));
    m_txtMessage->SetProperty("theme.colorToken", Value("textSecondary"));
    m_txtMessage->SetProperty("color", Value(ThemeManager::Instance().GetColor("textSecondary")));

    m_btnPrimary = std::make_shared<Button>(m_primaryText);
    m_btnPrimary->SetProperty("theme.backgroundToken", Value("accentColor"));
    m_btnPrimary->SetProperty("background", Value(ThemeManager::Instance().GetColor("accentColor")));
    m_btnPrimary->SetProperty("padding", Value(Thickness(16, 6, 16, 6)));
    m_btnPrimary->OnClick().Connect([this](UIElement*) {
        DialogResult res = DialogResult::Primary;
        Hide();
        if (m_callback) m_callback(res);
    });

    m_btnSecondary = std::make_shared<Button>(m_secondaryText);
    m_btnSecondary->SetProperty("theme.backgroundToken", Value("cardBorder"));
    m_btnSecondary->SetProperty("background", Value(ThemeManager::Instance().GetColor("cardBorder")));
    m_btnSecondary->SetProperty("padding", Value(Thickness(16, 6, 16, 6)));
    m_btnSecondary->OnClick().Connect([this](UIElement*) {
        DialogResult res = DialogResult::Secondary;
        Hide();
        if (m_callback) m_callback(res);
    });

    m_btnClose = std::make_shared<Button>(m_closeText);
    m_btnClose->SetProperty("theme.backgroundToken", Value("cardBackground"));
    m_btnClose->SetProperty("background", Value(ThemeManager::Instance().GetColor("cardBackground")));
    m_btnClose->SetProperty("padding", Value(Thickness(16, 6, 16, 6)));
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
    if (m_txtTitle) m_txtTitle->SetProperty("text", Value(title));
}

void ContentDialog::SetMessage(const std::string& message) {
    m_messageText = message;
    if (m_txtMessage) m_txtMessage->SetProperty("text", Value(message));
}

void ContentDialog::SetPrimaryButtonText(const std::string& text) {
    m_primaryText = text;
    if (m_btnPrimary) m_btnPrimary->SetProperty("text", Value(text));
}

void ContentDialog::SetSecondaryButtonText(const std::string& text) {
    m_secondaryText = text;
    if (m_btnSecondary) m_btnSecondary->SetProperty("text", Value(text));
}

void ContentDialog::SetCloseButtonText(const std::string& text) {
    m_closeText = text;
    if (m_btnClose) m_btnClose->SetProperty("text", Value(text));
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
}

void ContentDialog::Hide() {
    if (m_animState == 3 || m_animState == 0) return;
    m_animState = 3; // Closing
    m_animStartTime = std::chrono::steady_clock::now();
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
                m_parent->RemoveChildRaw(this);
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
        }
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
                m_parent->RemoveChildRaw(this);
            }
            return false;
        }
        return true;
    }

    return childAnim || (m_animState == 1 || m_animState == 3);
}

bool ContentDialog::HasSelfAnimation() const {
    return UIElement::AreAnimationsEnabled() && (m_animState == 1 || m_animState == 3);
}

void ContentDialog::CollectAnimationBounds(Rect& dirtyRect, bool& hasDirty) const {
    if (HasSelfAnimation()) {
        Rect windowRect = m_bounds;
        const UIElement* root = this;
        while (root->GetParent()) {
            root = root->GetParent();
        }
        if (root) {
            windowRect = root->GetBounds();
        }
        dirtyRect = hasDirty ? dirtyRect.Union(windowRect) : windowRect;
        hasDirty = true;
    }
}

Size ContentDialog::Measure(Size availableSize) {
    return availableSize;
}

void ContentDialog::Arrange(Rect finalRect) {
    m_bounds = finalRect;
}

void ContentDialog::OnRenderOverlay(GraphicsContext& ctx) {
    if (!m_isOpen || m_animProgress <= 0.001f) return;

    // 1. Get window root dimensions to span full backdrop over entire window area
    Rect windowRect = m_bounds;
    UIElement* root = this;
    while (root->GetParent()) {
        root = root->GetParent();
    }
    if (root) {
        windowRect = root->GetBounds();
    }

    // 2. Draw semi-transparent dark backdrop mask with subtle fade-in alpha (0 -> 0.4)
    float backdropAlpha = 0.40f * m_animProgress;
    ctx.FillRect(windowRect, D2D1::ColorF(0.0f, 0.0f, 0.0f, backdropAlpha));

    // 3. WinUI 3 Fluent PopupThemeTransition:
    // Entrance: Scale down slightly from 1.03f -> 1.0f (or slight zoom out pop-in)
    // Exit: Scale slightly from 1.0f -> 1.03f (fade out)
    float baseW = (std::min)(480.0f, windowRect.width - 40.0f);
    float baseH = 220.0f;

    // WinUI 3 Scale transition: 1.03f when starting -> 1.0f when settled
    float scale = 1.03f - 0.03f * m_animProgress;

    float cardW = baseW * scale;
    float cardH = baseH * scale;
    float cardX = windowRect.x + (windowRect.width - cardW) * 0.5f;
    float cardY = windowRect.y + (windowRect.height - cardH) * 0.5f;

    m_dialogBounds = Rect(cardX, cardY, cardW, cardH);

    // Card Outer Drop Shadow with fade
    ctx.FillRoundedRect(Rect(cardX - 6, cardY - 6, cardW + 12, cardH + 12), 12.0f, D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.35f * m_animProgress));

    // Card Background
    D2D1_COLOR_F cardBg = ResolveThemeColor("theme.backgroundToken", "cardBackground");
    cardBg.a = m_animProgress;
    D2D1_COLOR_F cardBorder = ResolveThemeColor("theme.borderToken", "cardBorder");
    cardBorder.a = m_animProgress;
    ctx.FillRoundedRect(m_dialogBounds, 8.0f, cardBg);
    ctx.DrawRoundedRect(m_dialogBounds, 8.0f, cardBorder, 1.0f);

    // 4. Layout Children inside Centered Card
    float innerX = cardX + 24.0f * scale;
    float innerY = cardY + 20.0f * scale;
    float innerW = cardW - 48.0f * scale;

    // Title
    if (m_txtTitle) {
        m_txtTitle->Measure(Size(innerW, 30.0f * scale));
        m_txtTitle->Arrange(Rect(innerX, innerY, innerW, 30.0f * scale));
        m_txtTitle->Render(ctx);
    }

    // Message Text
    if (m_txtMessage) {
        m_txtMessage->Measure(Size(innerW, 80.0f * scale));
        m_txtMessage->Arrange(Rect(innerX, innerY + 36.0f * scale, innerW, 80.0f * scale));
        m_txtMessage->Render(ctx);
    }

    // Footer Buttons Panel (Right aligned)
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
}

UIElement* ContentDialog::OnHitTestOverlay(float x, float y) {
    if (!m_isOpen) return nullptr;

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

    root->AddChild(dlg);
}

} // namespace CUI
