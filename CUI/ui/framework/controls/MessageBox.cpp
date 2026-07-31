#include "MessageBox.h"
#include <algorithm>

namespace CUI {

ContentDialog::ContentDialog() {
    m_txtTitle = std::make_shared<TextBlock>(m_titleText);
    m_txtTitle->SetProperty("fontSize", Value(18.0f));
    m_txtTitle->SetProperty("fontWeight", Value("Bold"));
    m_txtTitle->SetProperty("color", Value("#FFFFFF"));

    m_txtMessage = std::make_shared<TextBlock>(m_messageText);
    m_txtMessage->SetProperty("fontSize", Value(13.0f));
    m_txtMessage->SetProperty("color", Value("#CCCCCC"));

    m_btnPrimary = std::make_shared<Button>(m_primaryText);
    m_btnPrimary->SetProperty("background", Value("#007ACC"));
    m_btnPrimary->SetProperty("padding", Value(Thickness(16, 6, 16, 6)));
    m_btnPrimary->OnClick().Connect([this](UIElement*) {
        DialogResult res = DialogResult::Primary;
        Hide();
        if (m_callback) m_callback(res);
    });

    m_btnSecondary = std::make_shared<Button>(m_secondaryText);
    m_btnSecondary->SetProperty("background", Value("#3C3C3C"));
    m_btnSecondary->SetProperty("padding", Value(Thickness(16, 6, 16, 6)));
    m_btnSecondary->OnClick().Connect([this](UIElement*) {
        DialogResult res = DialogResult::Secondary;
        Hide();
        if (m_callback) m_callback(res);
    });

    m_btnClose = std::make_shared<Button>(m_closeText);
    m_btnClose->SetProperty("background", Value("#2D2D2D"));
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

void ContentDialog::Show(std::function<void(DialogResult)> callback) {
    m_isOpen = true;
    m_callback = callback;
}

void ContentDialog::Hide() {
    m_isOpen = false;
}

Size ContentDialog::Measure(Size availableSize) {
    return availableSize;
}

void ContentDialog::Arrange(Rect finalRect) {
    m_bounds = finalRect;
}

void ContentDialog::OnRenderOverlay(GraphicsContext& ctx) {
    if (!m_isOpen) return;

    // 1. Get window root dimensions to span full backdrop over entire window area
    Rect windowRect = m_bounds;
    UIElement* root = this;
    while (root->GetParent()) {
        root = root->GetParent();
    }
    if (root) {
        windowRect = root->GetBounds();
    }

    // 2. Draw semi-transparent dark backdrop mask over full window area
    ctx.FillRect(windowRect, D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.65f));

    // 3. Dialog Card Sizing & Centering inside entire window
    float cardW = (std::min)(480.0f, windowRect.width - 40.0f);
    float cardH = 220.0f;
    float cardX = windowRect.x + (windowRect.width - cardW) * 0.5f;
    float cardY = windowRect.y + (windowRect.height - cardH) * 0.5f;

    m_dialogBounds = Rect(cardX, cardY, cardW, cardH);

    // Card Outer Drop Shadow
    ctx.FillRoundedRect(Rect(cardX - 6, cardY - 6, cardW + 12, cardH + 12), 12.0f, D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.45f));

    // Card Background (#202020 WinUI Dark Surface)
    ctx.FillRoundedRect(m_dialogBounds, 8.0f, D2D1::ColorF(0x20 / 255.0f, 0x20 / 255.0f, 0x20 / 255.0f, 1.0f));
    ctx.DrawRoundedRect(m_dialogBounds, 8.0f, D2D1::ColorF(0x44 / 255.0f, 0x44 / 255.0f, 0x44 / 255.0f, 1.0f), 1.0f);

    // 4. Layout Children inside Centered Card
    float innerX = cardX + 24.0f;
    float innerY = cardY + 20.0f;
    float innerW = cardW - 48.0f;

    // Title
    if (m_txtTitle) {
        m_txtTitle->Measure(Size(innerW, 30.0f));
        m_txtTitle->Arrange(Rect(innerX, innerY, innerW, 30.0f));
        m_txtTitle->Render(ctx);
    }

    // Message Text
    if (m_txtMessage) {
        m_txtMessage->Measure(Size(innerW, 80.0f));
        m_txtMessage->Arrange(Rect(innerX, innerY + 36.0f, innerW, 80.0f));
        m_txtMessage->Render(ctx);
    }

    // Footer Buttons Panel (Right aligned)
    float btnY = cardY + cardH - 52.0f;
    float btnRight = cardX + cardW - 24.0f;

    if (!m_closeText.empty() && m_btnClose) {
        Size s = m_btnClose->Measure(Size(innerW, 32.0f));
        float w = (std::max)(80.0f, s.width);
        btnRight -= w;
        m_btnClose->Arrange(Rect(btnRight, btnY, w, 32.0f));
        m_btnClose->Render(ctx);
        btnRight -= 12.0f;
    }

    if (!m_secondaryText.empty() && m_btnSecondary) {
        Size s = m_btnSecondary->Measure(Size(innerW, 32.0f));
        float w = (std::max)(80.0f, s.width);
        btnRight -= w;
        m_btnSecondary->Arrange(Rect(btnRight, btnY, w, 32.0f));
        m_btnSecondary->Render(ctx);
        btnRight -= 12.0f;
    }

    if (!m_primaryText.empty() && m_btnPrimary) {
        Size s = m_btnPrimary->Measure(Size(innerW, 32.0f));
        float w = (std::max)(80.0f, s.width);
        btnRight -= w;
        m_btnPrimary->Arrange(Rect(btnRight, btnY, w, 32.0f));
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
