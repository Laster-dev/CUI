#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "MarkdownView.h"
#include "../style/ThemeManager.h"
#include "../core/Value.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cctype>
#include <windows.h>
#include <shellapi.h>

namespace CUI {
namespace {

constexpr float kPad = 16.0f;
constexpr float kSb = 10.0f;
constexpr float kCopyBtn = 26.0f;
constexpr float kCopyInset = 4.0f;

constexpr const char* kSvgCopy =
    "<svg viewBox=\"0 0 1024 1024\" xmlns=\"http://www.w3.org/2000/svg\">"
    "<path d=\"M661.333333 234.666667A64 64 0 0 1 725.333333 298.666667v597.333333a64 64 0 0 1-64 64h-469.333333A64 64 0 0 1 128 896V298.666667a64 64 0 0 1 64-64z m-21.333333 85.333333H213.333333v554.666667h426.666667v-554.666667z m191.829333-256a64 64 0 0 1 63.744 57.856l0.256 6.144v575.701333a42.666667 42.666667 0 0 1-85.034666 4.992l-0.298667-4.992V149.333333H384a42.666667 42.666667 0 0 1-42.368-37.674666L341.333333 106.666667a42.666667 42.666667 0 0 1 37.674667-42.368L384 64h447.829333z\"/>"
    "</svg>";

std::string JoinCodeLines(const std::vector<std::string>& lines) {
    std::string out;
    for (size_t i = 0; i < lines.size(); ++i) {
        if (i > 0) {
            out += '\n';
        }
        out += lines[i];
    }
    return out;
}

void StyleCopyButton(Button& btn) {
    btn.SetText("");
    btn.SetIcon(kSvgCopy);
    btn.SetToolTip("复制");
    btn.SetWidth(kCopyBtn);
    btn.SetHeight(kCopyBtn);
    btn.SetFontSize(16.0f);
    btn.SetPadding(Thickness(5.0f));
    btn.SetCornerRadius(4.0f);
    btn.SetBorderThickness(0.0f);
    btn.SetBackgroundToken(ThemeTokenId::Unset);
    btn.SetHoverBackgroundToken(ThemeTokenId::HoverBackground);
    btn.SetPressedBackgroundToken(ThemeTokenId::PressedBackground);
    btn.SetBackground(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f));
    btn.SetColorToken(ThemeTokenId::AccentColor);
}

bool CopyUtf8(const std::string& text) {
    const std::wstring w = Utf8ToUtf16(text);
    if (!OpenClipboard(nullptr)) {
        return false;
    }
    EmptyClipboard();
    const size_t bytes = (w.size() + 1) * sizeof(wchar_t);
    HGLOBAL mem = GlobalAlloc(GMEM_MOVEABLE, bytes);
    if (!mem) {
        CloseClipboard();
        return false;
    }
    if (void* dst = GlobalLock(mem)) {
        memcpy(dst, w.c_str(), bytes);
        GlobalUnlock(mem);
        SetClipboardData(CF_UNICODETEXT, mem);
    }
    CloseClipboard();
    return true;
}

int SelLo(int a, int b) { return (std::min)(a, b); }
int SelHi(int a, int b) { return (std::max)(a, b); }

bool IsWordChar(unsigned char c) {
    return std::isalnum(c) || c >= 0x80 || c == '_';
}

} // namespace

MarkdownView::MarkdownView() {
    SetBackgroundToken(ThemeTokenId::CardBackground);
    SetBorderToken(ThemeTokenId::CardBorder);
    SetColorToken(ThemeTokenId::TextPrimary);
    SetBorderThickness(1.0f);
    SetCornerRadius(6.0f);
    SetWidth(-1.0f);
    SetHeight(420.0f);
}

MarkdownView::MarkdownView(const std::string& markdown) : MarkdownView() {
    SetMarkdown(markdown);
}

