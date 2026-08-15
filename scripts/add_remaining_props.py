import os

# 1. Window.h
path_win = r'E:\C++project\CUI\CUI.Core\ui\framework\window\Window.h'
with open(path_win, 'r', encoding='utf-8') as f:
    win = f.read()

if 'struct WindowThemeModeProperty' not in win:
    win_props = '''    struct WindowThemeModeProperty {
        Window* owner = nullptr;
        WindowThemeModeProperty() = default;
        explicit WindowThemeModeProperty(Window* o) : owner(o) {}
        WindowThemeModeProperty& operator=(ThemeMode m) { if (owner) owner->SetThemeMode(m); return *this; }
        operator ThemeMode() const { return owner ? owner->GetThemeMode() : ThemeMode::System; }
        ThemeMode Get() const { return owner ? owner->GetThemeMode() : ThemeMode::System; }
    } ThemeMode;

    struct WindowBackdropTypeProperty {
        Window* owner = nullptr;
        WindowBackdropTypeProperty() = default;
        explicit WindowBackdropTypeProperty(Window* o) : owner(o) {}
        WindowBackdropTypeProperty& operator=(BackdropType b) { if (owner) owner->SetBackdropType(b); return *this; }
        operator BackdropType() const { return owner ? owner->GetBackdropType() : BackdropType::None; }
        BackdropType Get() const { return owner ? owner->GetBackdropType() : BackdropType::None; }
    } BackdropType;

    struct WindowRenderStatsProperty {
        Window* owner = nullptr;
        WindowRenderStatsProperty() = default;
        explicit WindowRenderStatsProperty(Window* o) : owner(o) {}
        WindowRenderStatsProperty& operator=(bool v) { if (owner) owner->SetRenderStatsOverlayVisible(v); return *this; }
        operator bool() const { return owner ? owner->IsRenderStatsOverlayVisible() : false; }
        bool Get() const { return owner ? owner->IsRenderStatsOverlayVisible() : false; }
    } RenderStatsOverlayVisible;

    struct WindowRootElementProperty {
        Window* owner = nullptr;
        WindowRootElementProperty() = default;
        explicit WindowRootElementProperty(Window* o) : owner(o) {}
        WindowRootElementProperty& operator=(std::shared_ptr<UIElement> r) { if (owner) owner->SetRootElement(std::move(r)); return *this; }
        operator std::shared_ptr<UIElement>() const { return owner ? owner->GetRootElement() : nullptr; }
        std::shared_ptr<UIElement> Get() const { return owner ? owner->GetRootElement() : nullptr; }
        std::shared_ptr<UIElement> operator->() const { return owner ? owner->GetRootElement() : nullptr; }
    } RootElement;

    struct WindowHWNDProperty {
        Window* owner = nullptr;
        WindowHWNDProperty() = default;
        explicit WindowHWNDProperty(Window* o) : owner(o) {}
        operator ::HWND() const { return owner ? owner->GetHWND() : nullptr; }
        ::HWND Get() const { return owner ? owner->GetHWND() : nullptr; }
    } HWND;

'''
    win = win.replace('    void SetRootElement(std::shared_ptr<UIElement> root);', win_props + '    void SetRootElement(std::shared_ptr<UIElement> root);')
    with open(path_win, 'w', encoding='utf-8') as f:
        f.write(win)

# Update Window.cpp constructor
path_win_cpp = r'E:\C++project\CUI\CUI.Core\ui\framework\window\Window.cpp'
with open(path_win_cpp, 'r', encoding='utf-8') as f:
    wcpp = f.read()
if 'ThemeMode(this)' not in wcpp:
    wcpp = wcpp.replace('Window::Window() {', 'Window::Window() : ThemeMode(this), BackdropType(this), RenderStatsOverlayVisible(this), RootElement(this), HWND(this) {')
    with open(path_win_cpp, 'w', encoding='utf-8') as f:
        f.write(wcpp)

