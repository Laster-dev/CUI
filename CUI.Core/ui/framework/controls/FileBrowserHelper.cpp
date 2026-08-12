#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "FileBrowserHelper.h"
#include "../core/Value.h"
#include "../style/ThemeManager.h"
#include "../animation/FrameScheduler.h"
#include "BreadcrumbBar.h"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <windows.h>

namespace CUI {

namespace fs = std::filesystem;

namespace {

fs::path ToPath(const std::string& utf8) {
    return fs::path(Utf8ToUtf16(utf8));
}

std::string FromPath(const fs::path& path) {
    return Utf16ToUtf8(path.wstring());
}

std::string ToLowerAscii(std::string s) {
    for (char& c : s) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return s;
}

float GetChromiumWheelStep(float viewportHeight) {
    UINT lines = 3;
    SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &lines, 0);
    if (lines == WHEEL_PAGESCROLL) {
        return (std::max)(40.0f, viewportHeight);
    }
    return (std::max)(1u, lines) * 40.0f;
}

} // namespace

void FileBrowserScrollController::Reset() {
    m_viewport = Rect();
    m_contentHeight = 0.0f;
    m_offsetY = 0.0f;
    m_animator.Reset(0.0f);
    m_draggingThumb = false;
    m_scrollbarHovered = false;
    m_lastAnimQpc = 0;
}

void FileBrowserScrollController::SetViewport(const Rect& viewport) {
    m_viewport = viewport;
    ClampOffset();
}

void FileBrowserScrollController::SetContentHeight(float height) {
    m_contentHeight = (std::max)(0.0f, height);
    ClampOffset();
}

float FileBrowserScrollController::GetMaxScroll() const {
    if (m_viewport.height <= 0.0f) {
        return 0.0f;
    }
    return (std::max)(0.0f, m_contentHeight - m_viewport.height);
}

void FileBrowserScrollController::ClampOffset() {
    m_offsetY = std::clamp(m_offsetY, 0.0f, GetMaxScroll());
    m_animator.ClampTo(0.0f, GetMaxScroll());
}

void FileBrowserScrollController::JumpTo(float offset) {
    m_offsetY = std::clamp(offset, 0.0f, GetMaxScroll());
    m_animator.JumpTo(m_offsetY);
}

void FileBrowserScrollController::EnsureVisible(float itemTop, float itemBottom, UIElement* owner) {
    const float maxScroll = GetMaxScroll();
    if (maxScroll <= 0.001f) {
        JumpTo(0.0f);
        return;
    }
    const float current = m_animator.Current();
    float target = current;
    if (itemTop < current) {
        target = itemTop;
    } else if (itemBottom > current + m_viewport.height) {
        target = itemBottom - m_viewport.height;
    }
    target = std::clamp(target, 0.0f, maxScroll);
    if (std::abs(target - current) < 0.01f) {
        return;
    }
    if (UIElement::AreAnimationsEnabled()) {
        m_animator.ScrollBy(target - current, 0.0f, maxScroll);
        if (m_lastAnimQpc == 0) {
            if (m_qpcFreq.QuadPart <= 0) {
                QueryPerformanceFrequency(&m_qpcFreq);
            }
            LARGE_INTEGER now{};
            QueryPerformanceCounter(&now);
            m_lastAnimQpc = now.QuadPart;
        }
        if (owner) {
            owner->RequestAnimationTicks();
        }
    } else {
        JumpTo(target);
    }
}

void FileBrowserScrollController::ScrollWheel(float delta, UIElement* owner) {
    const float maxScroll = GetMaxScroll();
    if (maxScroll <= 0.001f || m_viewport.height <= 0.0f) {
        return;
    }

    m_autoHide.NotifyActivity(owner);
    if (owner) {
        owner->RequestAnimationTicks();
    }
    if (!UIElement::AreAnimationsEnabled()) {
        JumpTo(std::clamp(m_offsetY - delta * GetChromiumWheelStep(m_viewport.height), 0.0f, maxScroll));
        if (FrameScheduler* sched = FrameScheduler::Current()) {
            sched->ScheduleFrame();
        }
        return;
    }

    m_animator.ScrollBy(-delta * GetChromiumWheelStep(m_viewport.height), 0.0f, maxScroll);
    if (m_lastAnimQpc == 0) {
        if (m_qpcFreq.QuadPart <= 0) {
            QueryPerformanceFrequency(&m_qpcFreq);
        }
        LARGE_INTEGER now{};
        QueryPerformanceCounter(&now);
        m_lastAnimQpc = now.QuadPart;
    }
}

