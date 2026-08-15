import os
import re
import sys

def safe_replace(content, pattern, replacement):
    return re.sub(pattern, replacement, content)

def update_file(path, transformer):
    with open(path, 'r', encoding='utf-8', errors='ignore') as f:
        content = f.read()
    new_content = transformer(content)
    if new_content != content:
        with open(path, 'w', encoding='utf-8') as f:
            f.write(new_content)
        print(f"Updated {path}")
        return True
    return False

# -------------------------------------------------------------
# 1. Update Core Headers
# -------------------------------------------------------------

def update_ui_element_h():
    path = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\UIElement.h'
    def trans(content):
        # ensure GetTag / SetTag exists
        if 'std::string GetTag()' not in content:
            content = content.replace(
                'void SetId(const std::string& id) { m_id = id; }',
                'void SetId(const std::string& id) { m_id = id; }\n    std::string GetTag() const { return m_tag; }\n    void SetTag(const std::string& tag) { m_tag = tag; }'
            )
        if 'std::string m_tag;' not in content:
            content = content.replace(
                'std::string m_id;',
                'std::string m_id;\n    std::string m_tag;'
            )
        return content
    update_file(path, trans)

def update_panel_h():
    path = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\Panel.h'
    def trans(content):
        if 'struct GridColumnsProperty' not in content:
            target = '    const std::vector<ColumnDefinition>& GetColumnDefinitions() const { return m_columns; }'
            replacement = '''    struct GridColumnsProperty {
        Grid* owner;
        GridColumnsProperty& operator=(const std::string& colDefsStr) { owner->SetColumnDefinitions(colDefsStr); return *this; }
        GridColumnsProperty& operator=(std::initializer_list<GridLength> lengths) {
            owner->m_columns.clear();
            for (const auto& len : lengths) {
                ColumnDefinition col;
                col.width = len;
                owner->m_columns.push_back(col);
            }
            owner->InvalidateMeasure();
            return *this;
        }
        GridColumnsProperty& operator=(const std::vector<ColumnDefinition>& cols) {
            owner->m_columns = cols;
            owner->InvalidateMeasure();
            return *this;
        }
        const std::vector<ColumnDefinition>& Get() const { return owner->m_columns; }
        operator const std::vector<ColumnDefinition>&() const { return owner->m_columns; }
    } Columns{this};

    struct GridRowsProperty {
        Grid* owner;
        GridRowsProperty& operator=(const std::string& rowDefsStr) { owner->SetRowDefinitions(rowDefsStr); return *this; }
        GridRowsProperty& operator=(std::initializer_list<GridLength> lengths) {
            owner->m_rows.clear();
            for (const auto& len : lengths) {
                RowDefinition row;
                row.height = len;
                owner->m_rows.push_back(row);
            }
            owner->InvalidateMeasure();
            return *this;
        }
        GridRowsProperty& operator=(const std::vector<RowDefinition>& rows) {
            owner->m_rows = rows;
            owner->InvalidateMeasure();
            return *this;
        }
        const std::vector<RowDefinition>& Get() const { return owner->m_rows; }
        operator const std::vector<RowDefinition>&() const { return owner->m_rows; }
    } Rows{this};

    const std::vector<ColumnDefinition>& GetColumnDefinitions() const { return m_columns; }'''
            content = content.replace(target, replacement)
        return content
    update_file(path, trans)

def update_canvas_control_h():
    path = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\CanvasControl.h'
    def trans(content):
        if 'CallbackProperty<void(GraphicsContext& ctx, Size size)> OnDraw;' not in content:
            target = '    DrawCallback m_onDraw;'
            replacement = '''public:
    CallbackProperty<void(GraphicsContext& ctx, Size size)> OnDraw;
    CallbackProperty<void(Point pt)> OnCanvasMouseDown;
    CallbackProperty<void(Point pt)> OnCanvasMouseUp;
    CallbackProperty<void(Point pt)> OnCanvasMouseMove;
    CallbackProperty<bool(float deltaSeconds)> OnTick;
private:
    DrawCallback m_onDraw;'''
            content = content.replace(target, replacement)
        return content
    update_file(path, trans)