# 2. WindowTitleBar.h
path_tb = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\WindowTitleBar.h'
with open(path_tb, 'r', encoding='utf-8') as f:
    tb = f.read()

if 'struct WindowTitleBarRightContentProperty' not in tb:
    tb_props = '''    struct WindowTitleBarRightContentProperty {
        WindowTitleBar* owner = nullptr;
        WindowTitleBarRightContentProperty() = default;
        explicit WindowTitleBarRightContentProperty(WindowTitleBar* o) : owner(o) {}
        WindowTitleBarRightContentProperty& operator=(std::shared_ptr<UIElement> c) { if (owner) owner->SetRightContent(std::move(c)); return *this; }
        operator std::shared_ptr<UIElement>() const { return owner ? owner->GetRightContent() : nullptr; }
        std::shared_ptr<UIElement> Get() const { return owner ? owner->GetRightContent() : nullptr; }
        std::shared_ptr<UIElement> operator->() const { return owner ? owner->GetRightContent() : nullptr; }
    } RightContent;

    struct WindowTitleBarMenuBarProperty {
        WindowTitleBar* owner = nullptr;
        WindowTitleBarMenuBarProperty() = default;
        explicit WindowTitleBarMenuBarProperty(WindowTitleBar* o) : owner(o) {}
        operator MenuBar&() const { return owner->GetMenuBar(); }
        MenuBar* operator->() const { return &owner->GetMenuBar(); }
        MenuBar& Get() const { return owner->GetMenuBar(); }
    } MenuBar;

'''
    tb = tb.replace('    MenuBar& GetMenuBar() { return *m_menuBar; }', tb_props + '    MenuBar& GetMenuBar() { return *m_menuBar; }')
    with open(path_tb, 'w', encoding='utf-8') as f:
        f.write(tb)

# Update WindowTitleBar.cpp constructor
path_tb_cpp = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\WindowTitleBar.cpp'
with open(path_tb_cpp, 'r', encoding='utf-8') as f:
    tb_cpp = f.read()
if 'RightContent(this)' not in tb_cpp:
    tb_cpp = tb_cpp.replace('WindowTitleBar::WindowTitleBar() {', 'WindowTitleBar::WindowTitleBar() : RightContent(this), MenuBar(this) {')
    with open(path_tb_cpp, 'w', encoding='utf-8') as f:
        f.write(tb_cpp)

# 3. RangeSlider.h
path_rs = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\RangeSlider.h'
with open(path_rs, 'r', encoding='utf-8') as f:
    rs = f.read()

