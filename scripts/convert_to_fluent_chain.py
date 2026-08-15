import os
import re

SUPPORTED_PROPS = {
    'Width': 'Width',
    'Height': 'Height',
    'MinWidth': 'MinWidth',
    'MinHeight': 'MinHeight',
    'MaxWidth': 'MaxWidth',
    'MaxHeight': 'MaxHeight',
    'Margin': 'Margin',
    'Padding': 'Padding',
    'Background': 'Background',
    'Foreground': 'Foreground',
    'Color': 'Color',
    'HoverBackground': 'Hover',
    'PressedBackground': 'Pressed',
    'Hover': 'Hover',
    'Pressed': 'Pressed',
    'CornerRadius': 'CornerRadius',
    'Border': 'Border',
    'BorderThickness': 'BorderThickness',
    'BorderBrush': 'BorderBrush',
    'FontSize': 'FontSize',
    'FontFamily': 'FontFamily',
    'FontWeight': 'FontWeight',
    'Text': 'Text',
    'ToolTip': 'ToolTip',
    'Icon': 'Icon',
    'Subtitle': 'Subtitle',
    'Title': 'Title',
    'Message': 'Message',
    'Placeholder': 'Placeholder',
    'Items': 'Items',
    'Value': 'Value',
    'Minimum': 'Minimum',
    'Maximum': 'Maximum',
    'Step': 'Step',
    'IsReadOnly': 'IsReadOnly',
    'IsExpanded': 'IsExpanded',
    'ColumnDefinitions': 'ColumnDefinitions',
    'RowDefinitions': 'RowDefinitions',
    'Enabled': 'Enabled',
    'Visibility': 'Visibility',
    'Gap': 'Gap',
    'Orientation': 'Orientation',
}

# DSL Factories that return ElementBuilder or can chain
BUILDER_FACTORIES = {
    'Button', 'ElevatedButton', 'ToggleButtonWidget', 'DropDownButtonWidget', 'SplitButtonWidget',
    'TextField', 'CheckboxTile', 'Container', 'CanvasWidget', 'GridWidget', 'WrapPanelWidget',
    'DockPanelWidget', 'UniformGridWidget', 'ComboBoxWidget', 'ListBoxWidget', 'ToggleSwitchWidget',
    'TreeViewWidget', 'FlyoutWidget', 'SingleChildScrollView', 'SliderWidget', 'RangeSliderWidget',
    'ProgressBarWidget', 'ProgressRingWidget', 'AutoSuggestBoxWidget', 'RatingWidget', 'StatusBarWidget',
    'TeachingTipWidget', 'Text', 'Column', 'Row', 'LineChartWidget', 'BarChartWidget', 'PieChartWidget',
    'MarkdownViewWidget', 'LogViewWidget', 'InfoBarWidget', 'CommandBarWidget', 'MenuBarWidget',
    'FilePickerWidget', 'FolderPickerWidget', 'PagingControlWidget', 'SplitterWidget', 'ExpanderWidget',
    'Badge', 'Card', 'SectionCard', 'CheckboxTile'
}

def transform_file(filepath):
    with open(filepath, 'r', encoding='utf-8') as f:
        content = f.read()

    lines = content.split('\n')
    new_lines = []
    i = 0
    transformed_count = 0

    while i < len(lines):
        line = lines[i]
        
        # Match declaration like: auto varName = Factory(...);
        # or auto varName = Factory<...>(...);
        decl_match = re.match(r'^(\s*)auto\s+([a-zA-Z0-9_]+)\s*=\s*([a-zA-Z0-9_]+(?:\s*<[^>]+>)?\s*\([^;]*\))\s*;\s*$', line)
        
        if decl_match:
            indent = decl_match.group(1)
            var_name = decl_match.group(2)
            init_expr = decl_match.group(3)
            factory_base = re.match(r'^([a-zA-Z0-9_]+)', init_expr).group(1)
            
            # Check if this factory is a candidate
            if factory_base in BUILDER_FACTORIES or factory_base.endswith('Widget') or factory_base.endswith('Button') or factory_base in {'Button', 'Text', 'Column', 'Row', 'Card'}:
                # Look ahead for assignments to var_name
                j = i + 1
                chain_calls = []
                consumed_lines = 0
                
                while j < len(lines):
                    next_line = lines[j]
                    
                    # Check for property assignment: varName->Prop = value;
                    assign_match = re.match(r'^\s*' + re.escape(var_name) + r'->([a-zA-Z0-9_]+)\s*=\s*(.+?);\s*$', next_line)
                    # Check for OnClick: varName->OnClick().Connect(...);
                    onclick_match = re.match(r'^\s*' + re.escape(var_name) + r'->OnClick\(\)\.Connect\((.+)\);\s*$', next_line)
                    
                    if assign_match:
                        prop_name = assign_match.group(1)
                        val = assign_match.group(2)
                        if prop_name in SUPPORTED_PROPS:
                            method_name = SUPPORTED_PROPS[prop_name]
                            chain_calls.append(f'.{method_name}({val})')
                            j += 1
                            consumed_lines += 1
                            continue
                    elif onclick_match:
                        handler = onclick_match.group(1)
                        # If handler is single line
                        chain_calls.append(f'.OnClick({handler})')
                        j += 1
                        consumed_lines += 1
                        continue
                    
                    # If empty line or comment, or other statement, stop chaining
                    break
                
                if chain_calls:
                    # Construct chained declaration
                    chain_indent = indent + '    '
                    chained_stmt = f'{indent}auto {var_name} = {init_expr}'
                    for call in chain_calls:
                        chained_stmt += f'\n{chain_indent}{call}'
                    chained_stmt += ';'
                    
                    new_lines.append(chained_stmt)
                    i = j
                    transformed_count += 1
                    continue
        
        new_lines.append(line)
        i += 1

    if transformed_count > 0:
        with open(filepath, 'w', encoding='utf-8') as f:
            f.write('\n'.join(new_lines))
        print(f'Transformed {transformed_count} chains in {os.path.basename(filepath)}')

def scan_and_transform():
    gallery_dir = r'E:\C++project\CUI\CUI.Gallery\src'
    for root, dirs, files in os.walk(gallery_dir):
        for f in files:
            if f.endswith('.cpp'):
                transform_file(os.path.join(root, f))

if __name__ == '__main__':
    scan_and_transform()