double FileBrowserScrollController::SecondsSinceLastTick() {
    if (m_qpcFreq.QuadPart <= 0) {
        QueryPerformanceFrequency(&m_qpcFreq);
    }
    LARGE_INTEGER now{};
    QueryPerformanceCounter(&now);
    if (m_lastAnimQpc == 0 || m_qpcFreq.QuadPart <= 0) {
        m_lastAnimQpc = now.QuadPart;
        return 1.0 / 120.0;
    }
    double dt = static_cast<double>(now.QuadPart - m_lastAnimQpc) / static_cast<double>(m_qpcFreq.QuadPart);
    m_lastAnimQpc = now.QuadPart;
    dt = std::clamp(dt, 0.0005, 0.050);
    return dt;
}

bool FileBrowserScrollController::AdvanceSmoothScroll(UIElement* owner) {
    if (m_draggingThumb) {
        m_animator.JumpTo(m_offsetY);
        return false;
    }
    const float previous = m_offsetY;
    const bool advanced = m_animator.Tick(SecondsSinceLastTick(), 0.0f, GetMaxScroll());
    if (!advanced) {
        m_lastAnimQpc = 0;
        return false;
    }
    m_offsetY = m_animator.Current();
    ClampOffset();
    if (std::abs(previous - m_offsetY) > 0.01f) {
        MarkScrollDirty(owner);
        m_autoHide.NotifyActivity(owner);
    }
    return m_animator.IsActive();
}

bool FileBrowserScrollController::Tick(UIElement* owner, float dtSeconds) {
    if (!UIElement::AreAnimationsEnabled()) {
        if (m_animator.IsActive()) {
            const float previous = m_offsetY;
            JumpTo(m_animator.Target());
            if (std::abs(previous - m_offsetY) > 0.01f) {
                MarkScrollDirty(owner);
            }
        }
        const float prevOpacity = m_autoHide.Opacity();
        const bool hideAnimating = m_autoHide.Tick(dtSeconds);
        if (std::abs(prevOpacity - m_autoHide.Opacity()) > 0.001f) {
            MarkScrollDirty(owner);
        }
        return hideAnimating;
    }

    const bool selfAnimating = AdvanceSmoothScroll(owner);
    const float prevOpacity = m_autoHide.Opacity();
    const bool hideAnimating = m_autoHide.Tick(dtSeconds);
    if (std::abs(prevOpacity - m_autoHide.Opacity()) > 0.001f) {
        MarkScrollDirty(owner);
    }
    return selfAnimating || hideAnimating;
}

bool FileBrowserScrollController::NeedsAnimationTicks() const {
    return (UIElement::AreAnimationsEnabled() && m_animator.IsActive())
        || m_autoHide.NeedsTicks();
}

void FileBrowserScrollController::MarkScrollDirty(UIElement* owner) const {
    if (!owner) {
        return;
    }
    owner->MarkRenderRectDirty(m_viewport.Union(GetTrackRect()).Inflate(2.0f));
}

Rect FileBrowserScrollController::GetTrackRect() const {
    const float trackX = m_viewport.x + m_viewport.width - kScrollbarInset - kScrollbarWidth;
    return Rect(trackX, m_viewport.y, kScrollbarWidth, m_viewport.height);
}

Rect FileBrowserScrollController::GetThumbRect() const {
    const Rect track = GetTrackRect();
    const float maxScroll = GetMaxScroll();
    if (maxScroll <= 0.001f || m_viewport.height <= 0.0f || m_contentHeight <= 0.001f) {
        return Rect(track.x, track.y, track.width, 0.0f);
    }
    float thumbHeight = (m_viewport.height / m_contentHeight) * track.height;
    thumbHeight = (std::max)(24.0f, (std::min)(thumbHeight, track.height));
    const float scrollRatio = m_animator.Current() / maxScroll;
    const float thumbY = track.y + scrollRatio * (track.height - thumbHeight);
    return Rect(track.x, thumbY, track.width, thumbHeight);
}

bool FileBrowserScrollController::IsPointerOverScrollbar(Point pt) const {
    return GetMaxScroll() > 0.001f && GetTrackRect().Contains(pt.x, pt.y);
}

bool FileBrowserScrollController::HandleMouseDown(Point pt, UIElement* owner) {
    if (GetMaxScroll() <= 0.001f) {
        return false;
    }

    const Rect track = GetTrackRect();
    const Rect thumb = GetThumbRect();
    if (!track.Contains(pt.x, pt.y)) {
        return false;
    }

    m_autoHide.NotifyActivity(owner);
    if (owner) {
        owner->RequestAnimationTicks();
    }

    if (thumb.Contains(pt.x, pt.y)) {
        m_draggingThumb = true;
        m_autoHide.SetDragging(true, owner);
        m_dragStartY = pt.y;
        m_dragStartOffset = m_offsetY;
        m_animator.JumpTo(m_offsetY);
        return true;
    }

    const float maxScroll = GetMaxScroll();
    const float trackH = track.height;
    const float thumbH = thumb.height;
    float clickRatio = (pt.y - track.y - thumbH * 0.5f) / (std::max)(1.0f, trackH - thumbH);
    clickRatio = std::clamp(clickRatio, 0.0f, 1.0f);
    JumpTo(clickRatio * maxScroll);
    MarkScrollDirty(owner);
    return true;
}