std::vector<PropertyMeta> MarkdownView::GetPropertyMetas() const {
    auto metas = UIElement::GetPropertyMetas();
    metas.push_back({ "text", "Markdown 源 (Markdown)", "内容", "string" });
    metas.push_back({ "showLineNumbers", "代码行号 (LineNumbers)", "代码", "bool" });
    return metas;
}

void MarkdownView::SetProperty(PropertyId id, const Value& val) {
    if (id == PropertyId::Text) {
        SetMarkdown(val.AsString(""));
        return;
    }
    Control::SetProperty(id, val);
}

void MarkdownView::SetMarkdown(const std::string& markdown) {
    if (GetText() == markdown && !m_blocks.empty()) {
        return;
    }
    UIElement::SetText(markdown);
    m_blocks = ParseMarkdown(markdown);
    m_layoutDirty = true;
    m_selA = m_selB = 0;
    m_scrollY = m_targetScrollY = 0.0f;
    m_scrollYAnim.Reset(0.0f);
    InvalidateMeasure();
    MarkRenderRectDirty(m_bounds);
}

void MarkdownView::SetShowCodeLineNumbers(bool show) {
    if (m_showLineNumbers == show) {
        return;
    }
    m_showLineNumbers = show;
    m_layoutDirty = true;
    MarkRenderRectDirty(m_bounds);
}

HCURSOR MarkdownView::GetCursor() const {
    if (!m_hoverHref.empty()) {
        return LoadCursor(nullptr, IDC_HAND);
    }
    return LoadCursor(nullptr, IDC_IBEAM);
}

Size MarkdownView::Measure(Size availableSize) {
    float w = GetWidth();
    float h = GetHeight();
    if (w < 0.0f) {
        w = availableSize.width > 0.0f ? availableSize.width : 560.0f;
    }
    if (h < 0.0f) {
        h = availableSize.height > 0.0f ? availableSize.height : 420.0f;
    }
    m_desiredSize = Size(w, h);
    return m_desiredSize;
}

void MarkdownView::EnsureLayout(GraphicsContext& ctx) {
    const float inner = (std::max)(80.0f, m_bounds.width - kPad * 2.0f - kSb);
    if (!m_layoutDirty && std::abs(inner - m_layoutWidth) < 0.5f) {
        return;
    }
    m_layout = LayoutMarkdown(ctx, m_blocks, inner, m_showLineNumbers);
    m_layoutWidth = inner;
    m_layoutDirty = false;
    ClampScroll();
    SyncCopyButtons();
    LayoutCopyButtons();
}

void MarkdownView::Arrange(Rect finalRect) {
    if (GetVisibility() == Visibility::Collapsed) {
        SetBounds(Rect());
        m_arrangeDirty = false;
        return;
    }
    const Thickness margin = GetMargin();
    Rect arranged(
        finalRect.x + margin.left,
        finalRect.y + margin.top,
        (std::max)(0.0f, finalRect.width - margin.left - margin.right),
        (std::max)(0.0f, finalRect.height - margin.top - margin.bottom));
    SetBounds(arranged);
    LayoutCopyButtons();
    m_arrangeDirty = false;
}

void MarkdownView::SyncCopyButtons() {
    std::vector<std::string> texts;
    texts.reserve(m_layout.blocks.size());
    for (const auto& block : m_layout.blocks) {
        if (block.type == MdPaintBlock::Type::Code) {
            texts.push_back(JoinCodeLines(block.codeLines));
        }
    }
    m_copyTexts = texts;

    while (m_copyBtns.size() < m_copyTexts.size()) {
        const size_t index = m_copyBtns.size();
        auto btn = std::make_shared<Button>();
        StyleCopyButton(*btn);
        btn->OnClick().Connect([this, index](UIElement*) { CopyCodeBlock(index); });
        AddChildQuiet(btn);
        m_copyBtns.push_back(std::move(btn));
    }
    while (m_copyBtns.size() > m_copyTexts.size()) {
        RemoveChildQuiet(m_copyBtns.back());
        m_copyBtns.pop_back();
    }
}

