import os
import re
import sys

def parse_balanced_arg(s, start_idx):
    # s[start_idx] is the '('
    depth = 1
    i = start_idx + 1
    n = len(s)
    in_string = False
    in_char = False
    quote_char = ''
    while i < n and depth > 0:
        ch = s[i]
        if not in_string and not in_char:
            if ch == '"':
                in_string = True
            elif ch == "'":
                in_char = True
            elif ch == '(':
                depth += 1
            elif ch == ')':
                depth -= 1
        elif in_string:
            if ch == '\\':
                i += 1
            elif ch == '"':
                in_string = False
        elif in_char:
            if ch == '\\':
                i += 1
            elif ch == "'":
                in_char = False
        i += 1
    if depth == 0:
        return s[start_idx+1:i-1], i
    return None, start_idx

def replace_set_calls(content, method_name, property_name):
    # Matches obj->SetMethod(...) or obj.SetMethod(...)
    pos = 0
    result = []
    pattern = re.compile(r'(\->|\.)' + re.escape(method_name) + r'\s*\(')
    while True:
        m = pattern.search(content, pos)
        if not m:
            result.append(content[pos:])
            break
        result.append(content[pos:m.start()])
        op = m.group(1) # -> or .
        open_paren_idx = m.end() - 1
        arg_content, end_idx = parse_balanced_arg(content, open_paren_idx)
        if arg_content is not None:
            # check if there's a following semicolon or chained call
            # obj->Prop = arg
            result.append(f'{op}{property_name} = {arg_content}')
            pos = end_idx
        else:
            result.append(content[m.start():m.end()])
            pos = m.end()
    return ''.join(result)

def replace_get_calls(content, method_name, property_name):
    pattern = re.compile(r'(\->|\.)' + re.escape(method_name) + r'\s*\(\s*\)')
    return pattern.sub(r'\1' + property_name, content)

SIMPLE_SET_MAP = [
    ('SetText', 'Text'),
    ('SetMarkdown', 'Markdown'),
    ('SetPlaceholder', 'Placeholder'),
    ('SetPassword', 'Password'),
    ('SetInputText', 'InputText'),
    ('SetHeader', 'Header'),
    ('SetSubtitle', 'Subtitle'),
    ('SetTitle', 'Title'),
    ('SetMessage', 'Message'),
    ('SetActionText', 'ActionText'),
    ('SetPrimaryButtonText', 'PrimaryButtonText'),
    ('SetSecondaryButtonText', 'SecondaryButtonText'),
    ('SetCloseButtonText', 'CloseButtonText'),
    ('SetGroupName', 'GroupName'),
    ('SetNavigateUri', 'NavigateUri'),
    ('SetPaneTitle', 'PaneTitle'),
    ('SetId', 'Id'),
    ('SetTag', 'Tag'),
    ('SetWidth', 'Width'),
    ('SetHeight', 'Height'),
    ('SetMinWidth', 'MinWidth'),
    ('SetMinHeight', 'MinHeight'),
    ('SetMaxWidth', 'MaxWidth'),
    ('SetMaxHeight', 'MaxHeight'),
    ('SetMargin', 'Margin'),
    ('SetPadding', 'Padding'),
    ('SetFlexGrow', 'FlexGrow'),
    ('SetOrientation', 'Orientation'),
    ('SetGap', 'Gap'),
    ('SetCornerRadius', 'CornerRadius'),
    ('SetBorderThickness', 'BorderThickness'),
    ('SetOpacity', 'Opacity'),
    ('SetClipToBounds', 'ClipToBounds'),
    ('SetItemWidth', 'ItemWidth'),
    ('SetItemHeight', 'ItemHeight'),
    ('SetRows', 'Rows'),
    ('SetColumns', 'Columns'),
    ('SetCanvasLeft', 'CanvasLeft'),
    ('SetCanvasTop', 'CanvasTop'),
    ('SetCanvasRight', 'CanvasRight'),
    ('SetCanvasBottom', 'CanvasBottom'),
    ('SetZIndex', 'ZIndex'),
    ('SetGridColumn', 'GridColumn'),
    ('SetGridRow', 'GridRow'),
    ('SetGridColumnSpan', 'GridColumnSpan'),
    ('SetGridRowSpan', 'GridRowSpan'),
    ('SetFontSize', 'FontSize'),
    ('SetFontFamily', 'FontFamily'),
    ('SetFontWeight', 'FontWeight'),
    ('SetFontStyle', 'FontStyle'),
    ('SetFontStretch', 'FontStretch'),
    ('SetToolTip', 'ToolTip'),
    ('SetIcon', 'Icon'),
    ('SetBackgroundToken', 'BackgroundToken'),
    ('SetBorderToken', 'BorderToken'),
    ('SetColorToken', 'ColorToken'),
    ('SetThemeMode', 'ThemeMode'),
    ('SetBackdropType', 'BackdropType'),
    ('SetRenderStatsOverlayVisible', 'RenderStatsOverlayVisible'),
    ('SetRootElement', 'RootElement'),
    ('SetRightContent', 'RightContent'),
    ('SetOnDraw', 'OnDraw'),
    ('SetOnTick', 'OnTick'),
    ('SetOnCanvasMouseDown', 'OnCanvasMouseDown'),
    ('SetOnCanvasMouseMove', 'OnCanvasMouseMove'),
    ('SetOnCanvasMouseUp', 'OnCanvasMouseUp'),
    ('SetItems', 'Items'),
    ('SetColumnDefinitions', 'Columns'),
    ('SetRowDefinitions', 'Rows'),
    ('SetSelectedIndex', 'SelectedIndex'),
    ('SetSelectedItem', 'SelectedItem'),
    ('SetValue', 'Value'),
    ('SetMinimum', 'Minimum'),
    ('SetMaximum', 'Maximum'),
    ('SetStep', 'Step'),
    ('SetLowerValue', 'LowerValue'),
    ('SetUpperValue', 'UpperValue'),
    ('SetMaxRating', 'MaxRating'),
    ('SetIsIndeterminate', 'IsIndeterminate'),
    ('SetSelectedColor', 'SelectedColor'),
    ('SetContent', 'Content'),
    ('SetContentFactory', 'ContentFactory'),
    ('SetAutoSuggestBox', 'AutoSuggestBox'),
    ('SetPaneDisplayMode', 'PaneDisplayMode'),
    ('SetMaxVisibleSuggestions', 'MaxVisibleSuggestions'),
    ('SetSuggestionItems', 'SuggestionItems'),
    ('SetSuggestionProvider', 'SuggestionProvider'),
    ('SetTotalPages', 'TotalPages'),
    ('SetCurrentPage', 'CurrentPage'),
    ('SetState', 'State'),
    ('SetShowGridLines', 'ShowGridLines'),
    ('SetRowHeight', 'RowHeight'),
    ('SetSelectionMode', 'SelectionMode'),
    ('SetSelectsOnInvoked', 'SelectsOnInvoked'),
    ('SetFollowTail', 'FollowTail'),
    ('SetIsOn', 'IsOn'),
    ('SetIsChecked', 'IsChecked'),
    ('SetIsExpanded', 'IsExpanded'),
    ('SetIsOpen', 'IsOpen'),
    ('SetIsCloseVisible', 'IsCloseVisible'),
    ('SetIsModal', 'IsModal'),
    ('SetIsSettingsVisible', 'IsSettingsVisible'),
    ('SetIsPaneOpen', 'IsPaneOpen'),
    ('SetIsThreeState', 'IsThreeState'),
    ('SetIsReadOnly', 'IsReadOnly'),
    ('SetAcceptsReturn', 'AcceptsReturn'),
    ('SetTextWrapping', 'TextWrapping'),
    ('SetShowRevealButton', 'ShowRevealButton'),
    ('SetIsPasswordMode', 'IsPasswordMode'),
    ('SetFill', 'Fill'),
    ('SetStroke', 'Stroke'),
    ('SetStrokeThickness', 'StrokeThickness'),
    ('SetPlacement', 'Placement'),
    ('SetPreferredPlacement', 'PreferredPlacement'),
    ('SetLineSpacing', 'LineSpacing'),
    ('SetTextAlign', 'TextAlign'),
    ('SetIndentWidth', 'IndentWidth'),
    ('SetAllowDrag', 'AllowDrag'),
    ('SetAllowDrop', 'AllowDrop'),
    ('SetCaretIndex', 'CaretIndex'),
    ('SetExpandDirection', 'ExpandDirection'),
    ('SetViewport', 'Viewport'),
    ('SetInputEnabled', 'InputEnabled'),
]