void FileBrowserScrollController::HandleMouseMove(Point pt, UIElement* owner) {
    const bool wasHovered = m_scrollbarHovered;
    m_scrollbarHovered = IsPointerOverScrollbar(pt);
    m_autoHide.SetPointerOver(m_scrollbarHovered, owner);
    if (wasHovered != m_scrollbarHovered) {
        MarkScrollDirty(owner);
        if (owner) {
            owner->RequestAnimationTicks();
        }
    }

    if (!m_draggingThumb) {
        return;
    }

    const float maxScroll = GetMaxScroll();
    const Rect track = GetTrackRect();
    const Rect thumb = GetThumbRect();
    const float scrollableTrack = (std::max)(1.0f, track.height - thumb.height);
    const float deltaY = pt.y - m_dragStartY;
    const float previous = m_offsetY;
    m_offsetY = std::clamp(m_dragStartOffset + (deltaY / scrollableTrack) * maxScroll, 0.0f, maxScroll);
    m_animator.JumpTo(m_offsetY);
    if (std::abs(previous - m_offsetY) > 0.01f) {
        MarkScrollDirty(owner);
    }
}

void FileBrowserScrollController::HandleMouseUp(UIElement* owner) {
    if (m_draggingThumb) {
        MarkScrollDirty(owner);
        if (owner) {
            owner->RequestAnimationTicks();
        }
    }
    m_draggingThumb = false;
    m_autoHide.SetDragging(false, owner);
}

void FileBrowserScrollController::HandleMouseLeave(UIElement* owner) {
    m_scrollbarHovered = false;
    m_autoHide.SetPointerOver(false, owner);
}

void FileBrowserScrollController::RenderChrome(GraphicsContext& ctx) const {
    if (GetMaxScroll() <= 0.001f || m_viewport.height <= 0.0f) {
        return;
    }
    const float visibility = m_autoHide.Opacity();
    if (visibility <= 0.01f) {
        return;
    }

    const auto& tokens = ThemeManager::Instance().GetTokens();
    const Rect track = GetTrackRect();
    const Rect thumb = GetThumbRect();
    const float trackAlpha = (m_scrollbarHovered || m_draggingThumb ? 0.18f : 0.08f) * visibility;
    ctx.FillRoundedRect(track, 4.0f, D2D1::ColorF(tokens.cardBorder.r, tokens.cardBorder.g, tokens.cardBorder.b, trackAlpha));

    const float thumbAlpha = (m_draggingThumb ? 0.75f : (m_scrollbarHovered ? 0.55f : 0.40f)) * visibility;
    ctx.FillRoundedRect(thumb, 4.0f, D2D1::ColorF(tokens.textMuted.r, tokens.textMuted.g, tokens.textMuted.b, thumbAlpha));
}

namespace {

bool WildcardMatchInsensitive(const std::string& text, const std::string& pattern) {
    const std::string t = ToLowerAscii(text);
    const std::string p = ToLowerAscii(pattern);
    size_t ti = 0;
    size_t pi = 0;
    size_t starT = std::string::npos;
    size_t starP = std::string::npos;

    while (ti < t.size()) {
        if (pi < p.size() && (p[pi] == '?' || p[pi] == t[ti])) {
            ++ti;
            ++pi;
        } else if (pi < p.size() && p[pi] == '*') {
            starP = pi++;
            starT = ti;
        } else if (starP != std::string::npos) {
            pi = starP + 1;
            ti = ++starT;
        } else {
            return false;
        }
    }
    while (pi < p.size() && p[pi] == '*') {
        ++pi;
    }
    return pi == p.size();
}

bool MatchFilterSpec(const std::string& fileName, const std::string& spec) {
    if (spec.empty() || spec == "*.*") {
        return true;
    }
    size_t start = 0;
    while (start <= spec.size()) {
        size_t end = spec.find(';', start);
        if (end == std::string::npos) {
            end = spec.size();
        }
        std::string one = spec.substr(start, end - start);
        while (!one.empty() && one.front() == ' ') {
            one.erase(one.begin());
        }
        while (!one.empty() && one.back() == ' ') {
            one.pop_back();
        }
        if (!one.empty() && WildcardMatchInsensitive(fileName, one)) {
            return true;
        }
        if (end == spec.size()) {
            break;
        }
        start = end + 1;
    }
    return false;
}

void EnumerateDrives(std::vector<FileBrowserEntry>& out) {
    const DWORD mask = GetLogicalDrives();
    for (int i = 0; i < 26; ++i) {
        if ((mask & (1u << i)) == 0) {
            continue;
        }
        char letter = static_cast<char>('A' + i);
        std::string root = std::string(1, letter) + ":\\";
        FileBrowserEntry entry;
        entry.name = std::string(1, letter) + ":";
        entry.fullPath = root;
        entry.isDirectory = true;
        out.push_back(std::move(entry));
    }
}

} // namespace