void MarkdownView::LayoutCopyButtons() {
    const float ox = m_bounds.x + kPad;
    const float oy = m_bounds.y + kPad - m_scrollY;
    size_t i = 0;
    for (const auto& block : m_layout.blocks) {
        if (block.type != MdPaintBlock::Type::Code) {
            continue;
        }
        if (i >= m_copyBtns.size()) {
            break;
        }
        auto& btn = m_copyBtns[i];
        const Rect world(block.bounds.x + ox, block.bounds.y + oy, block.bounds.width, block.bounds.height);
        const bool visible = world.y + kCopyBtn + kCopyInset > m_bounds.y
            && world.y < m_bounds.y + m_bounds.height
            && world.width > kCopyBtn + kCopyInset * 2.0f;
        btn->SetVisibility(visible ? Visibility::Visible : Visibility::Collapsed);
        if (visible) {
            const Rect slot(
                world.x + world.width - kCopyBtn - kCopyInset,
                world.y + kCopyInset,
                kCopyBtn,
                kCopyBtn);
            btn->Measure(Size(kCopyBtn, kCopyBtn));
            btn->Arrange(slot);
        }
        ++i;
    }
}

void MarkdownView::CopyCodeBlock(size_t index) {
    if (index >= m_copyTexts.size()) {
        return;
    }
    CopyUtf8(m_copyTexts[index]);
}

void MarkdownView::ClampScroll() {
    const float viewH = (std::max)(0.0f, m_bounds.height - kPad * 2.0f);
    m_maxScrollY = (std::max)(0.0f, m_layout.height - viewH);
    m_targetScrollY = std::clamp(m_targetScrollY, 0.0f, m_maxScrollY);
    m_scrollY = std::clamp(m_scrollY, 0.0f, m_maxScrollY);
    if (!UIElement::AreAnimationsEnabled()) {
        m_scrollY = m_targetScrollY;
        m_scrollYAnim.Reset(m_scrollY);
    }
}

Point MarkdownView::ToDoc(Point pt) const {
    return Point(pt.x - m_bounds.x - kPad, pt.y - m_bounds.y - kPad + m_scrollY);
}

const MdRun* MarkdownView::HitRun(Point docPt) const {
    const MdRun* best = nullptr;
    for (const auto& run : m_layout.runs) {
        if (run.bounds.Contains(docPt.x, docPt.y)) {
            return &run;
        }
        if (docPt.y >= run.bounds.y && docPt.y <= run.bounds.y + run.bounds.height) {
            best = &run;
        }
    }
    return best;
}

int MarkdownView::HitChar(Point docPt) const {
    const MdRun* run = HitRun(docPt);
    if (!run) {
        if (docPt.y <= 0.0f) {
            return 0;
        }
        return static_cast<int>(m_layout.plain.size());
    }
    if (run->text.empty()) {
        return run->plainStart;
    }
    int best = run->plainStart;
    float bestDist = 1e9f;
    std::string acc;
    for (size_t i = 0; i <= run->text.size();) {
        const float w = acc.empty() ? 0.0f : 8.0f * static_cast<float>(acc.size());
        (void)w;
        const float x = run->bounds.x + (run->text.empty()
            ? 0.0f
            : run->bounds.width * (static_cast<float>(i) / static_cast<float>(run->text.size())));
        const float d = std::abs(docPt.x - x);
        if (d < bestDist) {
            bestDist = d;
            best = run->plainStart + static_cast<int>(i);
        }
        if (i >= run->text.size()) {
            break;
        }
        unsigned char c = static_cast<unsigned char>(run->text[i]);
        if (c < 0x80) {
            ++i;
        } else if ((c & 0xE0) == 0xC0) {
            i += 2;
        } else if ((c & 0xF0) == 0xE0) {
            i += 3;
        } else {
            i += 4;
        }
        if (i > run->text.size()) {
            i = run->text.size();
        }
    }
    return std::clamp(best, run->plainStart, run->plainEnd);
}

