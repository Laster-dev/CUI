#include "framework/window/Window.h"
#include "framework/window/CUIWindow.h"
#include "framework/parser/UIMarkupParser.h"
#include "framework/controls/Button.h"
#include "framework/controls/TextBox.h"
#include "framework/controls/TextBlock.h"
#include "framework/controls/CheckBox.h"
#include "framework/controls/HyperlinkButton.h"
#include "framework/controls/ComboBox.h"
#include "framework/controls/ListBox.h"
#include "framework/controls/Image.h"
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

    // Create & attach ContextMenu to root with Multi-Level Submenus (多级右键菜单)
    auto globalMenu = std::make_shared<ContextMenu>();
    globalMenu->AddItem("Cut", "Ctrl+X", [logEvent]() { logEvent("ContextMenu Action: Cut"); });
    globalMenu->AddItem("Copy", "Ctrl+C", [logEvent]() { logEvent("ContextMenu Action: Copy"); });
    globalMenu->AddItem("Paste", "Ctrl+V", [logEvent]() { logEvent("ContextMenu Action: Paste"); });
    globalMenu->AddSeparator();

    // 1. Submenu Level 2: Encoding
    auto subEncoding = globalMenu->AddSubMenu("Encoding");
    subEncoding->AddItem("UTF-8", [logEvent]() { logEvent("Selected Encoding: UTF-8"); });
    subEncoding->AddItem("UTF-16 LE", [logEvent]() { logEvent("Selected Encoding: UTF-16 LE"); });
    subEncoding->AddItem("GBK / GB2312", [logEvent]() { logEvent("Selected Encoding: GBK"); });

    // 2. Submenu Level 2: Change Theme
    auto subTheme = globalMenu->AddSubMenu("Change Theme");
    subTheme->AddItem("VS Code Dark+ (Default)", [logEvent]() { logEvent("Applied Theme: VS Code Dark+"); });
    subTheme->AddItem("Monokai Dark", [logEvent]() { logEvent("Applied Theme: Monokai Dark"); });

    // 3. Submenu Level 2 & Level 3: Advanced Tools -> Color Accent Palette
    auto subTools = globalMenu->AddSubMenu("Advanced Tools");
    subTools->AddItem("Inspect Element Tree", [logEvent]() { logEvent("Tools Action: Inspect Element Tree"); });

    auto subColor = subTools->AddSubMenu("Color Accent Palette");
    subColor->AddItem("VS Code Blue (#007ACC)", [logEvent]() { logEvent("Accent Color: Blue"); });
    subColor->AddItem("Emerald Green (#10B981)", [logEvent]() { logEvent("Accent Color: Emerald"); });
    subColor->AddItem("Purple Velvet (#8B5CF6)", [logEvent]() { logEvent("Accent Color: Purple"); });

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

                // 1. Add Custom Control Items (CheckBox + Multiple Images + TextBlock + Action Button)
                std::vector<D2D1_COLOR_F> avatarColors = {
                    D2D1::ColorF(0x00 / 255.0f, 0x7A / 255.0f, 0xCC / 255.0f),
                    D2D1::ColorF(0x6B / 255.0f, 0x21 / 255.0f, 0xA8 / 255.0f),
                    D2D1::ColorF(0x05 / 255.0f, 0x96 / 255.0f, 0x69 / 255.0f),
                    D2D1::ColorF(0xD9 / 255.0f, 0x77 / 255.0f, 0x06 / 255.0f),
                    D2D1::ColorF(0xDC / 255.0f, 0x26 / 255.0f, 0x26 / 255.0f)
                };
                std::vector<std::string> initials = { "AG", "VS", "UI", "DX", "C2" };
                std::vector<std::string> fileExts = { "CPP", "PNG", "XML", "DLL", "EXE" };

                for (int i = 1; i <= 5; ++i) {
                    auto itemPanel = std::make_shared<StackPanel>();
                    itemPanel->SetProperty("orientation", Value("Horizontal"));
                    itemPanel->SetProperty("alignVertical", Value("Center"));

                    auto chk = std::make_shared<CheckBox>();
                    chk->SetProperty("margin", Value("4,0,4,0"));
                    chk->OnCheckStateChanged().Connect([logEvent, i](CheckBox*, CheckState state) {
                        logEvent("Custom Row #" + std::to_string(i) + " CheckBox State: " + std::to_string(static_cast<int>(state)));
                    });

                    // Image 1: Circular User Avatar Image
                    auto imgAvatar = std::make_shared<Image>(ImageType::Avatar, initials[i-1], avatarColors[i-1]);
                    imgAvatar->SetProperty("width", Value(20.0f));
                    imgAvatar->SetProperty("height", Value(20.0f));
                    imgAvatar->SetProperty("margin", Value("2,0,4,0"));

                    // Image 2: File Type Badge Image
                    auto imgFile = std::make_shared<Image>(ImageType::FileIcon, fileExts[i-1], D2D1::ColorF(0x33 / 255.0f, 0x33 / 255.0f, 0x33 / 255.0f));
                    imgFile->SetProperty("width", Value(28.0f));
                    imgFile->SetProperty("height", Value(18.0f));
                    imgFile->SetProperty("margin", Value("0,0,6,0"));

                    // Image 3: Online Status Dot Badge Image
                    auto imgStatus = std::make_shared<Image>(ImageType::StatusBadge, "", D2D1::ColorF(0x10 / 255.0f, 0xB9 / 255.0f, 0x81 / 255.0f));
                    imgStatus->SetProperty("width", Value(10.0f));
                    imgStatus->SetProperty("height", Value(10.0f));
                    imgStatus->SetProperty("margin", Value("0,0,6,0"));

                    auto txt = std::make_shared<TextBlock>();
                    txt->SetProperty("text", Value("User_" + initials[i-1]));
                    txt->SetProperty("color", Value(D2D1::ColorF(0xEE / 255.0f, 0xEE / 255.0f, 0xEE / 255.0f)));
                    txt->SetProperty("fontSize", Value(12.0f));

                    auto btn = std::make_shared<Button>();
                    btn->SetProperty("text", Value("Action"));
                    btn->SetProperty("width", Value(45.0f));
                    btn->SetProperty("height", Value(20.0f));
                    btn->SetProperty("fontSize", Value(10.0f));
                    btn->SetProperty("margin", Value("6,0,0,0"));
                    btn->OnClick().Connect([logEvent, i](UIElement*) {
                        logEvent("Custom Row #" + std::to_string(i) + " Action Button Clicked!");
                    });

                    itemPanel->AddChild(chk);
                    itemPanel->AddChild(imgAvatar);
                    itemPanel->AddChild(imgFile);
                    itemPanel->AddChild(imgStatus);
                    itemPanel->AddChild(txt);
                    itemPanel->AddChild(btn);

                    listbox->AddItem(itemPanel);
                }

                // 2. Populate Virtual String Items
                for (int i = 6; i <= 10000; ++i) {
                    listbox->AddItem("Virtual Item #" + std::to_string(i));
                }
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
            if (txtControlTitle) txtControlTitle->SetProperty("text", Value("8. ListView VirtualMode 100K Showcase"));
            if (txtControlDesc) txtControlDesc->SetProperty("text", Value("High-performance ListView with 100,000 rows using VirtualMode + DataSource callback. Only visible rows are rendered (O(visible))."));
            if (txtXmlPreview) txtXmlPreview->SetProperty("text", Value("<ListView width=\"520\" height=\"360\"/>"));
            logEvent("Switched to ListView 100K VirtualMode Showcase.");

            if (previewCanvas) {
                previewCanvas->ClearChildren();

                auto listview = std::make_shared<ListView>();
                listview->SetProperty("width", Value(520.0f));
                listview->SetProperty("height", Value(360.0f));

                listview->AddColumn("#", 50.0f);
                listview->AddColumn("Process Name", 180.0f);
                listview->AddColumn("PID", 80.0f);
                listview->AddColumn("Memory", 90.0f);
                listview->AddColumn("Status", 70.0f);
                listview->AddColumn("CPU %", 60.0f);

                // 100K virtual rows - data fetched on-demand
                struct VirtualData : ListViewDataSource {
                    std::string GetCellText(int row, int col) override {
                        switch (col) {
                        case 0: return std::to_string(row);
                        case 1: return "Process_" + std::to_string(row) + ".exe";
                        case 2: return std::to_string(10000 + row);
                        case 3: return std::to_string((row * 47) % 9999) + " MB";
                        case 4: return (row % 3 == 0) ? "Running" : (row % 3 == 1) ? "Idle" : "Paused";
                        case 5: return std::to_string(row % 100) + "." + std::to_string(row % 10);
                        default: return "";
                        }
                    }
                };

                static VirtualData vs;
                listview->SetVirtualMode(100000, &vs);

                listview->OnSelectionChanged().Connect([logEvent](ListView* lv, int) {
                    logEvent("100K ListView Selection Changed! Selected: " + std::to_string(lv->GetSelectedIndices().size()));
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