std::string GetDefaultUserDirectory() {
    wchar_t buf[MAX_PATH] = {};
    DWORD n = GetEnvironmentVariableW(L"USERPROFILE", buf, MAX_PATH);
    if (n > 0 && n < MAX_PATH) {
        return Utf16ToUtf8(buf);
    }
    return "C:\\";
}

bool PathExistsUtf8(const std::string& pathUtf8) {
    std::error_code ec;
    return !pathUtf8.empty() && fs::exists(ToPath(pathUtf8), ec);
}

bool IsDirectoryUtf8(const std::string& pathUtf8) {
    std::error_code ec;
    return !pathUtf8.empty() && fs::is_directory(ToPath(pathUtf8), ec);
}

std::string ParentPathUtf8(const std::string& pathUtf8) {
    if (pathUtf8.empty()) {
        return {};
    }
    fs::path p = ToPath(pathUtf8);
    if (p.has_parent_path() && p.parent_path() != p) {
        fs::path parent = p.parent_path();
        if (parent.wstring().size() == 2 && parent.wstring()[1] == L':') {
            return FromPath(parent) + "\\";
        }
        return FromPath(parent);
    }
    // Drive root → empty (drive list)
    if (p.wstring().size() >= 2 && p.wstring()[1] == L':') {
        return {};
    }
    return {};
}

std::string JoinPathUtf8(const std::string& dir, const std::string& name) {
    if (dir.empty()) {
        return name;
    }
    return FromPath(ToPath(dir) / Utf8ToUtf16(name));
}

std::vector<std::string> BuildFileBrowserBreadcrumb(const std::string& currentPath) {
    std::vector<std::string> nodes = { "此电脑" };
    if (currentPath.empty()) {
        return nodes;
    }

    const fs::path p = ToPath(currentPath);
    if (p.has_root_name()) {
        nodes.push_back(FromPath(p.root_name()));
    }
    if (p.has_relative_path()) {
        for (const auto& part : p.relative_path()) {
            if (!part.empty()) {
                nodes.push_back(FromPath(part));
            }
        }
    }
    return nodes;
}

std::string ResolveFileBrowserBreadcrumbPath(int index, const std::vector<std::string>& nodes) {
    if (index <= 0 || nodes.size() <= 1) {
        return {};
    }
    if (index == 1) {
        return nodes[1] + "\\";
    }

    std::string path = nodes[1] + "\\";
    for (int i = 2; i <= index && i < static_cast<int>(nodes.size()); ++i) {
        path = JoinPathUtf8(path, nodes[i]);
    }
    return path;
}

FileBrowserBreadcrumbHost::FileBrowserBreadcrumbHost() {
    m_bar = std::make_shared<BreadcrumbBar>();
    m_bar->SetFontFamily("Segoe UI");
    m_bar->SetFontSize(12.0f);
    m_bar->SetHeight(FileBrowserSession::kHeaderH - 1.0f);
    m_bar->SetBackground(D2D1::ColorF(0, 0, 0, 0));
    m_bar->SetBorderThickness(0.0f);
    m_bar->SetBackgroundToken(ThemeTokenId::Unset);
    m_bar->SetBorderToken(ThemeTokenId::Unset);
    m_bar->SetColorToken(ThemeTokenId::TextSecondary);
    m_bar->SetActiveColorToken(ThemeTokenId::TextPrimary);
    m_bar->OnItemClicked().Connect([this](BreadcrumbBar*, int index, const std::string&) {
        if (!m_onNavigate) {
            return;
        }
        const std::string path = ResolveFileBrowserBreadcrumbPath(index, m_bar->GetPath());
        m_onNavigate(path);
    });
}

void FileBrowserBreadcrumbHost::SetNavigateHandler(NavigateCallback handler) {
    m_onNavigate = std::move(handler);
}

void FileBrowserBreadcrumbHost::Sync(const FileBrowserSession& session) {
    m_bar->SetPath(BuildFileBrowserBreadcrumb(session.GetCurrentPath()));
}

void FileBrowserBreadcrumbHost::Layout(const FileBrowserSession& session, const Rect& pop) {
    m_bounds = session.BreadcrumbRect(pop);
    m_bar->SetBounds(m_bounds);
}