if 'struct RangeSliderMinimumProperty' not in rs:
    rs_props = '''    struct RangeSliderMinimumProperty {
        RangeSlider* owner = nullptr;
        RangeSliderMinimumProperty() = default;
        explicit RangeSliderMinimumProperty(RangeSlider* o) : owner(o) {}
        RangeSliderMinimumProperty& operator=(float v) { if (owner) owner->SetMinimum(v); return *this; }
        operator float() const { return owner ? owner->GetMinimum() : 0.0f; }
        float Get() const { return owner ? owner->GetMinimum() : 0.0f; }
    } Minimum;

    struct RangeSliderMaximumProperty {
        RangeSlider* owner = nullptr;
        RangeSliderMaximumProperty() = default;
        explicit RangeSliderMaximumProperty(RangeSlider* o) : owner(o) {}
        RangeSliderMaximumProperty& operator=(float v) { if (owner) owner->SetMaximum(v); return *this; }
        operator float() const { return owner ? owner->GetMaximum() : 100.0f; }
        float Get() const { return owner ? owner->GetMaximum() : 100.0f; }
    } Maximum;

    struct RangeSliderStepProperty {
        RangeSlider* owner = nullptr;
        RangeSliderStepProperty() = default;
        explicit RangeSliderStepProperty(RangeSlider* o) : owner(o) {}
        RangeSliderStepProperty& operator=(float v) { if (owner) owner->SetStep(v); return *this; }
        operator float() const { return owner ? owner->GetStep() : 1.0f; }
        float Get() const { return owner ? owner->GetStep() : 1.0f; }
    } Step;

    struct RangeSliderLowerProperty {
        RangeSlider* owner = nullptr;
        RangeSliderLowerProperty() = default;
        explicit RangeSliderLowerProperty(RangeSlider* o) : owner(o) {}
        RangeSliderLowerProperty& operator=(float v) { if (owner) owner->SetLowerValue(v); return *this; }
        operator float() const { return owner ? owner->GetLowerValue() : 0.0f; }
        float Get() const { return owner ? owner->GetLowerValue() : 0.0f; }
    } LowerValue;

    struct RangeSliderUpperProperty {
        RangeSlider* owner = nullptr;
        RangeSliderUpperProperty() = default;
        explicit RangeSliderUpperProperty(RangeSlider* o) : owner(o) {}
        RangeSliderUpperProperty& operator=(float v) { if (owner) owner->SetUpperValue(v); return *this; }
        operator float() const { return owner ? owner->GetUpperValue() : 100.0f; }
        float Get() const { return owner ? owner->GetUpperValue() : 100.0f; }
    } UpperValue;

'''
    rs = rs.replace('    float GetMinimum() const { return m_minimum; }', rs_props + '    float GetMinimum() const { return m_minimum; }')
    with open(path_rs, 'w', encoding='utf-8') as f:
        f.write(rs)

path_rs_cpp = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\RangeSlider.cpp'
with open(path_rs_cpp, 'r', encoding='utf-8') as f:
    rs_cpp = f.read()
if 'Minimum(this)' not in rs_cpp:
    rs_cpp = rs_cpp.replace('RangeSlider::RangeSlider() {', 'RangeSlider::RangeSlider() : Minimum(this), Maximum(this), Step(this), LowerValue(this), UpperValue(this) {')
    with open(path_rs_cpp, 'w', encoding='utf-8') as f:
        f.write(rs_cpp)

# 4. RatingControl.h
path_rc = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\RatingControl.h'
with open(path_rc, 'r', encoding='utf-8') as f:
    rc = f.read()

if 'struct RatingControlMaxRatingProperty' not in rc:
    rc_props = '''    struct RatingControlMaxRatingProperty {
        RatingControl* owner = nullptr;
        RatingControlMaxRatingProperty() = default;
        explicit RatingControlMaxRatingProperty(RatingControl* o) : owner(o) {}
        RatingControlMaxRatingProperty& operator=(int m) { if (owner) owner->SetMaxRating(m); return *this; }
        operator int() const { return owner ? owner->GetMaxRating() : 5; }
        int Get() const { return owner ? owner->GetMaxRating() : 5; }
    } MaxRating;

    struct RatingControlStepProperty {
        RatingControl* owner = nullptr;
        RatingControlStepProperty() = default;
        explicit RatingControlStepProperty(RatingControl* o) : owner(o) {}
        RatingControlStepProperty& operator=(float s) { if (owner) owner->SetStep(s); return *this; }
        operator float() const { return owner ? owner->GetStep() : 1.0f; }
        float Get() const { return owner ? owner->GetStep() : 1.0f; }
    } Step;

    struct RatingControlValueProperty {
        RatingControl* owner = nullptr;
        RatingControlValueProperty() = default;
        explicit RatingControlValueProperty(RatingControl* o) : owner(o) {}
        RatingControlValueProperty& operator=(float v) { if (owner) owner->SetValue(v); return *this; }
        operator float() const { return owner ? owner->GetValue() : 0.0f; }
        float Get() const { return owner ? owner->GetValue() : 0.0f; }
    } Value;

'''
    rc = rc.replace('    int GetMaxRating() const { return m_maxRating; }', rc_props + '    int GetMaxRating() const { return m_maxRating; }')
    with open(path_rc, 'w', encoding='utf-8') as f:
        f.write(rc)