def update_canvas_control_cpp():
    path = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\CanvasControl.cpp'
    def trans(content):
        content = content.replace('if (m_onDraw)', 'if (OnDraw) { OnDraw(ctx, GetBounds().GetSize()); } else if (m_onDraw)')
        content = content.replace('if (m_onMouseDown)', 'if (OnCanvasMouseDown) { OnCanvasMouseDown(localPt); } else if (m_onMouseDown)')
        content = content.replace('if (m_onMouseMove)', 'if (OnCanvasMouseMove) { OnCanvasMouseMove(localPt); } else if (m_onMouseMove)')
        content = content.replace('if (m_onMouseUp)', 'if (OnCanvasMouseUp) { OnCanvasMouseUp(localPt); } else if (m_onMouseUp)')
        content = content.replace('if (m_onTick)', 'if (OnTick) { bool keep = OnTick(delta); if (keep) RequestAnimationTicks(); return keep; } else if (m_onTick)')
        return content
    update_file(path, trans)

def update_list_box_h():
    path = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\ListBox.h'
    def trans(content):
        if 'struct ListBoxItemsProperty' not in content:
            target = '    // Items & Data Management'
            replacement = '''    struct ListBoxItemsProperty {
        ListBox* owner;
        ListBoxItemsProperty& operator=(const std::vector<std::string>& items) { owner->SetItems(items); return *this; }
        ListBoxItemsProperty& operator=(std::initializer_list<std::string> items) { owner->SetItems(std::vector<std::string>(items)); return *this; }
        ListBoxItemsProperty& operator=(const std::string& itemsCsv) { owner->SetItems(itemsCsv); return *this; }
        size_t size() const { return owner->GetItemCount(); }
    } Items{this};

    ReadOnlyProperty<size_t> RowCount{[this]() { return GetItemCount(); }};
    ReadOnlyProperty<size_t> ItemCount{[this]() { return GetItemCount(); }};

    struct ListBoxSelectionModeProperty {
        ListBox* owner;
        ListBoxSelectionModeProperty& operator=(ListBoxSelectionMode mode) { owner->SetSelectionMode(mode); return *this; }
        operator ListBoxSelectionMode() const { return owner->GetSelectionMode(); }
        ListBoxSelectionMode Get() const { return owner->GetSelectionMode(); }
    } SelectionMode{this};

    // Items & Data Management'''
            content = content.replace(target, replacement)
        return content
    update_file(path, trans)

def update_list_view_h():
    path = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\ListView.h'
    def trans(content):
        if 'struct ListViewRowsProperty' not in content:
            target = '    // In-Memory Data Rows Management'
            replacement = '''    struct ListViewRowsProperty {
        ListView* owner;
        ListViewRowsProperty& operator=(const std::vector<std::vector<std::string>>& rowsData) { owner->SetRows(rowsData); return *this; }
        ListViewRowsProperty& operator=(const std::vector<std::vector<ListViewCellData>>& rowsData) { owner->SetRows(rowsData); return *this; }
        size_t size() const { return owner->GetRowCount(); }
    } Rows{this};

    ReadOnlyProperty<size_t> RowCount{[this]() { return GetRowCount(); }};

    struct ListViewShowGridLinesProperty {
        ListView* owner;
        ListViewShowGridLinesProperty& operator=(bool show) { owner->SetShowGridLines(show); return *this; }
        operator bool() const { return owner->GetShowGridLines(); }
        bool Get() const { return owner->GetShowGridLines(); }
    } ShowGridLines{this};

    struct ListViewRowHeightProperty {
        ListView* owner;
        ListViewRowHeightProperty& operator=(float h) { owner->SetRowHeight(h); return *this; }
        operator float() const { return owner->GetRowHeight(); }
        float Get() const { return owner->GetRowHeight(); }
    } RowHeight{this};

    struct ListViewSelectionModeProperty {
        ListView* owner;
        ListViewSelectionModeProperty& operator=(ListViewSelectionMode mode) { owner->SetSelectionMode(mode); return *this; }
        operator ListViewSelectionMode() const { return owner->GetSelectionMode(); }
        ListViewSelectionMode Get() const { return owner->GetSelectionMode(); }
    } SelectionMode{this};

    // In-Memory Data Rows Management'''
            content = content.replace(target, replacement)
        return content
    update_file(path, trans)

def update_tree_view_h():
    path = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\TreeView.h'
    def trans(content):
        if 'void ExpandItem(' not in content:
            target = '    void SetItemExpanded(std::shared_ptr<TreeViewItem> item, bool expanded);'
            replacement = '''    void SetItemExpanded(std::shared_ptr<TreeViewItem> item, bool expanded);
    void ExpandItem(std::shared_ptr<TreeViewItem> item) { SetItemExpanded(item, true); }
    void CollapseItem(std::shared_ptr<TreeViewItem> item) { SetItemExpanded(item, false); }'''
            content = content.replace(target, replacement)
        return content
    update_file(path, trans)

