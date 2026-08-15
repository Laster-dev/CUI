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
    'Justified': 'Justified',
    'FillLastLine': 'FillLastLine',
}

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

def extract_balanced_call(lines, start_idx, prefix_pattern):
    """Extract multi-line expression that balances parentheses."""
    first_line = lines[start_idx]
    m = re.match(prefix_pattern, first_line)
    if not m:
        return None, start_idx
    
    prefix = m.group(0)
    consumed = [first_line[len(prefix):]]
    full_text = '\n'.join(consumed)
    
    open_p = full_text.count('(') - full_text.count(')')
    open_b = full_text.count('{') - full_text.count('}')
    
    curr = start_idx
    while (open_p > 0 or open_b > 0 or not full_text.rstrip().endswith(');')) and curr + 1 < len(lines):
        curr += 1
        consumed.append(lines[curr])
        full_text = '\n'.join(consumed)
        open_p = full_text.count('(') - full_text.count(')')
        open_b = full_text.count('{') - full_text.count('}')
    
    if full_text.rstrip().endswith(');'):
        arg_content = full_text.rstrip()[:-2].rstrip()
        return arg_content, curr
    
    return None, start_idx

def transform_code(content):
    lines = content.split('\n')
    new_lines = []
    i = 0
    transformed_count = 0

    while i < len(lines):
        line = lines[i]
        
        decl_match = re.match(r'^(\s*)auto\s+([a-zA-Z0-9_]+)\s*=\s*(.+?)\s*;\s*$', line)
        
        if decl_match:
            indent = decl_match.group(1)
            var_name = decl_match.group(2)
            init_expr = decl_match.group(3)
            factory_match = re.match(r'^([a-zA-Z0-9_]+)', init_expr)
            factory_base = factory_match.group(1) if factory_match else ''
            
            if factory_base in BUILDER_FACTORIES:
                has_build = init_expr.endswith('.Build()')
                if has_build:
                    clean_init_expr = init_expr[:-8]
                else:
                    clean_init_expr = init_expr
                
                j = i + 1
                chain_calls = []
                
                while j < len(lines):
                    next_line = lines[j]
                    
                    assign_match = re.match(r'^\s*' + re.escape(var_name) + r'(?:->|\.)([a-zA-Z0-9_]+)\s*=\s*(.+?);\s*$', next_line)
                    if assign_match:
                        prop_name = assign_match.group(1)
                        val = assign_match.group(2)
                        if re.search(r'\b' + re.escape(var_name) + r'\b', val):
                            break
                        if prop_name in SUPPORTED_PROPS:
                            method_name = SUPPORTED_PROPS[prop_name]
                            chain_calls.append(f'.{method_name}({val})')
                            j += 1
                            continue
                        break
                    
                    prefix_pattern = r'^\s*' + re.escape(var_name) + r'(?:->|\.)OnClick\(\)\.Connect\('
                    arg_content, end_j = extract_balanced_call(lines, j, prefix_pattern)
                    if arg_content is not None:
                        capture_match = re.match(r'^\[(.*?)\]', arg_content.strip())
                        if capture_match and re.search(r'\b' + re.escape(var_name) + r'\b', capture_match.group(1)):
                            break
                        
                        formatted_arg = arg_content.strip()
                        if '\n' in formatted_arg:
                            arg_lines = formatted_arg.split('\n')
                            reindented = [arg_lines[0]]
                            for l in arg_lines[1:]:
                                reindented.append(indent + '        ' + l.strip())
                            formatted_arg = '\n'.join(reindented)
                        
                        chain_calls.append(f'.OnClick({formatted_arg})')
                        j = end_j + 1
                        continue
                    
                    break
                
                if chain_calls:
                    chain_indent = indent + '    '
                    chained_stmt = f'{indent}auto {var_name} = {clean_init_expr}'
                    for call in chain_calls:
                        if '\n' in call:
                            call_lines = call.split('\n')
                            chained_stmt += f'\n{chain_indent}' + call_lines[0]
                            for cl in call_lines[1:]:
                                chained_stmt += f'\n{cl}'
                        else:
                            chained_stmt += f'\n{chain_indent}{call}'
                    
                    chained_stmt += ';'
                    
                    new_lines.append(chained_stmt)
                    i = j
                    transformed_count += 1
                    continue

        new_lines.append(line)
        i += 1

    return '\n'.join(new_lines), transformed_count

def run_conversion():
    gallery_dir = r'E:\C++project\CUI\CUI.Gallery\src'
    total_chains = 0
    total_files = 0
    
    for root, dirs, files in os.walk(gallery_dir):
        for f in files:
            if f.endswith('.cpp'):
                p = os.path.join(root, f)
                with open(p, 'r', encoding='utf-8') as fp:
                    code = fp.read()
                
                new_code, count = transform_code(code)
                if count > 0:
                    with open(p, 'w', encoding='utf-8') as fp:
                        fp.write(new_code)
                    total_chains += count
                    total_files += 1
                    print(f'Transformed {count} fluent chains in {f}')
    
    print(f'\nTotal: Successfully transformed {total_chains} fluent chains across {total_files} files.')

if __name__ == '__main__':
    run_conversion()
