#include "TerminalControl.h"
#include "ContextMenu.h"
#include "../style/ThemeManager.h"
#include "terminal/KeyboardTranslator.h"
#include "terminal/MouseReporter.h"
#include "terminal/UnicodeWidth.h"
#include <algorithm>
#include <shellapi.h>
#include <windows.h>

namespace CUI {

namespace {
const float kFindBarHeight = 34.0f;
const float kSurfacePadding = 4.0f;
const float kScrollBarWidth = 8.0f;
const float kBlinkIntervalMs = 530.0f;
const float kFlushIntervalMs = 8.0f;

const char* const kFindButtonLabels[3] = { "Prev", "Next", "x" };

bool SetClipboardUnicode(const std::wstring& text) {
    if (!OpenClipboard(nullptr)) {
        return false;
    }
    EmptyClipboard();
    const size_t bytes = (text.size() + 1) * sizeof(wchar_t);
    HGLOBAL handle = GlobalAlloc(GMEM_MOVEABLE, bytes);
    if (!handle) {
        CloseClipboard();
        return false;
    }
    void* data = GlobalLock(handle);
    if (data) {
        memcpy(data, text.c_str(), bytes);
        GlobalUnlock(handle);
        SetClipboardData(CF_UNICODETEXT, handle);
    } else {
        GlobalFree(handle);
    }
    CloseClipboard();
    return true;
}

bool GetClipboardUnicode(std::wstring& out) {
    if (!IsClipboardFormatAvailable(CF_UNICODETEXT)) {
        return false;
    }
    if (!OpenClipboard(nullptr)) {
        return false;
    }
    bool ok = false;
    HANDLE handle = GetClipboardData(CF_UNICODETEXT);
    if (handle) {
        const wchar_t* text = static_cast<const wchar_t*>(GlobalLock(handle));
        if (text) {
            out.assign(text);
            GlobalUnlock(handle);
            ok = true;
        }
    }
    CloseClipboard();
    return ok;
}
}

// Find input that forwards Enter/Escape to the owning terminal instead of the
// generic TextBox editing behaviour.
class TerminalControl::FindBox : public TextBox {
public:
    explicit FindBox(TerminalControl* owner) : m_owner(owner) {
        SetPlaceholder("Find");
    }

    const char* GetClassName() const override { return "TerminalFindBox"; }

    bool OnKeyDown(int vkCode) override {
        if (!m_owner) {
            return TextBox::OnKeyDown(vkCode);
        }
        if (vkCode == VK_RETURN) {
            const bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
            m_owner->DoFind(!shift);
            return true;
        }
        if (vkCode == VK_ESCAPE) {
            m_owner->ShowFind(false);
            return true;
        }
        const bool handled = TextBox::OnKeyDown(vkCode);
        m_owner->UpdateFindStatus();
        return handled;
    }