void FileBrowserBreadcrumbHost::Render(GraphicsContext& ctx) {
    if (m_bounds.IsEmpty()) {
        return;
    }
    m_bar->OnRender(ctx);
}

bool FileBrowserBreadcrumbHost::HandleMouseDown(Point pt) {
    if (!m_bounds.Contains(pt.x, pt.y)) {
        return false;
    }
    m_bar->SetBounds(m_bounds);
    m_bar->OnMouseDown(pt);
    return true;
}

void FileBrowserSession::Configure(
    FileBrowserMode mode,
    const std::string& startPath,
    const std::vector<std::pair<std::string, std::string>>& filters,
    int filterIndex) {
    m_mode = mode;
    m_title = (mode == FileBrowserMode::OpenFolder) ? "选择文件夹" : "选择文件";
    m_filters = filters;
    if (m_filters.empty()) {
        m_filters.emplace_back("所有文件", "*.*");
    }
    m_filterIndex = std::clamp(filterIndex, 0, static_cast<int>(m_filters.size()) - 1);
    m_currentPath = ResolveStartDirectory(startPath);
    m_selectedIndex = -1;
    m_scroll.Reset();
    Refresh();
}

std::string FileBrowserSession::ResolveStartDirectory(const std::string& startPath) const {
    if (!startPath.empty()) {
        if (IsDirectoryUtf8(startPath)) {
            return startPath;
        }
        std::string parent = ParentPathUtf8(startPath);
        if (!parent.empty() && IsDirectoryUtf8(parent)) {
            return parent;
        }
        if (PathExistsUtf8(startPath)) {
            return ParentPathUtf8(startPath);
        }
    }
    return GetDefaultUserDirectory();
}

bool FileBrowserSession::MatchesFilter(const std::string& fileName) const {
    if (m_filters.empty()) {
        return true;
    }
    const int idx = std::clamp(m_filterIndex, 0, static_cast<int>(m_filters.size()) - 1);
    return MatchFilterSpec(fileName, m_filters[static_cast<size_t>(idx)].second);
}

void FileBrowserSession::Refresh() {
    m_entries.clear();
    m_selectedIndex = -1;

    if (m_currentPath.empty()) {
        EnumerateDrives(m_entries);
        m_scroll.JumpTo(0.0f);
        m_scroll.SetContentHeight(ContentHeight());
        return;
    }

    std::error_code ec;
    fs::directory_iterator it(ToPath(m_currentPath), ec);
    if (ec) {
        m_scroll.JumpTo(0.0f);
        m_scroll.SetContentHeight(0.0f);
        return;
    }

    std::vector<FileBrowserEntry> dirs;
    std::vector<FileBrowserEntry> files;
    for (const auto& entry : it) {
        std::error_code entryEc;
        const bool isDir = entry.is_directory(entryEc);
        if (entryEc) {
            continue;
        }
        FileBrowserEntry item;
        item.name = Utf16ToUtf8(entry.path().filename().wstring());
        item.fullPath = FromPath(entry.path());
        item.isDirectory = isDir;
        if (isDir) {
            dirs.push_back(std::move(item));
        } else if (m_mode == FileBrowserMode::OpenFile && MatchesFilter(item.name)) {
            files.push_back(std::move(item));
        }
    }

    auto byName = [](const FileBrowserEntry& a, const FileBrowserEntry& b) {
        return ToLowerAscii(a.name) < ToLowerAscii(b.name);
    };
    std::sort(dirs.begin(), dirs.end(), byName);
    std::sort(files.begin(), files.end(), byName);
    m_entries.reserve(dirs.size() + files.size());
    m_entries.insert(m_entries.end(), dirs.begin(), dirs.end());
    m_entries.insert(m_entries.end(), files.begin(), files.end());
    m_scroll.JumpTo(0.0f);
    m_scroll.SetContentHeight(ContentHeight());
}

void FileBrowserSession::UpdateScrollMetrics(const Rect& pop) {
    m_scroll.SetViewport(ListRect(pop));
    m_scroll.SetContentHeight(ContentHeight());
}

void FileBrowserSession::ResetScroll() {
    m_scroll.JumpTo(0.0f);
}

void FileBrowserSession::EnsureRowVisible(int index, UIElement* owner) {
    if (index < 0) {
        return;
    }
    const float top = static_cast<float>(index) * kRowH;
    m_scroll.EnsureVisible(top, top + kRowH, owner);
}

void FileBrowserSession::GoUp() {
    if (m_currentPath.empty()) {
        return;
    }
    m_currentPath = ParentPathUtf8(m_currentPath);
    Refresh();
}

void FileBrowserSession::NavigateTo(const std::string& path) {
    if (path.empty()) {
        m_currentPath.clear();
        Refresh();
        return;
    }
    if (!IsDirectoryUtf8(path)) {
        return;
    }
    m_currentPath = path;
    Refresh();
}

