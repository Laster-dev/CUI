#include "UIMarkupParser.h"
#include "StyleManager.h"
#include "../controls/Panel.h"
#include "../controls/TextBlock.h"
#include "../controls/Button.h"
#include "../controls/TextBox.h"
#include "../controls/CheckBox.h"
#include "../controls/HyperlinkButton.h"
#include "../controls/Image.h"
#include "../controls/ComboBox.h"
#include "../controls/ListBox.h"
#include "../controls/ListView.h"
#include "../controls/ScrollViewer.h"
#include "../controls/MessageBox.h"
#include "../controls/TabView.h"
#include "../controls/CollapsePanel.h"
#include "../controls/MenuBar.h"
#include "../controls/VSCodeControls.h"
#include "../controls/PropertyGrid.h"
#include "../controls/Toast.h"
#include "../controls/ToastCenter.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

namespace CUI {

UIMarkupParser::UIMarkupParser() {
    // Register standard controls & WPF panels & WinUI ContentDialog & TabView & MenuBar
    RegisterElementFactory("UIElement", []() { return std::make_shared<UIElement>(); });
    RegisterElementFactory("Panel", []() { return std::make_shared<Panel>(); });
    RegisterElementFactory("StackPanel", []() { return std::make_shared<StackPanel>(); });
    RegisterElementFactory("Canvas", []() { return std::make_shared<Canvas>(); });
    RegisterElementFactory("Grid", []() { return std::make_shared<Grid>(); });
    RegisterElementFactory("WrapPanel", []() { return std::make_shared<WrapPanel>(); });
    RegisterElementFactory("DockPanel", []() { return std::make_shared<DockPanel>(); });
    RegisterElementFactory("UniformGrid", []() { return std::make_shared<UniformGrid>(); });

    RegisterElementFactory("TextBlock", []() { return std::make_shared<TextBlock>(); });
    RegisterElementFactory("Button", []() { return std::make_shared<Button>(); });
    RegisterElementFactory("TextBox", []() { return std::make_shared<TextBox>(); });
    RegisterElementFactory("CheckBox", []() { return std::make_shared<CheckBox>(); });
    RegisterElementFactory("HyperlinkButton", []() { return std::make_shared<HyperlinkButton>(); });
    RegisterElementFactory("Image", []() { return std::make_shared<Image>(); });
    RegisterElementFactory("ComboBox", []() { return std::make_shared<ComboBox>(); });
    RegisterElementFactory("ListBox", []() { return std::make_shared<ListBox>(); });
    RegisterElementFactory("ListView", []() { return std::make_shared<ListView>(); });
    RegisterElementFactory("ScrollViewer", []() { return std::make_shared<ScrollViewer>(); });
    RegisterElementFactory("ContentDialog", []() { return std::make_shared<ContentDialog>(); });
    RegisterElementFactory("TabView", []() { return std::make_shared<TabView>(); });
    RegisterElementFactory("CollapsePanel", []() { return std::make_shared<CollapsePanel>(); });
    RegisterElementFactory("MenuBar", []() { return std::make_shared<MenuBar>(); });
    RegisterElementFactory("PropertyGrid", []() { return std::make_shared<PropertyGrid>(); });
    RegisterElementFactory("Toast", []() { return std::make_shared<Toast>(); });
    RegisterElementFactory("ToastCenter", []() { return std::make_shared<ToastCenter>(); });

    // VS Code Specific Components
    RegisterElementFactory("TitleBar", []() { return std::make_shared<TitleBar>(); });
    RegisterElementFactory("ActivityBar", []() { return std::make_shared<ActivityBar>(); });
    RegisterElementFactory("SideBar", []() { return std::make_shared<SideBar>(); });
    RegisterElementFactory("TabBar", []() { return std::make_shared<TabBar>(); });
    RegisterElementFactory("EditorView", []() { return std::make_shared<EditorView>(); });
    RegisterElementFactory("StatusBar", []() { return std::make_shared<StatusBar>(); });
}

void UIMarkupParser::RegisterElementFactory(const std::string& tagName, std::function<std::shared_ptr<UIElement>()> factory) {
    m_factories[tagName] = factory;
}

std::shared_ptr<UIElement> UIMarkupParser::CreateElement(const std::string& tagName) {
    auto it = m_factories.find(tagName);
    if (it != m_factories.end()) {
        return it->second();
    }
    return std::make_shared<Panel>();
}

struct XmlNode {
    std::string tagName;
    std::unordered_map<std::string, std::string> attributes;
    std::vector<XmlNode> children;
};

static void SkipWhitespace(const std::string& str, size_t& pos) {
    while (pos < str.length() && (str[pos] == ' ' || str[pos] == '\t' || str[pos] == '\r' || str[pos] == '\n')) {
        pos++;
    }
}

static XmlNode ParseXmlNodeInternal(const std::string& xml, size_t& pos) {
    XmlNode node;
    SkipWhitespace(xml, pos);

    if (pos >= xml.length() || xml[pos] != '<') return node;
    pos++;

    if (pos < xml.length() && (xml[pos] == '?' || xml[pos] == '!')) {
        while (pos < xml.length() && xml[pos] != '>') pos++;
        if (pos < xml.length()) pos++;
        return ParseXmlNodeInternal(xml, pos);
    }

    size_t nameStart = pos;
    while (pos < xml.length() && !isspace(xml[pos]) && xml[pos] != '>' && xml[pos] != '/') {
        pos++;
    }
    node.tagName = xml.substr(nameStart, pos - nameStart);

    while (pos < xml.length()) {
        SkipWhitespace(xml, pos);
        if (pos >= xml.length() || xml[pos] == '>' || xml[pos] == '/') break;

        size_t attrStart = pos;
        while (pos < xml.length() && xml[pos] != '=' && !isspace(xml[pos]) && xml[pos] != '>' && xml[pos] != '/') {
            pos++;
        }
        std::string attrName = xml.substr(attrStart, pos - attrStart);

        SkipWhitespace(xml, pos);
        std::string attrVal = "";
        if (pos < xml.length() && xml[pos] == '=') {
            pos++;
            SkipWhitespace(xml, pos);
            if (pos < xml.length() && (xml[pos] == '"' || xml[pos] == '\'')) {
                char quote = xml[pos++];
                size_t valStart = pos;
                while (pos < xml.length() && xml[pos] != quote) {
                    pos++;
                }
                attrVal = xml.substr(valStart, pos - valStart);
                if (pos < xml.length()) pos++;
            }
        }
        if (!attrName.empty()) {
            node.attributes[attrName] = attrVal;
        }
    }

    SkipWhitespace(xml, pos);
    if (pos < xml.length() && xml[pos] == '/') {
        pos++;
        if (pos < xml.length() && xml[pos] == '>') pos++;
        return node;
    }

    if (pos < xml.length() && xml[pos] == '>') {
        pos++;
    }

    std::string closeTag = "</" + node.tagName + ">";
    while (pos < xml.length()) {
        SkipWhitespace(xml, pos);
        if (pos >= xml.length()) break;

        if (xml.compare(pos, closeTag.length(), closeTag) == 0) {
            pos += closeTag.length();
            break;
        }

        if (xml[pos] == '<') {
            if (xml.compare(pos, 2, "</") == 0) {
                while (pos < xml.length() && xml[pos] != '>') pos++;
                if (pos < xml.length()) pos++;
                break;
            }
            XmlNode child = ParseXmlNodeInternal(xml, pos);
            if (!child.tagName.empty()) {
                node.children.push_back(child);
            }
        } else {
            pos++;
        }
    }

    return node;
}

static std::shared_ptr<UIElement> BuildTreeFromXmlNode(UIMarkupParser& parser, const XmlNode& xmlNode, std::vector<DeferredBinding>& deferredBindings) {
    if (xmlNode.tagName.empty()) return nullptr;

    std::shared_ptr<UIElement> elem = parser.CreateElement(xmlNode.tagName);
    if (!elem) return nullptr;

    Grid* gridElem = dynamic_cast<Grid*>(elem.get());

    for (const auto& kv : xmlNode.attributes) {
        const std::string& attrName = kv.first;
        const std::string& attrVal = kv.second;

        if (attrName == "id") {
            elem->SetId(attrVal);
        } else if (attrName == "styleClass") {
            elem->SetStyleClass(attrVal);
        } else if (gridElem && (attrName == "columnDefinitions" || attrName == "ColumnDefinitions")) {
            gridElem->SetColumnDefinitions(attrVal);
        } else if (gridElem && (attrName == "rowDefinitions" || attrName == "RowDefinitions")) {
            gridElem->SetRowDefinitions(attrVal);
        } else {
            if (attrVal.find("{Binding") != std::string::npos) {
                size_t pathPos = attrVal.find("Path=");
                if (pathPos != std::string::npos) {
                    size_t pathStart = pathPos + 5;
                    size_t pathEnd = attrVal.find_first_of("} ", pathStart);
                    if (pathEnd == std::string::npos) pathEnd = attrVal.length();
                    std::string pathStr = attrVal.substr(pathStart, pathEnd - pathStart);

                    deferredBindings.push_back({ elem, attrName, pathStr, BindingMode::OneWay });
                }
            } else {
                elem->SetProperty(attrName, Value::ParseAuto(attrVal));
            }
        }
    }

    StyleManager::Instance().ApplyStyle(elem.get());

    TabView* tabViewElem = dynamic_cast<TabView*>(elem.get());

    for (const auto& childXml : xmlNode.children) {
        if (tabViewElem && childXml.tagName == "TabItem") {
            std::string header = "Tab";
            std::string icon = "";
            std::string srcFile = "";
            bool closable = false;

            auto hIt = childXml.attributes.find("header");
            if (hIt != childXml.attributes.end()) header = hIt->second;

            auto iIt = childXml.attributes.find("icon");
            if (iIt != childXml.attributes.end()) icon = iIt->second;

            auto cIt = childXml.attributes.find("isClosable");
            if (cIt != childXml.attributes.end()) closable = (cIt->second == "true" || cIt->second == "1");

            auto sIt = childXml.attributes.find("src");
            if (sIt != childXml.attributes.end()) srcFile = sIt->second;

            std::shared_ptr<UIElement> tabContent = nullptr;
            if (!srcFile.empty()) {
                tabContent = parser.ParseXmlFile("assets/" + srcFile);
            } else if (!childXml.children.empty()) {
                tabContent = BuildTreeFromXmlNode(parser, childXml.children[0], deferredBindings);
            }

            if (tabContent) {
                tabViewElem->AddTab(header, tabContent, icon, closable);
            }
        } else {
            auto childElem = BuildTreeFromXmlNode(parser, childXml, deferredBindings);
            if (childElem) {
                elem->AddChild(childElem);
            }
        }
    }

    return elem;
}

std::shared_ptr<UIElement> UIMarkupParser::ParseXmlString(const std::string& xmlText) {
    m_deferredBindings.clear();
    size_t pos = 0;
    XmlNode rootNode = ParseXmlNodeInternal(xmlText, pos);
    return BuildTreeFromXmlNode(*this, rootNode, m_deferredBindings);
}

std::shared_ptr<UIElement> UIMarkupParser::ParseXmlFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Failed to open XML layout file: " << filePath << std::endl;
        return nullptr;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return ParseXmlString(buffer.str());
}

void UIMarkupParser::ApplyBindings(std::shared_ptr<Object> dataContext) {
    if (!dataContext) return;

    for (const auto& db : m_deferredBindings) {
        new Binding(db.element, db.propertyName, dataContext, db.bindingPath, db.mode);
    }
}

} // namespace CUI