void MarkdownView::SetSelection(int a, int b) {
    a = std::clamp(a, 0, static_cast<int>(m_layout.plain.size()));
    b = std::clamp(b, 0, static_cast<int>(m_layout.plain.size()));
    if (a == m_selA && b == m_selB) {
        return;
    }
    m_selA = a;
    m_selB = b;
    MarkRenderRectDirty(m_bounds);
}

std::string MarkdownView::GetSelectedText() const {
    const int lo = SelLo(m_selA, m_selB);
    const int hi = SelHi(m_selA, m_selB);
    if (hi <= lo) {
        return {};
    }
    return m_layout.plain.substr(static_cast<size_t>(lo), static_cast<size_t>(hi - lo));
}

void MarkdownView::SelectAll() {
    SetSelection(0, static_cast<int>(m_layout.plain.size()));
}

bool MarkdownView::CopySelection() {
    std::string text = GetSelectedText();
    if (text.empty()) {
        text = m_layout.plain;
    }
    return CopyUtf8(text);
}

void MarkdownView::OpenLink(const std::string& href) {
    m_onLinkClicked.Invoke(this, href);
    if (href.rfind("http://", 0) == 0 || href.rfind("https://", 0) == 0 || href.rfind("mailto:", 0) == 0) {
        ShellExecuteW(nullptr, L"open", Utf8ToUtf16(href).c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    }
}

Rect MarkdownView::ScrollbarTrack() const {
    return Rect(m_bounds.x + m_bounds.width - kSb, m_bounds.y + 4.0f, 6.0f, m_bounds.height - 8.0f);
}

Rect MarkdownView::ScrollbarThumb() const {
    const Rect track = ScrollbarTrack();
    if (m_maxScrollY <= 0.0f) {
        return Rect();
    }
    const float viewH = (std::max)(1.0f, m_bounds.height - kPad * 2.0f);
    const float thumbH = std::clamp(track.height * (viewH / (viewH + m_maxScrollY)), 20.0f, track.height);
    const float t = m_scrollY / m_maxScrollY;
    return Rect(track.x, track.y + t * (track.height - thumbH), track.width, thumbH);
}

void MarkdownView::OnRender(GraphicsContext& ctx) {
    EnsureLayout(ctx);
    ClampScroll();

    auto& theme = ThemeManager::Instance();
    const float radius = GetCornerRadius() > 0.0f ? GetCornerRadius() : 6.0f;
    const auto bg = ResolveThemeColor(GetBackgroundToken(), ThemeTokenId::CardBackground);
    const auto border = ResolveThemeColor(GetBorderToken(), ThemeTokenId::CardBorder);
    const auto text = ResolveThemeColor(GetColorToken(), ThemeTokenId::TextPrimary);
    const auto muted = theme.GetColor(ThemeTokenId::TextMuted);
    const auto accent = theme.GetColor(ThemeTokenId::AccentColor);
    const auto codeBg = theme.GetColor(ThemeTokenId::InputBackground);
    const auto pane = theme.GetColor(ThemeTokenId::PaneBackground);

    ctx.FillRoundedRect(m_bounds, radius, bg);
    ctx.DrawRoundedRect(m_bounds, radius, border, GetBorderThickness() > 0.0f ? GetBorderThickness() : 1.0f);

    ctx.PushClip(Rect(m_bounds.x + 1.0f, m_bounds.y + 1.0f, m_bounds.width - 2.0f, m_bounds.height - 2.0f));
    const float ox = m_bounds.x + kPad;
    const float oy = m_bounds.y + kPad - m_scrollY;
    const Rect vis(0.0f, m_scrollY - 20.0f, m_layoutWidth + 40.0f, m_bounds.height + 40.0f);

    const int selLo = SelLo(m_selA, m_selB);
    const int selHi = SelHi(m_selA, m_selB);

    for (const auto& block : m_layout.blocks) {
        const Rect world(block.bounds.x + ox, block.bounds.y + oy, block.bounds.width, block.bounds.height);
        if (world.y + world.height < m_bounds.y || world.y > m_bounds.y + m_bounds.height) {
            continue;
        }
        if (block.type == MdPaintBlock::Type::Quote) {
            ctx.FillRoundedRect(world, 4.0f, D2D1::ColorF(accent.r, accent.g, accent.b, 0.08f));
            ctx.FillRect(Rect(world.x, world.y, 3.0f, world.height), accent);
        } else if (block.type == MdPaintBlock::Type::Code) {
            D2D1_COLOR_F fill = codeBg;
            fill.a = 1.0f;
            ctx.FillRoundedRect(world, 6.0f, fill);
            ctx.DrawRoundedRect(world, 6.0f, border, 1.0f);
        } else if (block.type == MdPaintBlock::Type::Hr) {
            ctx.FillRect(
                Rect(world.x, world.y + world.height * 0.5f, world.width, 1.0f),
                D2D1::ColorF(border.r, border.g, border.b, 0.85f));
        } else if (block.type == MdPaintBlock::Type::Table && block.tableCols > 0) {
            ctx.FillRoundedRect(world, 4.0f, pane);
            ctx.DrawRoundedRect(world, 4.0f, border, 1.0f);
            const float colW = world.width / static_cast<float>(block.tableCols);
            const float rowH = world.height / static_cast<float>((std::max)(1, block.tableRows));
            ctx.FillRect(Rect(world.x, world.y, world.width, rowH), D2D1::ColorF(accent.r, accent.g, accent.b, 0.10f));
            for (int c = 1; c < block.tableCols; ++c) {
                ctx.FillRect(Rect(world.x + colW * static_cast<float>(c), world.y, 1.0f, world.height), border);
            }
            for (int r = 1; r < block.tableRows; ++r) {
                ctx.FillRect(Rect(world.x, world.y + rowH * static_cast<float>(r), world.width, 1.0f), border);
            }
        } else if (block.headingLevel == 1 || block.headingLevel == 2) {
            ctx.FillRect(
                Rect(world.x, world.y + world.height + 2.0f, world.width, 1.0f),
                D2D1::ColorF(border.r, border.g, border.b, 0.55f));
        }
    }

    if (selHi > selLo) {
        for (const auto& run : m_layout.runs) {
            const int a = (std::max)(selLo, run.plainStart);
            const int b = (std::min)(selHi, run.plainEnd);
            if (b <= a || run.plainEnd <= run.plainStart) {
                continue;
            }
            const float span = static_cast<float>(run.plainEnd - run.plainStart);
            const float x0 = run.bounds.x + run.bounds.width * (static_cast<float>(a - run.plainStart) / span);
            const float x1 = run.bounds.x + run.bounds.width * (static_cast<float>(b - run.plainStart) / span);
            ctx.FillRect(
                Rect(ox + x0, oy + run.bounds.y, (std::max)(2.0f, x1 - x0), (std::max)(run.bounds.height, 16.0f)),
                D2D1::ColorF(accent.r, accent.g, accent.b, 0.28f));
        }
    }

    for (const auto& run : m_layout.runs) {
        if (run.bounds.y + run.bounds.height < vis.y || run.bounds.y > vis.y + vis.height) {
            continue;
        }
        D2D1_COLOR_F color = text;
        if (run.link) {
            color = accent;
        } else if (run.code && run.size <= 11.5f) {
            color = muted;
        }
        const Rect dest(ox + run.bounds.x, oy + run.bounds.y, (std::max)(1.0f, run.bounds.width + 2.0f), run.bounds.height);
        if (run.code && run.size > 11.5f && run.plainEnd > run.plainStart) {
            ctx.FillRoundedRect(
                Rect(dest.x - 3.0f, dest.y + 1.0f, dest.width + 6.0f, dest.height - 1.0f),
                3.0f,
                D2D1::ColorF(codeBg.r, codeBg.g, codeBg.b, 0.95f));
        }
        if (run.italic) {
            GraphicsContext::TextLayoutOptions opt;
            opt.wrapping = DWRITE_WORD_WRAPPING_NO_WRAP;
            auto layout = ctx.CreateTextLayout(Utf8ToUtf16(run.text), run.font, run.size, opt, run.weight);
            if (layout) {
                const std::wstring w = Utf8ToUtf16(run.text);
                layout->SetFontStyle(DWRITE_FONT_STYLE_ITALIC, DWRITE_TEXT_RANGE{ 0, static_cast<UINT32>(w.size()) });
                ctx.DrawTextLayout(layout.Get(), dest, color);
            }
        } else {
            ctx.DrawText(
                run.text,
                dest,
                color,
                run.font,
                run.size,
                DWRITE_TEXT_ALIGNMENT_LEADING,
                DWRITE_PARAGRAPH_ALIGNMENT_NEAR,
                run.weight);
        }
        if (run.link) {
            ctx.FillRect(
                Rect(dest.x, dest.y + dest.height - 2.0f, dest.width, 1.0f),
                accent);
        }
    }

    if (m_maxScrollY > 0.0f && m_scrollbarAutoHide.IsDrawn()) {
        const Rect thumb = ScrollbarThumb();
        const float visOp = m_scrollbarAutoHide.Opacity();
        ctx.FillRoundedRect(
            thumb,
            3.0f,
            D2D1::ColorF(border.r, border.g, border.b, (m_draggingScrollbar ? 0.85f : 0.45f) * visOp));
    }
    ctx.PopClip();
}

void MarkdownView::OnMouseDown(Point pt) {
    Control::OnMouseDown(pt);
    if (m_maxScrollY > 0.0f && ScrollbarTrack().Contains(pt.x, pt.y)) {
        m_draggingScrollbar = true;
        m_scrollbarAutoHide.SetDragging(true, this);
        m_scrollbarAutoHide.NotifyActivity(this);
        m_dragStartY = pt.y;
        m_dragStartScroll = m_scrollY;
        RequestAnimationTicks();
        return;
    }
    const Point doc = ToDoc(pt);
    if (const MdRun* run = HitRun(doc)) {
        m_hoverHref = run->link ? run->href : "";
    }
    m_selecting = true;
    const int idx = HitChar(doc);
    SetSelection(idx, idx);
}

void MarkdownView::OnMouseMove(Point pt) {
    Control::OnMouseMove(pt);
    const bool overBar = m_maxScrollY > 0.0f && pt.x >= m_bounds.x + m_bounds.width - kSb;
    m_scrollbarAutoHide.SetPointerOver(overBar, this);
    if (overBar) {
        RequestAnimationTicks();
    }

    if (m_draggingScrollbar && m_isPressed) {
        const Rect track = ScrollbarTrack();
        const Rect thumb = ScrollbarThumb();
        const float travel = (std::max)(1.0f, track.height - thumb.height);
        m_targetScrollY = m_dragStartScroll + (pt.y - m_dragStartY) / travel * m_maxScrollY;
        ClampScroll();
        m_scrollY = m_targetScrollY;
        m_scrollYAnim.Reset(m_scrollY);
        m_scrollbarAutoHide.NotifyActivity(this);
        LayoutCopyButtons();
        MarkRenderRectDirty(m_bounds);
        return;
    }

    const Point doc = ToDoc(pt);
    std::string href;
    if (const MdRun* run = HitRun(doc)) {
        if (run->link) {
            href = run->href;
        }
    }
    if (href != m_hoverHref) {
        m_hoverHref = href;
        MarkRenderRectDirty(m_bounds);
    }
    if (m_selecting && m_isPressed) {
        SetSelection(m_selA, HitChar(doc));
    }
}

void MarkdownView::OnMouseUp(Point pt) {
    const bool wasSelecting = m_selecting;
    const int lo = SelLo(m_selA, m_selB);
    const int hi = SelHi(m_selA, m_selB);
    Control::OnMouseUp(pt);
    m_draggingScrollbar = false;
    m_selecting = false;
    m_scrollbarAutoHide.SetDragging(false, this);
    if (wasSelecting && hi == lo && !m_hoverHref.empty()) {
        OpenLink(m_hoverHref);
    }
    RequestAnimationTicks();
}

void MarkdownView::OnMouseLeave() {
    Control::OnMouseLeave();
    m_hoverHref.clear();
    m_scrollbarAutoHide.SetPointerOver(false, this);
    RequestAnimationTicks();
}

void MarkdownView::OnMouseDblClick(Point pt) {
    Control::OnMouseDblClick(pt);
    const int idx = HitChar(ToDoc(pt));
    int a = idx;
    int b = idx;
    const auto& s = m_layout.plain;
    while (a > 0 && IsWordChar(static_cast<unsigned char>(s[static_cast<size_t>(a - 1)]))) {
        --a;
    }
    while (b < static_cast<int>(s.size()) && IsWordChar(static_cast<unsigned char>(s[static_cast<size_t>(b)]))) {
        ++b;
    }
    SetSelection(a, b);
}

void MarkdownView::OnMouseWheel(float delta) {
    if (m_maxScrollY <= 0.0f) {
        UIElement::OnMouseWheel(delta);
        return;
    }
    const float prev = m_targetScrollY;
    m_targetScrollY -= delta * 48.0f;
    ClampScroll();
    if (std::abs(m_targetScrollY - prev) < 0.001f) {
        UIElement::OnMouseWheel(delta);
        return;
    }
    m_scrollYAnim.SetTarget(m_targetScrollY);
    if (!UIElement::AreAnimationsEnabled()) {
        m_scrollY = m_targetScrollY;
        m_scrollYAnim.Reset(m_scrollY);
    } else {
        RequestAnimationTicks();
    }
    m_scrollbarAutoHide.NotifyActivity(this);
    LayoutCopyButtons();
    MarkRenderRectDirty(m_bounds);
}

bool MarkdownView::OnKeyDown(int vkCode) {
    const bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
    if (ctrl && vkCode == 'A') {
        SelectAll();
        return true;
    }
    if (ctrl && vkCode == 'C') {
        CopySelection();
        return true;
    }
    if (vkCode == VK_HOME) {
        m_targetScrollY = 0.0f;
        ClampScroll();
        m_scrollYAnim.SetTarget(m_targetScrollY);
        LayoutCopyButtons();
        RequestAnimationTicks();
        MarkRenderRectDirty(m_bounds);
        return true;
    }
    if (vkCode == VK_END) {
        m_targetScrollY = m_maxScrollY;
        ClampScroll();
        m_scrollYAnim.SetTarget(m_targetScrollY);
        LayoutCopyButtons();
        RequestAnimationTicks();
        MarkRenderRectDirty(m_bounds);
        return true;
    }
    return Control::OnKeyDown(vkCode);
}

bool MarkdownView::OnAnimationTick() {
    bool any = UIElement::OnAnimationTick();
    const float dt = UIElement::GetAnimationDeltaSeconds();
    const float prev = m_scrollY;
    if (m_scrollYAnim.Tick(dt, AnimationSpec{ 0.55f, 0.5f })) {
        m_scrollY = m_scrollYAnim.Current();
        any = true;
    } else if (!UIElement::AreAnimationsEnabled()) {
        m_scrollY = m_targetScrollY;
    }
    if (m_scrollbarAutoHide.Tick(dt)) {
        any = true;
    }
    if (any || std::abs(m_scrollY - prev) > 0.1f) {
        LayoutCopyButtons();
        MarkRenderRectDirty(m_bounds);
    }
    return any;
}

bool MarkdownView::HasSelfAnimation() const {
    return m_scrollYAnim.IsAnimating(0.05f) || m_scrollbarAutoHide.NeedsTicks();
}

void MarkdownView::OnThemeChanged() {
    m_layout = {};
    m_layoutDirty = true;
    m_layoutWidth = -1.0f;
    m_renderNode.GetLayer().ResetCache();
    for (auto& btn : m_copyBtns) {
        if (btn) {
            StyleCopyButton(*btn);
        }
    }
    UIElement::OnThemeChanged();
    MarkRenderRectDirty(m_bounds);
}

} // namespace CUI
