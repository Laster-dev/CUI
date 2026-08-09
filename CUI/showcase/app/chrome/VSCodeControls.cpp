#include "VSCodeControls.h"
#include "../../../ui/framework/window/Window.h"
#include "../../../ui/framework/window/Dpi.h"
#include "../../../ui/framework/style/ThemeManager.h"

namespace CUI {

// ==========================================
// 1. TitleBar Implementation
// ==========================================
TitleBar::TitleBar() {
    SetHeight(34.0f);
    SetBackgroundToken(ThemeTokenId::PaneBackground);
    SetHoverBackgroundToken(ThemeTokenId::PaneBackground);
    SetPressedBackgroundToken(ThemeTokenId::PaneBackground);
    SetColorToken(ThemeTokenId::TextPrimary);
    SetBackground(ThemeManager::Instance().GetColor("paneBackground"));
    SetHoverBackground(ThemeManager::Instance().GetColor("paneBackground"));
    SetPressedBackground(ThemeManager::Instance().GetColor("paneBackground"));
    SetTitle("CUI - Visual Studio Code [Direct2D UI Engine]");
    m_menuBar.SetParent(this);

    // Populate real interactive MenuBar dropdown menus
    auto fileMenu = m_menuBar.AddMenu("File");
    fileMenu->AddItem("New Text File", "Ctrl+N");
    fileMenu->AddItem("New File...", "Ctrl+Alt+Windows+N");
    fileMenu->AddItem("New Window", "Ctrl+Shift+N");
    fileMenu->AddSeparator();
    fileMenu->AddItem("Open File...", "Ctrl+O");
    fileMenu->AddItem("Open Folder...", "Ctrl+K Ctrl+O");
    fileMenu->AddSeparator();
    fileMenu->AddItem("Save", "Ctrl+S");
    fileMenu->AddItem("Save As...", "Ctrl+Shift+S");
    fileMenu->AddSeparator();
    fileMenu->AddItem("Exit", "Alt+F4");

    auto editMenu = m_menuBar.AddMenu("Edit");
    editMenu->AddItem("Undo", "Ctrl+Z");
    editMenu->AddItem("Redo", "Ctrl+Y");
    editMenu->AddSeparator();
    editMenu->AddItem("Cut", "Ctrl+X");
    editMenu->AddItem("Copy", "Ctrl+C");
    editMenu->AddItem("Paste", "Ctrl+V");
    editMenu->AddSeparator();
    editMenu->AddItem("Find", "Ctrl+F");
    editMenu->AddItem("Replace", "Ctrl+H");

    auto selMenu = m_menuBar.AddMenu("Selection");
    selMenu->AddItem("Select All", "Ctrl+A");
    selMenu->AddItem("Expand Selection", "Shift+Alt+Right");
    selMenu->AddItem("Shrink Selection", "Shift+Alt+Left");

    auto viewMenu = m_menuBar.AddMenu("View");
    viewMenu->AddItem("Command Palette...", "Ctrl+Shift+P");
    viewMenu->AddItem("Open View...", "Ctrl+Q");
    viewMenu->AddSeparator();
    viewMenu->AddItem("Appearance");
    viewMenu->AddItem("Editor Layout");

    auto goMenu = m_menuBar.AddMenu("Go");
    goMenu->AddItem("Back", "Alt+Left");
    goMenu->AddItem("Forward", "Alt+Right");
    goMenu->AddItem("Go to File...", "Ctrl+P");

    auto runMenu = m_menuBar.AddMenu("Run");
    runMenu->AddItem("Start Debugging", "F5");
    runMenu->AddItem("Run Without Debugging", "Ctrl+F5");

    auto termMenu = m_menuBar.AddMenu("Terminal");
    termMenu->AddItem("New Terminal", "Ctrl+Shift+`");
    termMenu->AddItem("Run Task...");

    auto helpMenu = m_menuBar.AddMenu("Help");
    helpMenu->AddItem("Welcome");
    helpMenu->AddItem("Documentation");
    helpMenu->AddItem("About CUI Engine");
}

void TitleBar::OnRender(GraphicsContext& ctx) {
    Control::OnRender(ctx);

    const ThemeTokens& tokens = ThemeManager::Instance().GetTokens();
    const bool lightTheme = ThemeManager::Instance().GetThemeMode() == ThemeMode::Light;

    // Draw app icon
    Rect iconRect(m_bounds.x + 10, m_bounds.y + (m_bounds.height - 18) * 0.5f, 18, 18);
    ctx.FillRoundedRect(iconRect, 4.0f, tokens.accentColor);
    ctx.DrawText("C", iconRect, tokens.accentForeground, "微软雅黑", 11.0f, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER, DWRITE_FONT_WEIGHT_BOLD);

    // Render Real Interactive MenuBar with dynamic content-fit width
    float calcMenuBarW = m_menuBar.GetTotalWidth(ctx);
    Rect menuBarRect(m_bounds.x + 36, m_bounds.y, calcMenuBarW, m_bounds.height);
    m_menuBar.Arrange(menuBarRect);
    m_menuBar.OnRender(ctx);

    // Render Windows 11 Native Vector System Action Buttons (Minimize, Maximize, Close)
    float btnW = 46.0f;
    float btnH = m_bounds.height;
    float rightX = m_bounds.x + m_bounds.width - btnW * 3;

    // Hover uses DIPs — same space as m_bounds / button layout. Raw physical
    // client pixels would light up the wrong caption button under DPI scaling.
    HWND hwnd = ctx.GetHwnd();
    bool isHoveredInTitle = false;
    float hoverX = -1.0f;
    float hoverY = -1.0f;

    if (hwnd && TryGetCursorClientLogical(hwnd, hoverX, hoverY)) {
        isHoveredInTitle = m_bounds.Contains(hoverX, hoverY);
    }

    // Query current window state
    ThemeMode curTheme = ThemeMode::Dark;
    if (hwnd) {
        Window* winObj = reinterpret_cast<Window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
        if (winObj) {
            curTheme = winObj->GetThemeMode();
        }
    }

    const D2D1_COLOR_F titleColor = tokens.textPrimary;
    const D2D1_COLOR_F chromeTextColor = tokens.textPrimary;
    const D2D1_COLOR_F chromeLineColor = tokens.textPrimary;
    const D2D1_COLOR_F subtleChromeBg = lightTheme
        ? D2D1::ColorF(tokens.textPrimary.r, tokens.textPrimary.g, tokens.textPrimary.b, isHoveredInTitle ? 0.08f : 0.04f)
        : D2D1::ColorF(tokens.textPrimary.r, tokens.textPrimary.g, tokens.textPrimary.b, isHoveredInTitle ? 0.14f : 0.08f);
    // Draw title in center
    const std::string& title = GetTitle();
    ctx.DrawText(title, m_bounds, titleColor, "微软雅黑", 12.0f, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    // 1. LowPerf / Animation Toggle Button
    Rect lowPerfHit = GetLowPerformanceToggleRect();
    constexpr float toggleVisualH = 22.0f;
    Rect lowPerfRect(
        lowPerfHit.x,
        lowPerfHit.y + (lowPerfHit.height - toggleVisualH) * 0.5f,
        lowPerfHit.width,
        toggleVisualH
    );
    bool lowPerfOn = !UIElement::AreAnimationsEnabled();
    bool isLowPerfHover = isHoveredInTitle && lowPerfHit.Contains(hoverX, hoverY);
    D2D1_COLOR_F toggleBg = lowPerfOn
        ? D2D1::ColorF(tokens.accentColor.r, tokens.accentColor.g, tokens.accentColor.b, isLowPerfHover ? 0.92f : 0.82f)
        : (lightTheme
            ? D2D1::ColorF(tokens.cardBorder.r, tokens.cardBorder.g, tokens.cardBorder.b, isLowPerfHover ? 0.34f : 0.22f)
            : D2D1::ColorF(tokens.cardBorder.r, tokens.cardBorder.g, tokens.cardBorder.b, isLowPerfHover ? 0.20f : 0.10f));
    D2D1_COLOR_F toggleBorder = lowPerfOn
        ? D2D1::ColorF(tokens.accentColor.r, tokens.accentColor.g, tokens.accentColor.b, 0.95f)
        : (lightTheme
            ? D2D1::ColorF(tokens.cardBorder.r, tokens.cardBorder.g, tokens.cardBorder.b, 0.42f)
            : D2D1::ColorF(tokens.cardBorder.r, tokens.cardBorder.g, tokens.cardBorder.b, 0.24f));
    D2D1_COLOR_F toggleText = lowPerfOn
        ? tokens.accentForeground
        : chromeTextColor;

    ctx.FillRoundedRect(lowPerfRect, 8.0f, toggleBg);
    ctx.DrawRoundedRect(lowPerfRect, 8.0f, toggleBorder, 1.0f);
    ctx.DrawText(
        lowPerfOn ? "低性能" : "动画",
        lowPerfRect,
        toggleText,
        "微软雅黑",
        11.0f,
        DWRITE_TEXT_ALIGNMENT_CENTER,
        DWRITE_PARAGRAPH_ALIGNMENT_CENTER,
        DWRITE_FONT_WEIGHT_SEMI_BOLD
    );

    // 2. Theme Toggle Button
    Rect themeHit = GetThemeToggleRect();
    Rect themeRect(
        themeHit.x,
        themeHit.y + (themeHit.height - toggleVisualH) * 0.5f,
        themeHit.width,
        toggleVisualH
    );
    bool isThemeHover = isHoveredInTitle && themeHit.Contains(hoverX, hoverY);
    const char* themeStr = (curTheme == ThemeMode::Dark) ? "🌙 暗色" : "☀️ 亮色";

    D2D1_COLOR_F themeBg = lightTheme
        ? D2D1::ColorF(tokens.accentColor.r, tokens.accentColor.g, tokens.accentColor.b, isThemeHover ? 0.22f : 0.14f)
        : D2D1::ColorF(tokens.cardBorder.r, tokens.cardBorder.g, tokens.cardBorder.b, isThemeHover ? 0.20f : 0.10f);
    D2D1_COLOR_F themeBorder = lightTheme
        ? D2D1::ColorF(tokens.accentColor.r, tokens.accentColor.g, tokens.accentColor.b, 0.55f)
        : D2D1::ColorF(tokens.cardBorder.r, tokens.cardBorder.g, tokens.cardBorder.b, 0.24f);
    D2D1_COLOR_F themeTextCol = lightTheme ? tokens.accentColor : chromeTextColor;

    ctx.FillRoundedRect(themeRect, 8.0f, themeBg);
    ctx.DrawRoundedRect(themeRect, 8.0f, themeBorder, 1.0f);
    ctx.DrawText(themeStr, themeRect, themeTextCol, "微软雅黑", 11.0f, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER, DWRITE_FONT_WEIGHT_SEMI_BOLD);

    // 1. Minimize Button (_)
    Rect minBtnRect(rightX, m_bounds.y, btnW, btnH);
    bool isMinHover = isHoveredInTitle && (hoverX >= rightX && hoverX < rightX + btnW);
    if (isMinHover) {
        ctx.FillRect(minBtnRect, subtleChromeBg);
    }
    // Draw vector line icon for minimize
    float iconCenterY = m_bounds.y + btnH * 0.5f;
    ctx.DrawLine(Point(rightX + 18.0f, iconCenterY), Point(rightX + 28.0f, iconCenterY), chromeLineColor, 1.0f);

    // 2. Maximize / Restore Button ([ ])
    Rect maxBtnRect(rightX + btnW, m_bounds.y, btnW, btnH);
    bool isMaxHover = isHoveredInTitle && (hoverX >= rightX + btnW && hoverX < rightX + btnW * 2);
    if (isMaxHover) {
        ctx.FillRect(maxBtnRect, subtleChromeBg);
    }
    // Draw vector square icon for maximize
    Rect maxIconRect(rightX + btnW + 18.0f, iconCenterY - 5.0f, 10.0f, 10.0f);
    ctx.DrawRect(maxIconRect, chromeLineColor, 1.0f);

    // 3. Close Button (X)
    Rect closeBtnRect(rightX + btnW * 2, m_bounds.y, btnW, btnH);
    bool isCloseHover = isHoveredInTitle && (hoverX >= rightX + btnW * 2);
    if (isCloseHover) {
        ctx.FillRect(closeBtnRect, tokens.dangerColor);
    }
    // Draw vector cross X icon for close
    D2D1_COLOR_F closeIconColor = isCloseHover ? tokens.accentForeground : chromeLineColor;
    float closeCenterX = rightX + btnW * 2 + 23.0f;
    ctx.DrawLine(Point(closeCenterX - 5.0f, iconCenterY - 5.0f), Point(closeCenterX + 5.0f, iconCenterY + 5.0f), closeIconColor, 1.2f);
    ctx.DrawLine(Point(closeCenterX + 5.0f, iconCenterY - 5.0f), Point(closeCenterX - 5.0f, iconCenterY + 5.0f), closeIconColor, 1.2f);

    // Bottom border line
    ctx.DrawLine(
        Point(m_bounds.x, m_bounds.y + m_bounds.height - 1),
        Point(m_bounds.x + m_bounds.width, m_bounds.y + m_bounds.height - 1),
        tokens.cardBorder
    );
}

void TitleBar::OnMouseDown(Point pt) {
    Control::OnMouseDown(pt);

    if (IsLowPerformanceToggleHit(pt.x, pt.y)) {
        m_onToggleLowPerformance.Invoke(this);
        return;
    }
    if (IsThemeToggleHit(pt.x, pt.y)) {
        m_onToggleTheme.Invoke(this);
        return;
    }
    m_menuBar.OnMouseDown(pt);
}

void TitleBar::OnMouseMove(Point pt) {
    Control::OnMouseMove(pt);
    const int previousHoverRegion = m_hoverRegion;
    m_hoverRegion = HitTestHoverRegion(pt.x, pt.y);
    if (previousHoverRegion != m_hoverRegion) {
        m_menuChromeDirty = true;
        MarkRenderRectDirty(m_bounds.Inflate(2.0f));
    }
    if (m_menuBar.HandleMouseMove(pt)) {
        m_menuChromeDirty = true;
    }
}

void TitleBar::OnMouseLeave() {
    Control::OnMouseLeave();
    if (m_hoverRegion != -1) {
        m_hoverRegion = -1;
        m_menuChromeDirty = true;
        MarkRenderRectDirty(m_bounds.Inflate(2.0f));
    }
    m_menuBar.OnMouseLeave();
}

void TitleBar::OnBlur() {
    Control::OnBlur();
    if (m_hoverRegion != -1) {
        m_hoverRegion = -1;
        m_menuChromeDirty = true;
        MarkRenderRectDirty(m_bounds.Inflate(2.0f));
    }
    m_menuBar.OnBlur();
}

bool TitleBar::IsMenuBarHit(float x, float y) const {
    return const_cast<MenuBar&>(m_menuBar).HitTest(x, y) != nullptr;
}

bool TitleBar::IsInteractiveHit(float x, float y) const {
    return IsLowPerformanceToggleHit(x, y) || IsThemeToggleHit(x, y) || IsMenuBarHit(x, y);
}

bool TitleBar::IsCaptionDragHit(float x, float y, UIElement* treeHit) const {
    if (!m_bounds.Contains(x, y)) {
        return false;
    }
    if (IsInteractiveHit(x, y)
        || GetMinimizeButtonRect().Contains(x, y)
        || GetMaximizeButtonRect().Contains(x, y)
        || GetCloseButtonRect().Contains(x, y)) {
        return false;
    }
    return !treeHit || (treeHit == this);
}

LRESULT TitleBar::HitTestNonClient(float x, float y) const {
    if (!m_bounds.Contains(x, y)) {
        return HTNOWHERE;
    }
    if (GetCloseButtonRect().Contains(x, y)) {
        return HTCLOSE;
    }
    if (GetMaximizeButtonRect().Contains(x, y)) {
        return HTMAXBUTTON;
    }
    if (GetMinimizeButtonRect().Contains(x, y)) {
        return HTMINBUTTON;
    }
    return HTNOWHERE;
}

Rect TitleBar::GetLowPerformanceToggleRect() const {
    constexpr float buttonWidth = 46.0f;
    constexpr float toggleWidth = 60.0f;
    constexpr float toggleGap = 6.0f;
    float rightX = m_bounds.x + m_bounds.width - buttonWidth * 3.0f;
    float x = rightX - toggleGap - toggleWidth;
    // Full title-bar height so top resize band / caption drag cannot steal clicks.
    return Rect(x, m_bounds.y, toggleWidth, m_bounds.height);
}

bool TitleBar::IsLowPerformanceToggleHit(float x, float y) const {
    return GetLowPerformanceToggleRect().Contains(x, y);
}

Rect TitleBar::GetThemeToggleRect() const {
    Rect lowPerfRect = GetLowPerformanceToggleRect();
    constexpr float toggleWidth = 68.0f;
    constexpr float toggleGap = 6.0f;
    float x = lowPerfRect.x - toggleGap - toggleWidth;
    return Rect(x, lowPerfRect.y, toggleWidth, lowPerfRect.height);
}

bool TitleBar::IsThemeToggleHit(float x, float y) const {
    return GetThemeToggleRect().Contains(x, y);
}

Rect TitleBar::GetMinimizeButtonRect() const {
    constexpr float buttonWidth = 46.0f;
    return Rect(m_bounds.x + m_bounds.width - buttonWidth * 3.0f, m_bounds.y, buttonWidth, m_bounds.height);
}

Rect TitleBar::GetMaximizeButtonRect() const {
    constexpr float buttonWidth = 46.0f;
    return Rect(m_bounds.x + m_bounds.width - buttonWidth * 2.0f, m_bounds.y, buttonWidth, m_bounds.height);
}

Rect TitleBar::GetCloseButtonRect() const {
    constexpr float buttonWidth = 46.0f;
    return Rect(m_bounds.x + m_bounds.width - buttonWidth, m_bounds.y, buttonWidth, m_bounds.height);
}

int TitleBar::HitTestHoverRegion(float x, float y) const {
    if (GetCloseButtonRect().Contains(x, y)) {
        return 5;
    }
    if (GetMaximizeButtonRect().Contains(x, y)) {
        return 4;
    }
    if (GetMinimizeButtonRect().Contains(x, y)) {
        return 3;
    }
    if (IsThemeToggleHit(x, y)) {
        return 2;
    }
    if (IsLowPerformanceToggleHit(x, y)) {
        return 1;
    }
    if (IsMenuBarHit(x, y)) {
        return 0;
    }
    return -1;
}

bool TitleBar::ConsumeChromeDirty() {
    const bool dirty = m_menuChromeDirty;
    m_menuChromeDirty = false;
    return dirty;
}

UIElement* TitleBar::HitTest(float x, float y) {
    if (IsInteractiveHit(x, y)) {
        return this;
    }
    UIElement* mbHit = m_menuBar.HitTest(x, y);
    if (mbHit) return this;
    return Control::HitTest(x, y);
}

// ==========================================
// 2. ActivityBar Implementation
// ==========================================
ActivityBar::ActivityBar() {
    SetWidth(48.0f);
    SetBackgroundToken(ThemeTokenId::PaneBackground);
    SetBackground(ThemeManager::Instance().GetTokens().paneBackground);

    m_items = {
        { "[E]", "Explorer" },
        { "[S]", "Search" },
        { "[G]", "Source Control" },
        { "[R]", "Run & Debug" },
        { "[X]", "Extensions" }
    };
}

void ActivityBar::OnRender(GraphicsContext& ctx) {
    Control::OnRender(ctx);

    const ThemeTokens& tokens = ThemeManager::Instance().GetTokens();
    float itemHeight = 48.0f;
    float y = m_bounds.y;

    for (size_t i = 0; i < m_items.size(); ++i) {
        Rect itemRect(m_bounds.x, y, m_bounds.width, itemHeight);
        bool isSelected = (static_cast<int>(i) == m_selectedIndex);

        if (isSelected) {
            // Draw left active indicator blue bar
            ctx.FillRect(Rect(m_bounds.x, y, 2.0f, itemHeight), tokens.accentColor);
        }

        D2D1_COLOR_F iconColor = isSelected ? tokens.accentForeground : tokens.textMuted;
        ctx.DrawText(m_items[i].icon, itemRect, iconColor, "微软雅黑", 16.0f, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER, isSelected ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL);

        y += itemHeight;
    }

    // Draw settings icon at bottom
    Rect gearRect(m_bounds.x, m_bounds.y + m_bounds.height - 48.0f, m_bounds.width, 48.0f);
    ctx.DrawText("[*]", gearRect, tokens.textMuted, "微软雅黑", 16.0f, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    // Right border line
    ctx.DrawLine(Point(m_bounds.x + m_bounds.width - 1, m_bounds.y), Point(m_bounds.x + m_bounds.width - 1, m_bounds.y + m_bounds.height), tokens.cardBackground);
}

void ActivityBar::OnMouseDown(Point pt) {
    Control::OnMouseDown(pt);

    float relY = pt.y - m_bounds.y;
    int idx = static_cast<int>(relY / 48.0f);
    if (idx >= 0 && idx < static_cast<int>(m_items.size())) {
        m_selectedIndex = idx;
    }
}

// ==========================================
// 3. SideBar Implementation
// ==========================================
SideBar::SideBar() {
    SetWidth(240.0f);
    SetBackgroundToken(ThemeTokenId::PaneBackground);
    SetBackground(ThemeManager::Instance().GetColor("paneBackground"));
    SetTitle("EXPLORER: CUI PROJECT");

    m_fileTree = {
        { ">", "CUI", 0, true, true },
        { "v", "framework", 1, true, true },
        { "#", "core", 2, true, true },
        { "c", "Object.h", 3, false, false },
        { "c", "Value.h", 3, false, false },
        { "c", "Binding.h", 3, false, false },
        { "#", "render", 2, true, true },
        { "c", "GraphicsContext.h", 3, false, false },
        { "c", "RenderResources.h", 3, false, false },
        { "#", "layout", 2, true, true },
        { "c", "Layout.h", 3, false, false },
        { "#", "parser", 2, true, true },
        { "c", "StyleManager.h", 3, false, false },
        { "#", "controls", 2, true, true },
        { "c", "UIElement.h", 3, false, false },
        { "c", "VSCodeControls.h", 3, false, false },
        { "v", "showcase", 1, true, true },
        { "c", "ShowcaseHelpers.cpp", 2, false, false },
        { "c", "main.cpp", 2, false, false }
    };
}

void SideBar::OnRender(GraphicsContext& ctx) {
    Control::OnRender(ctx);

    const ThemeTokens& tokens = ThemeManager::Instance().GetTokens();

    // Section title bar
    const std::string& title = GetTitle();
    Rect headerRect(m_bounds.x + 16, m_bounds.y + 8, m_bounds.width - 32, 24);
    ctx.DrawText(title, headerRect, tokens.textSecondary, "微软雅黑", 11.0f, DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER, DWRITE_FONT_WEIGHT_BOLD);

    // Draw file tree items
    float itemHeight = 22.0f;
    float y = m_bounds.y + 36.0f;

    for (size_t i = 0; i < m_fileTree.size(); ++i) {
        if (y + itemHeight > m_bounds.y + m_bounds.height) break;

        const auto& item = m_fileTree[i];
        Rect itemRect(m_bounds.x, y, m_bounds.width, itemHeight);

        bool isSelected = (static_cast<int>(i) == m_selectedIndex);
        if (isSelected) {
            ctx.FillRect(itemRect, tokens.hoverBackground);
        }

        float indent = m_bounds.x + 12.0f + item.level * 12.0f;

        // Draw icon
        D2D1_COLOR_F iconColor = item.isFolder ? tokens.textMuted : tokens.accentColor;
        ctx.DrawText(item.icon, Rect(indent, y, 16, itemHeight), iconColor, "微软雅黑", 12.0f, DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

        // Draw name
        D2D1_COLOR_F textColor = isSelected ? tokens.textPrimary : tokens.textSecondary;
        ctx.DrawText(item.name, Rect(indent + 18.0f, y, m_bounds.width - indent - 18.0f, itemHeight), textColor, "微软雅黑", 12.0f, DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

        y += itemHeight;
    }

    // Right border line
    ctx.DrawLine(Point(m_bounds.x + m_bounds.width - 1, m_bounds.y), Point(m_bounds.x + m_bounds.width - 1, m_bounds.y + m_bounds.height), tokens.windowBackground);
}

void SideBar::OnMouseDown(Point pt) {
    Control::OnMouseDown(pt);

    float relY = pt.y - (m_bounds.y + 36.0f);
    if (relY >= 0) {
        int idx = static_cast<int>(relY / 22.0f);
        if (idx >= 0 && idx < static_cast<int>(m_fileTree.size())) {
            m_selectedIndex = idx;
        }
    }
}

// ==========================================
// 4. TabBar Implementation
// ==========================================
TabBar::TabBar() {
    SetHeight(35.0f);
    SetBackgroundToken(ThemeTokenId::PaneBackground);
    SetBackground(ThemeManager::Instance().GetColor("paneBackground"));

    m_tabs = {
        { "c", "GraphicsContext.cpp", true },
        { "c", "ShowcaseHelpers.cpp", false },
        { "c", "main.cpp", false }
    };
}

void TabBar::AddTab(const std::string& icon, const std::string& title, bool modified) {
    m_tabs.push_back({ icon, title, modified });
}

void TabBar::OnRender(GraphicsContext& ctx) {
    Control::OnRender(ctx);

    const ThemeTokens& tokens = ThemeManager::Instance().GetTokens();
    float tabX = m_bounds.x;

    for (size_t i = 0; i < m_tabs.size(); ++i) {
        const auto& tab = m_tabs[i];
        bool isActive = (static_cast<int>(i) == m_activeIndex);

        Size titleSize = ctx.MeasureText(tab.title, "微软雅黑", 12.0f);
        float tabW = titleSize.width + 50.0f;
        Rect tabRect(tabX, m_bounds.y, tabW, m_bounds.height);

        D2D1_COLOR_F tabBg = isActive ? tokens.windowBackground : tokens.inputBackground;
        ctx.FillRect(tabRect, tabBg);

        if (isActive) {
            // Active top border highlight bar
            ctx.FillRect(Rect(tabX, m_bounds.y, tabW, 2.0f), tokens.accentColor);
        }

        // Draw icon
        D2D1_COLOR_F iconColor = (tab.icon == "x") ? tokens.dangerColor : tokens.accentColor;
        ctx.DrawText(tab.icon, Rect(tabX + 10, m_bounds.y, 16, m_bounds.height), iconColor, "微软雅黑", 12.0f, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

        // Draw title
        D2D1_COLOR_F textColor = isActive ? tokens.textPrimary : tokens.textMuted;
        ctx.DrawText(tab.title, Rect(tabX + 30, m_bounds.y, titleSize.width + 5, m_bounds.height), textColor, "微软雅黑", 12.0f, DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

        // Draw modified dot or close 'x'
        std::string rightSymbol = tab.isModified ? "o" : "x";
        D2D1_COLOR_F closeColor = tab.isModified ? tokens.textPrimary : tokens.textMuted;
        ctx.DrawText(rightSymbol, Rect(tabX + tabW - 20, m_bounds.y, 16, m_bounds.height), closeColor, "微软雅黑", 10.0f, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

        // Right separator line
        ctx.DrawLine(Point(tabX + tabW - 1, m_bounds.y), Point(tabX + tabW - 1, m_bounds.y + m_bounds.height), tokens.cardBackground);

        tabX += tabW;
    }
}

void TabBar::OnMouseDown(Point pt) {
    Control::OnMouseDown(pt);

    float tabX = m_bounds.x;
    GraphicsContext ctx;

    for (size_t i = 0; i < m_tabs.size(); ++i) {
        Size titleSize = ctx.MeasureText(m_tabs[i].title, "微软雅黑", 12.0f);
        float tabW = titleSize.width + 50.0f;
        if (pt.x >= tabX && pt.x <= tabX + tabW) {
            m_activeIndex = static_cast<int>(i);
            break;
        }
        tabX += tabW;
    }
}

// ==========================================
// 5. EditorView Implementation
// ==========================================
EditorView::EditorView() {
    SetBackgroundToken(ThemeTokenId::WindowBackground);
    SetBackground(ThemeManager::Instance().GetColor("windowBackground"));

    m_lines = {
        "// Direct2D High-Performance Render Loop",
        "#include \"GraphicsContext.h\"",
        "#include <d2d1_1.h>",
        "",
        "namespace CUI {",
        "",
        "void GraphicsContext::BeginDraw() {",
        "    if (m_d2dContext) {",
        "        m_d2dContext->BeginDraw();",
        "        m_d2dContext->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);",
        "        m_d2dContext->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE);",
        "    }",
        "}",
        "",
        "HRESULT GraphicsContext::EndDraw() {",
        "    if (!m_d2dContext) return E_POINTER;",
        "    HRESULT hr = m_d2dContext->EndDraw();",
        "    if (m_swapChain) {",
        "        m_swapChain->Present(1, 0); // 60FPS VSync",
        "    }",
        "    return hr;",
        "}",
        "",
        "} // namespace CUI"
    };
}

void EditorView::OnRender(GraphicsContext& ctx) {
    Control::OnRender(ctx);

    const ThemeTokens& tokens = ThemeManager::Instance().GetTokens();

    // 1. Breadcrumb bar at top
    Rect breadcrumbRect(m_bounds.x, m_bounds.y, m_bounds.width, 22.0f);
    ctx.FillRect(breadcrumbRect, tokens.windowBackground);
    ctx.DrawText("src > framework > render > GraphicsContext.cpp > GraphicsContext::BeginDraw", Rect(m_bounds.x + 12, m_bounds.y, m_bounds.width, 22.0f), tokens.textMuted, "Consolas", 11.0f, DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    ctx.DrawLine(Point(m_bounds.x, m_bounds.y + 21), Point(m_bounds.x + m_bounds.width, m_bounds.y + 21), tokens.cardBorder);

    // 2. Line Numbers column & Code Area
    float startY = m_bounds.y + 26.0f;
    float lineHeight = 19.0f;
    float lineNoW = 48.0f;

    for (size_t i = 0; i < m_lines.size(); ++i) {
        float y = startY + i * lineHeight;
        if (y + lineHeight > m_bounds.y + m_bounds.height) break;

        bool isCurrentLine = (static_cast<int>(i + 1) == m_cursorLine);

        // Highlight current line
        if (isCurrentLine) {
            ctx.FillRect(Rect(m_bounds.x, y, m_bounds.width, lineHeight), tokens.hoverBackground);
            ctx.DrawRect(Rect(m_bounds.x, y, m_bounds.width, lineHeight), tokens.cardBorder);
        }

        // Line number
        std::string lineNoStr = std::to_string(i + 1);
        D2D1_COLOR_F lineNoColor = isCurrentLine ? tokens.textSecondary : tokens.textMuted;
        ctx.DrawText(lineNoStr, Rect(m_bounds.x, y, lineNoW - 8, lineHeight), lineNoColor, "Consolas", 12.0f, DWRITE_TEXT_ALIGNMENT_TRAILING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

        // Code text
        const std::string& codeStr = m_lines[i];
        D2D1_COLOR_F codeColor = tokens.textPrimary;
        if (!codeStr.empty() && codeStr[0] == '#') {
            codeColor = tokens.accentColor;
        } else if (!codeStr.empty() && codeStr.find("//") == 0) {
            codeColor = tokens.textMuted;
        }

        ctx.DrawText(codeStr, Rect(m_bounds.x + lineNoW + 12, y, m_bounds.width - lineNoW - 12, lineHeight), codeColor, "Consolas", 13.0f, DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

        // Draw cursor line indicator if current line
        if (isCurrentLine) {
            float cursorX = m_bounds.x + lineNoW + 12 + (m_cursorCol - 1) * 7.5f;
            ctx.FillRect(Rect(cursorX, y + 2, 2.0f, lineHeight - 4), tokens.textSecondary);
        }
    }
}

void EditorView::OnMouseDown(Point pt) {
    Control::OnMouseDown(pt);

    float relY = pt.y - (m_bounds.y + 26.0f);
    if (relY >= 0) {
        int line = static_cast<int>(relY / 19.0f) + 1;
        if (line >= 1 && line <= static_cast<int>(m_lines.size())) {
            m_cursorLine = line;
        }
    }
}

// ==========================================
// 6. StatusBar Implementation
// ==========================================
StatusBar::StatusBar() {
    SetHeight(22.0f);
    SetBackgroundToken(ThemeTokenId::AccentColor);
    SetColorToken(ThemeTokenId::AccentForeground);
    SetBackground(ThemeManager::Instance().GetColor("accentColor"));
    SetColor(ThemeManager::Instance().GetTokens().accentForeground);
}

void StatusBar::OnRender(GraphicsContext& ctx) {
    Control::OnRender(ctx);

    D2D1_COLOR_F textColor = ResolveThemeColor(GetColorToken(), ThemeTokenId::AccentForeground);

    // Left items: Git Branch
    std::string branchStr = "[git] " + GetBranch();
    ctx.DrawText(branchStr, Rect(m_bounds.x + 10, m_bounds.y, 120, m_bounds.height), textColor, "微软雅黑", 11.0f, DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    // Errors & Warnings
    ctx.DrawText("(x) 0  (!) 0", Rect(m_bounds.x + 130, m_bounds.y, 100, m_bounds.height), textColor, "微软雅黑", 11.0f, DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    // Right items: Ln, Col, Spaces, UTF-8, C++
    int line = GetLine();
    int col = GetCol();
    std::string posStr = "Ln " + std::to_string(line) + ", Col " + std::to_string(col);

    float rightX = m_bounds.x + m_bounds.width - 320;
    ctx.DrawText(posStr, Rect(rightX, m_bounds.y, 100, m_bounds.height), textColor, "微软雅黑", 11.0f, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    ctx.DrawText("Spaces: 4", Rect(rightX + 100, m_bounds.y, 70, m_bounds.height), textColor, "微软雅黑", 11.0f, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    ctx.DrawText("UTF-8", Rect(rightX + 170, m_bounds.y, 60, m_bounds.height), textColor, "微软雅黑", 11.0f, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    ctx.DrawText("C++20", Rect(rightX + 230, m_bounds.y, 60, m_bounds.height), textColor, "微软雅黑", 11.0f, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
}

} // namespace CUI