    void OnCharInput(wchar_t ch) override {
        TextBox::OnCharInput(ch);
        if (m_owner) {
            m_owner->UpdateFindStatus();
        }
    }

private:
    TerminalControl* m_owner = nullptr;
};

TerminalControl::TerminalControl() {
    InitTerminal("cmd.exe");
}

TerminalControl::TerminalControl(const std::string& shellPath) {
    InitTerminal(shellPath);
}

TerminalControl::~TerminalControl() {
    DetachBackend();
    if (m_terminal) {
        m_terminal->RedrawRequested = nullptr;
        m_terminal->OutputFlushRequested = nullptr;
        m_terminal->ScrollChanged = nullptr;
        m_terminal->TitleChanged = nullptr;
        m_terminal->ThemeChanged = nullptr;
        m_terminal->ClipboardSetRequested = nullptr;
        m_terminal->ClipboardGetRequested = nullptr;
    }
}

void TerminalControl::InitTerminal(const std::string& shellPath) {
    Term::TerminalOptions options;
    options.Cols = 80;
    options.Rows = 24;
    options.Scrollback = 3000;
    options.Theme = Term::TerminalTheme::Dark();

    m_terminal = std::make_unique<Term::Terminal>(options);
    m_renderer = std::make_unique<Term::TerminalRenderer>(m_terminal->Options());
    m_pendingShell = shellPath;

    m_terminal->RedrawRequested = [this]() { QueueRedraw(); };
    m_terminal->OutputFlushRequested = [this]() {
        m_outputPending.store(true);
        RequestWindowRepaint();
    };
    m_terminal->ScrollChanged = [this]() { MarkViewportDirty(); };
    m_terminal->TitleChanged = [this](const std::string& title) {
        m_terminalTitle = title;
        MarkRenderContentDirty();
    };
    m_terminal->ThemeChanged = [this]() {
        m_renderer->ApplyTheme(m_terminal->Options().Theme);
        MarkViewportDirty();
    };
    m_terminal->ClipboardSetRequested = [](const std::string& text) {
        SetClipboardUnicode(Term::Utf16FromUtf8(text));
    };
    m_terminal->ClipboardGetRequested = [](std::string& text) -> bool {
        std::wstring wide;
        if (!GetClipboardUnicode(wide)) {
            return false;
        }
        text = Term::Utf8FromUtf16(wide);
        return true;
    };

    SetFontFamily(Term::TerminalOptions::DefaultFontFamily());
    SetFontSize(m_terminal->Options().FontSize);
    SetBackground(m_terminal->Options().Theme.Background.ToD2D());
    SetColor(m_terminal->Options().Theme.Foreground.ToD2D());
    SetBorderToken(ThemeTokenId::CardBorder);
    SetBorderThickness(1.0f);
    SetCornerRadius(4.0f);
    SetWidth(680.0f);
    SetHeight(420.0f);

    BuildFindBar();
    BuildContextMenu();
}

Value TerminalControl::GetProperty(PropertyId id) const {
    if (id == PropertyId::Shell) {
        return Value(m_pendingShell);
    }
    return Control::GetProperty(id);
}

bool TerminalControl::HasProperty(PropertyId id) const {
    return id == PropertyId::Shell || Control::HasProperty(id);
}

void TerminalControl::SetProperty(PropertyId id, const Value& val) {
    if (id == PropertyId::Shell) {
        SetShell(val.AsString());
        return;
    }
    Control::SetProperty(id, val);
    if ((id == PropertyId::FontSize || id == PropertyId::FontFamily) && m_terminal && m_renderer) {
        auto& options = m_terminal->Options();
        options.FontFamily = GetFontFamily();
        options.FontSize = GetFontSize();
        m_renderer->UpdateFont(options.FontFamily, options.FontSize);
        m_lastCols = -1;
        m_lastRows = -1;
        MarkViewportDirty();
    }
}

HCURSOR TerminalControl::GetCursor() const {
    const Rect surface = GetSurfaceRect();
    if (surface.Contains(m_lastMousePos.x, m_lastMousePos.y)) {
        return LoadCursor(nullptr, IDC_IBEAM);
    }
    return nullptr;
}

void TerminalControl::BuildFindBar() {
    m_findBox = std::make_shared<FindBox>(this);
    m_findBox->SetVisibility(Visibility::Collapsed);
    m_findBox->SetWidth(220.0f);
    m_findBox->SetHeight(26.0f);
    AddChild(m_findBox);
}

void TerminalControl::BuildContextMenu() {
    auto menu = std::make_shared<ContextMenu>();
    menu->AddItem("Copy", "Ctrl+Shift+C", [this]() { CopySelectionToClipboard(); });
    menu->AddItem("Paste", "Ctrl+V", [this]() { PasteFromClipboard(); });
    menu->AddSeparator();
    menu->AddItem("Select All", "Ctrl+A", [this]() { SelectAll(); });
    menu->AddItem("Clear", "Ctrl+L", [this]() {
        m_terminal->Clear();
        MarkViewportDirty();
    });
    menu->AddItem("Find...", "Ctrl+F", [this]() { ShowFind(true); });
    SetContextMenu(menu);
}

void TerminalControl::AttachBackend(Term::ITerminalBackend* backend) {
    DetachBackend();
    m_backend = backend;
    m_backendStartAttempted = true;
    m_terminal->Attach(backend);
}

void TerminalControl::AttachConPty(const std::string& shellPath, const std::string& arguments) {
    DetachBackend();
    m_ownedBackend = std::make_unique<Term::ConPtyBackend>(
        Term::Utf16FromUtf8(shellPath), Term::Utf16FromUtf8(arguments));
    m_backend = m_ownedBackend.get();
    m_backendStartAttempted = true;
    m_terminal->Attach(m_backend);
}

void TerminalControl::DetachBackend() {
    if (m_terminal) {
        m_terminal->Detach();
    }
    if (m_ownedBackend) {
        m_ownedBackend->Stop();
        m_ownedBackend.reset();
    }
    m_backend = nullptr;
}

void TerminalControl::StartShell(const std::string& shellPath) {
    AttachConPty(shellPath);
}

void TerminalControl::StopShell() {
    DetachBackend();
}

void TerminalControl::WriteInput(const std::string& text) {
    m_terminal->SendData(text);
}

void TerminalControl::ApplyTheme(const Term::TerminalTheme& theme) {
    m_terminal->Options().Theme = theme;
    m_renderer->ApplyTheme(theme);
    SetBackground(theme.Background.ToD2D());
    SetColor(theme.Foreground.ToD2D());
    MarkViewportDirty();
}

void TerminalControl::Zoom(int deltaSteps) {
    auto& options = m_terminal->Options();
    const float next = std::clamp(options.FontSize + static_cast<float>(deltaSteps),
                                  options.MinFontSize, options.MaxFontSize);
    if (std::fabs(next - options.FontSize) < 0.01f) {
        return;
    }
    options.FontSize = next;
    SetFontSize(next);
    m_renderer->UpdateFont(options.FontFamily, options.FontSize);
    m_lastCols = -1;
    m_lastRows = -1;
    MarkViewportDirty();
}

void TerminalControl::ShowFind(bool show) {
    m_findVisible = show;
    if (m_findBox) {
        m_findBox->SetVisibility(show ? Visibility::Visible : Visibility::Collapsed);
        if (show) {
            m_findBox->SelectAll();
        }
    }
    m_lastCols = -1;
    m_lastRows = -1;
    UpdateFindStatus();
    MarkRenderContentDirty();
}

void TerminalControl::CopySelectionToClipboard() {
    const std::string text = m_terminal->GetSelectionText();
    if (text.empty()) {
        return;
    }
    SetClipboardUnicode(Term::Utf16FromUtf8(text));
}

void TerminalControl::PasteFromClipboard() {
    std::wstring wide;
    if (!GetClipboardUnicode(wide)) {
        return;
    }
    std::string text = Term::Utf8FromUtf16(wide);
    // Terminals expect CR line breaks in pasted payloads.
    std::string normalized;
    normalized.reserve(text.size());
    for (size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '\r' && i + 1 < text.size() && text[i + 1] == '\n') {
            normalized.push_back('\r');
            ++i;
        } else if (text[i] == '\n') {
            normalized.push_back('\r');
        } else {
            normalized.push_back(text[i]);
        }
    }