void FileBrowserSession::SetFilterIndex(int index) {
    if (m_mode != FileBrowserMode::OpenFile || m_filters.empty()) {
        return;
    }
    const int next = std::clamp(index, 0, static_cast<int>(m_filters.size()) - 1);
    if (next == m_filterIndex) {
        return;
    }
    m_filterIndex = next;
    Refresh();
}

const std::string& FileBrowserSession::GetActiveFilterLabel() const {
    static const std::string kEmpty;
    if (m_filters.empty()) {
        return kEmpty;
    }
    const int idx = std::clamp(m_filterIndex, 0, static_cast<int>(m_filters.size()) - 1);
    return m_filters[static_cast<size_t>(idx)].first;
}

void FileBrowserSession::SetSelectedIndex(int index) {
    if (index < -1 || index >= static_cast<int>(m_entries.size())) {
        m_selectedIndex = -1;
        return;
    }
    m_selectedIndex = index;
}

bool FileBrowserSession::TryConfirm(std::string& outPath) const {
    outPath.clear();
    if (m_mode == FileBrowserMode::OpenFolder) {
        if (m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>(m_entries.size())) {
            const auto& e = m_entries[static_cast<size_t>(m_selectedIndex)];
            if (e.isDirectory) {
                outPath = e.fullPath;
                return true;
            }
        }
        if (!m_currentPath.empty() && IsDirectoryUtf8(m_currentPath)) {
            outPath = m_currentPath;
            return true;
        }
        return false;
    }

    if (m_selectedIndex < 0 || m_selectedIndex >= static_cast<int>(m_entries.size())) {
        return false;
    }
    const auto& e = m_entries[static_cast<size_t>(m_selectedIndex)];
    if (e.isDirectory) {
        return false;
    }
    outPath = e.fullPath;
    return true;
}

bool FileBrowserSession::ActivateSelected(std::string& outPath) {
    outPath.clear();
    if (m_selectedIndex < 0 || m_selectedIndex >= static_cast<int>(m_entries.size())) {
        return false;
    }
    const auto& e = m_entries[static_cast<size_t>(m_selectedIndex)];
    if (e.isDirectory) {
        NavigateTo(e.fullPath);
        return false;
    }
    if (m_mode == FileBrowserMode::OpenFile) {
        outPath = e.fullPath;
        return true;
    }
    return false;
}

Rect FileBrowserSession::HeaderRect(const Rect& pop) const {
    return Rect(pop.x, pop.y, pop.width, kHeaderH);
}

Rect FileBrowserSession::UpButtonRect(const Rect& pop) const {
    return Rect(pop.x + 8.0f, pop.y + 6.0f, 28.0f, 24.0f);
}

Rect FileBrowserSession::BreadcrumbRect(const Rect& pop) const {
    const Rect up = UpButtonRect(pop);
    const float rightEdge = (m_mode == FileBrowserMode::OpenFile)
        ? FilterButtonRect(pop).x - 6.0f
        : pop.x + pop.width - 8.0f;
    return Rect(
        up.x + up.width + 4.0f,
        pop.y,
        (std::max)(24.0f, rightEdge - (up.x + up.width + 4.0f)),
        kHeaderH - 1.0f);
}

Rect FileBrowserSession::FilterButtonRect(const Rect& pop) const {
    return Rect(pop.x + pop.width - 140.0f, pop.y + 6.0f, 128.0f, 24.0f);
}

float FileBrowserSession::FilterDropdownHeight() const {
    if (m_mode != FileBrowserMode::OpenFile || m_filters.empty()) {
        return 0.0f;
    }
    return static_cast<float>(m_filters.size()) * kFilterItemH + 4.0f;
}

Rect FileBrowserSession::FilterDropdownRect(const Rect& pop) const {
    const Rect btn = FilterButtonRect(pop);
    const float h = FilterDropdownHeight();
    if (h <= 0.0f) {
        return Rect();
    }
    return Rect(btn.x, btn.y + btn.height + 2.0f, btn.width, h);
}

Rect FileBrowserSession::FilterItemRect(const Rect& pop, int index) const {
    const Rect menu = FilterDropdownRect(pop);
    return Rect(
        menu.x + 2.0f,
        menu.y + 2.0f + static_cast<float>(index) * kFilterItemH,
        menu.width - 4.0f,
        kFilterItemH);
}

int FileBrowserSession::HitTestFilterItem(const Rect& pop, Point pt) const {
    const Rect menu = FilterDropdownRect(pop);
    if (menu.IsEmpty() || !menu.Contains(pt.x, pt.y)) {
        return -1;
    }
    const float y = pt.y - (menu.y + 2.0f);
    if (y < 0.0f) {
        return -1;
    }
    const int index = static_cast<int>(y / kFilterItemH);
    if (index < 0 || index >= static_cast<int>(m_filters.size())) {
        return -1;
    }
    return index;
}

