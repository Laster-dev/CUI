import os

path_h = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\AutoSuggestBox.h'
with open(path_h, 'r', encoding='utf-8') as f:
    content = f.read()

content = content.replace(
    '    using SuggestionProvider = std::function<std::vector<std::string>(const std::string& query)>;',
    '    using SuggestionProviderFn = std::function<std::vector<std::string>(const std::string& query)>;'
)
content = content.replace(
    'AutoSuggestProviderProperty& operator=(SuggestionProvider provider)',
    'AutoSuggestProviderProperty& operator=(SuggestionProviderFn provider)'
)
content = content.replace(
    'void SetSuggestionProvider(SuggestionProvider provider);',
    'void SetSuggestionProvider(SuggestionProviderFn provider);'
)

with open(path_h, 'w', encoding='utf-8') as f:
    f.write(content)

path_cpp = r'E:\C++project\CUI\CUI.Core\ui\framework\controls\AutoSuggestBox.cpp'
with open(path_cpp, 'r', encoding='utf-8') as f:
    cpp = f.read()

cpp = cpp.replace(
    'void AutoSuggestBox::SetSuggestionProvider(SuggestionProvider provider)',
    'void AutoSuggestBox::SetSuggestionProvider(SuggestionProviderFn provider)'
)

with open(path_cpp, 'w', encoding='utf-8') as f:
    f.write(cpp)

print("Updated AutoSuggestBox.h and AutoSuggestBox.cpp")
