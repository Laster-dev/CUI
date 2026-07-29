#include "VSCodeControls.h"

namespace CUI {

// ==========================================
// 1. TitleBar Implementation
// ==========================================
TitleBar::TitleBar() {
    SetProperty("height", Value(34.0f));
    SetProperty("background", Value(D2D1::ColorF(0x1F / 255.0f, 0x1F / 255.0f, 0x1F / 255.0f, 1.0f))); // VS Code Title bar dark
    SetProperty("title", Value("CUI - Visual Studio Code [Direct2D UI Engine]"));
}

void TitleBar::OnRender(GraphicsContext& ctx) {
    Control::OnRender(ctx);

    // Draw app icon
    Rect iconRect(m_bounds.x + 10, m_bounds.y + (m_bounds.height - 18) * 0.5f, 18, 18);
    ctx.FillRoundedRect(iconRect, 4.0f, D2D1::ColorF(0x00 / 255.0f, 0x7A / 255.0f, 0xCC / 255.0f, 1.0f));
    ctx.DrawText("C", iconRect, D2D1::ColorF(1.0f, 1.0f, 1.0f), "Segoe UI", 11.0f, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER, DWRITE_FONT_WEIGHT_BOLD);

    // Draw menu items
    std::vector<std::string> menus = { "File", "Edit", "Selection", "View", "Go", "Run", "Terminal", "Help" };
    float menuX = m_bounds.x + 36;
    D2D1_COLOR_F menuColor = D2D1::ColorF(0xCC / 255.0f, 0xCC / 255.0f, 0xCC / 255.0f);

    for (const auto& item : menus) {
        Size sz = ctx.MeasureText(item, "Segoe UI", 12.0f);
        Rect itemRect(menuX, m_bounds.y, sz.width + 12, m_bounds.height);
        ctx.DrawText(item, itemRect, menuColor, "Segoe UI", 12.0f, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        menuX += sz.width + 12;
    }

    // Draw title in center
    std::string title = GetProperty("title").AsString();
    ctx.DrawText(title, m_bounds, D2D1::ColorF(0x99 / 255.0f, 0x99 / 255.0f, 0x99 / 255.0f), "Segoe UI", 12.0f, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    // Render Windows 11 Native Vector System Action Buttons (Minimize, Maximize, Close)
    float btnW = 46.0f;
    float btnH = m_bounds.height;
    float rightX = m_bounds.x + m_bounds.width - btnW * 3;

    // Detect mouse hover state based on cursor position
    POINT pt;
    GetCursorPos(&pt);
    HWND hwnd = WindowFromPoint(pt);
    bool isHoveredInTitle = false;
    float hoverX = -1.0f;

    // Check if mouse is within titlebar action buttons
    RECT windowRc;
    if (GetWindowRect(hwnd, &windowRc)) {
        float clientX = static_cast<float>(pt.x - windowRc.left);
        float clientY = static_cast<float>(pt.y - windowRc.top);
        if (clientY >= 0 && clientY <= m_bounds.height) {
            isHoveredInTitle = true;
            hoverX = clientX;
        }
    }

    // 1. Minimize Button (_)
    Rect minBtnRect(rightX, m_bounds.y, btnW, btnH);
    bool isMinHover = isHoveredInTitle && (hoverX >= rightX && hoverX < rightX + btnW);
    if (isMinHover) {
        ctx.FillRect(minBtnRect, D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.1f)); // Hover grey background
    }
    // Draw vector line icon for minimize
    float iconCenterY = m_bounds.y + btnH * 0.5f;
    ctx.DrawLine(Point(rightX + 18.0f, iconCenterY), Point(rightX + 28.0f, iconCenterY), D2D1::ColorF(0xE0 / 255.0f, 0xE0 / 255.0f, 0xE0 / 255.0f), 1.0f);

    // 2. Maximize / Restore Button ([ ])
    Rect maxBtnRect(rightX + btnW, m_bounds.y, btnW, btnH);
    bool isMaxHover = isHoveredInTitle && (hoverX >= rightX + btnW && hoverX < rightX + btnW * 2);
    if (isMaxHover) {
        ctx.FillRect(maxBtnRect, D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.1f)); // Hover grey background
    }
    // Draw vector square icon for maximize
    Rect maxIconRect(rightX + btnW + 18.0f, iconCenterY - 5.0f, 10.0f, 10.0f);
    ctx.DrawRect(maxIconRect, D2D1::ColorF(0xE0 / 255.0f, 0xE0 / 255.0f, 0xE0 / 255.0f), 1.0f);

    // 3. Close Button (X)
    Rect closeBtnRect(rightX + btnW * 2, m_bounds.y, btnW, btnH);
    bool isCloseHover = isHoveredInTitle && (hoverX >= rightX + btnW * 2);
    if (isCloseHover) {
        ctx.FillRect(closeBtnRect, D2D1::ColorF(0xC4 / 255.0f, 0x2B / 255.0f, 0x1C / 255.0f, 1.0f)); // Win11 Hover Red background (#C42B1C)
    }
    // Draw vector cross X icon for close
    D2D1_COLOR_F closeIconColor = isCloseHover ? D2D1::ColorF(1.0f, 1.0f, 1.0f) : D2D1::ColorF(0xE0 / 255.0f, 0xE0 / 255.0f, 0xE0 / 255.0f);
    float closeCenterX = rightX + btnW * 2 + 23.0f;
    ctx.DrawLine(Point(closeCenterX - 5.0f, iconCenterY - 5.0f), Point(closeCenterX + 5.0f, iconCenterY + 5.0f), closeIconColor, 1.2f);
    ctx.DrawLine(Point(closeCenterX + 5.0f, iconCenterY - 5.0f), Point(closeCenterX - 5.0f, iconCenterY + 5.0f), closeIconColor, 1.2f);

    // Bottom border line
    ctx.DrawLine(Point(m_bounds.x, m_bounds.y + m_bounds.height - 1), Point(m_bounds.x + m_bounds.width, m_bounds.y + m_bounds.height - 1), D2D1::ColorF(0x2B / 255.0f, 0x2B / 255.0f, 0x2B / 255.0f, 1.0f));
}

// ==========================================
// 2. ActivityBar Implementation
// ==========================================
ActivityBar::ActivityBar() {
    SetProperty("width", Value(48.0f));
    SetProperty("background", Value(D2D1::ColorF(0x33 / 255.0f, 0x33 / 255.0f, 0x33 / 255.0f, 1.0f))); // VS Code Activity bar dark

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

    float itemHeight = 48.0f;
    float y = m_bounds.y;

    for (size_t i = 0; i < m_items.size(); ++i) {
        Rect itemRect(m_bounds.x, y, m_bounds.width, itemHeight);
        bool isSelected = (static_cast<int>(i) == m_selectedIndex);

        if (isSelected) {
            // Draw left active indicator blue bar
            ctx.FillRect(Rect(m_bounds.x, y, 2.0f, itemHeight), D2D1::ColorF(0x00 / 255.0f, 0x7A / 255.0f, 0xCC / 255.0f));
        }

        D2D1_COLOR_F iconColor = isSelected ? D2D1::ColorF(1.0f, 1.0f, 1.0f) : D2D1::ColorF(0x85 / 255.0f, 0x85 / 255.0f, 0x85 / 255.0f);
        ctx.DrawText(m_items[i].icon, itemRect, iconColor, "Segoe UI", 16.0f, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER, isSelected ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL);

        y += itemHeight;
    }

    // Draw settings icon at bottom
    Rect gearRect(m_bounds.x, m_bounds.y + m_bounds.height - 48.0f, m_bounds.width, 48.0f);
    ctx.DrawText("[*]", gearRect, D2D1::ColorF(0x85 / 255.0f, 0x85 / 255.0f, 0x85 / 255.0f), "Segoe UI", 16.0f, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    // Right border line
    ctx.DrawLine(Point(m_bounds.x + m_bounds.width - 1, m_bounds.y), Point(m_bounds.x + m_bounds.width - 1, m_bounds.y + m_bounds.height), D2D1::ColorF(0x25 / 255.0f, 0x25 / 255.0f, 0x26 / 255.0f));
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
    SetProperty("width", Value(240.0f));
    SetProperty("background", Value(D2D1::ColorF(0x25 / 255.0f, 0x25 / 255.0f, 0x26 / 255.0f, 1.0f))); // SideBar dark background
    SetProperty("title", Value("EXPLORER: CUI PROJECT"));

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
        { "c", "UIMarkupParser.h", 3, false, false },
        { "#", "controls", 2, true, true },
        { "c", "UIElement.h", 3, false, false },
        { "c", "VSCodeControls.h", 3, false, false },
        { "v", "assets", 1, true, true },
        { "x", "vscode_layout.xml", 2, false, false },
        { "c", "CUI.cpp", 1, false, false }
    };
}

void SideBar::OnRender(GraphicsContext& ctx) {
    Control::OnRender(ctx);

    // Section title bar
    std::string title = GetProperty("title").AsString();
    Rect headerRect(m_bounds.x + 16, m_bounds.y + 8, m_bounds.width - 32, 24);
    ctx.DrawText(title, headerRect, D2D1::ColorF(0xBB / 255.0f, 0xBB / 255.0f, 0xBB / 255.0f), "Segoe UI", 11.0f, DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER, DWRITE_FONT_WEIGHT_BOLD);

    // Draw file tree items
    float itemHeight = 22.0f;
    float y = m_bounds.y + 36.0f;

    for (size_t i = 0; i < m_fileTree.size(); ++i) {
        if (y + itemHeight > m_bounds.y + m_bounds.height) break;

        const auto& item = m_fileTree[i];
        Rect itemRect(m_bounds.x, y, m_bounds.width, itemHeight);

        bool isSelected = (static_cast<int>(i) == m_selectedIndex);
        if (isSelected) {
            ctx.FillRect(itemRect, D2D1::ColorF(0x37 / 255.0f, 0x37 / 255.0f, 0x3D / 255.0f));
        }

        float indent = m_bounds.x + 12.0f + item.level * 12.0f;

        // Draw icon
        D2D1_COLOR_F iconColor = item.isFolder ? D2D1::ColorF(0xDC / 255.0f, 0xA6 / 255.0f, 0x62 / 255.0f) : D2D1::ColorF(0x51 / 255.0f, 0x9A / 255.0f, 0xD9 / 255.0f);
        ctx.DrawText(item.icon, Rect(indent, y, 16, itemHeight), iconColor, "Segoe UI", 12.0f, DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

        // Draw name
        D2D1_COLOR_F textColor = isSelected ? D2D1::ColorF(1.0f, 1.0f, 1.0f) : D2D1::ColorF(0xCC / 255.0f, 0xCC / 255.0f, 0xCC / 255.0f);
        ctx.DrawText(item.name, Rect(indent + 18.0f, y, m_bounds.width - indent - 18.0f, itemHeight), textColor, "Segoe UI", 12.0f, DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

        y += itemHeight;
    }

    // Right border line
    ctx.DrawLine(Point(m_bounds.x + m_bounds.width - 1, m_bounds.y), Point(m_bounds.x + m_bounds.width - 1, m_bounds.y + m_bounds.height), D2D1::ColorF(0x1E / 255.0f, 0x1E / 255.0f, 0x1E / 255.0f));
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
    SetProperty("height", Value(35.0f));
    SetProperty("background", Value(D2D1::ColorF(0x25 / 255.0f, 0x25 / 255.0f, 0x26 / 255.0f, 1.0f))); // Inactive tabs background

    m_tabs = {
        { "c", "GraphicsContext.cpp", true },
        { "x", "vscode_layout.xml", false },
        { "c", "CUI.cpp", false }
    };
}

void TabBar::AddTab(const std::string& icon, const std::string& title, bool modified) {
    m_tabs.push_back({ icon, title, modified });
}

void TabBar::OnRender(GraphicsContext& ctx) {
    Control::OnRender(ctx);

    float tabX = m_bounds.x;

    for (size_t i = 0; i < m_tabs.size(); ++i) {
        const auto& tab = m_tabs[i];
        bool isActive = (static_cast<int>(i) == m_activeIndex);

        Size titleSize = ctx.MeasureText(tab.title, "Segoe UI", 12.0f);
        float tabW = titleSize.width + 50.0f;
        Rect tabRect(tabX, m_bounds.y, tabW, m_bounds.height);

        D2D1_COLOR_F tabBg = isActive ? D2D1::ColorF(0x1E / 255.0f, 0x1E / 255.0f, 0x1E / 255.0f) : D2D1::ColorF(0x2D / 255.0f, 0x2D / 255.0f, 0x2D / 255.0f);
        ctx.FillRect(tabRect, tabBg);

        if (isActive) {
            // Active top border blue highlight bar
            ctx.FillRect(Rect(tabX, m_bounds.y, tabW, 2.0f), D2D1::ColorF(0x00 / 255.0f, 0x7A / 255.0f, 0xCC / 255.0f));
        }

        // Draw icon
        D2D1_COLOR_F iconColor = (tab.icon == "x") ? D2D1::ColorF(0xE3 / 255.0f, 0x66 / 255.0f, 0x29 / 255.0f) : D2D1::ColorF(0x51 / 255.0f, 0x9A / 255.0f, 0xD9 / 255.0f);
        ctx.DrawText(tab.icon, Rect(tabX + 10, m_bounds.y, 16, m_bounds.height), iconColor, "Segoe UI", 12.0f, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

        // Draw title
        D2D1_COLOR_F textColor = isActive ? D2D1::ColorF(1.0f, 1.0f, 1.0f) : D2D1::ColorF(0x96 / 255.0f, 0x96 / 255.0f, 0x96 / 255.0f);
        ctx.DrawText(tab.title, Rect(tabX + 30, m_bounds.y, titleSize.width + 5, m_bounds.height), textColor, "Segoe UI", 12.0f, DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

        // Draw modified dot or close 'x'
        std::string rightSymbol = tab.isModified ? "o" : "x";
        D2D1_COLOR_F closeColor = tab.isModified ? D2D1::ColorF(1.0f, 1.0f, 1.0f) : D2D1::ColorF(0x85 / 255.0f, 0x85 / 255.0f, 0x85 / 255.0f);
        ctx.DrawText(rightSymbol, Rect(tabX + tabW - 20, m_bounds.y, 16, m_bounds.height), closeColor, "Segoe UI", 10.0f, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

        // Right separator line
        ctx.DrawLine(Point(tabX + tabW - 1, m_bounds.y), Point(tabX + tabW - 1, m_bounds.y + m_bounds.height), D2D1::ColorF(0x25 / 255.0f, 0x25 / 255.0f, 0x26 / 255.0f));

        tabX += tabW;
    }
}

void TabBar::OnMouseDown(Point pt) {
    Control::OnMouseDown(pt);

    float tabX = m_bounds.x;
    GraphicsContext ctx;

    for (size_t i = 0; i < m_tabs.size(); ++i) {
        Size titleSize = ctx.MeasureText(m_tabs[i].title, "Segoe UI", 12.0f);
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
    SetProperty("background", Value(D2D1::ColorF(0x1E / 255.0f, 0x1E / 255.0f, 0x1E / 255.0f, 1.0f))); // Main editor dark background

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

    // 1. Breadcrumb bar at top
    Rect breadcrumbRect(m_bounds.x, m_bounds.y, m_bounds.width, 22.0f);
    ctx.FillRect(breadcrumbRect, D2D1::ColorF(0x1E / 255.0f, 0x1E / 255.0f, 0x1E / 255.0f));
    ctx.DrawText("src > framework > render > GraphicsContext.cpp > GraphicsContext::BeginDraw", Rect(m_bounds.x + 12, m_bounds.y, m_bounds.width, 22.0f), D2D1::ColorF(0xA0 / 255.0f, 0xA0 / 255.0f, 0xA0 / 255.0f), "Consolas", 11.0f, DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    ctx.DrawLine(Point(m_bounds.x, m_bounds.y + 21), Point(m_bounds.x + m_bounds.width, m_bounds.y + 21), D2D1::ColorF(0x28 / 255.0f, 0x28 / 255.0f, 0x28 / 255.0f));

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
            ctx.FillRect(Rect(m_bounds.x, y, m_bounds.width, lineHeight), D2D1::ColorF(0x28 / 255.0f, 0x28 / 255.0f, 0x28 / 255.0f));
            ctx.DrawRect(Rect(m_bounds.x, y, m_bounds.width, lineHeight), D2D1::ColorF(0x35 / 255.0f, 0x35 / 255.0f, 0x35 / 255.0f));
        }

        // Line number
        std::string lineNoStr = std::to_string(i + 1);
        D2D1_COLOR_F lineNoColor = isCurrentLine ? D2D1::ColorF(0xC6 / 255.0f, 0xC6 / 255.0f, 0xC6 / 255.0f) : D2D1::ColorF(0x85 / 255.0f, 0x85 / 255.0f, 0x85 / 255.0f);
        ctx.DrawText(lineNoStr, Rect(m_bounds.x, y, lineNoW - 8, lineHeight), lineNoColor, "Consolas", 12.0f, DWRITE_TEXT_ALIGNMENT_TRAILING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

        // Code text
        const std::string& codeStr = m_lines[i];
        D2D1_COLOR_F codeColor = D2D1::ColorF(0xD4 / 255.0f, 0xD4 / 255.0f, 0xD4 / 255.0f);
        if (!codeStr.empty() && codeStr[0] == '#') {
            codeColor = D2D1::ColorF(0xC5 / 255.0f, 0x86 / 255.0f, 0xC0 / 255.0f); // Magenta include
        } else if (!codeStr.empty() && codeStr.find("//") == 0) {
            codeColor = D2D1::ColorF(0x6A / 255.0f, 0x99 / 255.0f, 0x55 / 255.0f); // Green comment
        }

        ctx.DrawText(codeStr, Rect(m_bounds.x + lineNoW + 12, y, m_bounds.width - lineNoW - 12, lineHeight), codeColor, "Consolas", 13.0f, DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

        // Draw cursor line indicator if current line
        if (isCurrentLine) {
            float cursorX = m_bounds.x + lineNoW + 12 + (m_cursorCol - 1) * 7.5f;
            ctx.FillRect(Rect(cursorX, y + 2, 2.0f, lineHeight - 4), D2D1::ColorF(0xAE / 255.0f, 0xAF / 255.0f, 0xAD / 255.0f));
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
    SetProperty("height", Value(22.0f));
    SetProperty("background", Value(D2D1::ColorF(0x00 / 255.0f, 0x7A / 255.0f, 0xCC / 255.0f, 1.0f))); // VS Code StatusBar blue
    SetProperty("branch", Value("main*"));
    SetProperty("status", Value("Ready"));
    SetProperty("line", Value(14));
    SetProperty("col", Value(28));
}

void StatusBar::OnRender(GraphicsContext& ctx) {
    Control::OnRender(ctx);

    D2D1_COLOR_F textColor = D2D1::ColorF(1.0f, 1.0f, 1.0f);

    // Left items: Git Branch
    std::string branchStr = "[git] " + GetProperty("branch").AsString("main*");
    ctx.DrawText(branchStr, Rect(m_bounds.x + 10, m_bounds.y, 120, m_bounds.height), textColor, "Segoe UI", 11.0f, DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    // Errors & Warnings
    ctx.DrawText("(x) 0  (!) 0", Rect(m_bounds.x + 130, m_bounds.y, 100, m_bounds.height), textColor, "Segoe UI", 11.0f, DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    // Right items: Ln, Col, Spaces, UTF-8, C++
    int line = GetProperty("line").AsInt(14);
    int col = GetProperty("col").AsInt(28);
    std::string posStr = "Ln " + std::to_string(line) + ", Col " + std::to_string(col);

    float rightX = m_bounds.x + m_bounds.width - 320;
    ctx.DrawText(posStr, Rect(rightX, m_bounds.y, 100, m_bounds.height), textColor, "Segoe UI", 11.0f, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    ctx.DrawText("Spaces: 4", Rect(rightX + 100, m_bounds.y, 70, m_bounds.height), textColor, "Segoe UI", 11.0f, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    ctx.DrawText("UTF-8", Rect(rightX + 170, m_bounds.y, 60, m_bounds.height), textColor, "Segoe UI", 11.0f, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    ctx.DrawText("C++20", Rect(rightX + 230, m_bounds.y, 60, m_bounds.height), textColor, "Segoe UI", 11.0f, DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
}

} // namespace CUI