Rect FileBrowserSession::ListRect(const Rect& pop) const {
    return Rect(
        pop.x + 8.0f,
        pop.y + kHeaderH,
        pop.width - 16.0f,
        (std::max)(0.0f, pop.height - kHeaderH - kFooterH));
}

Rect FileBrowserSession::FooterRect(const Rect& pop) const {
    return Rect(pop.x, pop.y + pop.height - kFooterH, pop.width, kFooterH);
}

Rect FileBrowserSession::CancelButtonRect(const Rect& pop) const {
    return Rect(pop.x + pop.width - 180.0f, pop.y + pop.height - 36.0f, 78.0f, 26.0f);
}

Rect FileBrowserSession::ConfirmButtonRect(const Rect& pop) const {
    return Rect(pop.x + pop.width - 92.0f, pop.y + pop.height - 36.0f, 78.0f, 26.0f);
}

Rect FileBrowserSession::RowRect(const Rect& pop, int index) const {
    const Rect list = ListRect(pop);
    return Rect(
        list.x,
        list.y + static_cast<float>(index) * kRowH - m_scroll.GetOffset(),
        list.width - 2.0f,
        kRowH);
}

float FileBrowserSession::ContentHeight() const {
    return static_cast<float>(m_entries.size()) * kRowH;
}

int FileBrowserSession::HitTestRow(const Rect& pop, Point pt) const {
    const Rect list = ListRect(pop);
    if (!list.Contains(pt.x, pt.y)) {
        return -1;
    }
    const float y = pt.y - list.y + m_scroll.GetOffset();
    if (y < 0.0f) {
        return -1;
    }
    const int index = static_cast<int>(y / kRowH);
    if (index < 0 || index >= static_cast<int>(m_entries.size())) {
        return -1;
    }
    return index;
}

