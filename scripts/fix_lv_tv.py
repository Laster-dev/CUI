import os
import re

# 1. ListView.h
path_lv = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\ListView.h'
with open(path_lv, 'r', encoding='utf-8') as f:
    lv = f.read()

# remove any old struct
lv = re.sub(r'struct ListViewSelectedIndicesProperty \{[\s\S]*?\} SelectedIndices\{this\};\s*', '', lv)
lv = re.sub(r'struct ListViewCaretIndexProperty \{[\s\S]*?\} CaretIndex\{this\};\s*', '', lv)

new_lv_props = '''    struct ListViewSelectedIndicesProperty {
        ListView* owner;
        operator const std::unordered_set<int>&() const { return owner->GetSelectedIndices(); }
        const std::unordered_set<int>& Get() const { return owner->GetSelectedIndices(); }
        size_t size() const { return owner->GetSelectedIndices().size(); }
        bool empty() const { return owner->GetSelectedIndices().empty(); }
    } SelectedIndices{this};

    struct ListViewCaretIndexProperty {
        ListView* owner;
        ListViewCaretIndexProperty& operator=(int idx) { owner->SetCaretIndex(idx); return *this; }
        operator int() const { return owner->GetCaretIndex(); }
        int Get() const { return owner->GetCaretIndex(); }
    } CaretIndex{this};

'''
lv = lv.replace('    // Columns Management', new_lv_props + '    // Columns Management')
with open(path_lv, 'w', encoding='utf-8') as f:
    f.write(lv)

# 2. TreeView.h
path_tv = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\TreeView.h'
with open(path_tv, 'r', encoding='utf-8') as f:
    tv = f.read()

tv = re.sub(r'struct TreeViewItemsProperty \{[\s\S]*?\} Items;\s*', '', tv)
tv = re.sub(r'struct TreeViewSelectedItemProperty \{[\s\S]*?\} SelectedItem;\s*', '', tv)
tv = re.sub(r'struct TreeViewIndentWidthProperty \{[\s\S]*?\} IndentWidth;\s*', '', tv)

new_tv_props = '''    struct TreeViewItemsProperty {
        TreeView* owner;
        TreeViewItemsProperty& operator=(const std::vector<std::shared_ptr<TreeViewItem>>& items) { owner->SetItems(items); return *this; }
        operator const std::vector<std::shared_ptr<TreeViewItem>>&() const { return owner->GetItems(); }
        const std::vector<std::shared_ptr<TreeViewItem>>& Get() const { return owner->GetItems(); }
        const std::vector<std::shared_ptr<TreeViewItem>>* operator->() const { return &owner->GetItems(); }
        size_t size() const { return owner->GetItems().size(); }
        bool empty() const { return owner->GetItems().empty(); }
        const std::shared_ptr<TreeViewItem>& front() const { return owner->GetItems().front(); }
        const std::shared_ptr<TreeViewItem>& back() const { return owner->GetItems().back(); }
        auto begin() const { return owner->GetItems().begin(); }
        auto end() const { return owner->GetItems().end(); }
        const std::shared_ptr<TreeViewItem>& operator[](size_t idx) const { return owner->GetItems()[idx]; }
    } Items{this};

    struct TreeViewSelectedItemProperty {
        TreeView* owner;
        TreeViewSelectedItemProperty& operator=(std::shared_ptr<TreeViewItem> item) { owner->SetSelectedItem(std::move(item)); return *this; }
        operator std::shared_ptr<TreeViewItem>() const { return owner->GetSelectedItem(); }
        std::shared_ptr<TreeViewItem> Get() const { return owner->GetSelectedItem(); }
        std::shared_ptr<TreeViewItem> operator->() const { return owner->GetSelectedItem(); }
        bool operator!() const { return !owner->GetSelectedItem(); }
        explicit operator bool() const { return static_cast<bool>(owner->GetSelectedItem()); }
        TreeViewItem* get() const { return owner->GetSelectedItem().get(); }
    } SelectedItem{this};

    struct TreeViewIndentWidthProperty {
        TreeView* owner;
        TreeViewIndentWidthProperty& operator=(float w) { owner->SetIndentWidth(w); return *this; }
        operator float() const { return owner->GetIndentWidth(); }
        float Get() const { return owner->GetIndentWidth(); }
    } IndentWidth{this};

'''
tv = tv.replace('    void ClearItems();', new_tv_props + '    void ClearItems();')
with open(path_tv, 'w', encoding='utf-8') as f:
    f.write(tv)

# 3. TreeView.cpp constructor
path_tv_cpp = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\TreeView.cpp'
with open(path_tv_cpp, 'r', encoding='utf-8') as f:
    tv_cpp = f.read()
tv_cpp = tv_cpp.replace('TreeView::TreeView() : Items(this), SelectedItem(this), IndentWidth(this) {', 'TreeView::TreeView() {')
with open(path_tv_cpp, 'w', encoding='utf-8') as f:
    f.write(tv_cpp)

print("Updated ListView.h and TreeView.h cleanly.")