    std::string payload;
    if (Term::KeyboardTranslator::FromTextInput(normalized, m_terminal->Input().BracketedPasteMode(), payload)) {
        m_terminal->ScrollToBottom();
        m_terminal->SendData(payload);
    }
}

void TerminalControl::SelectAll() {
    auto& buf = m_terminal->Buffers().Active();
    auto& model = m_terminal->Selection().Model();
    model.HasSelection = true;
    model.StartRow = 0;
    model.StartCol = 0;
    model.EndRow = (std::max)(0, buf.Length() - 1);
    model.EndCol = m_terminal->Cols() - 1;
    MarkViewportDirty();
}

Size TerminalControl::Measure(Size availableSize) {
    (void)availableSize;
    float expW = GetWidth(); if (expW < 0) expW = 680.0f;
    float expH = GetHeight(); if (expH < 0) expH = 420.0f;
    m_desiredSize = Size(expW, expH);

    if (m_findBox) {
        m_findBox->Measure(Size(220.0f, 26.0f));
    }
    return m_desiredSize;
}

void TerminalControl::Arrange(Rect finalRect) {
    SetBounds(finalRect);

    if (m_findBox) {
        if (m_findVisible) {
            const Rect bar = GetFindBarRect();
            m_findBox->Arrange(Rect(bar.x + 52.0f, bar.y + 4.0f, 220.0f, 26.0f));
        } else {
            m_findBox->Arrange(Rect(0.0f, 0.0f, 0.0f, 0.0f));
        }
    }
}

Rect TerminalControl::GetFindBarRect() const {
    if (!m_findVisible) {
        return Rect();
    }
    return Rect(m_bounds.x + 1.0f, m_bounds.y + 1.0f,
                (std::max)(0.0f, m_bounds.width - 2.0f), kFindBarHeight);
}

Rect TerminalControl::GetSurfaceRect() const {
    const float top = m_bounds.y + (m_findVisible ? kFindBarHeight + 1.0f : 0.0f) + kSurfacePadding;
    float width = m_bounds.width - kSurfacePadding * 2.0f;
    const bool hasScroll = m_terminal && m_terminal->Buffers().Active().BaseY() > 0;
    if (hasScroll) {
        width -= kScrollBarWidth + 2.0f;
    }
    const float height = m_bounds.y + m_bounds.height - kSurfacePadding - top;
    return Rect(m_bounds.x + kSurfacePadding, top,
                (std::max)(0.0f, width), (std::max)(0.0f, height));
}

Rect TerminalControl::GetScrollBarRect() const {
    const Rect surface = GetSurfaceRect();
    return Rect(m_bounds.x + m_bounds.width - kSurfacePadding - kScrollBarWidth, surface.y,
                kScrollBarWidth, surface.height);
}

Rect TerminalControl::GetScrollThumbRect() const {
    const Rect track = GetScrollBarRect();
    if (!m_terminal) {
        return Rect();
    }
    auto& buf = m_terminal->Buffers().Active();
    const int max = buf.BaseY();
    if (max <= 0 || track.height <= 0.0f) {
        return Rect();
    }
    const float total = static_cast<float>(max + m_terminal->Rows());
    const float thumbH = (std::max)(24.0f, track.height * (static_cast<float>(m_terminal->Rows()) / total));
    const float span = (std::max)(0.0f, track.height - thumbH);
    const float t = 1.0f - static_cast<float>(buf.YDisp) / static_cast<float>(max);
    return Rect(track.x, track.y + span * std::clamp(t, 0.0f, 1.0f), track.width, thumbH);
}