path_rc_cpp = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\RatingControl.cpp'
with open(path_rc_cpp, 'r', encoding='utf-8') as f:
    rc_cpp = f.read()
if 'MaxRating(this)' not in rc_cpp:
    rc_cpp = rc_cpp.replace('RatingControl::RatingControl() {', 'RatingControl::RatingControl() : MaxRating(this), Step(this), Value(this) {')
    with open(path_rc_cpp, 'w', encoding='utf-8') as f:
        f.write(rc_cpp)

# 5. ListBox.h
path_lb = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\ListBox.h'
with open(path_lb, 'r', encoding='utf-8') as f:
    lb = f.read()

if 'struct ListBoxSelectedIndicesProperty' not in lb:
    lb_props = '''    struct ListBoxSelectedIndicesProperty {
        ListBox* owner = nullptr;
        ListBoxSelectedIndicesProperty() = default;
        explicit ListBoxSelectedIndicesProperty(ListBox* o) : owner(o) {}
        operator std::vector<int>() const { return owner ? owner->GetSelectedIndices() : std::vector<int>{}; }
        std::vector<int> Get() const { return owner ? owner->GetSelectedIndices() : std::vector<int>{}; }
    } SelectedIndices;

    struct ListBoxSelectedItemProperty {
        ListBox* owner = nullptr;
        ListBoxSelectedItemProperty() = default;
        explicit ListBoxSelectedItemProperty(ListBox* o) : owner(o) {}
        ListBoxSelectedItemProperty& operator=(const std::string& item) { if (owner) owner->SetSelectedItem(item); return *this; }
        operator std::string() const { return owner ? owner->GetSelectedItem() : ""; }
        std::string Get() const { return owner ? owner->GetSelectedItem() : ""; }
    } SelectedItem;

    struct ListBoxAllowDragProperty {
        ListBox* owner = nullptr;
        ListBoxAllowDragProperty() = default;
        explicit ListBoxAllowDragProperty(ListBox* o) : owner(o) {}
        ListBoxAllowDragProperty& operator=(bool d) { if (owner) owner->SetAllowDrag(d); return *this; }
        operator bool() const { return owner ? owner->GetAllowDrag() : false; }
        bool Get() const { return owner ? owner->GetAllowDrag() : false; }
    } AllowDrag;

    struct ListBoxAllowDropProperty {
        ListBox* owner = nullptr;
        ListBoxAllowDropProperty() = default;
        explicit ListBoxAllowDropProperty(ListBox* o) : owner(o) {}
        ListBoxAllowDropProperty& operator=(bool d) { if (owner) owner->SetAllowDrop(d); return *this; }
        operator bool() const { return owner ? owner->GetAllowDrop() : false; }
        bool Get() const { return owner ? owner->GetAllowDrop() : false; }
    } AllowDrop;

'''
    lb = lb.replace('    std::vector<int> GetSelectedIndices() const;', lb_props + '    std::vector<int> GetSelectedIndices() const;')
    with open(path_lb, 'w', encoding='utf-8') as f:
        f.write(lb)

path_lb_cpp = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\ListBox.cpp'
with open(path_lb_cpp, 'r', encoding='utf-8') as f:
    lb_cpp = f.read()
if 'SelectedIndices(this)' not in lb_cpp:
    lb_cpp = lb_cpp.replace('ListBox::ListBox() {', 'ListBox::ListBox() : SelectedIndices(this), SelectedItem(this), AllowDrag(this), AllowDrop(this) {')
    with open(path_lb_cpp, 'w', encoding='utf-8') as f:
        f.write(lb_cpp)

# 6. ListView.h
path_lv = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\ListView.h'
with open(path_lv, 'r', encoding='utf-8') as f:
    lv = f.read()

