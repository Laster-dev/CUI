#include "framework/window/Window.h"
#include "framework/parser/UIMarkupParser.h"
#include "framework/controls/Button.h"
#include "framework/controls/TextBox.h"
#include "framework/controls/TextBlock.h"
#include "framework/controls/CheckBox.h"
#include "framework/controls/HyperlinkButton.h"
#include "framework/controls/ComboBox.h"
#include "framework/controls/ListBox.h"
#include "framework/controls/ListView.h"
#include "framework/controls/ContextMenu.h"
#include "framework/controls/Panel.h"
#include "framework/controls/VSCodeControls.h"
#include "framework/core/Binding.h"
#include <iostream>
#include <objbase.h>

using namespace CUI;

void SetupGalleryInteractions(std::shared_ptr<UIElement> root, Window& window) {
    if (!root) return;

    auto txtControlTitle = root->FindElementById("txtControlTitle");
    auto txtControlDesc = root->FindElementById("txtControlDesc");
    auto txtXmlPreview = root->FindElementById("txtXmlPreview");
    auto txtEventLog = root->FindElementById("txtEventLog");
    auto previewCanvas = root->FindElementById("previewCanvas");

    auto btnNavButton = root->FindElementById("btnNavButton");
    auto btnNavTextBlock = root->FindElementById("btnNavTextBlock");
    auto btnNavTextBox = root->FindElementById("btnNavTextBox");
    auto btnNavCheckBox = root->FindElementById("btnNavCheckBox");
    auto btnNavHyperlink = root->FindElementById("btnNavHyperlink");
    auto btnNavComboBox = root->FindElementById("btnNavComboBox");
    auto btnNavListBox = root->FindElementById("btnNavListBox");
    auto btnNavListView = root->FindElementById("btnNavListView");
    auto btnNavPanel = root->FindElementById("btnNavPanel");
    auto btnNavScroll = root->FindElementById("btnNavScroll");
    auto btnNavVSCode = root->FindElementById("btnNavVSCode");

    std::vector<std::shared_ptr<UIElement>> navButtons = { btnNavButton, btnNavTextBlock, btnNavTextBox, btnNavCheckBox, btnNavHyperlink, btnNavComboBox, btnNavListBox, btnNavListView, btnNavPanel, btnNavScroll, btnNavVSCode };

    auto logEvent = [txtEventLog, &window](const std::string& msg) {
        if (txtEventLog) {
            txtEventLog->SetProperty("text", Value("[LOG] " + msg));
            InvalidateRect(window.GetHWND(), nullptr, FALSE);
        }
    };

    // Create & attach ContextMenu to root
    auto globalMenu = std::make_shared<ContextMenu>();
    globalMenu->AddItem("Cut", "Ctrl+X", [logEvent]() { logEvent("ContextMenu Action: Cut"); });
    globalMenu->AddItem("Copy", "Ctrl+C", [logEvent]() { logEvent("ContextMenu Action: Copy"); });
    globalMenu->AddItem("Paste", "Ctrl+V", [logEvent]() { logEvent("ContextMenu Action: Paste"); });
    globalMenu->AddSeparator();
    globalMenu->AddItem("Select All", "Ctrl+A", [logEvent]() { logEvent("ContextMenu Action: Select All"); });
    globalMenu->AddSeparator();
    globalMenu->AddItem("Refresh Gallery UI", "F5", [logEvent]() { logEvent("ContextMenu Action: Refresh UI"); });

    root->SetContextMenu(globalMenu);

    auto updateActiveNav = [navButtons, &window](std::shared_ptr<UIElement> activeBtn) {
        for (auto btn : navButtons) {
            if (btn) {
                if (btn == activeBtn) {
                    btn->SetProperty("background", Value("#007ACC"));
                    btn->SetProperty("color", Value("#FFFFFF"));
                } else {
                    btn->SetProperty("background", Value("#2D2D2D"));
                    btn->SetProperty("color", Value("#CCCCCC"));
                }
            }
        }
        InvalidateRect(window.GetHWND(), nullptr, FALSE);
    };

    auto chkOptIsEnabled = std::dynamic_pointer_cast<CheckBox>(root->FindElementById("chkOptIsEnabled"));
    auto txtOptText = std::dynamic_pointer_cast<TextBox>(root->FindElementById("txtOptText"));
    auto txtOptRadius = std::dynamic_pointer_cast<TextBox>(root->FindElementById("txtOptRadius"));
    auto comboOptTheme = std::dynamic_pointer_cast<ComboBox>(root->FindElementById("comboOptTheme"));

    if (comboOptTheme) {
        comboOptTheme->AddItem("Default Blue (#007ACC)");
        comboOptTheme->AddItem("Success Green (#2DA042)");
        comboOptTheme->AddItem("Warning Orange (#E36629)");
        comboOptTheme->AddItem("Danger Red (#D13438)");
    }

    auto applyOptionsToPreview = [=, &window]() {
        if (!previewCanvas) return;
        bool isEnabled = chkOptIsEnabled ? (chkOptIsEnabled->GetState() == CheckState::Checked) : true;
        std::string txt = txtOptText ? txtOptText->GetText() : "";
        float radius = txtOptRadius ? static_cast<float>(atof(txtOptRadius->GetText().c_str())) : 4.0f;

        std::string themeColor = "#007ACC";
        if (comboOptTheme) {
            int sel = comboOptTheme->GetSelectedIndex();
            if (sel == 1) themeColor = "#2DA042";
            else if (sel == 2) themeColor = "#E36629";
            else if (sel == 3) themeColor = "#D13438";
        }

        for (const auto& child : previewCanvas->GetChildren()) {
            if (child) {
                child->SetIsEnabled(isEnabled);
                if (!txt.empty() && std::string(child->GetClassName()) != "ComboBox") {
                    child->SetProperty("text", Value(txt));
                }
                child->SetProperty("cornerRadius", Value(radius));
                child->SetProperty("background", Value(themeColor));
            }
        }

        RECT rc;
        GetClientRect(window.GetHWND(), &rc);
        root->Measure(Size(static_cast<float>(rc.right), static_cast<float>(rc.bottom)));
        root->Arrange(Rect(0, 0, static_cast<float>(rc.right), static_cast<float>(rc.bottom)));
        InvalidateRect(window.GetHWND(), nullptr, FALSE);
    };

    if (chkOptIsEnabled) {
        chkOptIsEnabled->OnCheckStateChanged().Connect([=](CheckBox*, CheckState) {
            applyOptionsToPreview();
        });
    }
    if (txtOptText) {
        txtOptText->OnPropertyChanged().Connect([=](const std::string& prop, const Value&) {
            if (prop == "text") applyOptionsToPreview();
        });
    }
    if (txtOptRadius) {
        txtOptRadius->OnPropertyChanged().Connect([=](const std::string& prop, const Value&) {
            if (prop == "text") applyOptionsToPreview();
        });
    }
    if (comboOptTheme) {
        comboOptTheme->OnSelectionChanged().Connect([=](ComboBox*, int, const std::string&) {
            applyOptionsToPreview();
        });
    }

    // Helper to bind demo button clicks
    auto bindDemoButtons = [logEvent](std::shared_ptr<UIElement> parent) {
        if (!parent) return;
        for (const auto& child : parent->GetChildren()) {
            child->OnClick().Connect([logEvent, child](UIElement*) {
                std::string btnText = child->GetProperty("text").AsString("Button");
                std::string btnId = child->GetId();
                logEvent("Button '" + btnText + "' (" + btnId + ") clicked!");
            });
        }
    };

    // Bind initial demo buttons in gallery_layout.xml
    bindDemoButtons(previewCanvas);

    // Real-time XML Live Reloading Listener
    if (txtXmlPreview) {
        txtXmlPreview->OnPropertyChanged().Connect([=, &window](const std::string& propName, const Value& val) {
            if (propName == "text" && previewCanvas) {
                std::string xmlCode = val.AsString();
                if (!xmlCode.empty()) {
                    UIMarkupParser liveParser;
                    auto parsedElem = liveParser.ParseXmlString(xmlCode);
                    if (parsedElem) {
                        previewCanvas->ClearChildren();
                        previewCanvas->AddChild(parsedElem);
                        bindDemoButtons(previewCanvas);

                        RECT rc;
                        GetClientRect(window.GetHWND(), &rc);
                        root->Measure(Size(static_cast<float>(rc.right), static_cast<float>(rc.bottom)));
                        root->Arrange(Rect(0, 0, static_cast<float>(rc.right), static_cast<float>(rc.bottom)));
                        logEvent("XML Live Reload OK: Control updated in real-time!");
                    }
                }
            }
        });
    }

    // 1. Switch to [1] Button Gallery Page
    if (btnNavButton) {
        btnNavButton->OnClick().Connect([=, &window](UIElement*) {
            updateActiveNav(btnNavButton);
            if (txtControlTitle) txtControlTitle->SetProperty("text", Value("1. Button Control Showcase"));
            if (txtControlDesc) txtControlDesc->SetProperty("text", Value("Interactive buttons with normal, hover, pressed states and event callbacks."));
            if (txtXmlPreview) txtXmlPreview->SetProperty("text", Value("<Button text=\"Primary Button\" background=\"#007ACC\" hoverBackground=\"#0098FF\"/>"));
            logEvent("Switched to Button Gallery Showcase.");

            if (previewCanvas) {
                previewCanvas->ClearChildren();

                auto b1 = std::make_shared<Button>("Primary Button");
                b1->SetId("btnPrimary");
                b1->SetProperty("background", Value("#007ACC"));
                b1->SetProperty("padding", Value(Thickness(12, 6, 12, 6)));

                auto b2 = std::make_shared<Button>("Secondary Button");
                b2->SetId("btnSecondary");
                b2->SetProperty("background", Value("#3C3C3C"));
                b2->SetProperty("padding", Value(Thickness(12, 6, 12, 6)));

                auto b3 = std::make_shared<Button>("Icon Button");
                b3->SetId("btnIcon");
                b3->SetProperty("icon", Value("[+]"));
                b3->SetProperty("background", Value("#0E639C"));
                b3->SetProperty("padding", Value(Thickness(12, 6, 12, 6)));

                previewCanvas->AddChild(b1);
                previewCanvas->AddChild(b2);
                previewCanvas->AddChild(b3);

                bindDemoButtons(previewCanvas);

                // Trigger layout measure & arrange
                RECT rc;
                GetClientRect(window.GetHWND(), &rc);
                root->Measure(Size(static_cast<float>(rc.right), static_cast<float>(rc.bottom)));
                root->Arrange(Rect(0, 0, static_cast<float>(rc.right), static_cast<float>(rc.bottom)));
            }
        });
    }

    // 2. Switch to [2] TextBlock Gallery Page
    if (btnNavTextBlock) {
        btnNavTextBlock->OnClick().Connect([=, &window](UIElement*) {
            updateActiveNav(btnNavTextBlock);
            if (txtControlTitle) txtControlTitle->SetProperty("text", Value("2. TextBlock Control Showcase"));
            if (txtControlDesc) txtControlDesc->SetProperty("text", Value("DirectWrite high-performance text rendering supporting fonts, sizes, colors, and weights."));
            if (txtXmlPreview) txtXmlPreview->SetProperty("text", Value("<TextBlock text=\"Header Text\" fontSize=\"18\" fontWeight=\"Bold\" color=\"#4EC9B0\"/>"));
            logEvent("Switched to TextBlock Gallery Showcase.");

            if (previewCanvas) {
                previewCanvas->ClearChildren();

                auto t1 = std::make_shared<TextBlock>("Title Header (18pt Bold)");
                t1->SetProperty("fontSize", Value(18.0f));
                t1->SetProperty("fontWeight", Value("Bold"));
                t1->SetProperty("color", Value("#4EC9B0"));

                auto t2 = std::make_shared<TextBlock>("Normal Text (13pt Segoe UI)");
                t2->SetProperty("fontSize", Value(13.0f));
                t2->SetProperty("color", Value("#CCCCCC"));

                auto t3 = std::make_shared<TextBlock>("Highlight Accent (12pt Consolas)");
                t3->SetProperty("fontSize", Value(12.0f));
                t3->SetProperty("fontFamily", Value("Consolas"));
                t3->SetProperty("color", Value("#CE9178"));

                previewCanvas->AddChild(t1);
                previewCanvas->AddChild(t2);
                previewCanvas->AddChild(t3);

                RECT rc;
                GetClientRect(window.GetHWND(), &rc);
                root->Measure(Size(static_cast<float>(rc.right), static_cast<float>(rc.bottom)));
                root->Arrange(Rect(0, 0, static_cast<float>(rc.right), static_cast<float>(rc.bottom)));
            }
        });
    }

    // 3. Switch to [3] TextBox Gallery Page
    if (btnNavTextBox) {
        btnNavTextBox->OnClick().Connect([=, &window](UIElement*) {
            updateActiveNav(btnNavTextBox);
            if (txtControlTitle) txtControlTitle->SetProperty("text", Value("3. TextBox Input Showcase"));
            if (txtControlDesc) txtControlDesc->SetProperty("text", Value("Text input control supporting placeholder text, focus state, and interactive typing."));
            if (txtXmlPreview) txtXmlPreview->SetProperty("text", Value("<TextBox placeholder=\"Click and type text...\" background=\"#3C3C3C\" color=\"#FFFFFF\"/>"));
            logEvent("Switched to TextBox Input Showcase.");

            if (previewCanvas) {
                previewCanvas->ClearChildren();

                auto tb1 = std::make_shared<TextBox>("Click & type text here...");
                tb1->SetId("tbDemo");
                tb1->SetProperty("width", Value(300.0f));
                tb1->SetProperty("height", Value(34.0f));

                previewCanvas->AddChild(tb1);

                RECT rc;
                GetClientRect(window.GetHWND(), &rc);
                root->Measure(Size(static_cast<float>(rc.right), static_cast<float>(rc.bottom)));
                root->Arrange(Rect(0, 0, static_cast<float>(rc.right), static_cast<float>(rc.bottom)));
            }
        });
    }

    // 4. Switch to [4] CheckBox Gallery Page
    if (btnNavCheckBox) {
        btnNavCheckBox->OnClick().Connect([=, &window](UIElement*) {
            updateActiveNav(btnNavCheckBox);
            if (txtControlTitle) txtControlTitle->SetProperty("text", Value("4. CheckBox Control Showcase"));
            if (txtControlDesc) txtControlDesc->SetProperty("text", Value("Supports 3 states: Unchecked, Checked (✓), and Indeterminate (-) with tri-state toggling."));
            if (txtXmlPreview) txtXmlPreview->SetProperty("text", Value("<CheckBox text=\"Enable Feature\" checkState=\"Checked\" isThreeState=\"true\"/>"));
            logEvent("Switched to CheckBox Showcase.");

            if (previewCanvas) {
                previewCanvas->ClearChildren();

                auto cb1 = std::make_shared<CheckBox>("Standard Unchecked");
                cb1->SetState(CheckState::Unchecked);

                auto cb2 = std::make_shared<CheckBox>("Checked Feature");
                cb2->SetState(CheckState::Checked);

                auto cb3 = std::make_shared<CheckBox>("Tri-state (Indeterminate)");
                cb3->SetIsThreeState(true);
                cb3->SetState(CheckState::Indeterminate);

                auto logState = [logEvent](CheckBox* cb, CheckState st) {
                    std::string stStr = "Unchecked";
                    if (st == CheckState::Checked) stStr = "Checked (v)";
                    else if (st == CheckState::Indeterminate) stStr = "Indeterminate (-)";
                    logEvent("CheckBox '" + cb->GetProperty("text").AsString() + "' state -> " + stStr);
                };

                cb1->OnCheckStateChanged().Connect(logState);
                cb2->OnCheckStateChanged().Connect(logState);
                cb3->OnCheckStateChanged().Connect(logState);

                previewCanvas->AddChild(cb1);
                previewCanvas->AddChild(cb2);
                previewCanvas->AddChild(cb3);

                RECT rc;
                GetClientRect(window.GetHWND(), &rc);
                root->Measure(Size(static_cast<float>(rc.right), static_cast<float>(rc.bottom)));
                root->Arrange(Rect(0, 0, static_cast<float>(rc.right), static_cast<float>(rc.bottom)));
            }
        });
    }

    // 5. Switch to [5] HyperlinkButton Gallery Page
    if (btnNavHyperlink) {
        btnNavHyperlink->OnClick().Connect([=, &window](UIElement*) {
            updateActiveNav(btnNavHyperlink);
            if (txtControlTitle) txtControlTitle->SetProperty("text", Value("5. HyperlinkButton Showcase"));
            if (txtControlDesc) txtControlDesc->SetProperty("text", Value("Interactive hyperlink button with hand pointer cursor and hover underline."));
            if (txtXmlPreview) txtXmlPreview->SetProperty("text", Value("<HyperlinkButton text=\"Visit CUI Documentation\" navigateUri=\"https://github.com\"/>"));
            logEvent("Switched to HyperlinkButton Showcase.");

            if (previewCanvas) {
                previewCanvas->ClearChildren();

                auto hl1 = std::make_shared<HyperlinkButton>("Visit CUI Framework Documentation", "https://github.com");
                hl1->OnClick().Connect([logEvent](UIElement*) {
                    logEvent("HyperlinkClicked: Opening https://github.com ...");
                });

                previewCanvas->AddChild(hl1);

                RECT rc;
                GetClientRect(window.GetHWND(), &rc);
                root->Measure(Size(static_cast<float>(rc.right), static_cast<float>(rc.bottom)));
                root->Arrange(Rect(0, 0, static_cast<float>(rc.right), static_cast<float>(rc.bottom)));
            }
        });
    }

    // 6. Switch to [6] ComboBox Gallery Page
    if (btnNavComboBox) {
        btnNavComboBox->OnClick().Connect([=, &window](UIElement*) {
            updateActiveNav(btnNavComboBox);
            if (txtControlTitle) txtControlTitle->SetProperty("text", Value("6. ComboBox DropDown Showcase"));
            if (txtControlDesc) txtControlDesc->SetProperty("text", Value("DropDown menu selection box with item selection callbacks and custom popup rendering."));
            if (txtXmlPreview) txtXmlPreview->SetProperty("text", Value("<ComboBox items=\"Dark+ (default),Light+ (VS Code),Monokai Dark,Solarized Dark\" width=\"220\"/>"));
            logEvent("Switched to ComboBox Showcase.");

            if (previewCanvas) {
                previewCanvas->ClearChildren();

                auto combo = std::make_shared<ComboBox>();
                combo->AddItem("Dark+ (default)");
                combo->AddItem("Light+ (VS Code)");
                combo->AddItem("Monokai Dark");
                combo->AddItem("Solarized Dark");

                combo->OnSelectionChanged().Connect([logEvent](ComboBox*, int idx, const std::string& item) {
                    logEvent("ComboBox Selected: [" + std::to_string(idx) + "] " + item);
                });

                previewCanvas->AddChild(combo);

                RECT rc;
                GetClientRect(window.GetHWND(), &rc);
                root->Measure(Size(static_cast<float>(rc.right), static_cast<float>(rc.bottom)));
                root->Arrange(Rect(0, 0, static_cast<float>(rc.right), static_cast<float>(rc.bottom)));
            }
        });
    }

    // 7. Switch to [7] ListBox Gallery Page
    if (btnNavListBox) {
        btnNavListBox->OnClick().Connect([=, &window](UIElement*) {
            updateActiveNav(btnNavListBox);
            if (txtControlTitle) txtControlTitle->SetProperty("text", Value("7. ListBox High-Performance Showcase (100,000 Virtual Items)"));
            if (txtControlDesc) txtControlDesc->SetProperty("text", Value("High-performance virtualized ListBox rendering 100,000 items at 60 FPS smooth scrolling, vector scrollbar & arrow navigation."));
            if (txtXmlPreview) txtXmlPreview->SetProperty("text", Value("<ListBox items=\"Item 1,Item 2,Item 3,Item 4,Item 5\" width=\"280\" height=\"240\"/>"));
            logEvent("Switched to ListBox Showcase.");

            if (previewCanvas) {
                previewCanvas->ClearChildren();

                auto listbox = std::make_shared<ListBox>();
                listbox->SetProperty("width", Value(280.0f));
                listbox->SetProperty("height", Value(240.0f));

                // Populate 100,000 Virtual Items to demonstrate ultra performance!
                std::vector<std::string> virtualItems;
                virtualItems.reserve(100000);
                for (int i = 1; i <= 100000; ++i) {
                    virtualItems.push_back("Virtual Item #" + std::to_string(i));
                }
                listbox->SetItems(virtualItems);
                listbox->SetSelectedIndex(0);

                listbox->OnSelectionChanged().Connect([logEvent](ListBox*, int idx, const std::string& item) {
                    logEvent("ListBox Selected: [" + std::to_string(idx) + "] " + item);
                });

                listbox->OnItemDoubleClicked().Connect([logEvent](ListBox*, int idx, const std::string& item) {
                    logEvent("ListBox Double-Clicked: [" + std::to_string(idx) + "] " + item);
                });

                previewCanvas->AddChild(listbox);

                RECT rc;
                GetClientRect(window.GetHWND(), &rc);
                root->Measure(Size(static_cast<float>(rc.right), static_cast<float>(rc.bottom)));
                root->Arrange(Rect(0, 0, static_cast<float>(rc.right), static_cast<float>(rc.bottom)));
            }
        });
    }

    // 8. Switch to [8] ListView Gallery Page
    if (btnNavListView) {
        btnNavListView->OnClick().Connect([=, &window](UIElement*) {
            updateActiveNav(btnNavListView);
            if (txtControlTitle) txtControlTitle->SetProperty("text", Value("8. ListView Multi-Column & Rubber-Band Showcase"));
            if (txtControlDesc) txtControlDesc->SetProperty("text", Value("High-performance ListView with multi-column headers, live column width drag resizing, rubber-band marquee drag selection & Ctrl/Shift multi-select."));
            if (txtXmlPreview) txtXmlPreview->SetProperty("text", Value("<ListView width=\"480\" height=\"280\"/>"));
            logEvent("Switched to ListView Showcase.");

            if (previewCanvas) {
                previewCanvas->ClearChildren();

                auto listview = std::make_shared<ListView>();
                listview->SetProperty("width", Value(480.0f));
                listview->SetProperty("height", Value(280.0f));

                listview->AddColumn("ID", 60.0f);
                listview->AddColumn("Process Name", 160.0f);
                listview->AddColumn("Status", 100.0f);
                listview->AddColumn("Memory Usage", 120.0f);

                std::vector<std::vector<std::string>> sampleRows = {
                    { "1001", "CUI_Renderer.exe", "Running", "45.2 MB" },
                    { "1002", "VSCode_Service.exe", "Active", "128.5 MB" },
                    { "1003", "Direct2D_Engine.dll", "Loaded", "12.4 MB" },
                    { "1004", "DirectWrite_Font.dll", "Loaded", "8.1 MB" },
                    { "1005", "Antigravity_Agent.exe", "Running", "89.6 MB" },
                    { "1006", "MSBuild_Worker.exe", "Idle", "34.0 MB" },
                    { "1007", "Windows_Kernel.sys", "System", "512.0 MB" },
                    { "1008", "UI_Markup_Parser.dll", "Active", "15.8 MB" },
                    { "1009", "ContextMenu_Host.exe", "Running", "22.3 MB" },
                    { "1010", "Virtualizing_Grid.dll", "Active", "64.1 MB" }
                };
                listview->SetRows(sampleRows);

                listview->OnSelectionChanged().Connect([logEvent](ListView* lv, int) {
                    logEvent("ListView Selection Changed! Total Selected: " + std::to_string(lv->GetSelectedIndices().size()));
                });

                previewCanvas->AddChild(listview);

                RECT rc;
                GetClientRect(window.GetHWND(), &rc);
                root->Measure(Size(static_cast<float>(rc.right), static_cast<float>(rc.bottom)));
                root->Arrange(Rect(0, 0, static_cast<float>(rc.right), static_cast<float>(rc.bottom)));
            }
        });
    }

    // 7. Switch to StackPanel Gallery Page
    if (btnNavPanel) {
        btnNavPanel->OnClick().Connect([=, &window](UIElement*) {
            updateActiveNav(btnNavPanel);
            if (txtControlTitle) txtControlTitle->SetProperty("text", Value("4. StackPanel Flex Layout Showcase"));
            if (txtControlDesc) txtControlDesc->SetProperty("text", Value("Two-pass Flexbox layout engine supporting Horizontal/Vertical orientations and Gap spacing."));
            if (txtXmlPreview) txtXmlPreview->SetProperty("text", Value("<StackPanel orientation=\"Horizontal\" gap=\"8\">\n  <Button text=\"Box 1\"/>\n  <Button text=\"Box 2\"/>\n</StackPanel>"));
            logEvent("Switched to StackPanel Layout Showcase.");

            if (previewCanvas) {
                previewCanvas->ClearChildren();

                auto p1 = std::make_shared<Button>("Flex Box 1 (grow: 1)");
                p1->SetProperty("flexGrow", Value(1.0f));
                p1->SetProperty("background", Value("#0E639C"));
                p1->SetProperty("padding", Value(Thickness(8, 8, 8, 8)));

                auto p2 = std::make_shared<Button>("Flex Box 2 (grow: 2)");
                p2->SetProperty("flexGrow", Value(2.0f));
                p2->SetProperty("background", Value("#1177BB"));
                p2->SetProperty("padding", Value(Thickness(8, 8, 8, 8)));

                previewCanvas->AddChild(p1);
                previewCanvas->AddChild(p2);

                bindDemoButtons(previewCanvas);

                RECT rc;
                GetClientRect(window.GetHWND(), &rc);
                root->Measure(Size(static_cast<float>(rc.right), static_cast<float>(rc.bottom)));
                root->Arrange(Rect(0, 0, static_cast<float>(rc.right), static_cast<float>(rc.bottom)));
            }
        });
    }

    // 5. Switch to [5] ScrollViewer Gallery Page
    if (btnNavScroll) {
        btnNavScroll->OnClick().Connect([=, &window](UIElement*) {
            updateActiveNav(btnNavScroll);
            if (txtControlTitle) txtControlTitle->SetProperty("text", Value("5. ScrollViewer Container Showcase"));
            if (txtControlDesc) txtControlDesc->SetProperty("text", Value("Scrollable view container with customizable scrollbar indicator and view clipping."));
            if (txtXmlPreview) txtXmlPreview->SetProperty("text", Value("<ScrollViewer>\n  <StackPanel orientation=\"Vertical\">...</StackPanel>\n</ScrollViewer>"));
            logEvent("Switched to ScrollViewer Showcase.");

            if (previewCanvas) {
                previewCanvas->ClearChildren();

                auto sb1 = std::make_shared<Button>("Item 1 - Scrollable List");
                sb1->SetProperty("background", Value("#2D2D2D"));
                sb1->SetProperty("padding", Value(Thickness(8, 4, 8, 4)));

                auto sb2 = std::make_shared<Button>("Item 2 - Scrollable List");
                sb2->SetProperty("background", Value("#333333"));
                sb2->SetProperty("padding", Value(Thickness(8, 4, 8, 4)));

                previewCanvas->AddChild(sb1);
                previewCanvas->AddChild(sb2);

                bindDemoButtons(previewCanvas);

                RECT rc;
                GetClientRect(window.GetHWND(), &rc);
                root->Measure(Size(static_cast<float>(rc.right), static_cast<float>(rc.bottom)));
                root->Arrange(Rect(0, 0, static_cast<float>(rc.right), static_cast<float>(rc.bottom)));
            }
        });
    }

    // 6. Switch to [6] VS Code Full IDE Gallery Page
    if (btnNavVSCode) {
        btnNavVSCode->OnClick().Connect([=, &window](UIElement*) {
            updateActiveNav(btnNavVSCode);
            if (txtControlTitle) txtControlTitle->SetProperty("text", Value("6. VS Code Composite IDE Showcase"));
            if (txtControlDesc) txtControlDesc->SetProperty("text", Value("Complete Visual Studio Code Dark+ theme layout constructed entirely via XML descriptor."));
            if (txtXmlPreview) txtXmlPreview->SetProperty("text", Value("<TitleBar/> <ActivityBar/> <SideBar/> <TabBar/> <EditorView/> <StatusBar/>"));
            logEvent("Switched to VS Code Composite IDE Showcase.");

            if (previewCanvas) {
                previewCanvas->ClearChildren();

                auto actBar = std::make_shared<ActivityBar>();
                auto editor = std::make_shared<EditorView>();
                editor->SetProperty("flexGrow", Value(1.0f));

                previewCanvas->AddChild(actBar);
                previewCanvas->AddChild(editor);

                RECT rc;
                GetClientRect(window.GetHWND(), &rc);
                root->Measure(Size(static_cast<float>(rc.right), static_cast<float>(rc.bottom)));
                root->Arrange(Rect(0, 0, static_cast<float>(rc.right), static_cast<float>(rc.bottom)));
            }
        });
    }
}