Rect TerminalControl::GetFindButtonRect(int index) const {
    const Rect bar = GetFindBarRect();
    if (bar.IsEmpty()) {
        return Rect();
    }
    const float widths[3] = { 52.0f, 52.0f, 26.0f };
    float x = bar.x + bar.width - 8.0f;
    for (int i = 2; i > index; --i) {
        x -= widths[i] + 4.0f;
    }
    x -= widths[index];
    return Rect(x, bar.y + 5.0f, widths[index], 24.0f);
}

Rect TerminalControl::GetRowRect(int row) const {
    const Rect surface = GetSurfaceRect();
    const float h = m_renderer->CellHeight();
    return Rect(surface.x, surface.y + row * h, surface.width, h);
}

void TerminalControl::RecalculateSize(GraphicsContext& ctx) {
    (void)ctx;
    const Rect surface = GetSurfaceRect();
    const float cw = m_renderer->CellWidth();
    const float ch = m_renderer->CellHeight();
    if (surface.width <= 0.0f || surface.height <= 0.0f || cw <= 0.0f || ch <= 0.0f) {
        return;
    }

    const int cols = (std::max)(2, static_cast<int>(surface.width / cw));
    const int rows = (std::max)(1, static_cast<int>(surface.height / ch));
    if (cols == m_lastCols && rows == m_lastRows) {
        return;
    }

    m_lastCols = cols;
    m_lastRows = rows;
    m_terminal->Resize(cols, rows);

    if (m_backend == nullptr && !m_backendStartAttempted) {
        m_backendStartAttempted = true;
        const std::string& shell = GetShell();
        AttachConPty(shell.empty() ? std::string("cmd.exe") : shell);
    }

    MarkViewportDirty();
}

void TerminalControl::MarkViewportDirty() {
    if (!m_terminal) {
        return;
    }
    auto& buf = m_terminal->Buffers().Active();
    for (int row = 0; row < m_terminal->Rows(); ++row) {
        buf.GetViewportLine(row).SetIsDirty(true);
    }
    std::fill(m_boundLines.begin(), m_boundLines.end(), nullptr);
    MarkRenderContentDirty();
}

void TerminalControl::MarkDirtyRows() {
    if (!m_terminal || !m_renderer->HasMetrics()) {
        MarkRenderContentDirty();
        return;
    }

    auto& buf = m_terminal->Buffers().Active();
    const int rows = m_terminal->Rows();
    if (static_cast<int>(m_boundLines.size()) != rows) {
        m_boundLines.assign(static_cast<size_t>(rows), nullptr);
        MarkRenderContentDirty();
        return;
    }

    if (buf.YDisp != m_lastYDisp) {
        m_lastYDisp = buf.YDisp;
        MarkViewportDirty();
        return;
    }

    bool any = false;
    for (int row = 0; row < rows; ++row) {
        Term::BufferLine* line = &buf.GetViewportLine(row);
        if (m_boundLines[static_cast<size_t>(row)] != line || line->IsDirty()) {
            MarkRenderRectDirty(GetRowRect(row));
            any = true;
        }
    }

    // The cursor and selection overlay live outside the per-row bands.
    if (any) {
        MarkRenderRectDirty(GetRowRect(buf.CursorY));
    }
}

void TerminalControl::QueueRedraw() {
    m_redrawQueued = true;
}

void TerminalControl::RequestWindowRepaint() {
    HWND hwnd = m_hwnd;
    if (hwnd != nullptr) {
        // Cross-thread InvalidateRect marks an update region but does not
        // guarantee WaitMessage() will wake immediately, so poke the queue too.
        InvalidateRect(hwnd, nullptr, FALSE);
        PostMessage(hwnd, WM_NULL, 0, 0);
    }
}

bool TerminalControl::HasSelfAnimation() const {
    if (!m_terminal) {
        return false;
    }
    if (m_scrollbarAutoHide.NeedsTicks()) {
        return true;
    }
    if (m_outputPending.load() || m_redrawQueued) {
        return true;
    }
    const bool blink = m_terminal->Options().CursorBlink && m_terminal->Input().CursorBlink();
    return m_isFocused && blink && m_terminal->Input().CursorVisible();
}

bool TerminalControl::OnAnimationTick() {
    bool more = Control::OnAnimationTick();
    if (!m_terminal) {
        return more;
    }

    const float dtMs = GetAnimationDeltaSeconds() * 1000.0f;
    const float prevOpacity = m_scrollbarAutoHide.Opacity();
    if (m_scrollbarAutoHide.Tick(GetAnimationDeltaSeconds())) {
        more = true;
    }
    if (std::abs(prevOpacity - m_scrollbarAutoHide.Opacity()) > 0.001f) {
        MarkRenderRectDirty(GetScrollBarRect());
    }

    if (m_outputPending.load()) {
        m_flushAccumMs += dtMs;
        if (m_flushAccumMs >= kFlushIntervalMs) {
            m_flushAccumMs = 0.0f;
            const bool remaining = m_terminal->FlushPendingOutput();
            m_outputPending.store(remaining || m_terminal->HasPendingOutput());
        }
        more = true;
    } else {
        m_flushAccumMs = kFlushIntervalMs;
    }

    const bool blink = m_terminal->Options().CursorBlink && m_terminal->Input().CursorBlink();
    if (m_isFocused && blink && m_terminal->Input().CursorVisible()) {
        m_blinkAccumMs += dtMs;
        if (m_blinkAccumMs >= kBlinkIntervalMs) {
            m_blinkAccumMs = 0.0f;
            m_cursorOn = !m_cursorOn;
            MarkRenderRectDirty(GetRowRect(m_terminal->Buffers().Active().CursorY));
        }
        more = true;
    } else if (!m_cursorOn) {
        m_cursorOn = true;
        m_blinkAccumMs = 0.0f;
        MarkRenderContentDirty();
    }

    if (m_redrawQueued) {
        m_redrawQueued = false;
        MarkDirtyRows();
        more = true;
    }

    return more;
}

