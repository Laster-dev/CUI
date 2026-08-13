#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "LogView.h"
#include "Button.h"
#include "TextBox.h"
#include "ScrollViewer.h"
#include "../window/Window.h"
#include "../style/ThemeManager.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <windows.h>

namespace CUI {
namespace {

constexpr const char* kLevelNames[] = { "TRC", "DBG", "INF", "WRN", "ERR", "FTL" };
constexpr const char* kChipNames[] = { "T", "D", "I", "W", "E", "F" };

constexpr const char* kSvgCopy =
    "<svg viewBox=\"0 0 1024 1024\" xmlns=\"http://www.w3.org/2000/svg\">"
    "<path d=\"M661.333333 234.666667A64 64 0 0 1 725.333333 298.666667v597.333333a64 64 0 0 1-64 64h-469.333333A64 64 0 0 1 128 896V298.666667a64 64 0 0 1 64-64z m-21.333333 85.333333H213.333333v554.666667h426.666667v-554.666667z m191.829333-256a64 64 0 0 1 63.744 57.856l0.256 6.144v575.701333a42.666667 42.666667 0 0 1-85.034666 4.992l-0.298667-4.992V149.333333H384a42.666667 42.666667 0 0 1-42.368-37.674666L341.333333 106.666667a42.666667 42.666667 0 0 1 37.674667-42.368L384 64h447.829333z\"/>"
    "</svg>";

constexpr const char* kSvgClear =
    "<svg viewBox=\"0 0 1024 1024\" xmlns=\"http://www.w3.org/2000/svg\">"
    "<path d=\"M229.12 475.52l687.296 183.872-1.408 1.472-1.024 12.864c-2.56 34.048-6.4 62.976-17.92 139.52l-6.528 43.648c-3.84 26.048-6.528 46.976-8.768 66.304-4.224 36.544-23.488 65.28-54.656 81.6-15.424 8.128-32 12.672-45.824 13.44l-4.032 0.192-90.432-24.32a18.88 18.88 0 0 1-12.16-26.688l35.392-71.552-20.352 26.24a515.328 515.328 0 0 1-47.36 50.56 19.008 19.008 0 0 1-18.112 4.672l-75.456-20.16a18.88 18.88 0 0 1-12.16-26.624l35.456-71.552-20.352 26.24a510.336 510.336 0 0 1-47.36 50.56 18.944 18.944 0 0 1-18.112 4.672l-88.448-23.68a18.944 18.944 0 0 1-12.096-26.56l35.392-71.552-20.352 26.24a515.84 515.84 0 0 1-47.36 50.56 18.944 18.944 0 0 1-18.112 4.672l-62.528-16.704a18.944 18.944 0 0 1-12.096-26.688l35.392-71.488-20.352 26.24c-8.32 10.688-20.48 24-36.48 39.936a44.736 44.736 0 0 1-43.136 11.52l-78.08-20.864-23.104-8.704c-29.632-25.536-44.608-61.44-39.36-95.552 5.76-37.12 32.32-67.648 82.112-88.704 34.368-14.656 67.584-74.176 89.664-157.44l3.84-14.464-1.024-1.792z m-42.88-76.16a60.224 60.224 0 0 1 5.056-6.4l123.776-135.424c9.92-10.752 24.832-19.392 41.856-23.936 16.64-4.416 33.984-4.672 48.64-0.768l123.584 33.024 4.096-2.368 55.36-205.824c8.704-32.448 43.968-51.456 78.72-42.24l83.2 22.272c34.752 9.28 55.808 43.392 47.104 75.84l-55.36 205.824 2.368 4.096 118.528 31.68c14.592 3.84 29.568 12.8 41.728 24.96 12.416 12.352 21.12 27.2 24.192 41.472l39.36 179.072a58.688 58.688 0 0 1 1.28 9.856c-0.704 6.592-5.312 10.304-13.696 11.2L194.944 418.048c-10.432-2.944-13.376-9.216-8.768-18.752z\"/>"
    "</svg>";

constexpr const char* kSvgFollow =
    "<svg viewBox=\"0 0 1024 1024\" xmlns=\"http://www.w3.org/2000/svg\">"
    "<path d=\"M512 64a448 448 0 1 1 0 896A448 448 0 0 1 512 64z m0 76.8a371.2 371.2 0 1 0 0 742.4A371.2 371.2 0 0 0 512 140.8zM576 384a64 64 0 0 1 64 64v128a64 64 0 0 1-64 64H448a64 64 0 0 1-64-64V448a64 64 0 0 1 64-64h128z\"/>"
    "</svg>";

bool CopyUtf8(const std::string& text) {
    if (text.empty()) {
        return false;
    }
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

float GetChromiumWheelStep(float viewportHeight) {
    UINT lines = 3;
    SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &lines, 0);
    if (lines == WHEEL_PAGESCROLL) {
        return (std::max)(40.0f, viewportHeight);
    }
    return (std::max)(1u, lines) * 40.0f;
}

void FoldAsciiInPlace(std::string& s) {
    for (char& c : s) {
        if (c >= 'A' && c <= 'Z') {
            c = static_cast<char>(c + 32);
        }
    }
}

bool ContainsFolded(const std::string& hay, const std::string& needleFold) {
    if (needleFold.empty()) {
        return true;
    }
    const size_t n = needleFold.size();
    const size_t h = hay.size();
    if (n > h) {
        return false;
    }
    for (size_t i = 0; i + n <= h; ++i) {
        bool ok = true;
        for (size_t j = 0; j < n; ++j) {
            char c = hay[i + j];
            if (c >= 'A' && c <= 'Z') {
                c = static_cast<char>(c + 32);
            }
            if (c != needleFold[j]) {
                ok = false;
                break;
            }
        }
        if (ok) {
            return true;
        }
    }
    return false;
}

void StampTime(char out[13]) {
    SYSTEMTIME st{};
    GetLocalTime(&st);
    std::snprintf(out, 13, "%02u:%02u:%02u.%03u",
        st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
}

D2D1_COLOR_F WithAlpha(D2D1_COLOR_F c, float a) {
    c.a = std::clamp(a, 0.0f, 1.0f);
    return c;
}

} // namespace

LogView::LogView() {
    m_buf.resize(m_cap);
    SetBackgroundToken(ThemeTokenId::CardBackground);
    SetBorderToken(ThemeTokenId::CardBorder);
    SetColorToken(ThemeTokenId::TextPrimary);
    SetBorderThickness(1.0f);
    SetCornerRadius(6.0f);
    SetAlign(Alignment::Stretch);
    SetClipToBounds(true);
    m_expandedHeight = 280.0f;
    BuildChrome();
    SyncChipAnimTargets(true);
    m_expandAnim.Reset(m_expanded ? 1.0f : 0.0f);
    SetHeight(ExpandHeight());
}

LogView::~LogView() {
    FlushPersist();
}

void LogView::StyleIconButton(Button& btn, const char* svg, const char* tooltip, ThemeTokenId color) {
    btn.SetText("");
    btn.SetIcon(svg);
    btn.SetToolTip(tooltip);
    btn.SetWidth(kIconBtn);
    btn.SetHeight(kIconBtn);
    btn.SetFontSize(16.0f);
    btn.SetPadding(Thickness(5.0f));
    btn.SetCornerRadius(4.0f);
    btn.SetBorderThickness(0.0f);
    btn.SetBackgroundToken(ThemeTokenId::Unset);
    btn.SetHoverBackgroundToken(ThemeTokenId::HoverBackground);
    btn.SetPressedBackgroundToken(ThemeTokenId::PressedBackground);
    btn.SetBackground(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f));
    btn.SetColorToken(color);
}

void LogView::BuildChrome() {
    m_search = std::make_shared<TextBox>();
    m_search->SetPlaceholder("搜索 消息 / 分类");
    m_search->SetHeight(26.0f);
    m_search->SetFontSize(12.0f);
    m_search->SetPadding(Thickness(8.0f, 2.0f, 8.0f, 2.0f));
    m_search->SetCornerRadius(4.0f);
    m_search->SetBackgroundToken(ThemeTokenId::InputBackground);
    m_search->OnTextChanged().Connect([this](TextBox*, const std::string& text) {
        if (!m_syncingUi) {
            SetFilterText(text);
        }
    });

    m_btnCopy = std::make_shared<Button>();
    StyleIconButton(*m_btnCopy, kSvgCopy, "复制", ThemeTokenId::AccentColor);
    m_btnCopy->OnClick().Connect([this](UIElement*) { CopySelection(); });

    m_btnClear = std::make_shared<Button>();
    StyleIconButton(*m_btnClear, kSvgClear, "清空", ThemeTokenId::AccentColor);
    m_btnClear->OnClick().Connect([this](UIElement*) { Clear(); });

    m_btnFollow = std::make_shared<Button>();
    StyleIconButton(*m_btnFollow, kSvgFollow, "跟随", ThemeTokenId::TextPrimary);
    m_btnFollow->OnClick().Connect([this](UIElement*) { SetFollowTail(!m_follow); });

    AddChild(m_search);
    AddChild(m_btnCopy);
    AddChild(m_btnClear);
    AddChild(m_btnFollow);
    ApplyChromeVisibility();
    SyncActionButtons();
}

void LogView::ApplyChromeVisibility() {
    const Visibility vis = (ExpandProgress() > 0.08f) ? Visibility::Visible : Visibility::Collapsed;
    auto apply = [vis](const std::shared_ptr<UIElement>& el) {
        if (el && el->GetVisibility() != vis) {
            el->SetVisibility(vis);
        }
    };
    apply(m_search);
    apply(m_btnCopy);
    apply(m_btnClear);
    apply(m_btnFollow);
}

void LogView::SyncActionButtons() {
    if (!m_btnFollow) {
        return;
    }
    m_btnFollow->SetColorToken(m_follow ? ThemeTokenId::AccentColor : ThemeTokenId::TextPrimary);
    m_btnFollow->SetToolTip(m_follow ? "跟随中" : "跟随");
}

float LogView::ExpandProgress() const {
    return std::clamp(m_expandAnim.Current(), 0.0f, 1.0f);
}

float LogView::ExpandHeight() const {
    const float openH = (m_expandedHeight > kHeaderH) ? m_expandedHeight : 280.0f;
    return kHeaderH + (openH - kHeaderH) * ExpandProgress();
}

bool LogView::BodyInteractive() const {
    return ExpandProgress() > 0.35f;
}

void LogView::ApplyExpandLayout() {
    SetHeight(ExpandHeight());
    ApplyChromeVisibility();
    InvalidateMeasure();
    for (UIElement* walk = GetParent(); walk; walk = walk->GetParent()) {
        if (auto* scroll = dynamic_cast<ScrollViewer*>(walk)) {
            scroll->InvalidateContentLayout();
            break;
        }
    }
    if (Window* window = Window::Current()) {
        window->Relayout();
    }
}

void LogView::SyncChipAnimTargets(bool snap) {
    for (int i = 0; i < 6; ++i) {
        const float t = GetLevelEnabled(static_cast<LogLevel>(i)) ? 1.0f : 0.0f;
        if (snap || !UIElement::AreAnimationsEnabled()) {
            m_chipAnim[i].Reset(t);
        } else {
            m_chipAnim[i].SetTarget(t);
        }
    }
    if (!snap && UIElement::AreAnimationsEnabled()) {
        RequestAnimationTicks();
    }
}

void LogView::LayoutChrome() {
    if (ExpandProgress() < 0.05f) {
        return;
    }
    auto place = [](const std::shared_ptr<UIElement>& el, const Rect& r) {
        if (!el || el->GetVisibility() == Visibility::Collapsed) {
            return;
        }
        el->Measure(Size(r.width, r.height));
        el->Arrange(r);
    };
    place(m_search, SearchRect());
    place(m_btnCopy, ToolBtnRect(0));
    place(m_btnClear, ToolBtnRect(1));
    place(m_btnFollow, ToolBtnRect(2));
}

std::vector<PropertyMeta> LogView::GetPropertyMetas() const {
    auto metas = UIElement::GetPropertyMetas();
    metas.push_back({ "text", "搜索过滤 (Filter)", "日志配置", "string" });
    metas.push_back({ "isOn", "持久化 (Persist)", "日志配置", "bool" });
    return metas;
}

Value LogView::GetProperty(PropertyId id) const {
    switch (id) {
    case PropertyId::Text: return Value(GetFilterText());
    case PropertyId::IsOn: return Value(m_persistEnabled);
    default: return Control::GetProperty(id);
    }
}

bool LogView::HasProperty(PropertyId id) const {
    return id == PropertyId::Text || id == PropertyId::IsOn || Control::HasProperty(id);
}

void LogView::SetProperty(PropertyId id, const Value& val) {
    switch (id) {
    case PropertyId::Text: SetFilterText(val.AsString()); return;
    case PropertyId::IsOn: SetPersistEnabled(val.AsBool()); return;
    default: UIElement::SetProperty(id, val); return;
    }
}

HCURSOR LogView::GetCursor() const {
    if (!IsEnabled()) {
        return nullptr;
    }
    if (m_hoverPart == Part::Header || m_hoverPart == Part::Chip) {
        return LoadCursor(nullptr, IDC_HAND);
    }
    return LoadCursor(nullptr, IDC_ARROW);
}

Size LogView::Measure(Size availableSize) {
    (void)availableSize;
    if (m_search) {
        m_search->Measure(Size(200.0f, 26.0f));
    }
    float w = GetWidth();
    if (w < 0.0f) {
        w = 520.0f;
    }
    m_desiredSize = Size(w, ExpandHeight());
    m_measureDirty = false;
    m_lastMeasureAvailable = availableSize;
    return m_desiredSize;
}

void LogView::Arrange(Rect finalRect) {
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
    LayoutChrome();
    m_arrangeDirty = false;
}

const LogView::Record& LogView::AtLogical(uint32_t logical) const {
    return m_buf[(m_begin + logical) % m_cap];
}

const LogView::Record* LogView::RecordBySeq(uint32_t seq) const {
    if (m_size == 0 || seq < m_firstSeq || seq >= m_nextSeq) {
        return nullptr;
    }
    return &AtLogical(seq - m_firstSeq);
}

size_t LogView::GetVisibleCount() const {
    if (!m_filterActive) {
        return m_size;
    }
    return m_visibleSeq.size() - m_visibleHead;
}

uint32_t LogView::VisibleSeqAt(size_t i) const {
    if (!m_filterActive) {
        return m_firstSeq + static_cast<uint32_t>(i);
    }
    return m_visibleSeq[m_visibleHead + i];
}

const LogView::Record* LogView::VisibleRecord(size_t i) const {
    if (i >= GetVisibleCount()) {
        return nullptr;
    }
    return RecordBySeq(VisibleSeqAt(i));
}

uint32_t LogView::GetLevelCount(LogLevel level) const {
    const int i = static_cast<int>(level);
    return (i >= 0 && i < 6) ? m_levelCounts[i] : 0;
}

bool LogView::GetLevelEnabled(LogLevel level) const {
    const int i = static_cast<int>(level);
    return (i >= 0 && i < 6) && ((m_levelMask & (1u << i)) != 0);
}

void LogView::SetLevelEnabled(LogLevel level, bool enabled) {
    const int i = static_cast<int>(level);
    if (i < 0 || i >= 6) {
        return;
    }
    const uint8_t bit = static_cast<uint8_t>(1u << i);
    SetLevelMask(enabled ? (m_levelMask | bit) : static_cast<uint8_t>(m_levelMask & ~bit));
}

void LogView::SetLevelMask(uint8_t mask) {
    mask &= kAllLevels;
    if (mask == m_levelMask) {
        return;
    }
    m_levelMask = mask;
    SyncChipAnimTargets(false);
    RebuildVisible();
    ClampScroll();
    DirtyHeader();
    DirtyBody();
}

std::string LogView::GetFilterText() const {
    return m_search ? m_search->GetText() : std::string();
}

void LogView::SetFilterText(const std::string& text) {
    m_filterFold = text;
    FoldAsciiInPlace(m_filterFold);
    if (m_search && m_search->GetText() != text) {
        m_syncingUi = true;
        m_search->SetText(text);
        m_syncingUi = false;
    }
    RebuildVisible();
    ClampScroll();
    if (m_follow) {
        ScrollToTail();
    }
    NotifyFieldChanged(PropertyId::Text, Value(text));
    DirtyHeader();
    DirtyBody();
}

bool LogView::Matches(const Record& rec) const {
    const int lv = static_cast<int>(rec.level);
    if (lv < 0 || lv > 5 || (m_levelMask & (1u << lv)) == 0) {
        return false;
    }
    if (m_filterFold.empty()) {
        return true;
    }
    return ContainsFolded(rec.message, m_filterFold) || ContainsFolded(rec.category, m_filterFold);
}

void LogView::RebuildVisible() {
    m_visibleSeq.clear();
    m_visibleHead = 0;
    m_filterActive = (m_levelMask != kAllLevels) || !m_filterFold.empty();
    if (!m_filterActive) {
        return;
    }
    m_visibleSeq.reserve(m_size);
    for (uint32_t i = 0; i < m_size; ++i) {
        const Record& rec = AtLogical(i);
        if (Matches(rec)) {
            m_visibleSeq.push_back(rec.seq);
        }
    }
}

void LogView::DropOldestIfFull() {
    if (m_size < m_cap) {
        return;
    }
    const Record& old = AtLogical(0);
    const int lv = static_cast<int>(old.level);
    if (lv >= 0 && lv < 6 && m_levelCounts[lv] > 0) {
        --m_levelCounts[lv];
    }
    if (m_filterActive && m_visibleHead < m_visibleSeq.size()
        && m_visibleSeq[m_visibleHead] == old.seq) {
        ++m_visibleHead;
        if (m_visibleHead > 1024 && m_visibleHead * 2 > m_visibleSeq.size()) {
            m_visibleSeq.erase(m_visibleSeq.begin(), m_visibleSeq.begin() + static_cast<int>(m_visibleHead));
            m_visibleHead = 0;
        }
    }
    m_begin = (m_begin + 1) % m_cap;
    --m_size;
    ++m_firstSeq;
}

void LogView::Append(LogLevel level, std::string message) {
    Append(level, std::string(), std::move(message));
}

void LogView::Append(LogLevel level, std::string category, std::string message) {
    DropOldestIfFull();
    Record rec;
    rec.seq = m_nextSeq++;
    rec.level = level;
    StampTime(rec.time);
    rec.category = std::move(category);
    rec.message = std::move(message);
    const uint32_t slot = (m_begin + m_size) % m_cap;
    m_buf[slot] = std::move(rec);
    ++m_size;
    const int lv = static_cast<int>(level);
    if (lv >= 0 && lv < 6) {
        ++m_levelCounts[lv];
    }
    const Record& stored = m_buf[slot];
    if (m_filterActive && Matches(stored)) {
        m_visibleSeq.push_back(stored.seq);
    }
    QueuePersist(stored);
    if (m_follow) {
        ScrollToTail();
    } else {
        ClampScroll();
    }
    DirtyHeader();
    if (m_expanded && m_follow) {
        DirtyBody();
    }
}

void LogView::Clear() {
    m_begin = 0;
    m_size = 0;
    m_nextSeq = 1;
    m_firstSeq = 1;
    std::memset(m_levelCounts, 0, sizeof(m_levelCounts));
    m_visibleSeq.clear();
    m_visibleHead = 0;
    m_scrollY = 0.0f;
    m_maxScrollY = 0.0f;
    m_scrollAnimator.JumpTo(0.0f);
    m_selA = m_selB = m_hoverRow = -1;
    MarkRenderRectDirty(m_bounds);
}

void LogView::SetExpanded(bool expanded) {
    if (m_expanded == expanded) {
        return;
    }
    const Rect prev = m_bounds;
    if (ExpandProgress() > 0.95f && GetHeight() > kHeaderH + 0.5f) {
        m_expandedHeight = GetHeight();
    }
    m_expanded = expanded;
    if (!m_expanded) {
        m_dragScrollbar = false;
        m_selecting = false;
    }
    const bool live = UIElement::AreAnimationsEnabled() && GetParent() != nullptr;
    if (live) {
        m_expandAnim.SetTarget(expanded ? 1.0f : 0.0f);
        RequestAnimationTicks();
    } else {
        m_expandAnim.Reset(expanded ? 1.0f : 0.0f);
    }
    ApplyExpandLayout();
    m_onExpandedChanged.Invoke(this);
    MarkRenderRectDirty(prev.Union(m_bounds).Inflate(4.0f));
}

void LogView::SetMaxEntries(uint32_t capacity) {
    capacity = (std::max)(64u, (std::min)(capacity, 200000u));
    if (capacity == m_cap) {
        return;
    }
    std::vector<Record> next(capacity);
    const uint32_t keep = (std::min)(m_size, capacity);
    const uint32_t skip = m_size - keep;
    for (uint32_t i = 0; i < keep; ++i) {
        next[i] = AtLogical(skip + i);
    }
    m_buf.swap(next);
    m_cap = capacity;
    m_begin = 0;
    m_size = keep;
    if (keep > 0) {
        m_firstSeq = AtLogical(0).seq;
    }
    RebuildVisible();
    ClampScroll();
    MarkRenderRectDirty(m_bounds);
}

void LogView::SetPersistEnabled(bool enabled) {
    if (m_persistEnabled == enabled) {
        return;
    }
    m_persistEnabled = enabled;
    if (enabled) {
        EnsurePersistPath();
    } else {
        FlushPersist();
    }
    SyncActionButtons();
    NotifyFieldChanged(PropertyId::IsOn, Value(enabled));
    DirtyHeader();
}

void LogView::SetPersistPath(std::string path) {
    if (m_persistPath == path) {
        return;
    }
    FlushPersist();
    m_persistPath = std::move(path);
}

void LogView::EnsurePersistPath() {
    if (!m_persistPath.empty()) {
        return;
    }
    wchar_t tmp[MAX_PATH]{};
    GetTempPathW(MAX_PATH, tmp);
    m_persistPath = Utf16ToUtf8(std::wstring(tmp) + L"cui-logview.log");
}

void LogView::QueuePersist(const Record& rec) {
    if (!m_persistEnabled) {
        return;
    }
    EnsurePersistPath();
    m_persistBuf.append(rec.time);
    m_persistBuf.push_back(' ');
    m_persistBuf.push_back('[');
    m_persistBuf.append(LevelTag(rec.level));
    m_persistBuf.append("] ");
    if (!rec.category.empty()) {
        m_persistBuf.append(rec.category);
        m_persistBuf.push_back(' ');
    }
    m_persistBuf.append(rec.message);
    m_persistBuf.push_back('\n');
    if (m_persistBuf.size() >= 8192) {
        FlushPersist();
    }
}

void LogView::FlushPersist() {
    if (m_persistBuf.empty() || m_persistPath.empty()) {
        return;
    }
    FILE* f = nullptr;
    const std::wstring wpath = Utf8ToUtf16(m_persistPath);
    if (_wfopen_s(&f, wpath.c_str(), L"ab") != 0 || !f) {
        m_persistBuf.clear();
        return;
    }
    fwrite(m_persistBuf.data(), 1, m_persistBuf.size(), f);
    fclose(f);
    m_persistBuf.clear();
}

void LogView::SetFollowTail(bool follow) {
    if (m_follow == follow) {
        return;
    }
    m_follow = follow;
    SyncActionButtons();
    if (m_follow) {
        ClampScroll();
        SetScrollTarget(m_maxScrollY, true);
        DirtyBody();
    }
}

int LogView::ViewRowCount() const {
    return (std::max)(0, static_cast<int>(BodyRect().height / kRowH));
}

void LogView::ScrollToTail() {
    ClampScroll();
    m_scrollY = m_maxScrollY;
    m_scrollAnimator.JumpTo(m_scrollY);
}

void LogView::SetScrollTarget(float y, bool animate) {
    ClampScroll();
    y = std::clamp(y, 0.0f, m_maxScrollY);
    if (!animate || !UIElement::AreAnimationsEnabled()) {
        m_scrollY = y;
        m_scrollAnimator.JumpTo(y);
        return;
    }
    const float from = m_scrollAnimator.IsActive() ? m_scrollAnimator.Target() : m_scrollY;
    m_scrollAnimator.ScrollBy(y - from, 0.0f, m_maxScrollY);
    RequestAnimationTicks();
}

void LogView::StopSmoothScroll() {
    m_scrollAnimator.JumpTo(m_scrollY);
}

bool LogView::AdvanceSmoothScroll() {
    if (m_dragScrollbar) {
        StopSmoothScroll();
        return false;
    }
    ClampScroll();
    if (!UIElement::AreAnimationsEnabled()) {
        if (m_scrollAnimator.IsActive()) {
            m_scrollY = std::clamp(m_scrollAnimator.Target(), 0.0f, m_maxScrollY);
            m_scrollAnimator.JumpTo(m_scrollY);
            DirtyBody();
            return true;
        }
        return false;
    }
    const float previous = m_scrollY;
    if (!m_scrollAnimator.Tick(static_cast<double>(UIElement::GetAnimationDeltaSeconds()), 0.0f, m_maxScrollY)) {
        return false;
    }
    m_scrollY = m_scrollAnimator.Current();
    if (std::abs(previous - m_scrollY) > 0.01f) {
        m_sbHide.NotifyActivity(this);
        DirtyBody();
    }
    return true;
}

void LogView::ClampScroll() {
    const int n = static_cast<int>(GetVisibleCount());
    const int view = ViewRowCount();
    m_maxScrollY = (std::max)(0.0f, static_cast<float>(n - view) * kRowH);
    m_scrollY = std::clamp(m_scrollY, 0.0f, m_maxScrollY);
    m_scrollAnimator.ClampTo(0.0f, m_maxScrollY);
}

Rect LogView::HeaderRect() const {
    return Rect(m_bounds.x, m_bounds.y, m_bounds.width, kHeaderH);
}

Rect LogView::ToolbarRect() const {
    if (ExpandProgress() < 0.01f) {
        return Rect();
    }
    return Rect(m_bounds.x, m_bounds.y + kHeaderH, m_bounds.width, kToolH);
}

Rect LogView::ChipGroupRect() const {
    const Rect tool = ToolbarRect();
    const float w = 4.0f + 6.0f * kChipW + 5.0f * kChipGap;
    return Rect(tool.x + 6.0f, tool.y + 6.0f, w, kChipH);
}

Rect LogView::ChipRect(int i) const {
    const Rect g = ChipGroupRect();
    return Rect(g.x + 2.0f + static_cast<float>(i) * (kChipW + kChipGap), g.y, kChipW, kChipH);
}

Rect LogView::SearchRect() const {
    const Rect tool = ToolbarRect();
    const Rect chips = ChipGroupRect();
    const float x = chips.x + chips.width + 8.0f;
    const float rightBtns = 8.0f + static_cast<float>(kIconCount) * kIconBtn
        + static_cast<float>(kIconCount - 1) * kIconGap;
    const float w = (std::max)(80.0f, tool.width - (x - tool.x) - rightBtns);
    return Rect(x, tool.y + 4.0f, w, 26.0f);
}

Rect LogView::ToolBtnRect(int slot) const {
    const Rect tool = ToolbarRect();
    const float stride = kIconBtn + kIconGap;
    const float x = tool.x + tool.width - 8.0f - static_cast<float>(kIconCount - slot) * stride + kIconGap;
    return Rect(x, tool.y + 4.0f, kIconBtn, kIconBtn);
}

Rect LogView::BodyRect() const {
    if (ExpandProgress() < 0.01f) {
        return Rect();
    }
    const float y = m_bounds.y + kHeaderH + kToolH;
    const float h = (std::max)(0.0f, m_bounds.y + m_bounds.height - y);
    return Rect(m_bounds.x, y, m_bounds.width, h);
}

Rect LogView::ScrollbarTrack() const {
    const Rect body = BodyRect();
    return Rect(body.x + body.width - kSbW - 2.0f, body.y + 2.0f, kSbW, (std::max)(0.0f, body.height - 4.0f));
}

Rect LogView::ScrollbarThumb() const {
    const Rect track = ScrollbarTrack();
    if (m_maxScrollY <= 0.0f || track.height <= 16.0f) {
        return Rect();
    }
    const float bodyH = static_cast<float>(ViewRowCount()) * kRowH;
    const float content = static_cast<float>(GetVisibleCount()) * kRowH;
    const float thumbH = std::clamp(track.height * (bodyH / (std::max)(content, 1.0f)), 16.0f, track.height);
    const float t = (m_maxScrollY > 0.0f) ? (m_scrollY / m_maxScrollY) : 0.0f;
    return Rect(track.x, track.y + t * (track.height - thumbH), track.width, thumbH);
}

int LogView::RowIndexFromY(float y) const {
    const Rect body = BodyRect();
    if (body.IsEmpty() || y < body.y || y > body.y + body.height) {
        return -1;
    }
    const int i = static_cast<int>((y - body.y + m_scrollY) / kRowH);
    if (i < 0 || i >= static_cast<int>(GetVisibleCount())) {
        return -1;
    }
    return i;
}

LogView::Part LogView::HitPart(Point pt, int* index) const {
    if (index) {
        *index = -1;
    }
    if (HeaderRect().Contains(pt.x, pt.y)) {
        return Part::Header;
    }
    if (!BodyInteractive()) {
        return Part::None;
    }
    for (int i = 0; i < 6; ++i) {
        if (ChipRect(i).Contains(pt.x, pt.y)) {
            if (index) {
                *index = i;
            }
            return Part::Chip;
        }
    }
    const Rect thumb = ScrollbarThumb();
    if ((!thumb.IsEmpty() && thumb.Contains(pt.x, pt.y))
        || (ScrollbarTrack().Contains(pt.x, pt.y) && m_maxScrollY > 0.0f)) {
        return Part::Scrollbar;
    }
    const int row = RowIndexFromY(pt.y);
    if (row >= 0) {
        if (index) {
            *index = row;
        }
        return Part::Row;
    }
    return Part::None;
}

void LogView::SetSelection(int a, int b) {
    const int n = static_cast<int>(GetVisibleCount());
    if (n <= 0) {
        m_selA = m_selB = -1;
        return;
    }
    a = std::clamp(a, 0, n - 1);
    b = std::clamp(b, 0, n - 1);
    if (a == m_selA && b == m_selB) {
        return;
    }
    m_selA = a;
    m_selB = b;
    DirtyBody();
}

void LogView::SelectAllVisible() {
    const int n = static_cast<int>(GetVisibleCount());
    if (n <= 0) {
        ClearSelection();
        return;
    }
    SetSelection(0, n - 1);
}

void LogView::ClearSelection() {
    if (m_selA < 0 && m_selB < 0) {
        return;
    }
    m_selA = m_selB = -1;
    DirtyBody();
}

std::string LogView::FormatRecord(const Record& rec) const {
    std::string line;
    line.reserve(32 + rec.category.size() + rec.message.size());
    line.append(rec.time);
    line.append("  ");
    line.append(LevelTag(rec.level));
    line.append("  ");
    if (!rec.category.empty()) {
        line.append(rec.category);
        if (rec.category.size() < 4) {
            line.append(4 - rec.category.size(), ' ');
        }
        line.append("  ");
    } else {
        line.append("      ");
    }
    line.append(rec.message);
    return line;
}

std::string LogView::CollectRows(bool selectedOnly) const {
    std::string out;
    const int n = static_cast<int>(GetVisibleCount());
    int lo = 0;
    int hi = n - 1;
    if (selectedOnly) {
        if (m_selA < 0 || m_selB < 0) {
            return out;
        }
        lo = (std::min)(m_selA, m_selB);
        hi = (std::max)(m_selA, m_selB);
    }
    for (int i = lo; i <= hi && i < n; ++i) {
        if (const Record* rec = VisibleRecord(static_cast<size_t>(i))) {
            if (!out.empty()) {
                out.push_back('\n');
            }
            out.append(FormatRecord(*rec));
        }
    }
    return out;
}

bool LogView::CopySelection() const {
    std::string text = CollectRows(true);
    if (text.empty()) {
        text = CollectRows(false);
    }
    return CopyUtf8(text);
}

bool LogView::CopyVisible() const {
    return CopyUtf8(CollectRows(false));
}

void LogView::DirtyHeader() {
    MarkRenderRectDirty(HeaderRect().Inflate(1.0f));
}

void LogView::DirtyBody() {
    if (m_expanded) {
        MarkRenderRectDirty(Rect(m_bounds.x, m_bounds.y + kHeaderH,
            m_bounds.width, (std::max)(0.0f, m_bounds.height - kHeaderH)));
    } else {
        DirtyHeader();
    }
}

D2D1_COLOR_F LogView::LevelColor(LogLevel level) const {
    auto& theme = ThemeManager::Instance();
    switch (level) {
    case LogLevel::Trace: return theme.GetColor(ThemeTokenId::TextMuted);
    case LogLevel::Debug: return theme.GetColor(ThemeTokenId::TextSecondary);
    case LogLevel::Info: return theme.GetColor(ThemeTokenId::AccentColor);
    case LogLevel::Warn: return D2D1::ColorF(0.90f, 0.68f, 0.12f, 1.0f);
    case LogLevel::Error: return theme.GetColor(ThemeTokenId::DangerColor);
    case LogLevel::Fatal: return D2D1::ColorF(0.78f, 0.16f, 0.22f, 1.0f);
    default: return theme.GetColor(ThemeTokenId::TextPrimary);
    }
}

const char* LogView::LevelTag(LogLevel level) const {
    const int i = static_cast<int>(level);
    return (i >= 0 && i < 6) ? kLevelNames[i] : "INF";
}

void LogView::PaintHeader(GraphicsContext& ctx) {
    const Rect hdr = HeaderRect();
    const auto border = ResolveThemeColor(GetBorderToken(), ThemeTokenId::CardBorder);
    const auto text = ResolveThemeColor(GetColorToken(), ThemeTokenId::TextPrimary);
    const auto muted = ThemeManager::Instance().GetColor(ThemeTokenId::TextSecondary);
    const auto accent = ThemeManager::Instance().GetColor(ThemeTokenId::AccentColor);
    const auto danger = ThemeManager::Instance().GetColor(ThemeTokenId::DangerColor);

    ctx.DrawChevron(
        Rect(hdr.x + 8.0f, hdr.y + 9.0f, 14.0f, 14.0f),
        muted,
        ExpandProgress() >= 0.5f
            ? GraphicsContext::ChevronDirection::Down
            : GraphicsContext::ChevronDirection::Right,
        1.6f);

    char counts[48];
    std::snprintf(counts, sizeof(counts), "日志  %u", m_size);
    ctx.DrawText(counts, Rect(hdr.x + 28.0f, hdr.y, 72.0f, hdr.height), text, "微软雅黑", 12.0f,
        DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER, DWRITE_FONT_WEIGHT_SEMI_BOLD);

    float badgeX = hdr.x + 96.0f;
    const uint32_t errN = m_levelCounts[static_cast<int>(LogLevel::Error)]
        + m_levelCounts[static_cast<int>(LogLevel::Fatal)];
    const uint32_t wrnN = m_levelCounts[static_cast<int>(LogLevel::Warn)];
    if (errN > 0) {
        char b[16];
        std::snprintf(b, sizeof(b), "E%u", errN);
        const Rect br(badgeX, hdr.y + 7.0f, 36.0f, 18.0f);
        ctx.FillRoundedRect(br, 3.0f, WithAlpha(danger, 0.18f));
        ctx.DrawText(b, br, danger, "微软雅黑", 10.0f, DWRITE_TEXT_ALIGNMENT_CENTER,
            DWRITE_PARAGRAPH_ALIGNMENT_CENTER, DWRITE_FONT_WEIGHT_BOLD);
        badgeX += 40.0f;
    }
    if (wrnN > 0) {
        char b[16];
        std::snprintf(b, sizeof(b), "W%u", wrnN);
        const Rect br(badgeX, hdr.y + 7.0f, 36.0f, 18.0f);
        const auto warn = LevelColor(LogLevel::Warn);
        ctx.FillRoundedRect(br, 3.0f, WithAlpha(warn, 0.18f));
        ctx.DrawText(b, br, warn, "微软雅黑", 10.0f, DWRITE_TEXT_ALIGNMENT_CENTER,
            DWRITE_PARAGRAPH_ALIGNMENT_CENTER, DWRITE_FONT_WEIGHT_BOLD);
        badgeX += 40.0f;
    }
    if (m_persistEnabled) {
        ctx.DrawText("●", Rect(hdr.x + hdr.width - 22.0f, hdr.y, 16.0f, hdr.height), accent, "微软雅黑", 9.0f,
            DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    }
    const float previewFade = 1.0f - ExpandProgress();
    if (previewFade > 0.02f && m_size > 0) {
        const Record& latest = AtLogical(m_size - 1);
        std::string preview = latest.time;
        preview.push_back(' ');
        preview.append(LevelTag(latest.level));
        preview.push_back(' ');
        preview.append(latest.message);
        const float x = badgeX + 8.0f;
        const float w = (std::max)(0.0f, hdr.x + hdr.width - x - 28.0f);
        ctx.DrawText(preview, Rect(x, hdr.y, w, hdr.height), WithAlpha(muted, muted.a * previewFade),
            "Consolas", 11.0f, DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER,
            DWRITE_FONT_WEIGHT_NORMAL, true);
    }
    ctx.FillRect(Rect(hdr.x, hdr.y + hdr.height - 1.0f, hdr.width, 1.0f), WithAlpha(border, 0.7f));
}

void LogView::PaintChips(GraphicsContext& ctx) {
    if (ExpandProgress() < 0.01f) {
        return;
    }
    const auto muted = ThemeManager::Instance().GetColor(ThemeTokenId::TextSecondary);
    const Rect group = ChipGroupRect();
    ctx.FillRoundedRect(group, 4.0f, WithAlpha(muted, 0.10f));
    for (int i = 0; i < 6; ++i) {
        const Rect chip = ChipRect(i);
        const float on = std::clamp(m_chipAnim[i].Current(), 0.0f, 1.0f);
        const bool hover = (m_hoverChip == i);
        const auto lc = LevelColor(static_cast<LogLevel>(i));
        if (hover) {
            ctx.FillRoundedRect(chip, 3.0f, WithAlpha(lc, 0.08f + 0.10f * on));
        }
        const auto off = WithAlpha(muted, 0.45f);
        const auto tc = BlendColor(off, lc, on);
        ctx.DrawText(kChipNames[i], chip, tc, "微软雅黑", 11.0f,
            DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER,
            on > 0.5f ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_SEMI_BOLD);
        if (on > 0.01f) {
            const float barW = (chip.width - 6.0f) * on;
            ctx.FillRoundedRect(
                Rect(chip.x + (chip.width - barW) * 0.5f, chip.y + chip.height - 3.0f, barW, 2.0f),
                1.0f, WithAlpha(lc, on));
        }
    }
    const Rect tool = ToolbarRect();
    const auto border = ResolveThemeColor(GetBorderToken(), ThemeTokenId::CardBorder);
    ctx.FillRect(Rect(tool.x, tool.y + tool.height - 1.0f, tool.width, 1.0f), WithAlpha(border, 0.55f));
}

void LogView::PaintRows(GraphicsContext& ctx) {
    const Rect body = BodyRect();
    if (body.IsEmpty()) {
        return;
    }
    ctx.PushClip(body);
    const size_t n = GetVisibleCount();
    const int first = (std::max)(0, static_cast<int>(m_scrollY / kRowH));
    const int last = (std::min)(static_cast<int>(n), first + static_cast<int>(body.height / kRowH) + 2);
    const int selLo = (std::min)(m_selA, m_selB);
    const int selHi = (std::max)(m_selA, m_selB);
    const auto text = ThemeManager::Instance().GetColor(ThemeTokenId::TextPrimary);
    const auto muted = ThemeManager::Instance().GetColor(ThemeTokenId::TextSecondary);
    const auto selBg = ThemeManager::Instance().GetColor(ThemeTokenId::SelectedBackground);
    const auto hoverBg = ThemeManager::Instance().GetColor(ThemeTokenId::HoverBackground);

    for (int i = first; i < last; ++i) {
        const Record* rec = VisibleRecord(static_cast<size_t>(i));
        if (!rec) {
            continue;
        }
        const float y = std::floor(body.y + static_cast<float>(i) * kRowH - m_scrollY + 0.5f);
        const Rect row(body.x, y, body.width - kSbW - 4.0f, kRowH);
        if (m_selA >= 0 && i >= selLo && i <= selHi) {
            ctx.FillRect(row, selBg);
        } else if (i == m_hoverRow) {
            ctx.FillRect(row, hoverBg);
        }
        const auto lc = LevelColor(rec->level);
        const float barX = row.x + 3.0f;
        ctx.FillRect(Rect(barX, row.y + 5.0f, 2.0f, row.height - 10.0f), lc);
        const float timeX = barX + 2.0f + 3.0f;
        const float levelX = timeX + kTimeCol + 6.0f;
        const float catX = levelX + kLevelCol + 8.0f;
        const float msgX = catX + kCatCol + 8.0f;
        ctx.DrawText(rec->time, Rect(timeX, row.y, kTimeCol, row.height), muted, "Consolas", 11.0f,
            DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        ctx.DrawText(LevelTag(rec->level), Rect(levelX, row.y, kLevelCol, row.height), lc, "Consolas", 11.0f,
            DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER, DWRITE_FONT_WEIGHT_BOLD);
        if (!rec->category.empty()) {
            ctx.DrawText(rec->category, Rect(catX, row.y, kCatCol, row.height), muted, "Consolas", 11.0f,
                DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER, DWRITE_FONT_WEIGHT_NORMAL, true);
        }
        ctx.DrawText(rec->message, Rect(msgX, row.y, (std::max)(0.0f, row.x + row.width - msgX - 4.0f), row.height),
            text, "Consolas", 11.0f, DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER,
            DWRITE_FONT_WEIGHT_NORMAL, true);
    }
    ctx.PopClip();
}

void LogView::PaintScrollbar(GraphicsContext& ctx) {
    if (!m_sbHide.IsDrawn() || m_maxScrollY <= 0.0f) {
        return;
    }
    const Rect thumb = ScrollbarThumb();
    if (thumb.IsEmpty()) {
        return;
    }
    auto c = ThemeManager::Instance().GetColor(ThemeTokenId::TextMuted);
    c.a *= m_sbHide.Opacity();
    ctx.FillRoundedRect(thumb, 3.0f, c);
}

void LogView::OnRender(GraphicsContext& ctx) {
    Control::OnRender(ctx);
    ClampScroll();
    PaintHeader(ctx);
    const float t = ExpandProgress();
    if (t > 0.01f) {
        const float fade = (std::min)(1.0f, t / 0.22f);
        const bool faded = fade < 0.999f;
        if (faded) {
            ctx.PushOpacity(fade);
        }
        PaintChips(ctx);
        PaintRows(ctx);
        PaintScrollbar(ctx);
        if (faded) {
            ctx.PopOpacity();
        }
    }
}

void LogView::OnMouseDown(Point pt) {
    if (!IsEnabled()) {
        return;
    }
    UIElement::OnMouseDown(pt);
    int idx = -1;
    const Part part = HitPart(pt, &idx);
    switch (part) {
    case Part::Header:
        SetExpanded(!m_expanded);
        break;
    case Part::Chip:
        if (idx >= 0 && idx < 6) {
            const auto lv = static_cast<LogLevel>(idx);
            SetLevelEnabled(lv, !GetLevelEnabled(lv));
        }
        break;
    case Part::Scrollbar: {
        const Rect thumb = ScrollbarThumb();
        if (thumb.Contains(pt.x, pt.y)) {
            m_dragScrollbar = true;
            m_dragStartY = pt.y;
            m_dragStartScroll = m_scrollY;
            StopSmoothScroll();
            m_sbHide.SetDragging(true, this);
        } else {
            const Rect track = ScrollbarTrack();
            const float t = (track.height > 0.0f) ? std::clamp((pt.y - track.y) / track.height, 0.0f, 1.0f) : 0.0f;
            SetScrollTarget(t * m_maxScrollY, false);
            m_follow = false;
            SyncActionButtons();
            DirtyBody();
        }
        break;
    }
    case Part::Row: {
        const bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
        if (shift && m_selA >= 0) {
            SetSelection(m_selA, idx);
        } else {
            SetSelection(idx, idx);
        }
        m_selecting = true;
        break;
    }
    default:
        break;
    }
}

void LogView::OnMouseMove(Point pt) {
    UIElement::OnMouseMove(pt);
    if (m_dragScrollbar) {
        const Rect track = ScrollbarTrack();
        const float thumbH = ScrollbarThumb().height;
        const float travel = (std::max)(1.0f, track.height - thumbH);
        m_scrollY = m_dragStartScroll + (pt.y - m_dragStartY) / travel * m_maxScrollY;
        ClampScroll();
        m_scrollAnimator.JumpTo(m_scrollY);
        const bool follow = (m_scrollY >= m_maxScrollY - 0.5f);
        if (follow != m_follow) {
            m_follow = follow;
            SyncActionButtons();
        }
        DirtyBody();
        return;
    }
    if (m_selecting) {
        const int row = RowIndexFromY(pt.y);
        if (row >= 0) {
            SetSelection(m_selA, row);
        }
        return;
    }
    int idx = -1;
    const Part part = HitPart(pt, &idx);
    const int row = (part == Part::Row) ? idx : -1;
    const int chip = (part == Part::Chip) ? idx : -1;
    if (part != m_hoverPart || row != m_hoverRow || chip != m_hoverChip) {
        m_hoverPart = part;
        m_hoverRow = row;
        m_hoverChip = chip;
        if (m_expanded) {
            DirtyBody();
        } else {
            DirtyHeader();
        }
    }
    m_sbHide.SetPointerOver(ScrollbarTrack().Contains(pt.x, pt.y), this);
}

void LogView::OnMouseUp(Point pt) {
    UIElement::OnMouseUp(pt);
    m_dragScrollbar = false;
    m_selecting = false;
    m_sbHide.SetDragging(false, this);
}

void LogView::OnMouseLeave() {
    UIElement::OnMouseLeave();
    m_sbHide.SetPointerOver(false, this);
    if (m_hoverPart != Part::None || m_hoverRow >= 0) {
        m_hoverPart = Part::None;
        m_hoverRow = -1;
        m_hoverChip = -1;
        if (m_expanded) {
            DirtyBody();
        }
    }
}

void LogView::OnMouseDblClick(Point pt) {
    int idx = -1;
    if (HitPart(pt, &idx) == Part::Row) {
        SetSelection(idx, idx);
        CopySelection();
    }
}

void LogView::OnMouseWheel(float delta) {
    if (!m_expanded) {
        UIElement::OnMouseWheel(delta);
        return;
    }
    ClampScroll();
    if (m_maxScrollY <= 0.0f) {
        UIElement::OnMouseWheel(delta);
        return;
    }
    const float step = GetChromiumWheelStep(BodyRect().height);
    const float prevTarget = m_scrollAnimator.IsActive() ? m_scrollAnimator.Target() : m_scrollY;
    if (!UIElement::AreAnimationsEnabled()) {
        m_scrollY = std::clamp(m_scrollY - delta * step, 0.0f, m_maxScrollY);
        m_scrollAnimator.JumpTo(m_scrollY);
        DirtyBody();
    } else {
        m_scrollAnimator.ScrollBy(-delta * step, 0.0f, m_maxScrollY);
        RequestAnimationTicks();
    }
    const float newTarget = m_scrollAnimator.IsActive() ? m_scrollAnimator.Target() : m_scrollY;
    if (std::abs(newTarget - prevTarget) < 0.001f && !m_scrollAnimator.IsActive()) {
        UIElement::OnMouseWheel(delta);
        return;
    }
    const bool follow = (newTarget >= m_maxScrollY - 0.5f);
    if (follow != m_follow) {
        m_follow = follow;
        SyncActionButtons();
    }
    m_sbHide.NotifyActivity(this);
    DirtyBody();
}

bool LogView::OnKeyDown(int vkCode) {
    const bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
    if (ctrl && vkCode == 'F') {
        if (!m_expanded) {
            SetExpanded(true);
        }
        return true;
    }
    if (ctrl && vkCode == 'C') {
        CopySelection();
        return true;
    }
    if (ctrl && vkCode == 'A') {
        SelectAllVisible();
        return true;
    }
    if (vkCode == VK_ESCAPE && m_expanded) {
        SetExpanded(false);
        return true;
    }
    if (vkCode == VK_SPACE && !m_expanded) {
        SetExpanded(true);
        return true;
    }
    if (!m_expanded) {
        return false;
    }
    const int n = static_cast<int>(GetVisibleCount());
    if (vkCode == VK_UP || vkCode == VK_DOWN) {
        int cur = (m_selB >= 0) ? m_selB : 0;
        cur += (vkCode == VK_DOWN) ? 1 : -1;
        cur = std::clamp(cur, 0, (std::max)(0, n - 1));
        const bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
        SetSelection(shift && m_selA >= 0 ? m_selA : cur, cur);
        const float y = static_cast<float>(cur) * kRowH;
        const Rect body = BodyRect();
        if (y < m_scrollY) {
            SetScrollTarget(y, true);
        } else if (y + kRowH > m_scrollY + body.height) {
            SetScrollTarget(y + kRowH - body.height, true);
        }
        m_follow = false;
        SyncActionButtons();
        DirtyBody();
        return true;
    }
    if (vkCode == VK_HOME) {
        SetScrollTarget(0.0f, true);
        m_follow = false;
        SyncActionButtons();
        DirtyBody();
        return true;
    }
    if (vkCode == VK_END) {
        ClampScroll();
        SetScrollTarget(m_maxScrollY, true);
        m_follow = true;
        SyncActionButtons();
        DirtyBody();
        return true;
    }
    if (vkCode == VK_PRIOR) {
        SetScrollTarget(m_scrollY - BodyRect().height, true);
        m_follow = false;
        SyncActionButtons();
        DirtyBody();
        return true;
    }
    if (vkCode == VK_NEXT) {
        ClampScroll();
        SetScrollTarget(m_scrollY + BodyRect().height, true);
        m_follow = (m_scrollAnimator.Target() >= m_maxScrollY - 0.5f);
        SyncActionButtons();
        DirtyBody();
        return true;
    }
    return false;
}

bool LogView::OnAnimationTick() {
    const bool base = UIElement::OnAnimationTick();
    const float dt = UIElement::GetAnimationDeltaSeconds();
    bool any = base;

    if (!UIElement::AreAnimationsEnabled()) {
        m_expandAnim.Reset(m_expanded ? 1.0f : 0.0f);
        SyncChipAnimTargets(true);
    } else {
        m_expandAnim.SetTarget(m_expanded ? 1.0f : 0.0f);
        const float before = m_expandAnim.Current();
        const bool expanding = m_expandAnim.Tick(dt, AnimationSpec{ 0.22f, 0.01f, 0.28f });
        if (std::abs(m_expandAnim.Current() - before) > 0.0005f) {
            ApplyExpandLayout();
            MarkRenderRectDirty(m_bounds.Inflate(4.0f));
        }
        any = any || expanding;

        bool chips = false;
        for (int i = 0; i < 6; ++i) {
            chips = m_chipAnim[i].Tick(dt, AnimationSpec{ 0.16f, 0.01f, 0.16f }) || chips;
        }
        if (chips) {
            MarkRenderRectDirty(ToolbarRect().Inflate(2.0f));
        }
        any = any || chips;
    }

    const bool scrolling = AdvanceSmoothScroll();
    any = any || scrolling;

    const float prev = m_sbHide.Opacity();
    const bool hide = m_sbHide.Tick(dt);
    if (std::abs(prev - m_sbHide.Opacity()) > 0.001f) {
        MarkRenderRectDirty(ScrollbarTrack().Inflate(2.0f));
    }
    if (any || hide || m_scrollAnimator.IsActive()) {
        RequestAnimationTicks();
    }
    return any || hide || m_scrollAnimator.IsActive();
}

bool LogView::HasSelfAnimation() const {
    if (m_sbHide.NeedsTicks() || m_expandAnim.IsAnimating(0.01f) || m_scrollAnimator.IsActive()) {
        return true;
    }
    for (int i = 0; i < 6; ++i) {
        if (m_chipAnim[i].IsAnimating(0.01f)) {
            return true;
        }
    }
    return false;
}

void LogView::OnThemeChanged() {
    if (m_btnCopy) {
        StyleIconButton(*m_btnCopy, kSvgCopy, "复制", ThemeTokenId::AccentColor);
    }
    if (m_btnClear) {
        StyleIconButton(*m_btnClear, kSvgClear, "清空", ThemeTokenId::AccentColor);
    }
    if (m_btnFollow) {
        StyleIconButton(*m_btnFollow, kSvgFollow, "跟随", ThemeTokenId::TextPrimary);
    }
    SyncActionButtons();
    UIElement::OnThemeChanged();
    MarkRenderRectDirty(m_bounds);
}

} // namespace CUI