int main() {
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) {
        std::cerr << "Failed to initialize COM library." << std::endl;
        return -1;
    }

    std::cout << "========================================================\n";
    std::cout << "        CUI Control Gallery Showcase App v1.0          \n";
    std::cout << "========================================================\n";
    std::cout << "[Step 1] Initializing Control Gallery...\n";

    auto dataContext = std::make_shared<Object>();
    dataContext->SetProperty("AppTitle", Value("CUI Control Gallery"));

    UIMarkupParser parser;
    std::string layoutPath = "assets/gallery_layout.xml";
    std::cout << "[Step 2] Loading Gallery Layout XML: " << layoutPath << " ...\n";

    auto rootElement = parser.ParseXmlFile(layoutPath);
    if (!rootElement) {
        std::cerr << "Failed to load layout from " << layoutPath << std::endl;
        CoUninitialize();
        return -1;
    }

    parser.ApplyBindings(dataContext);

    std::cout << "[Step 3] UI Tree Created. Initializing Direct2D Host Window...\n";

    Window window;
    if (!window.Create("CUI Control Gallery - Step-by-Step Interactive Showcase", 1280, 780)) {
        std::cerr << "Failed to create Direct2D host window." << std::endl;
        CoUninitialize();
        return -1;
    }

    // Setup interactive gallery callbacks
    SetupGalleryInteractions(rootElement, window);

    window.SetRootElement(rootElement);
    window.Show();

    std::cout << "[Step 4] Control Gallery Window Active. Ready for interaction!\n";

    window.RunMessageLoop();

    CoUninitialize();
    return 0;
}
