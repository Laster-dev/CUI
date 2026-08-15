import os

# 1. Update Pages.h
path_pages_h = r'E:\C++project\CUI\CUI.Gallery\src\pages\BasicInput\Pages.h'
with open(path_pages_h, 'r', encoding='utf-8') as f:
    pages_h = f.read()

if 'BuildTopologyPage()' not in pages_h:
    pages_h = pages_h.replace('CUI::Element BuildTerminalPage();',
                              'CUI::Element BuildTerminalPage();\nCUI::Element BuildTopologyPage();')
    with open(path_pages_h, 'w', encoding='utf-8') as f:
        f.write(pages_h)

# 2. Update CategoryRegistrations.cpp
path_cat = r'E:\C++project\CUI\CUI.Gallery\src\catalog\CategoryRegistrations.cpp'
with open(path_cat, 'r', encoding='utf-8') as f:
    cat = f.read()

if 'BuildTopologyPage' not in cat:
    cat = cat.replace('    entries.push_back({ "terminal", "Terminal", "终端控件。", Category::Media, BuildTerminalPage });',
                      '    entries.push_back({ "terminal", "Terminal", "终端控件。", Category::Media, BuildTerminalPage });\n    entries.push_back({ "topology", "TopologyView", "可折叠带动画拓扑图。", Category::Media, BuildTopologyPage });')
    with open(path_cat, 'w', encoding='utf-8') as f:
        f.write(cat)

# 3. Update CUI.Gallery.vcxproj
path_proj = r'E:\C++project\CUI\CUI.Gallery\CUI.Gallery.vcxproj'
with open(path_proj, 'r', encoding='utf-8') as f:
    proj = f.read()

if 'src\\pages\\Media\\TopologyPage.cpp' not in proj:
    proj = proj.replace('    <ClCompile Include="src\\pages\\Media\\TerminalPage.cpp" />',
                        '    <ClCompile Include="src\\pages\\Media\\TerminalPage.cpp" />\n    <ClCompile Include="src\\pages\\Media\\TopologyPage.cpp" />')
    with open(path_proj, 'w', encoding='utf-8') as f:
        f.write(proj)

# 4. Update TopologyPage.cpp function signature to BuildTopologyPage
path_topo = r'E:\C++project\CUI\CUI.Gallery\src\pages\Media\TopologyPage.cpp'
with open(path_topo, 'r', encoding='utf-8') as f:
    topo = f.read()

if 'std::shared_ptr<UIElement> CreateTopologyPage()' in topo:
    topo = topo.replace('std::shared_ptr<UIElement> CreateTopologyPage()', 'Element BuildTopologyPage()')
    with open(path_topo, 'w', encoding='utf-8') as f:
        f.write(topo)

print("Successfully registered TopologyPage in Gallery.")
