#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "FileBrowserHelper.h"
#include "../core/Value.h"
#include "../style/ThemeManager.h"
#include "BreadcrumbBar.h"
#include "TreeView.h"
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

bool IsPlaceholderChild(const std::shared_ptr<TreeViewItem>& item) {
    return item && item->tag == "__placeholder__";
}

bool HasOnlyPlaceholder(const std::shared_ptr<TreeViewItem>& item) {
    return item && item->children.size() == 1 && IsPlaceholderChild(item->children[0]);
}

std::string NormalizeDirPath(std::string path) {
    if (path.empty()) {
        return path;
    }
    while (path.size() > 3 && (path.back() == '\\' || path.back() == '/')) {
        path.pop_back();
    }
    // Keep drive root as "C:\"
    if (path.size() == 2 && path[1] == ':') {
        path.push_back('\\');
    }
    return path;
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
}

void FileBrowserSession::SetFilterIndex(int index) {
    if (m_filters.empty()) {
        return;
    }
    m_filterIndex = std::clamp(index, 0, static_cast<int>(m_filters.size()) - 1);
}

void FileBrowserSession::SetCurrentPath(const std::string& path) {
    m_currentPath = NormalizeDirPath(path);
}

std::string FileBrowserSession::ResolveStartDirectory(const std::string& startPath) const {
    if (!startPath.empty()) {
        if (IsDirectoryUtf8(startPath)) {
            return NormalizeDirPath(startPath);
        }
        std::string parent = ParentPathUtf8(startPath);
        if (!parent.empty() && IsDirectoryUtf8(parent)) {
            return NormalizeDirPath(parent);
        }
        if (PathExistsUtf8(startPath)) {
            return NormalizeDirPath(ParentPathUtf8(startPath));
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

const std::string& FileBrowserSession::GetActiveFilterLabel() const {
    static const std::string kEmpty;
    if (m_filters.empty()) {
        return kEmpty;
    }
    const int idx = std::clamp(m_filterIndex, 0, static_cast<int>(m_filters.size()) - 1);
    return m_filters[static_cast<size_t>(idx)].first;
}

Rect FileBrowserSession::HeaderRect(const Rect& pop) const {
    return Rect(pop.x, pop.y, pop.width, kHeaderH);
}

Rect FileBrowserSession::UpButtonRect(const Rect& pop) const {
    return Rect(pop.x + 8.0f, pop.y + 6.0f, 28.0f, 24.0f);
}

Rect FileBrowserSession::FilterButtonRect(const Rect& pop) const {
    return Rect(pop.x + pop.width - 140.0f, pop.y + 6.0f, 128.0f, 24.0f);
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
    // Flush to popup edges — no nested padded "card" around the tree.
    return Rect(
        pop.x + 1.0f,
        pop.y + kHeaderH,
        pop.width - 2.0f,
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

void FileBrowserSession::RenderChrome(
    GraphicsContext& ctx,
    const Rect& popRect,
    float animProgress,
    bool hoverUp,
    bool hoverFilter,
    bool hoverCancel,
    bool hoverConfirm,
    bool filterDropDownOpen) const {
    if (animProgress <= 0.001f) {
        return;
    }

    const auto& tokens = ThemeManager::Instance().GetTokens();
    ctx.FillRoundedRect(popRect, 6.0f, tokens.cardBackground);
    ctx.DrawRoundedRect(popRect, 6.0f, tokens.cardBorder, 1.5f);

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

    D2D1_COLOR_F okBg = tokens.accentColor;
    if (!hoverConfirm) {
        okBg.a = 0.90f;
    }
    ctx.FillRoundedRect(confirm, 4.0f, okBg);
    const char* okLabel = (m_mode == FileBrowserMode::OpenFolder) ? "选择" : "打开";
    ctx.DrawText(okLabel, confirm, D2D1::ColorF(1, 1, 1, 1), "Segoe UI", 12.0f,
        DWRITE_TEXT_ALIGNMENT_CENTER, DWRITE_PARAGRAPH_ALIGNMENT_CENTER, DWRITE_FONT_WEIGHT_SEMI_BOLD);
}

void FileBrowserSession::RenderFilterDropdown(
    GraphicsContext& ctx,
    const Rect& popRect,
    float animProgress,
    int hoverFilterItem) const {
    if (animProgress <= 0.001f
        || m_mode != FileBrowserMode::OpenFile
        || m_filters.empty()) {
        return;
    }

    const auto& tokens = ThemeManager::Instance().GetTokens();
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
        m_onNavigate(ResolveFileBrowserBreadcrumbPath(index, m_bar->GetPath()));
    });
}

void FileBrowserBreadcrumbHost::AttachTo(UIElement* owner) {
    if (!owner || !m_bar) {
        return;
    }
    m_bar->SetOverlayComposed(true);
    owner->AddChildQuiet(m_bar);
}

void FileBrowserBreadcrumbHost::SetNavigateHandler(NavigateCallback handler) {
    m_onNavigate = std::move(handler);
}

void FileBrowserBreadcrumbHost::Sync(const std::string& currentPath) {
    m_bar->SetPath(BuildFileBrowserBreadcrumb(currentPath));
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

UIElement* FileBrowserBreadcrumbHost::HitTest(float x, float y) {
    return m_bar ? m_bar->HitTest(x, y) : nullptr;
}

void FileBrowserBreadcrumbHost::DismissOverflowMenu() {
    if (m_bar) {
        m_bar->DismissOverflowMenu();
    }
}

bool FileBrowserBreadcrumbHost::IsOverflowMenuOpen() const {
    return m_bar && m_bar->IsOverflowMenuOpen();
}

Rect FileBrowserBreadcrumbHost::GetOverflowMenuClientBounds() const {
    return m_bar ? m_bar->GetOverflowMenuClientBounds() : Rect();
}

FileBrowserTreeHost::FileBrowserTreeHost() {
    m_tree = std::make_shared<TreeView>();
    m_tree->SetFontFamily("Segoe UI");
    m_tree->SetFontSize(12.0f);
    // Same surface as the popup chrome — no nested rounded border box.
    m_tree->SetCornerRadius(0.0f);
    m_tree->SetBorderThickness(0.0f);
    m_tree->SetBackgroundToken(ThemeTokenId::CardBackground);
    m_tree->SetBorderToken(ThemeTokenId::Unset);
    m_tree->SetColorToken(ThemeTokenId::TextPrimary);
    m_tree->SetHoverBackgroundToken(ThemeTokenId::HoverBackground);
    m_tree->SetSelectedBackgroundToken(ThemeTokenId::SelectedBackground);
    m_tree->SetIndentWidth(16.0f);

    m_tree->OnSelectionChanged().Connect([this](TreeView*, std::shared_ptr<TreeViewItem> item) {
        if (!item || item->tag.empty() || IsPlaceholderChild(item)) {
            return;
        }
        if (m_onPathChanged) {
            if (IsDirectoryUtf8(item->tag) || item->tag.size() >= 2) {
                if (IsDirectoryUtf8(item->tag)) {
                    m_onPathChanged(NormalizeDirPath(item->tag));
                } else {
                    m_onPathChanged(NormalizeDirPath(ParentPathUtf8(item->tag)));
                }
            }
        }
    });

    m_tree->OnItemToggled().Connect([this](TreeView*, std::shared_ptr<TreeViewItem> item) {
        if (!item || m_loadingGuard) {
            return;
        }
        if (item->isExpanded) {
            EnsureChildrenLoaded(item, MakeSession(item->tag));
        }
    });

    m_tree->OnItemDoubleClicked().Connect([this](TreeView*, std::shared_ptr<TreeViewItem> item) {
        if (!item || item->tag.empty() || IsPlaceholderChild(item)) {
            return;
        }
        if (IsDirectoryUtf8(item->tag) || item->tag.empty()) {
            m_tree->ToggleExpanded(item);
            if (item->isExpanded) {
                EnsureChildrenLoaded(item, MakeSession(item->tag));
            }
            return;
        }
        if (m_mode == FileBrowserMode::OpenFile && m_onConfirm) {
            m_onConfirm(item->tag);
        }
    });
}

void FileBrowserTreeHost::AttachTo(UIElement* owner) {
    if (!owner || !m_tree) {
        return;
    }
    m_tree->SetOverlayComposed(true);
    owner->AddChildQuiet(m_tree);
}

FileBrowserSession FileBrowserTreeHost::MakeSession(const std::string& pathHint) const {
    FileBrowserSession session;
    session.Configure(m_mode, pathHint.empty() ? GetDefaultUserDirectory() : pathHint, m_filters, m_filterIndex);
    return session;
}

void FileBrowserTreeHost::SetPathChangedHandler(PathChangedCallback handler) {
    m_onPathChanged = std::move(handler);
}

void FileBrowserTreeHost::SetConfirmHandler(ConfirmCallback handler) {
    m_onConfirm = std::move(handler);
}

std::shared_ptr<TreeViewItem> FileBrowserTreeHost::MakePlaceholderChild() const {
    auto placeholder = std::make_shared<TreeViewItem>();
    placeholder->header = "...";
    placeholder->tag = "__placeholder__";
    return placeholder;
}

std::shared_ptr<TreeViewItem> FileBrowserTreeHost::MakeDirNode(
    const std::string& name,
    const std::string& fullPath) const {
    auto node = std::make_shared<TreeViewItem>();
    node->header = name;
    node->icon = "📁";
    node->tag = NormalizeDirPath(fullPath);
    node->children.push_back(MakePlaceholderChild());
    return node;
}

std::shared_ptr<TreeViewItem> FileBrowserTreeHost::MakeFileNode(
    const std::string& name,
    const std::string& fullPath) const {
    auto node = std::make_shared<TreeViewItem>();
    node->header = name;
    node->icon = "📄";
    node->tag = fullPath;
    return node;
}

void FileBrowserTreeHost::LoadChildren(
    const std::shared_ptr<TreeViewItem>& node,
    const FileBrowserSession& session) {
    if (!node) {
        return;
    }
    node->children.clear();

    if (node->tag.empty()) {
        // "此电脑" root → drives
        const DWORD mask = GetLogicalDrives();
        for (int i = 0; i < 26; ++i) {
            if ((mask & (1u << i)) == 0) {
                continue;
            }
            char letter = static_cast<char>('A' + i);
            std::string root = std::string(1, letter) + ":\\";
            node->children.push_back(MakeDirNode(std::string(1, letter) + ":", root));
        }
        return;
    }

    if (!IsDirectoryUtf8(node->tag)) {
        return;
    }

    std::error_code ec;
    fs::directory_iterator it(ToPath(node->tag), ec);
    if (ec) {
        return;
    }

    std::vector<std::shared_ptr<TreeViewItem>> dirs;
    std::vector<std::shared_ptr<TreeViewItem>> files;
    for (const auto& entry : it) {
        std::error_code entryEc;
        const bool isDir = entry.is_directory(entryEc);
        if (entryEc) {
            continue;
        }
        const std::string name = Utf16ToUtf8(entry.path().filename().wstring());
        const std::string full = FromPath(entry.path());
        if (isDir) {
            dirs.push_back(MakeDirNode(name, full));
        } else if (session.GetMode() == FileBrowserMode::OpenFile && session.MatchesFilter(name)) {
            files.push_back(MakeFileNode(name, full));
        }
    }

    auto byName = [](const std::shared_ptr<TreeViewItem>& a, const std::shared_ptr<TreeViewItem>& b) {
        return ToLowerAscii(a->header) < ToLowerAscii(b->header);
    };
    std::sort(dirs.begin(), dirs.end(), byName);
    std::sort(files.begin(), files.end(), byName);
    node->children.insert(node->children.end(), dirs.begin(), dirs.end());
    node->children.insert(node->children.end(), files.begin(), files.end());
}

bool FileBrowserTreeHost::EnsureChildrenLoaded(
    const std::shared_ptr<TreeViewItem>& node,
    const FileBrowserSession& session) {
    if (!node) {
        return false;
    }
    // Computer root or placeholder-backed folder needs (re)load.
    const bool needsLoad = node->tag.empty() || HasOnlyPlaceholder(node);
    if (!needsLoad) {
        return false;
    }

    m_loadingGuard = true;
    LoadChildren(node, session);
    for (auto& child : node->children) {
        child->parent = node.get();
    }
    m_tree->InvalidateVisibleItems();
    m_loadingGuard = false;
    return true;
}

std::shared_ptr<TreeViewItem> FileBrowserTreeHost::FindNodeByPath(
    const std::vector<std::shared_ptr<TreeViewItem>>& items,
    const std::string& path) const {
    const std::string target = NormalizeDirPath(path);
    for (const auto& item : items) {
        if (!item) {
            continue;
        }
        if (NormalizeDirPath(item->tag) == target) {
            return item;
        }
        auto found = FindNodeByPath(item->children, path);
        if (found) {
            return found;
        }
    }
    return nullptr;
}

std::shared_ptr<TreeViewItem> FileBrowserTreeHost::FindNodeByPath(const std::string& path) const {
    return FindNodeByPath(m_tree->GetItems(), path);
}

void FileBrowserTreeHost::RebuildRoots(const FileBrowserSession& session) {
    m_mode = session.GetMode();
    m_tree->ClearItems();

    auto computer = std::make_shared<TreeViewItem>();
    computer->header = "此电脑";
    computer->icon = "💻";
    computer->tag.clear();
    computer->isExpanded = true;
    computer->expandAnim.Reset(1.0f);
    LoadChildren(computer, session);
    for (auto& child : computer->children) {
        child->parent = computer.get();
    }
    m_tree->AddItem(computer);
}

void FileBrowserTreeHost::ExpandToPath(const std::string& path, const FileBrowserSession& session) {
    if (path.empty()) {
        auto roots = m_tree->GetItems();
        if (!roots.empty()) {
            m_tree->SetSelectedItem(roots[0]);
        }
        return;
    }

    const auto nodes = BuildFileBrowserBreadcrumb(path);
    if (nodes.size() < 2) {
        return;
    }

    // Walk: 此电脑 -> C: -> Users -> ...
    std::shared_ptr<TreeViewItem> current;
    if (!m_tree->GetItems().empty()) {
        current = m_tree->GetItems()[0];
    }
    if (!current) {
        return;
    }

    std::string walked;
    for (size_t i = 1; i < nodes.size(); ++i) {
        if (i == 1) {
            walked = nodes[1] + "\\";
        } else {
            walked = JoinPathUtf8(walked, nodes[i]);
        }
        EnsureChildrenLoaded(current, session);
        std::shared_ptr<TreeViewItem> next;
        for (auto& child : current->children) {
            if (IsPlaceholderChild(child)) {
                continue;
            }
            if (NormalizeDirPath(child->tag) == NormalizeDirPath(walked)
                || ToLowerAscii(child->header) == ToLowerAscii(nodes[i])
                || ToLowerAscii(child->header) == ToLowerAscii(nodes[i] + ":")) {
                next = child;
                break;
            }
        }
        if (!next) {
            break;
        }
        if (!next->children.empty()) {
            next->isExpanded = true;
            next->expandAnim.Reset(1.0f);
        }
        current = next;
    }

    if (current) {
        EnsureChildrenLoaded(current, session);
        current->isExpanded = true;
        current->expandAnim.Reset(1.0f);
        m_tree->InvalidateVisibleItems();
        m_tree->SetSelectedItem(current);
    }
}

void FileBrowserTreeHost::Configure(const FileBrowserSession& session) {
    m_mode = session.GetMode();
    m_filters = session.GetFilters();
    m_filterIndex = session.GetFilterIndex();
    RebuildRoots(session);
    ExpandToPath(session.GetCurrentPath(), session);
}

void FileBrowserTreeHost::ApplyFilter(const FileBrowserSession& session) {
    m_mode = session.GetMode();
    m_filters = session.GetFilters();
    m_filterIndex = session.GetFilterIndex();

    std::function<void(const std::shared_ptr<TreeViewItem>&)> reload =
        [&](const std::shared_ptr<TreeViewItem>& node) {
            if (!node || IsPlaceholderChild(node)) {
                return;
            }
            if (node->tag.empty() || node->isExpanded) {
                LoadChildren(node, session);
                for (auto& child : node->children) {
                    child->parent = node.get();
                }
            }
            for (const auto& child : node->children) {
                if (!IsPlaceholderChild(child) && child->isExpanded) {
                    reload(child);
                }
            }
        };

    for (const auto& root : m_tree->GetItems()) {
        reload(root);
    }
    m_tree->InvalidateVisibleItems();
    ExpandToPath(session.GetCurrentPath(), session);
}

void FileBrowserTreeHost::NavigateTo(const std::string& path, const FileBrowserSession& session) {
    ExpandToPath(path, session);
}

void FileBrowserTreeHost::GoUp(const FileBrowserSession& session) {
    const std::string parent = ParentPathUtf8(session.GetCurrentPath());
    ExpandToPath(parent, session);
}

void FileBrowserTreeHost::Layout(const Rect& listRect) {
    m_tree->SetBounds(listRect);
    m_tree->SetWidth(listRect.width);
    m_tree->SetHeight(listRect.height);
}

void FileBrowserTreeHost::Render(GraphicsContext& ctx) {
    if (m_tree->GetBounds().IsEmpty()) {
        return;
    }
    m_tree->Render(ctx);
}

UIElement* FileBrowserTreeHost::HitTest(float x, float y) {
    return m_tree ? m_tree->HitTest(x, y) : nullptr;
}

bool FileBrowserTreeHost::Tick() {
    return m_tree->OnAnimationTick();
}

bool FileBrowserTreeHost::NeedsAnimationTicks() const {
    return m_tree->HasSelfAnimation();
}

std::string FileBrowserTreeHost::GetSelectedPath() const {
    auto selected = m_tree->GetSelectedItem();
    if (!selected || IsPlaceholderChild(selected)) {
        return {};
    }
    return selected->tag;
}

bool FileBrowserTreeHost::TryConfirm(const FileBrowserSession& session, std::string& outPath) const {
    outPath.clear();
    const std::string selected = GetSelectedPath();
    if (session.GetMode() == FileBrowserMode::OpenFolder) {
        if (!selected.empty() && IsDirectoryUtf8(selected)) {
            outPath = NormalizeDirPath(selected);
            return true;
        }
        if (!session.GetCurrentPath().empty() && IsDirectoryUtf8(session.GetCurrentPath())) {
            outPath = NormalizeDirPath(session.GetCurrentPath());
            return true;
        }
        return false;
    }

    if (!selected.empty() && !IsDirectoryUtf8(selected) && selected != "__placeholder__") {
        outPath = selected;
        return true;
    }
    return false;
}

} // namespace CUI