def update_navigation_view_h():
    path = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\NavigationView.h'
    def trans(content):
        if 'struct NavPaneDisplayModeProperty' not in content:
            target = '    // --- Pane mode (WinUI) ---'
            replacement = '''    struct NavPaneDisplayModeProperty {
        NavigationView* owner;
        NavPaneDisplayModeProperty& operator=(NavigationViewPaneDisplayMode mode) { owner->SetPaneDisplayMode(mode); return *this; }
        operator NavigationViewPaneDisplayMode() const { return owner->GetPaneDisplayMode(); }
        NavigationViewPaneDisplayMode Get() const { return owner->GetPaneDisplayMode(); }
    } PaneDisplayMode{this};

    struct NavSelectedItemProperty {
        NavigationView* owner;
        NavSelectedItemProperty& operator=(NavigationViewItem* item) { owner->SetSelectedItem(item); return *this; }
        operator NavigationViewItem*() const { return owner->GetSelectedItem(); }
        NavigationViewItem* Get() const { return owner->GetSelectedItem(); }
        NavigationViewItem* operator->() const { return owner->GetSelectedItem(); }
    } SelectedItem{this};

    struct NavContentProperty {
        NavigationView* owner;
        NavContentProperty& operator=(const std::shared_ptr<UIElement>& content) { owner->SetContent(content); return *this; }
        operator std::shared_ptr<UIElement>() const { return owner->GetContent(); }
        std::shared_ptr<UIElement> Get() const { return owner->GetContent(); }
        std::shared_ptr<UIElement> operator->() const { return owner->GetContent(); }
    } Content{this};

    struct NavContentFactoryProperty {
        NavigationView* owner;
        NavContentFactoryProperty& operator=(std::function<std::shared_ptr<UIElement>()> factory) { owner->SetContentFactory(std::move(factory)); return *this; }
    } ContentFactory{this};

    struct NavAutoSuggestBoxProperty {
        NavigationView* owner;
        NavAutoSuggestBoxProperty& operator=(const std::shared_ptr<UIElement>& box) { owner->SetAutoSuggestBox(box); return *this; }
        operator std::shared_ptr<UIElement>() const { return owner->GetAutoSuggestBox(); }
        std::shared_ptr<UIElement> Get() const { return owner->GetAutoSuggestBox(); }
    } AutoSuggestBox{this};

    // --- Pane mode (WinUI) ---'''
            content = content.replace(target, replacement)
        return content
    update_file(path, trans)

def update_navigation_view_item_h():
    path = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\NavigationViewItem.h'
    def trans(content):
        if 'struct NavItemSelectsOnInvokedProperty' not in content:
            target = '    void SetSelectsOnInvoked(bool selects) { m_selectsOnInvoked = selects; }'
            replacement = '''    void SetSelectsOnInvoked(bool selects) { m_selectsOnInvoked = selects; }
    struct NavItemSelectsOnInvokedProperty {
        NavigationViewItem* owner;
        NavItemSelectsOnInvokedProperty& operator=(bool s) { owner->SetSelectsOnInvoked(s); return *this; }
        operator bool() const { return owner->SelectsOnInvoked(); }
        bool Get() const { return owner->SelectsOnInvoked(); }
    } SelectsOnInvoked{this};

    struct NavItemContentProperty {
        NavigationViewItem* owner;
        NavItemContentProperty& operator=(const std::string& text) { owner->SetContent(text); return *this; }
        NavItemContentProperty& operator=(std::shared_ptr<UIElement> elem) { owner->SetContent(elem); return *this; }
    } Content{this};'''
            content = content.replace(target, replacement)
        return content
    update_file(path, trans)