void FileBrowserSession::Render(
    GraphicsContext& ctx,
    const Rect& popRect,
    float animProgress,
    int hoverRow,
    bool hoverUp,
    bool hoverFilter,
    bool hoverCancel,
    bool hoverConfirm,
    bool filterDropDownOpen,
    int hoverFilterItem) const {
    if (animProgress <= 0.001f) {
        return;
    }

    const float currentH = (animProgress >= 0.98f) ? popRect.height : (popRect.height * animProgress);
    const Rect clip(popRect.x, popRect.y, popRect.width, currentH);
    ctx.PushClip(clip);

    const auto& tokens = ThemeManager::Instance().GetTokens();
    ctx.FillRoundedRect(popRect, 6.0f, tokens.cardBackground);
    ctx.DrawRoundedRect(popRect, 6.0f, tokens.cardBorder, 1.5f);

    // Header
    const Rect up = UpButtonRect(popRect);
    D2D1_COLOR_F upBg = hoverUp ? tokens.hoverBackground : tokens.cardBackground;
    ctx.FillRoundedRect(up, 4.0f, upBg);
    ctx.DrawText("↑", up, tokens.accentColor, "Segoe UI", 14.0f,
        DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER, DWRITE_FONT_WEIGHT_BOLD);

    if (m_mode == FileBrowserMode::OpenFile) {
        const Rect filterBtn = FilterButtonRect(popRect);
        const bool filterActive = filterDropDownOpen || hoverFilter;
        D2D1_COLOR_F fBg = filterActive ? tokens.hoverBackground : tokens.inputBackground;
        ctx.FillRoundedRect(filterBtn, 4.0f, fBg);
        ctx.DrawRoundedRect(filterBtn, 4.0f,
            filterDropDownOpen ? tokens.focusedBorder : tokens.inputBorder,
            filterDropDownOpen ? 1.5f : 1.0f);

        Rect filterText(
            filterBtn.x + 8.0f,
            filterBtn.y,
            filterBtn.width - 26.0f,
            filterBtn.height);
        ctx.DrawText(GetActiveFilterLabel(), filterText, tokens.textPrimary, "Segoe UI", 11.0f,
            DWRITE_TEXT_ALIGNMENT_LEADING, DWRITE_PARAGRAPH_ALIGNMENT_CENTER, DWRITE_FONT_WEIGHT_NORMAL, true);

        Rect chevronBox(filterBtn.x + filterBtn.width - 18.0f, filterBtn.y, 16.0f, filterBtn.height);
        ctx.DrawChevron(
            chevronBox,
            tokens.textSecondary,
            filterDropDownOpen ? GraphicsContext::ChevronDirection::Up : GraphicsContext::ChevronDirection::Down,
            1.4f);
    }

    ctx.DrawLine(
        Point(popRect.x + 6.0f, popRect.y + kHeaderH - 1.0f),
        Point(popRect.x + popRect.width - 6.0f, popRect.y + kHeaderH - 1.0f),
        tokens.cardBorder, 1.0f);

    // List
    const Rect list = ListRect(popRect);
    ctx.PushClip(list);
    for (int i = 0; i < static_cast<int>(m_entries.size()); ++i) {
        Rect row = RowRect(popRect, i);
        if (row.y + row.height < list.y || row.y > list.y + list.height) {
            continue;
        }
        const bool selected = (i == m_selectedIndex);
        const bool hovered = (i == hoverRow);
        if (selected) {
            D2D1_COLOR_F sel = tokens.accentColor;
            sel.a = 0.22f;
            ctx.FillRoundedRect(Rect(row.x + 2.0f, row.y + 2.0f, row.width - 4.0f, row.height - 4.0f), 4.0f, sel);
        } else if (hovered) {
            ctx.FillRoundedRect(Rect(row.x + 2.0f, row.y + 2.0f, row.width - 4.0f, row.height - 4.0f), 4.0f, tokens.hoverBackground);
        }

        const auto& entry = m_entries[static_cast<size_t>(i)];
        const char* icon = entry.isDirectory ? "📁 " : "📄 ";
        ctx.DrawText(
            std::string(icon) + entry.name,
            Rect(row.x + 8.0f, row.y, row.width - 12.0f, row.height),
            tokens.textPrimary,
            "Segoe UI",
            12.0f,
            DWRITE_TEXT_ALIGNMENT_LEADING,
            DWRITE_PARAGRAPH_ALIGNMENT_CENTER,
            DWRITE_FONT_WEIGHT_NORMAL,
            true);
    }
    ctx.PopClip();

    m_scroll.RenderChrome(ctx);

    // Footer
    ctx.DrawLine(
        Point(popRect.x + 6.0f, popRect.y + popRect.height - kFooterH + 1.0f),
        Point(popRect.x + popRect.width - 6.0f, popRect.y + popRect.height - kFooterH + 1.0f),
        tokens.cardBorder, 1.0f);

    const Rect cancel = CancelButtonRect(popRect);
    const Rect confirm = ConfirmButtonRect(popRect);
    ctx.FillRoundedRect(cancel, 4.0f, hoverCancel ? tokens.hoverBackground : tokens.inputBackground);
    ctx.DrawRoundedRect(cancel, 4.0f, tokens.inputBorder, 1.0f);
    ctx.DrawText("取消", cancel, tokens.textPrimary, "Segoe UI", 12.0f,
        DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    D2D1_COLOR_F okBg = hoverConfirm ? tokens.accentColor : tokens.accentColor;
    if (!hoverConfirm) {
        okBg.a = 0.90f;
    }
    ctx.FillRoundedRect(confirm, 4.0f, okBg);
    const char* okLabel = (m_mode == FileBrowserMode::OpenFolder) ? "选择" : "打开";
    ctx.DrawText(okLabel, confirm, D2D1::ColorF(1, 1, 1, 1), "Segoe UI", 12.0f,
        DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER, DWRITE_FONT_WEIGHT_SEMI_BOLD);

    if (filterDropDownOpen && m_mode == FileBrowserMode::OpenFile && !m_filters.empty()) {
        const Rect menu = FilterDropdownRect(popRect);
        ctx.FillRoundedRect(menu, 4.0f, tokens.cardBackground);
        ctx.DrawRoundedRect(menu, 4.0f, tokens.cardBorder, 1.5f);

        for (int i = 0; i < static_cast<int>(m_filters.size()); ++i) {
            const Rect item = FilterItemRect(popRect, i);
            const bool selected = (i == m_filterIndex);
            const bool hovered = (i == hoverFilterItem);
            if (selected) {
                D2D1_COLOR_F sel = tokens.accentColor;
                sel.a = 0.18f;
                ctx.FillRoundedRect(item, 3.0f, sel);
            } else if (hovered) {
                ctx.FillRoundedRect(item, 3.0f, tokens.hoverBackground);
            }

            D2D1_COLOR_F itemColor = selected ? tokens.accentColor : tokens.textPrimary;
            ctx.DrawText(
                m_filters[static_cast<size_t>(i)].first,
                Rect(item.x + 8.0f, item.y, item.width - 12.0f, item.height),
                itemColor,
                "Segoe UI",
                11.0f,
                DWRITE_TEXT_ALIGNMENT_LEADING,
                DWRITE_PARAGRAPH_ALIGNMENT_CENTER,
                selected ? DWRITE_FONT_WEIGHT_SEMI_BOLD : DWRITE_FONT_WEIGHT_NORMAL,
                true);
        }
    }

    ctx.PopClip();
}

} // namespace CUI
