import os

# 1. Update CUI.Core.vcxproj
path_vcxproj = r'E:\C++project\CUI\CUI.Core\CUI.Core.vcxproj'
with open(path_vcxproj, 'r', encoding='utf-8') as f:
    proj = f.read()

if 'ui\\framework\\controls\\topology\\TopologyView.cpp' not in proj:
    proj = proj.replace('    <ClCompile Include="ui\\framework\\controls\\WindowTitleBar.cpp" />',
                        '    <ClCompile Include="ui\\framework\\controls\\WindowTitleBar.cpp" />\n    <ClCompile Include="ui\\framework\\controls\\topology\\TopologyView.cpp" />')

if 'ui\\framework\\controls\\topology\\TopologyView.h' not in proj:
    proj = proj.replace('    <ClInclude Include="ui\\framework\\controls\\WindowTitleBar.h" />',
                        '    <ClInclude Include="ui\\framework\\controls\\WindowTitleBar.h" />\n    <ClInclude Include="ui\\framework\\controls\\topology\\TopologyModel.h" />\n    <ClInclude Include="ui\\framework\\controls\\topology\\TopologyView.h" />')

with open(path_vcxproj, 'w', encoding='utf-8') as f:
    f.write(proj)

# 2. Update CUIDsl.h
path_dsl = r'E:\C++project\CUI\CUI.Core\ui\framework\core\CUIDsl.h'
with open(path_dsl, 'r', encoding='utf-8') as f:
    dsl = f.read()

if '#include "../controls/topology/TopologyView.h"' not in dsl:
    dsl = dsl.replace('#include "../controls/CanvasControl.h"', '#include "../controls/CanvasControl.h"\n#include "../controls/topology/TopologyView.h"')

if 'inline ElementBuilder<TopologyView> TopologyWidget()' not in dsl:
    widget_fn = '''inline ElementBuilder<TopologyView> TopologyWidget() {
    return ElementBuilder<TopologyView>();
}

'''
    dsl = dsl.replace('inline ElementBuilder<Canvas> CanvasWidget()', widget_fn + 'inline ElementBuilder<Canvas> CanvasWidget()')

# Add Nodes and Edges chaining methods to ElementBuilder
if 'ElementBuilder& Nodes(' not in dsl:
    nodes_edges_methods = '''    ElementBuilder& Nodes(const std::vector<std::shared_ptr<TopologyNode>>& nodes) {
        if constexpr (requires { m_ptr->Nodes = nodes; }) {
            m_ptr->Nodes = nodes;
        } else if constexpr (requires { m_ptr->SetNodes(nodes); }) {
            m_ptr->SetNodes(nodes);
        }
        return *this;
    }

    ElementBuilder& Edges(const std::vector<TopologyEdge>& edges) {
        if constexpr (requires { m_ptr->Edges = edges; }) {
            m_ptr->Edges = edges;
        } else if constexpr (requires { m_ptr->SetEdges(edges); }) {
            m_ptr->SetEdges(edges);
        }
        return *this;
    }

    ElementBuilder& LayoutType(TopologyLayoutType t) {
        if constexpr (requires { m_ptr->LayoutType = t; }) {
            m_ptr->LayoutType = t;
        } else if constexpr (requires { m_ptr->SetLayoutType(t); }) {
            m_ptr->SetLayoutType(t);
        }
        return *this;
    }

    ElementBuilder& FlowParticles(bool enabled = true) {
        if constexpr (requires { m_ptr->FlowParticles = enabled; }) {
            m_ptr->FlowParticles = enabled;
        } else if constexpr (requires { m_ptr->SetFlowParticlesEnabled(enabled); }) {
            m_ptr->SetFlowParticlesEnabled(enabled);
        }
        return *this;
    }
'''
    idx = dsl.find('};\n\nstruct ChildArgument')
    if idx != -1:
        dsl = dsl[:idx] + '\n' + nodes_edges_methods + dsl[idx:]

with open(path_dsl, 'w', encoding='utf-8') as f:
    f.write(dsl)

print("Registered TopologyView in CUI.Core.vcxproj and CUIDsl.h.")