void TerminalControl::OnRender(GraphicsContext& ctx) {
    m_hwnd = ctx.GetHwnd();
    m_renderer->SetDpi(ctx.GetDpiScale());
    m_renderer->EnsureMetrics(ctx);
    RecalculateSize(ctx);

    // Guarantee forward progress even if animation ticks are throttled.
    // Cap work here so a paint never digests a huge flood before drawing —
    // OnAnimationTick continues draining remaining chunks.
    if (m_outputPending.load()) {
        const bool remaining = m_terminal->FlushPendingOutput();
        m_outputPending.store(remaining || m_terminal->HasPendingOutput());
    }

    const float radius = GetCornerRadius();
    const D2D1_COLOR_F bg = m_terminal->Options().Theme.Background.ToD2D();
    const D2D1_COLOR_F border = ResolveThemeColor(GetBorderToken(), ThemeTokenId::CardBorder);
    const float borderThick = GetBorderThickness();

    ctx.FillRoundedRect(m_bounds, radius, bg);
    if (borderThick > 0.0f) {
        ctx.DrawRoundedRect(m_bounds, radius, border, borderThick);
    }

    if (m_findVisible) {
        const Rect bar = GetFindBarRect();
        ctx.FillRect(bar, D2D1::ColorF(0x2D2D2D));
        ctx.DrawText("Find:", Rect(bar.x + 8.0f, bar.y, 44.0f, bar.height),
                     D2D1::ColorF(D2D1::ColorF::White), "微软雅黑", 12.0f,
                     DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        ctx.DrawText(m_findStatus, Rect(bar.x + 280.0f, bar.y, 96.0f, bar.height),
                     D2D1::ColorF(D2D1::ColorF::Gray), "微软雅黑", 12.0f,
                     DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        for (int i = 0; i < 3; ++i) {
            const Rect btn = GetFindButtonRect(i);
            const bool hot = (m_hoveredFindButton == i);
            ctx.FillRoundedRect(btn, 3.0f, hot ? D2D1::ColorF(0x4A4A4A) : D2D1::ColorF(0x3A3A3A));
            ctx.DrawText(kFindButtonLabels[i], btn, D2D1::ColorF(D2D1::ColorF::White),
                         "微软雅黑", 12.0f,
                         DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        }
    }

    const Rect surface = GetSurfaceRect();
    if (surface.width <= 0.0f || surface.height <= 0.0f || !m_renderer->HasMetrics()) {
        return;
    }

    ctx.PushClip(surface);

    auto& buf = m_terminal->Buffers().Active();
    const int cols = m_terminal->Cols();
    const int rows = m_terminal->Rows();
    if (static_cast<int>(m_boundLines.size()) != rows) {
        m_boundLines.assign(static_cast<size_t>(rows), nullptr);
    }

    // Dirty-row painting: skip bands the compositor did not ask us to repaint.
    for (int row = 0; row < rows; ++row) {
        const Rect rowRect = GetRowRect(row);
        if (!ctx.IntersectsPaintBounds(rowRect)) {
            continue;
        }
        Term::BufferLine& line = buf.GetViewportLine(row);
        ctx.FillRect(rowRect, bg);
        m_renderer->PaintRow(ctx, line, cols, surface.x, rowRect.y);
        line.SetIsDirty(false);
        m_boundLines[static_cast<size_t>(row)] = &line;
    }

    const bool blink = m_terminal->Options().CursorBlink && m_terminal->Input().CursorBlink();
    const bool showCursor = m_isFocused && (!blink || m_cursorOn);
    m_renderer->PaintOverlay(ctx, *m_terminal, showCursor, surface.x, surface.y, m_imePreedit);

    ctx.PopClip();

    const Rect thumb = GetScrollThumbRect();
    if (!thumb.IsEmpty() && m_scrollbarAutoHide.IsDrawn()) {
        const Rect track = GetScrollBarRect();
        const float vis = m_scrollbarAutoHide.Opacity();
        ctx.FillRoundedRect(track, kScrollBarWidth * 0.5f, D2D1::ColorF(0x1A1A1A, 0.6f * vis));
        ctx.FillRoundedRect(thumb, kScrollBarWidth * 0.5f,
                            D2D1::ColorF(0x808080, (m_draggingScrollbar ? 0.9f : 0.6f) * vis));
    }

    m_lastYDisp = buf.YDisp;
}

unsigned TerminalControl::CurrentModifiers() {
    unsigned mods = Term::ModNone;
    if (GetKeyState(VK_CONTROL) & 0x8000) mods |= Term::ModControl;
    if (GetKeyState(VK_SHIFT) & 0x8000) mods |= Term::ModShift;
    if (GetKeyState(VK_MENU) & 0x8000) mods |= Term::ModAlt;
    return mods;
}

bool TerminalControl::HitTestCell(Point pt, int& col, int& row) const {
    const Rect surface = GetSurfaceRect();
    const float cw = m_renderer->CellWidth();
    const float ch = m_renderer->CellHeight();
    if (cw <= 0.0f || ch <= 0.0f) {
        return false;
    }
    col = std::clamp(static_cast<int>((pt.x - surface.x) / cw), 0, (std::max)(0, m_terminal->Cols() - 1));
    row = std::clamp(static_cast<int>((pt.y - surface.y) / ch), 0, (std::max)(0, m_terminal->Rows() - 1));
    return surface.Contains(pt.x, pt.y);
}

int TerminalControl::AbsoluteRow(int viewportRow) const {
    const auto& buf = m_terminal->Buffers().Active();
    return buf.BaseY() - buf.YDisp + viewportRow;
}

void TerminalControl::SendKeySequence(const std::string& seq) {
    if (seq.empty()) {
        return;
    }
    m_terminal->ScrollToBottom();
    m_terminal->SendData(seq);
}

bool TerminalControl::OnKeyDown(int vkCode) {
    const unsigned mods = CurrentModifiers();
    const bool ctrl = (mods & Term::ModControl) != 0;
    const bool shift = (mods & Term::ModShift) != 0;

    if (ctrl && !shift && vkCode == 'F') {
        ShowFind(true);
        m_suppressCharCount++;
        return true;
    }
    if (ctrl && !shift && vkCode == 'L') {
        m_terminal->Clear();
        MarkViewportDirty();
        m_suppressCharCount++;
        return true;
    }
    if (ctrl && shift && vkCode == 'C') {
        CopySelectionToClipboard();
        m_suppressCharCount++;
        return true;
    }
    if (ctrl && !shift && vkCode == 'C') {
        if (!m_terminal->GetSelectionText().empty()) {
            CopySelectionToClipboard();
            m_suppressCharCount++;
            return true;
        }
    }
    if (ctrl && !shift && vkCode == 'V') {
        PasteFromClipboard();
        m_suppressCharCount++;
        return true;
    }
    if (shift && vkCode == VK_INSERT) {
        PasteFromClipboard();
        m_suppressCharCount++;
        return true;
    }
    if (ctrl && shift && vkCode == 'A') {
        SelectAll();
        m_suppressCharCount++;
        return true;
    }

    std::string seq;
    if (Term::KeyboardTranslator::Translate(vkCode, mods, m_terminal->Input().ApplicationCursorKeys(), seq)) {
        SendKeySequence(seq);
        m_suppressCharCount++;
        return true;
    }
    return false;
}

void TerminalControl::OnCharInput(wchar_t ch) {
    if (m_suppressCharCount > 0) {
        m_suppressCharCount--;
        return;
    }
    if (ch < 0x20 && ch != 0x7F) {
        // Control codes already went out through OnKeyDown.
        return;
    }

    m_imePreedit.clear();
    std::wstring wide(1, ch);
    std::string payload;
    if (Term::KeyboardTranslator::FromTextInput(Term::Utf8FromUtf16(wide),
                                                m_terminal->Input().BracketedPasteMode(), payload)) {
        SendKeySequence(payload);
    }
}

void TerminalControl::OnMouseDown(Point pt) {
    Control::OnMouseDown(pt);
    m_lastMousePos = pt;

    if (m_findVisible) {
        for (int i = 0; i < 3; ++i) {
            if (GetFindButtonRect(i).Contains(pt.x, pt.y)) {
                if (i == 0) DoFind(false);
                else if (i == 1) DoFind(true);
                else ShowFind(false);
                return;
            }
        }
    }

    const Rect thumb = GetScrollThumbRect();
    if (!thumb.IsEmpty() && GetScrollBarRect().Contains(pt.x, pt.y)) {
        m_draggingScrollbar = true;
        m_scrollbarAutoHide.SetDragging(true, this);
        m_scrollbarAutoHide.NotifyActivity(this);
        RequestAnimationTicks();
        m_scrollGrabOffset = thumb.Contains(pt.x, pt.y) ? (pt.y - thumb.y) : thumb.height * 0.5f;
        SyncScrollFromThumb(pt.y);
        return;
    }

    int col = 0;
    int row = 0;
    if (!HitTestCell(pt, col, row)) {
        return;
    }

    const unsigned mods = CurrentModifiers();
    const bool forceSelect = (mods & Term::ModShift) != 0;
    const int abs = AbsoluteRow(row);

    if (!forceSelect && (mods & Term::ModControl) != 0) {
        const std::string url = m_terminal->GetLinkAt(abs, col);
        if (!url.empty()) {
            ShellExecuteW(nullptr, L"open", Term::Utf16FromUtf8(url).c_str(),
                          nullptr, nullptr, SW_SHOWNORMAL);
            return;
        }
    }

    // Track multi-clicks manually; CUI only surfaces single and double clicks.
    const auto now = std::chrono::steady_clock::now();
    const auto sinceMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastClickTime).count();
    if (sinceMs <= static_cast<long long>(GetDoubleClickTime()) &&
        col == m_lastClickCol && row == m_lastClickRow) {
        m_clickCount = (std::min)(3, m_clickCount + 1);
    } else {
        m_clickCount = 1;
    }
    m_lastClickTime = now;
    m_lastClickCol = col;
    m_lastClickRow = row;

    if (!forceSelect && m_terminal->Input().MouseTracking()) {
        const int btn = Term::MouseReporter::ButtonFromLogical(Term::TermMouseButton::Left);
        if (m_terminal->TryReportMouse(btn, col, row, true, mods,
                                       m_renderer->CellWidth(), m_renderer->CellHeight())) {
            m_mouseReporting = true;
            m_pressedButton = btn;
            m_lastMouseCol = col;
            m_lastMouseRow = row;
            return;
        }
    }

    if (m_clickCount >= 3) {
        m_terminal->Selection().SelectLine(abs, m_terminal->Cols());
        MaybeCopyOnSelect();
        MarkViewportDirty();
        return;
    }
    if (m_clickCount == 2) {
        auto& buf = m_terminal->Buffers().Active();
        m_terminal->Selection().SelectWord(abs, col, [&buf](int y) -> Term::BufferLine* {
            return (y >= 0 && y < buf.Length()) ? &buf.GetLine(y) : nullptr;
        }, m_terminal->Cols());
        MaybeCopyOnSelect();
        MarkViewportDirty();
        return;
    }

    m_terminal->Selection().Begin(abs, col);
    MarkViewportDirty();
}

void TerminalControl::OnMouseDblClick(Point pt) {
    // Handled through the click counter in OnMouseDown.
    (void)pt;
}

void TerminalControl::OnMouseMove(Point pt) {
    Control::OnMouseMove(pt);
    m_lastMousePos = pt;

    const bool overBar = !GetScrollThumbRect().IsEmpty() && GetScrollBarRect().Contains(pt.x, pt.y);
    m_scrollbarAutoHide.SetPointerOver(overBar, this);
    if (overBar) {
        RequestAnimationTicks();
        MarkRenderRectDirty(GetScrollBarRect());
    }

    if (m_findVisible) {
        int hovered = -1;
        for (int i = 0; i < 3; ++i) {
            if (GetFindButtonRect(i).Contains(pt.x, pt.y)) {
                hovered = i;
                break;
            }
        }
        if (hovered != m_hoveredFindButton) {
            m_hoveredFindButton = hovered;
            MarkRenderRectDirty(GetFindBarRect());
        }
    }

    if (m_draggingScrollbar) {
        SyncScrollFromThumb(pt.y);
        m_scrollbarAutoHide.NotifyActivity(this);
        return;
    }

    int col = 0;
    int row = 0;
    HitTestCell(pt, col, row);
    const unsigned mods = CurrentModifiers();

    if (m_mouseReporting && m_terminal->Input().MouseTracking() && (mods & Term::ModShift) == 0) {
        const bool pressed = (GetKeyState(VK_LBUTTON) & 0x8000) != 0;
        if (m_terminal->Input().MouseAnyEvent() ||
            (m_terminal->Input().MouseButtonEvent() && pressed)) {
            if (col != m_lastMouseCol || row != m_lastMouseRow) {
                const int btn = ((m_pressedButton >= 0) ? m_pressedButton : 0) | Term::MouseReporter::Motion;
                m_terminal->TryReportMouse(btn, col, row, true, mods,
                                           m_renderer->CellWidth(), m_renderer->CellHeight());
                m_lastMouseCol = col;
                m_lastMouseRow = row;
            }
        }
        return;
    }

    if (m_isPressed && m_terminal->Selection().IsSelecting()) {
        m_terminal->Selection().Update(AbsoluteRow(row), col);
        MarkViewportDirty();
    }
}

void TerminalControl::OnMouseUp(Point pt) {
    m_lastMousePos = pt;

    if (m_draggingScrollbar) {
        m_draggingScrollbar = false;
        m_scrollbarAutoHide.SetDragging(false, this);
        Control::OnMouseUp(pt);
        MarkRenderRectDirty(GetScrollBarRect());
        RequestAnimationTicks();
        return;
    }

    int col = 0;
    int row = 0;
    HitTestCell(pt, col, row);
    const unsigned mods = CurrentModifiers();

    if (m_mouseReporting && m_terminal->Input().MouseTracking()) {
        const int btn = Term::MouseReporter::ButtonFromLogical(Term::TermMouseButton::Left);
        if (m_terminal->Input().MouseEncoding() == Term::MouseReporter::Encoding::Sgr) {
            m_terminal->TryReportMouse(btn, col, row, false, mods,
                                       m_renderer->CellWidth(), m_renderer->CellHeight());
        } else {
            m_terminal->TryReportMouse(Term::MouseReporter::ButtonRelease, col, row, true, mods,
                                       m_renderer->CellWidth(), m_renderer->CellHeight());
        }
        m_mouseReporting = false;
        m_pressedButton = -1;
        Control::OnMouseUp(pt);
        return;
    }

    if (m_terminal->Selection().IsSelecting()) {
        m_terminal->Selection().End();
        MaybeCopyOnSelect();
        MarkViewportDirty();
    }
    Control::OnMouseUp(pt);
}

void TerminalControl::OnMouseWheel(float delta) {
    const unsigned mods = CurrentModifiers();
    if ((mods & Term::ModControl) != 0) {
        Zoom(delta > 0.0f ? 1 : -1);
        return;
    }

    int col = 0;
    int row = 0;
    HitTestCell(m_lastMousePos, col, row);

    if (m_terminal->Input().MouseTracking() && (mods & Term::ModShift) == 0) {
        const int btn = delta > 0.0f
            ? Term::MouseReporter::ButtonWheelUp
            : Term::MouseReporter::ButtonWheelDown;
        if (m_terminal->TryReportMouse(btn, col, row, true, mods,
                                       m_renderer->CellWidth(), m_renderer->CellHeight())) {
            return;
        }
    }

    m_terminal->ScrollLines(delta > 0.0f ? 3 : -3);
    m_scrollbarAutoHide.NotifyActivity(this);
    RequestAnimationTicks();
    MarkViewportDirty();
}

void TerminalControl::OnMouseLeave() {
    Control::OnMouseLeave();
    m_scrollbarAutoHide.SetPointerOver(false, this);
    RequestAnimationTicks();
    if (m_hoveredFindButton != -1) {
        m_hoveredFindButton = -1;
        MarkRenderRectDirty(GetFindBarRect());
    }
}

void TerminalControl::OnFocus() {
    Control::OnFocus();
    m_cursorOn = true;
    m_blinkAccumMs = 0.0f;
    m_terminal->NotifyFocus(true);
    MarkRenderContentDirty();
}

void TerminalControl::OnBlur() {
    Control::OnBlur();
    m_terminal->NotifyFocus(false);
    MarkRenderContentDirty();
}

void TerminalControl::SyncScrollFromThumb(float y) {
    const Rect track = GetScrollBarRect();
    auto& buf = m_terminal->Buffers().Active();
    const int max = buf.BaseY();
    const Rect thumb = GetScrollThumbRect();
    if (max <= 0 || thumb.IsEmpty()) {
        return;
    }
    const float span = (std::max)(1.0f, track.height - thumb.height);
    const float t = std::clamp((y - track.y - m_scrollGrabOffset) / span, 0.0f, 1.0f);
    m_terminal->SetScrollDisp(static_cast<int>(std::lround((1.0f - t) * max)));
    MarkViewportDirty();
}

void TerminalControl::MaybeCopyOnSelect() {
    if (!m_terminal->Options().CopyOnSelect) {
        return;
    }
    CopySelectionToClipboard();
}

void TerminalControl::DoFind(bool forward) {
    if (!m_findBox) {
        return;
    }
    const std::wstring query = Term::Utf16FromUtf8(m_findBox->GetText());
    if (query.empty()) {
        return;
    }

    bool hit;
    if (forward) {
        m_findCol += 1;
        hit = m_terminal->FindNext(query, m_findRow, m_findCol);
        if (!hit) {
            m_findRow = 0;
            m_findCol = 0;
            hit = m_terminal->FindNext(query, m_findRow, m_findCol);
        } else {
            m_findCol += static_cast<int>(query.size());
        }
    } else {
        hit = m_terminal->FindPrev(query, m_findRow, m_findCol);
        if (!hit) {
            m_findRow = (std::max)(0, m_terminal->Buffers().Active().Length() - 1);
            m_findCol = m_terminal->Cols();
            m_terminal->FindPrev(query, m_findRow, m_findCol);
        }
    }
    UpdateFindStatus();
    MarkViewportDirty();
}

void TerminalControl::UpdateFindStatus() {
    if (!m_findBox) {
        return;
    }
    const std::wstring query = Term::Utf16FromUtf8(m_findBox->GetText());
    if (query.empty()) {
        m_findStatus.clear();
    } else {
        m_findStatus = std::to_string(m_terminal->CountMatches(query)) + " hits";
    }
    if (m_findVisible) {
        MarkRenderRectDirty(GetFindBarRect());
    }
}

} // namespace CUI