SIMPLE_GET_MAP = [
    ('GetBounds', 'Bounds'),
    ('GetHWND', 'HWND'),
    ('GetRowCount', 'RowCount'),
    ('GetItemCount', 'ItemCount'),
    ('GetSelectedIndex', 'SelectedIndex'),
    ('GetSelectedItem', 'SelectedItem'),
    ('GetSelectedColor', 'SelectedColor'),
    ('GetChildren', 'Children'),
    ('GetItems', 'Items'),
    ('GetText', 'Text'),
    ('GetInputText', 'InputText'),
    ('GetPassword', 'Password'),
    ('GetValue', 'Value'),
    ('GetMinimum', 'Minimum'),
    ('GetMaximum', 'Maximum'),
    ('GetTag', 'Tag'),
    ('GetId', 'Id'),
    ('GetNavigateUri', 'NavigateUri'),
    ('GetBackdropType', 'BackdropType'),
    ('GetThemeMode', 'ThemeMode'),
    ('GetHeader', 'Header'),
    ('GetTitle', 'Title'),
    ('GetMessage', 'Message'),
    ('GetSubtitle', 'Subtitle'),
    ('GetActionText', 'ActionText'),
    ('GetIsOpen', 'IsOpen'),
    ('GetShowGridLines', 'ShowGridLines'),
    ('GetSelectedIndices', 'SelectedIndices'),
    ('GetFormattedDate', 'FormattedDate'),
    ('GetFormattedTime', 'FormattedTime'),
    ('GetMenuBar', 'MenuBar'),
]

def migrate_gallery_file(path):
    with open(path, 'r', encoding='utf-8', errors='ignore') as f:
        content = f.read()

    original = content

    # 1. Expand/Collapse Item
    content = re.sub(r'(\->|\.)SetItemExpanded\(([^,]+),\s*true\)', r'\1ExpandItem(\2)', content)
    content = re.sub(r'(\->|\.)SetItemExpanded\(([^,]+),\s*false\)', r'\1CollapseItem(\2)', content)

    # 2. Simple Set methods
    for method, prop in SIMPLE_SET_MAP:
        content = replace_set_calls(content, method, prop)

    # 3. Simple Get methods
    for method, prop in SIMPLE_GET_MAP:
        content = replace_get_calls(content, method, prop)

    if content != original:
        with open(path, 'w', encoding='utf-8') as f:
            f.write(content)
        print(f"Migrated: {path}")
        return True
    return False

def run_migration():
    count = 0
    gallery_dir = r'E:\C++project\CUI\CUI.Gallery\src'
    for root, dirs, files in os.walk(gallery_dir):
        for f in files:
            if f.endswith('.cpp') or f.endswith('.h'):
                path = os.path.join(root, f)
                if migrate_gallery_file(path):
                    count += 1
    print(f"Total Gallery files migrated: {count}")

run_migration()