if 'struct ListViewSelectedIndicesProperty' not in lv:
    lv_props = '''    struct ListViewSelectedIndicesProperty {
        ListView* owner = nullptr;
        ListViewSelectedIndicesProperty() = default;
        explicit ListViewSelectedIndicesProperty(ListView* o) : owner(o) {}
        operator std::vector<int>() const { return owner ? owner->GetSelectedIndices() : std::vector<int>{}; }
        std::vector<int> Get() const { return owner ? owner->GetSelectedIndices() : std::vector<int>{}; }
    } SelectedIndices;

    struct ListViewCaretIndexProperty {
        ListView* owner = nullptr;
        ListViewCaretIndexProperty() = default;
        explicit ListViewCaretIndexProperty(ListView* o) : owner(o) {}
        ListViewCaretIndexProperty& operator=(int idx) { if (owner) owner->SetCaretIndex(idx); return *this; }
        operator int() const { return owner ? owner->GetCaretIndex() : 0; }
        int Get() const { return owner ? owner->GetCaretIndex() : 0; }
    } CaretIndex;

'''
    lv = lv.replace('    std::vector<int> GetSelectedIndices() const;', lv_props + '    std::vector<int> GetSelectedIndices() const;')
    with open(path_lv, 'w', encoding='utf-8') as f:
        f.write(lv)

path_lv_cpp = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\ListView.cpp'
with open(path_lv_cpp, 'r', encoding='utf-8') as f:
    lv_cpp = f.read()
if 'SelectedIndices(this)' not in lv_cpp:
    lv_cpp = lv_cpp.replace('ListView::ListView() {', 'ListView::ListView() : SelectedIndices(this), CaretIndex(this) {')
    with open(path_lv_cpp, 'w', encoding='utf-8') as f:
        f.write(lv_cpp)

# 7. TreeView.h
path_tv = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\TreeView.h'
with open(path_tv, 'r', encoding='utf-8') as f:
    tv = f.read()

if 'struct TreeViewSelectedItemProperty' not in tv:
    tv_props = '''    struct TreeViewItemsProperty {
        TreeView* owner = nullptr;
        TreeViewItemsProperty() = default;
        explicit TreeViewItemsProperty(TreeView* o) : owner(o) {}
        TreeViewItemsProperty& operator=(const std::vector<std::shared_ptr<TreeViewItem>>& items) { if (owner) owner->SetItems(items); return *this; }
        operator const std::vector<std::shared_ptr<TreeViewItem>>&() const { return owner->GetItems(); }
        const std::vector<std::shared_ptr<TreeViewItem>>& Get() const { return owner->GetItems(); }
    } Items;

    struct TreeViewSelectedItemProperty {
        TreeView* owner = nullptr;
        TreeViewSelectedItemProperty() = default;
        explicit TreeViewSelectedItemProperty(TreeView* o) : owner(o) {}
        TreeViewSelectedItemProperty& operator=(std::shared_ptr<TreeViewItem> item) { if (owner) owner->SetSelectedItem(std::move(item)); return *this; }
        operator std::shared_ptr<TreeViewItem>() const { return owner ? owner->GetSelectedItem() : nullptr; }
        std::shared_ptr<TreeViewItem> Get() const { return owner ? owner->GetSelectedItem() : nullptr; }
        std::shared_ptr<TreeViewItem> operator->() const { return owner ? owner->GetSelectedItem() : nullptr; }
    } SelectedItem;

    struct TreeViewIndentWidthProperty {
        TreeView* owner = nullptr;
        TreeViewIndentWidthProperty() = default;
        explicit TreeViewIndentWidthProperty(TreeView* o) : owner(o) {}
        TreeViewIndentWidthProperty& operator=(float w) { if (owner) owner->SetIndentWidth(w); return *this; }
        operator float() const { return owner ? owner->GetIndentWidth() : 20.0f; }
        float Get() const { return owner ? owner->GetIndentWidth() : 20.0f; }
    } IndentWidth;

'''
    tv = tv.replace('    void SetItems(const std::vector<std::shared_ptr<TreeViewItem>>& items);', tv_props + '    void SetItems(const std::vector<std::shared_ptr<TreeViewItem>>& items);')
    with open(path_tv, 'w', encoding='utf-8') as f:
        f.write(tv)