def update_auto_suggest_box_h():
    path = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\AutoSuggestBox.h'
    def trans(content):
        if 'struct AutoSuggestItemsProperty' not in content:
            target = '    // Full catalog; filtered with case-insensitive substring match unless a provider is set.'
            replacement = '''    struct AutoSuggestItemsProperty {
        AutoSuggestBox* owner;
        AutoSuggestItemsProperty& operator=(const std::vector<std::string>& items) { owner->SetSuggestionItems(items); return *this; }
        operator const std::vector<std::string>&() const { return owner->GetSuggestionItems(); }
    } SuggestionItems{this};

    struct AutoSuggestProviderProperty {
        AutoSuggestBox* owner;
        AutoSuggestProviderProperty& operator=(SuggestionProvider provider) { owner->SetSuggestionProvider(std::move(provider)); return *this; }
    } SuggestionProvider{this};

    struct AutoSuggestMaxVisibleProperty {
        AutoSuggestBox* owner;
        AutoSuggestMaxVisibleProperty& operator=(int n) { owner->SetMaxVisibleSuggestions(n); return *this; }
        operator int() const { return owner->GetMaxVisibleSuggestions(); }
        int Get() const { return owner->GetMaxVisibleSuggestions(); }
    } MaxVisibleSuggestions{this};

    // Full catalog; filtered with case-insensitive substring match unless a provider is set.'''
            content = content.replace(target, replacement)
        return content
    update_file(path, trans)

def update_window_h():
    path = r'E:\C++project\CUI\CUI.Core\ui\framework\window\Window.h'
    def trans(content):
        if 'struct WindowThemeModeProperty' not in content:
            target = '    // -------------------------------------------------------------------------'
            replacement = '''    struct WindowThemeModeProperty {
        Window* owner;
        WindowThemeModeProperty& operator=(CUI::ThemeMode mode) { owner->SetThemeMode(mode); return *this; }
        operator CUI::ThemeMode() const { return owner->GetThemeMode(); }
        CUI::ThemeMode Get() const { return owner->GetThemeMode(); }
    } ThemeMode{this};

    struct WindowBackdropTypeProperty {
        Window* owner;
        WindowBackdropTypeProperty& operator=(CUI::BackdropType type) { owner->SetBackdropType(type); return *this; }
        operator CUI::BackdropType() const { return owner->GetBackdropType(); }
        CUI::BackdropType Get() const { return owner->GetBackdropType(); }
    } BackdropType{this};

    struct WindowRenderStatsOverlayVisibleProperty {
        Window* owner;
        WindowRenderStatsOverlayVisibleProperty& operator=(bool v) { owner->SetRenderStatsOverlayVisible(v); return *this; }
        operator bool() const { return owner->IsRenderStatsOverlayVisible(); }
        bool Get() const { return owner->IsRenderStatsOverlayVisible(); }
    } RenderStatsOverlayVisible{this};

    struct WindowRootElementProperty {
        Window* owner;
        WindowRootElementProperty& operator=(std::shared_ptr<UIElement> root) { owner->SetRootElement(std::move(root)); return *this; }
        operator std::shared_ptr<UIElement>() const { return owner->GetRootElement(); }
        std::shared_ptr<UIElement> Get() const { return owner->GetRootElement(); }
        std::shared_ptr<UIElement> operator->() const { return owner->GetRootElement(); }
    } RootElement{this};

    ReadOnlyProperty<::HWND> HWND{[this]() { return GetHWND(); }};

    // -------------------------------------------------------------------------'''
            content = content.replace(target, replacement, 1)
        return content
    update_file(path, trans)

def update_cui_dsl_h():
    path = r'E:\C++project\CUI\CUI.Core\ui\framework\core\CUIDsl.h'
    def trans(content):
        # Update ElementBuilder to inherit from ElementRef<T>
        if 'class ElementBuilder : public ElementRef<T>' not in content:
            content = content.replace(
                'template <typename T>\nclass ElementBuilder {',
                'template <typename T>\nclass ElementBuilder : public ElementRef<T> {'
            )
            content = content.replace(
                '    ElementBuilder() : m_element(std::make_shared<T>()) {}',
                '    using ElementRef<T>::m_ptr;\n    ElementBuilder() : ElementRef<T>(std::make_shared<T>()) {}\n    explicit ElementBuilder(std::shared_ptr<T> elem) : ElementRef<T>(elem) {}\n    template<typename U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>> ElementBuilder(const ElementRef<U>& ref) : ElementRef<T>(ref.Shared()) {}'
            )
            content = content.replace('m_element', 'm_ptr')
        return content
    update_file(path, trans)

print("Updating Core headers...")
update_ui_element_h()
update_panel_h()
update_canvas_control_h()
update_canvas_control_cpp()
update_list_box_h()
update_list_view_h()
update_tree_view_h()
update_navigation_view_h()
update_navigation_view_item_h()
update_auto_suggest_box_h()
update_window_h()
update_cui_dsl_h()
print("Core headers updated successfully.")