path_tv_cpp = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\TreeView.cpp'
with open(path_tv_cpp, 'r', encoding='utf-8') as f:
    tv_cpp = f.read()
if 'Items(this)' not in tv_cpp:
    tv_cpp = tv_cpp.replace('TreeView::TreeView() {', 'TreeView::TreeView() : Items(this), SelectedItem(this), IndentWidth(this) {')
    with open(path_tv_cpp, 'w', encoding='utf-8') as f:
        f.write(tv_cpp)

# 8. NavigationViewItem.h
path_nvi = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\NavigationViewItem.h'
with open(path_nvi, 'r', encoding='utf-8') as f:
    nvi = f.read()

nvi = nvi.replace('bool SelectsOnInvoked() const { return m_selectsOnInvoked; }', 'bool GetSelectsOnInvoked() const { return m_selectsOnInvoked; }')

if 'struct NavItemSelectsOnInvokedProperty' not in nvi:
    nvi_props = '''    struct NavItemSelectsOnInvokedProperty {
        NavigationViewItem* owner = nullptr;
        NavItemSelectsOnInvokedProperty() = default;
        explicit NavItemSelectsOnInvokedProperty(NavigationViewItem* o) : owner(o) {}
        NavItemSelectsOnInvokedProperty& operator=(bool v) { if (owner) owner->SetSelectsOnInvoked(v); return *this; }
        operator bool() const { return owner ? owner->GetSelectsOnInvoked() : true; }
        bool Get() const { return owner ? owner->GetSelectsOnInvoked() : true; }
    } SelectsOnInvoked;

'''
    nvi = nvi.replace('    void SetSelectsOnInvoked(bool value) { m_selectsOnInvoked = value; }', nvi_props + '    void SetSelectsOnInvoked(bool value) { m_selectsOnInvoked = value; }')
    with open(path_nvi, 'w', encoding='utf-8') as f:
        f.write(nvi)

path_nvi_cpp = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\NavigationViewItem.cpp'
with open(path_nvi_cpp, 'r', encoding='utf-8') as f:
    nvi_cpp = f.read()
if 'SelectsOnInvoked(this)' not in nvi_cpp:
    nvi_cpp = nvi_cpp.replace('NavigationViewItem::NavigationViewItem()\n    : NavigationViewItemBase() {',
                              'NavigationViewItem::NavigationViewItem()\n    : NavigationViewItemBase(), SelectsOnInvoked(this) {')
    nvi_cpp = nvi_cpp.replace('NavigationViewItem::NavigationViewItem(const std::string& content, const std::string& icon)\n    : NavigationViewItemBase(), m_content(content), m_icon(icon) {',
                              'NavigationViewItem::NavigationViewItem(const std::string& content, const std::string& icon)\n    : NavigationViewItemBase(), m_content(content), m_icon(icon), SelectsOnInvoked(this) {')
    with open(path_nvi_cpp, 'w', encoding='utf-8') as f:
        f.write(nvi_cpp)

# 9. CanvasPage.cpp
path_cp = r'E:\C++project\CUI\CUI.Gallery\src\pages\Layout\CanvasPage.cpp'
with open(path_cp, 'r', encoding='utf-8') as f:
    cp = f.read()
cp = cp.replace('world->Viewport = size.width, size.height;', 'world->SetViewport(size.width, size.height);')
with open(path_cp, 'w', encoding='utf-8') as f:
    f.write(cp)

print("Applied all remaining properties.")
