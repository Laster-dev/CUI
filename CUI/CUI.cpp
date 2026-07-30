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
#include "framework/controls/Image.h"
#include "framework/controls/Panel.h"
#include "framework/controls/ScrollViewer.h"
#include "framework/controls/MessageBox.h"
#include "framework/controls/TabView.h"
#include "framework/controls/MenuBar.h"
#include "framework/controls/PropertyGrid.h"
#include <iostream>
#include <vector>
#include <memory>
#include <thread>
#include <atomic>

using namespace CUI;

static std::atomic<bool> g_isStreaming{ false };
static std::thread g_streamThread;
static Window* g_activeWindow = nullptr;
static std::shared_ptr<Image> g_streamImage = nullptr;

static void StartStreamingThread() {
    if (g_isStreaming) return;
    if (!g_streamImage || !g_activeWindow) return;

    g_isStreaming = true;
    g_streamThread = std::thread([]() {
        UINT w = 420, h = 240;
        std::vector<uint32_t> pixels(w * h);
        float frameTime = 0.0f;
        while (g_isStreaming) {
            frameTime += 0.05f;
            for (UINT y = 0; y < h; ++y) {
                float fy = static_cast<float>(y) / 24.0f;
                for (UINT x = 0; x < w; ++x) {
                    float fx = static_cast<float>(x) / 24.0f;
                    float v = std::sin(fx + frameTime) + std::sin(fy + frameTime) + std::sin((fx + fy + frameTime) / 2.0f);
                    uint8_t r = static_cast<uint8_t>((std::sin(v * 3.14159f) + 1.0f) * 127.5f);
                    uint8_t g = static_cast<uint8_t>((std::cos(v * 3.14159f) + 1.0f) * 127.5f);
                    uint8_t b = static_cast<uint8_t>(255 - r);
                    pixels[y * w + x] = (255 << 24) | (r << 16) | (g << 8) | b;
                }
            }
            if (g_streamImage) {
                g_streamImage->UpdatePixelBuffer(pixels.data(), w, h);
            }
            if (g_activeWindow && g_activeWindow->GetHWND()) {
                InvalidateRect(g_activeWindow->GetHWND(), nullptr, FALSE);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
    });
}

static void StopStreamingThread() {
    if (g_isStreaming) {
        g_isStreaming = false;
        if (g_streamThread.joinable()) {
            g_streamThread.join();
        }
    }
}

static void NotifyPropertyUpdated(UIElement* element, Window& window) {
    if (window.GetRootElement()) {
        auto root = window.GetRootElement();
        RECT rc;
        GetClientRect(window.GetHWND(), &rc);
        Size avail(static_cast<float>(rc.right - rc.left), static_cast<float>(rc.bottom - rc.top));
        root->Measure(avail);
        root->Arrange(Rect(0, 0, avail.width, avail.height));
    }
    if (window.GetHWND()) {
        InvalidateRect(window.GetHWND(), nullptr, FALSE);
    }
}

void BindParsedXmlInteractions(std::shared_ptr<UIElement> root, Window& window) {
    if (!root) return;

    // Connect Automatic PropertyGrid Inspectors
    auto propGrid = std::dynamic_pointer_cast<PropertyGrid>(root->FindElementById("propGrid"));
    auto btnElem = root->FindElementById("targetBtn");
    if (propGrid && btnElem) {
        propGrid->SetTargetElement(btnElem, &window);
    }

    auto propGridText = std::dynamic_pointer_cast<PropertyGrid>(root->FindElementById("propGridText"));
    auto textElem = root->FindElementById("targetText");
    if (propGridText && textElem) {
        propGridText->SetTargetElement(textElem, &window);
    }

    auto propGridTb = std::dynamic_pointer_cast<PropertyGrid>(root->FindElementById("propGridTb"));
    auto tbElem = root->FindElementById("targetTb");
    if (propGridTb && tbElem) {
        propGridTb->SetTargetElement(tbElem, &window);
    }

    auto txtLogMsg = root->FindElementById("txtLogMsg");
    if (btnElem && txtLogMsg) {
        btnElem->OnClick().Connect([txtLogMsg, &window](UIElement*) {
            txtLogMsg->SetProperty("text", Value("[日志] Button 按钮 OnClick 被成功触发！"));
            NotifyPropertyUpdated(nullptr, window);
        });
    }
    auto targetBtn = btnElem;
    auto inputText = std::dynamic_pointer_cast<TextBox>(root->FindElementById("inputText"));
    auto inputIcon = std::dynamic_pointer_cast<TextBox>(root->FindElementById("inputIcon"));
    auto chkEnabled = std::dynamic_pointer_cast<CheckBox>(root->FindElementById("chkEnabled"));
    auto comboBg = std::dynamic_pointer_cast<ComboBox>(root->FindElementById("comboBg"));
    auto inputFontSize = std::dynamic_pointer_cast<TextBox>(root->FindElementById("inputFontSize"));
    auto inputRadius = std::dynamic_pointer_cast<TextBox>(root->FindElementById("inputRadius"));
    auto inputWidth = std::dynamic_pointer_cast<TextBox>(root->FindElementById("inputWidth"));

    if (targetBtn && txtLogMsg) {
        targetBtn->OnClick().Connect([txtLogMsg, &window](UIElement*) {
            txtLogMsg->SetProperty("text", Value("[日志] Button 按钮 OnClick 被成功触发！"));
            NotifyPropertyUpdated(nullptr, window);
        });
    }
    if (targetBtn && inputText) {
        inputText->OnTextChanged().Connect([targetBtn, &window](TextBox*, const std::string& val) {
            targetBtn->SetProperty("text", Value(val));
            NotifyPropertyUpdated(targetBtn.get(), window);
        });
    }
    if (targetBtn && inputIcon) {
        inputIcon->OnTextChanged().Connect([targetBtn, &window](TextBox*, const std::string& val) {
            targetBtn->SetProperty("icon", Value(val));
            NotifyPropertyUpdated(targetBtn.get(), window);
        });
    }
    if (targetBtn && chkEnabled) {
        chkEnabled->OnCheckStateChanged().Connect([targetBtn, &window](CheckBox*, CheckState st) {
            targetBtn->SetIsEnabled(st == CheckState::Checked);
            NotifyPropertyUpdated(targetBtn.get(), window);
        });
    }
    if (targetBtn && comboBg) {
        comboBg->AddItem("#007ACC (VS 科技蓝)"); comboBg->AddItem("#10B981 (翡翠绿)");
        comboBg->AddItem("#D13438 (宝石红)"); comboBg->AddItem("#8E44AD (高贵紫)");
        comboBg->OnSelectionChanged().Connect([targetBtn, &window](ComboBox*, int idx, const std::string&) {
            if (idx == 0) targetBtn->SetProperty("background", Value("#007ACC"));
            else if (idx == 1) targetBtn->SetProperty("background", Value("#10B981"));
            else if (idx == 2) targetBtn->SetProperty("background", Value("#D13438"));
            else if (idx == 3) targetBtn->SetProperty("background", Value("#8E44AD"));
            NotifyPropertyUpdated(targetBtn.get(), window);
        });
    }
    if (targetBtn && inputFontSize) {
        inputFontSize->OnTextChanged().Connect([targetBtn, &window](TextBox*, const std::string& valStr) {
            float fs = static_cast<float>(atof(valStr.c_str()));
            if (fs > 0.0f) { targetBtn->SetProperty("fontSize", Value(fs)); NotifyPropertyUpdated(targetBtn.get(), window); }
        });
    }
    if (targetBtn && inputRadius) {
        inputRadius->OnTextChanged().Connect([targetBtn, &window](TextBox*, const std::string& valStr) {
            float r = static_cast<float>(atof(valStr.c_str()));
            if (r >= 0.0f) { targetBtn->SetProperty("cornerRadius", Value(r)); NotifyPropertyUpdated(targetBtn.get(), window); }
        });
    }
    auto comboHoverBg = std::dynamic_pointer_cast<ComboBox>(root->FindElementById("comboHoverBg"));
    auto comboPressedBg = std::dynamic_pointer_cast<ComboBox>(root->FindElementById("comboPressedBg"));
    auto inputHeight = std::dynamic_pointer_cast<TextBox>(root->FindElementById("inputHeight"));
    auto inputPadding = std::dynamic_pointer_cast<TextBox>(root->FindElementById("inputPadding"));
    auto inputMargin = std::dynamic_pointer_cast<TextBox>(root->FindElementById("inputMargin"));

    if (targetBtn && comboHoverBg) {
        comboHoverBg->AddItem("#0098FF (亮炫蓝)"); comboHoverBg->AddItem("#34D399 (柔和绿)"); comboHoverBg->AddItem("#F87171 (珊瑚红)");
        comboHoverBg->OnSelectionChanged().Connect([targetBtn, &window](ComboBox*, int idx, const std::string&) {
            if (idx == 0) targetBtn->SetProperty("hoverBackground", Value("#0098FF"));
            else if (idx == 1) targetBtn->SetProperty("hoverBackground", Value("#34D399"));
            else if (idx == 2) targetBtn->SetProperty("hoverBackground", Value("#F87171"));
            NotifyPropertyUpdated(targetBtn.get(), window);
        });
    }
    if (targetBtn && comboPressedBg) {
        comboPressedBg->AddItem("#005A9E (深压蓝)"); comboPressedBg->AddItem("#059669 (暗沉绿)"); comboPressedBg->AddItem("#B91C1C (深沉红)");
        comboPressedBg->OnSelectionChanged().Connect([targetBtn, &window](ComboBox*, int idx, const std::string&) {
            if (idx == 0) targetBtn->SetProperty("pressedBackground", Value("#005A9E"));
            else if (idx == 1) targetBtn->SetProperty("pressedBackground", Value("#059669"));
            else if (idx == 2) targetBtn->SetProperty("pressedBackground", Value("#B91C1C"));
            NotifyPropertyUpdated(targetBtn.get(), window);
        });
    }
    if (targetBtn && inputHeight) {
        inputHeight->OnTextChanged().Connect([targetBtn, &window](TextBox*, const std::string& valStr) {
            float h = static_cast<float>(atof(valStr.c_str()));
            targetBtn->SetProperty("height", Value(h));
            NotifyPropertyUpdated(targetBtn.get(), window);
        });
    }
    if (targetBtn && inputPadding) {
        inputPadding->OnTextChanged().Connect([targetBtn, &window](TextBox*, const std::string& valStr) {
            targetBtn->SetProperty("padding", Value(Thickness::Parse(valStr)));
            NotifyPropertyUpdated(targetBtn.get(), window);
        });
    }
    if (targetBtn && inputMargin) {
        inputMargin->OnTextChanged().Connect([targetBtn, &window](TextBox*, const std::string& valStr) {
            targetBtn->SetProperty("margin", Value(Thickness::Parse(valStr)));
            NotifyPropertyUpdated(targetBtn.get(), window);
        });
    }

    // 2. TextBlock Page
    auto targetText = root->FindElementById("targetText");
    auto inputContent = std::dynamic_pointer_cast<TextBox>(root->FindElementById("inputContent"));
    auto comboFont = std::dynamic_pointer_cast<ComboBox>(root->FindElementById("comboFont"));
    auto inputSize = std::dynamic_pointer_cast<TextBox>(root->FindElementById("inputSize"));
    auto comboWeight = std::dynamic_pointer_cast<ComboBox>(root->FindElementById("comboWeight"));
    auto comboTextColor = std::dynamic_pointer_cast<ComboBox>(root->FindElementById("comboTextColor"));
    auto inputTextWidth = std::dynamic_pointer_cast<TextBox>(root->FindElementById("inputTextWidth"));
    auto inputTextHeight = std::dynamic_pointer_cast<TextBox>(root->FindElementById("inputTextHeight"));
    auto inputTextPadding = std::dynamic_pointer_cast<TextBox>(root->FindElementById("inputTextPadding"));
    auto inputTextMargin = std::dynamic_pointer_cast<TextBox>(root->FindElementById("inputTextMargin"));

    if (targetText && inputContent) {
        inputContent->OnTextChanged().Connect([targetText, &window](TextBox*, const std::string& val) {
            targetText->SetProperty("text", Value(val)); NotifyPropertyUpdated(targetText.get(), window);
        });
    }
    if (targetText && comboFont) {
        comboFont->AddItem("Segoe UI"); comboFont->AddItem("Consolas"); comboFont->AddItem("微软雅黑"); comboFont->AddItem("Times New Roman");
        comboFont->OnSelectionChanged().Connect([targetText, &window](ComboBox*, int, const std::string& fontName) {
            targetText->SetProperty("fontFamily", Value(fontName)); NotifyPropertyUpdated(targetText.get(), window);
        });
    }
    if (targetText && inputSize) {
        inputSize->OnTextChanged().Connect([targetText, &window](TextBox*, const std::string& valStr) {
            float fs = static_cast<float>(atof(valStr.c_str()));
            if (fs > 0.0f) { targetText->SetProperty("fontSize", Value(fs)); NotifyPropertyUpdated(targetText.get(), window); }
        });
    }
    if (targetText && comboWeight) {
        comboWeight->AddItem("Normal (常规)"); comboWeight->AddItem("Bold (加粗)"); comboWeight->AddItem("Light (细体)");
        comboWeight->OnSelectionChanged().Connect([targetText, &window](ComboBox*, int idx, const std::string&) {
            if (idx == 0) targetText->SetProperty("fontWeight", Value("Normal"));
            else if (idx == 1) targetText->SetProperty("fontWeight", Value("Bold"));
            else if (idx == 2) targetText->SetProperty("fontWeight", Value("Light"));
            NotifyPropertyUpdated(targetText.get(), window);
        });
    }
    if (targetText && comboTextColor) {
        comboTextColor->AddItem("#4EC9B0 (青绿色)"); comboTextColor->AddItem("#FFFFFF (纯白色)"); comboTextColor->AddItem("#F59E0B (琥珀黄)"); comboTextColor->AddItem("#EF4444 (鲜红色)");
        comboTextColor->OnSelectionChanged().Connect([targetText, &window](ComboBox*, int idx, const std::string&) {
            if (idx == 0) targetText->SetProperty("color", Value("#4EC9B0"));
            else if (idx == 1) targetText->SetProperty("color", Value("#FFFFFF"));
            else if (idx == 2) targetText->SetProperty("color", Value("#F59E0B"));
            else if (idx == 3) targetText->SetProperty("color", Value("#EF4444"));
            NotifyPropertyUpdated(targetText.get(), window);
        });
    }
    if (targetText && inputTextWidth) {
        inputTextWidth->OnTextChanged().Connect([targetText, &window](TextBox*, const std::string& valStr) {
            float w = static_cast<float>(atof(valStr.c_str()));
            targetText->SetProperty("width", Value(w)); NotifyPropertyUpdated(targetText.get(), window);
        });
    }
    if (targetText && inputTextHeight) {
        inputTextHeight->OnTextChanged().Connect([targetText, &window](TextBox*, const std::string& valStr) {
            float h = static_cast<float>(atof(valStr.c_str()));
            targetText->SetProperty("height", Value(h)); NotifyPropertyUpdated(targetText.get(), window);
        });
    }
    if (targetText && inputTextPadding) {
        inputTextPadding->OnTextChanged().Connect([targetText, &window](TextBox*, const std::string& valStr) {
            targetText->SetProperty("padding", Value(Thickness::Parse(valStr))); NotifyPropertyUpdated(targetText.get(), window);
        });
    }
    if (targetText && inputTextMargin) {
        inputTextMargin->OnTextChanged().Connect([targetText, &window](TextBox*, const std::string& valStr) {
            targetText->SetProperty("margin", Value(Thickness::Parse(valStr))); NotifyPropertyUpdated(targetText.get(), window);
        });
    }

    // 3. TextBox Page
    auto targetTb = root->FindElementById("targetTb");
    auto inputTbText = std::dynamic_pointer_cast<TextBox>(root->FindElementById("inputTbText"));
    auto inputPh = std::dynamic_pointer_cast<TextBox>(root->FindElementById("inputPh"));
    auto chkTbEnabled = std::dynamic_pointer_cast<CheckBox>(root->FindElementById("chkTbEnabled"));
    auto chkWrap = std::dynamic_pointer_cast<CheckBox>(root->FindElementById("chkWrap"));
    auto chkReturn = std::dynamic_pointer_cast<CheckBox>(root->FindElementById("chkReturn"));
    auto inputTbFontSize = std::dynamic_pointer_cast<TextBox>(root->FindElementById("inputTbFontSize"));
    auto inputTbWidth = std::dynamic_pointer_cast<TextBox>(root->FindElementById("inputTbWidth"));
    auto inputTbHeight = std::dynamic_pointer_cast<TextBox>(root->FindElementById("inputTbHeight"));
    auto inputTbRadius = std::dynamic_pointer_cast<TextBox>(root->FindElementById("inputTbRadius"));
    auto inputTbPadding = std::dynamic_pointer_cast<TextBox>(root->FindElementById("inputTbPadding"));
    auto inputTbMargin = std::dynamic_pointer_cast<TextBox>(root->FindElementById("inputTbMargin"));

    if (targetTb && inputTbText) {
        inputTbText->OnTextChanged().Connect([targetTb, &window](TextBox*, const std::string& val) {
            targetTb->SetProperty("text", Value(val)); NotifyPropertyUpdated(targetTb.get(), window);
        });
    }
    if (targetTb && inputPh) {
        inputPh->OnTextChanged().Connect([targetTb, &window](TextBox*, const std::string& ph) {
            targetTb->SetProperty("placeholder", Value(ph)); NotifyPropertyUpdated(targetTb.get(), window);
        });
    }
    if (targetTb && chkTbEnabled) {
        chkTbEnabled->OnCheckStateChanged().Connect([targetTb, &window](CheckBox*, CheckState st) {
            targetTb->SetIsEnabled(st == CheckState::Checked); NotifyPropertyUpdated(targetTb.get(), window);
        });
    }
    if (targetTb && chkWrap) {
        chkWrap->OnCheckStateChanged().Connect([targetTb, &window](CheckBox*, CheckState st) {
            targetTb->SetProperty("TextWrapping", Value(st == CheckState::Checked ? "Wrap" : "NoWrap")); NotifyPropertyUpdated(targetTb.get(), window);
        });
    }
    if (targetTb && chkReturn) {
        chkReturn->OnCheckStateChanged().Connect([targetTb, &window](CheckBox*, CheckState st) {
            targetTb->SetProperty("AcceptsReturn", Value(st == CheckState::Checked)); NotifyPropertyUpdated(targetTb.get(), window);
        });
    }
    if (targetTb && inputTbFontSize) {
        inputTbFontSize->OnTextChanged().Connect([targetTb, &window](TextBox*, const std::string& valStr) {
            float fs = static_cast<float>(atof(valStr.c_str()));
            if (fs > 0.0f) { targetTb->SetProperty("fontSize", Value(fs)); NotifyPropertyUpdated(targetTb.get(), window); }
        });
    }
    if (targetTb && inputTbWidth) {
        inputTbWidth->OnTextChanged().Connect([targetTb, &window](TextBox*, const std::string& valStr) {
            float w = static_cast<float>(atof(valStr.c_str()));
            targetTb->SetProperty("width", Value(w)); NotifyPropertyUpdated(targetTb.get(), window);
        });
    }
    if (targetTb && inputTbHeight) {
        inputTbHeight->OnTextChanged().Connect([targetTb, &window](TextBox*, const std::string& valStr) {
            float h = static_cast<float>(atof(valStr.c_str()));
            targetTb->SetProperty("height", Value(h)); NotifyPropertyUpdated(targetTb.get(), window);
        });
    }
    if (targetTb && inputTbRadius) {
        inputTbRadius->OnTextChanged().Connect([targetTb, &window](TextBox*, const std::string& valStr) {
            float r = static_cast<float>(atof(valStr.c_str()));
            if (r >= 0.0f) { targetTb->SetProperty("cornerRadius", Value(r)); NotifyPropertyUpdated(targetTb.get(), window); }
        });
    }
    if (targetTb && inputTbPadding) {
        inputTbPadding->OnTextChanged().Connect([targetTb, &window](TextBox*, const std::string& valStr) {
            targetTb->SetProperty("padding", Value(Thickness::Parse(valStr))); NotifyPropertyUpdated(targetTb.get(), window);
        });
    }
    if (targetTb && inputTbMargin) {
        inputTbMargin->OnTextChanged().Connect([targetTb, &window](TextBox*, const std::string& valStr) {
            targetTb->SetProperty("margin", Value(Thickness::Parse(valStr))); NotifyPropertyUpdated(targetTb.get(), window);
        });
    }

    // 4. CheckBox Page
    auto targetCb = std::dynamic_pointer_cast<CheckBox>(root->FindElementById("targetCb"));
    auto inputCbLabel = std::dynamic_pointer_cast<TextBox>(root->FindElementById("inputCbLabel"));
    auto chkCbEnabled = std::dynamic_pointer_cast<CheckBox>(root->FindElementById("chkCbEnabled"));
    auto chkThreeState = std::dynamic_pointer_cast<CheckBox>(root->FindElementById("chkThreeState"));
    auto comboCbState = std::dynamic_pointer_cast<ComboBox>(root->FindElementById("comboCbState"));
    auto inputCbFontSize = std::dynamic_pointer_cast<TextBox>(root->FindElementById("inputCbFontSize"));
    auto inputCbWidth = std::dynamic_pointer_cast<TextBox>(root->FindElementById("inputCbWidth"));
    auto inputCbHeight = std::dynamic_pointer_cast<TextBox>(root->FindElementById("inputCbHeight"));
    auto inputCbPadding = std::dynamic_pointer_cast<TextBox>(root->FindElementById("inputCbPadding"));
    auto inputCbMargin = std::dynamic_pointer_cast<TextBox>(root->FindElementById("inputCbMargin"));

    if (targetCb && inputCbLabel) {
        inputCbLabel->OnTextChanged().Connect([targetCb, &window](TextBox*, const std::string& val) {
            targetCb->SetProperty("text", Value(val)); NotifyPropertyUpdated(targetCb.get(), window);
        });
    }
    if (targetCb && chkCbEnabled) {
        chkCbEnabled->OnCheckStateChanged().Connect([targetCb, &window](CheckBox*, CheckState st) {
            targetCb->SetIsEnabled(st == CheckState::Checked); NotifyPropertyUpdated(targetCb.get(), window);
        });
    }
    if (targetCb && chkThreeState) {
        chkThreeState->OnCheckStateChanged().Connect([targetCb, &window](CheckBox*, CheckState st) {
            targetCb->SetIsThreeState(st == CheckState::Checked); NotifyPropertyUpdated(targetCb.get(), window);
        });
    }
    if (targetCb && comboCbState) {
        comboCbState->AddItem("Unchecked (未选中)"); comboCbState->AddItem("Checked (选中 v)"); comboCbState->AddItem("Indeterminate (不定 -)");
        comboCbState->OnSelectionChanged().Connect([targetCb, &window](ComboBox*, int idx, const std::string&) {
            if (idx == 0) targetCb->SetState(CheckState::Unchecked);
            else if (idx == 1) targetCb->SetState(CheckState::Checked);
            else if (idx == 2) targetCb->SetState(CheckState::Indeterminate);
            NotifyPropertyUpdated(targetCb.get(), window);
        });
    }
    if (targetCb && inputCbFontSize) {
        inputCbFontSize->OnTextChanged().Connect([targetCb, &window](TextBox*, const std::string& valStr) {
            float fs = static_cast<float>(atof(valStr.c_str()));
            if (fs > 0.0f) { targetCb->SetProperty("fontSize", Value(fs)); NotifyPropertyUpdated(targetCb.get(), window); }
        });
    }
    if (targetCb && inputCbWidth) {
        inputCbWidth->OnTextChanged().Connect([targetCb, &window](TextBox*, const std::string& valStr) {
            float w = static_cast<float>(atof(valStr.c_str()));
            targetCb->SetProperty("width", Value(w)); NotifyPropertyUpdated(targetCb.get(), window);
        });
    }
    if (targetCb && inputCbHeight) {
        inputCbHeight->OnTextChanged().Connect([targetCb, &window](TextBox*, const std::string& valStr) {
            float h = static_cast<float>(atof(valStr.c_str()));
            targetCb->SetProperty("height", Value(h)); NotifyPropertyUpdated(targetCb.get(), window);
        });
    }
    if (targetCb && inputCbPadding) {
        inputCbPadding->OnTextChanged().Connect([targetCb, &window](TextBox*, const std::string& valStr) {
            targetCb->SetProperty("padding", Value(Thickness::Parse(valStr))); NotifyPropertyUpdated(targetCb.get(), window);
        });
    }
    if (targetCb && inputCbMargin) {
        inputCbMargin->OnTextChanged().Connect([targetCb, &window](TextBox*, const std::string& valStr) {
            targetCb->SetProperty("margin", Value(Thickness::Parse(valStr))); NotifyPropertyUpdated(targetCb.get(), window);
        });
    }

    // 5. ComboBox Page
    auto targetCombo = std::dynamic_pointer_cast<ComboBox>(root->FindElementById("targetCombo"));
    auto chkComboEnabled = std::dynamic_pointer_cast<CheckBox>(root->FindElementById("chkComboEnabled"));
    auto inputNewItem = std::dynamic_pointer_cast<TextBox>(root->FindElementById("inputNewItem"));
    auto btnAddItem = root->FindElementById("btnAddItem");
    auto inputComboWidth = std::dynamic_pointer_cast<TextBox>(root->FindElementById("inputComboWidth"));
    auto inputComboHeight = std::dynamic_pointer_cast<TextBox>(root->FindElementById("inputComboHeight"));
    auto inputComboMargin = std::dynamic_pointer_cast<TextBox>(root->FindElementById("inputComboMargin"));

    if (targetCombo) {
        targetCombo->AddItem("Dark+ (VS Code 默认黑)"); targetCombo->AddItem("Light+ (WinUI 3 亮色)");
        targetCombo->AddItem("Monokai Pro"); targetCombo->AddItem("Solarized Dark");
    }
    if (targetCombo && chkComboEnabled) {
        chkComboEnabled->OnCheckStateChanged().Connect([targetCombo, &window](CheckBox*, CheckState st) {
            targetCombo->SetIsEnabled(st == CheckState::Checked); NotifyPropertyUpdated(targetCombo.get(), window);
        });
    }
    if (targetCombo && inputNewItem && btnAddItem) {
        btnAddItem->OnClick().Connect([targetCombo, inputNewItem, &window](UIElement*) {
            std::string text = inputNewItem->GetText();
            if (!text.empty()) { targetCombo->AddItem(text); NotifyPropertyUpdated(targetCombo.get(), window); }
        });
    }
    if (targetCombo && inputComboWidth) {
        inputComboWidth->OnTextChanged().Connect([targetCombo, &window](TextBox*, const std::string& valStr) {
            float w = static_cast<float>(atof(valStr.c_str()));
            targetCombo->SetProperty("width", Value(w)); NotifyPropertyUpdated(targetCombo.get(), window);
        });
    }
    if (targetCombo && inputComboHeight) {
        inputComboHeight->OnTextChanged().Connect([targetCombo, &window](TextBox*, const std::string& valStr) {
            float h = static_cast<float>(atof(valStr.c_str()));
            targetCombo->SetProperty("height", Value(h)); NotifyPropertyUpdated(targetCombo.get(), window);
        });
    }
    if (targetCombo && inputComboMargin) {
        inputComboMargin->OnTextChanged().Connect([targetCombo, &window](TextBox*, const std::string& valStr) {
            targetCombo->SetProperty("margin", Value(Thickness::Parse(valStr))); NotifyPropertyUpdated(targetCombo.get(), window);
        });
    }

    // 6. ListBox Page (100k Virtual)
    auto targetList = std::dynamic_pointer_cast<ListBox>(root->FindElementById("targetList"));
    auto inputItemH = std::dynamic_pointer_cast<TextBox>(root->FindElementById("inputItemH"));
    auto inputListWidth = std::dynamic_pointer_cast<TextBox>(root->FindElementById("inputListWidth"));
    auto inputListHeight = std::dynamic_pointer_cast<TextBox>(root->FindElementById("inputListHeight"));
    auto inputListMargin = std::dynamic_pointer_cast<TextBox>(root->FindElementById("inputListMargin"));

    if (targetList) {
        targetList->SetVirtualCount(100000);
        targetList->SetSelectedIndex(0);
    }
    if (targetList && inputItemH) {
        inputItemH->OnTextChanged().Connect([targetList, &window](TextBox*, const std::string& valStr) {
            float ih = static_cast<float>(atof(valStr.c_str()));
            if (ih > 10.0f) { targetList->SetItemHeight(ih); NotifyPropertyUpdated(targetList.get(), window); }
        });
    }
    if (targetList && inputListWidth) {
        inputListWidth->OnTextChanged().Connect([targetList, &window](TextBox*, const std::string& valStr) {
            float w = static_cast<float>(atof(valStr.c_str()));
            targetList->SetProperty("width", Value(w)); NotifyPropertyUpdated(targetList.get(), window);
        });
    }
    if (targetList && inputListHeight) {
        inputListHeight->OnTextChanged().Connect([targetList, &window](TextBox*, const std::string& valStr) {
            float h = static_cast<float>(atof(valStr.c_str()));
            targetList->SetProperty("height", Value(h)); NotifyPropertyUpdated(targetList.get(), window);
        });
    }
    if (targetList && inputListMargin) {
        inputListMargin->OnTextChanged().Connect([targetList, &window](TextBox*, const std::string& valStr) {
            targetList->SetProperty("margin", Value(Thickness::Parse(valStr))); NotifyPropertyUpdated(targetList.get(), window);
        });
    }

    // 7. ListView Page (100k Multi-Column)
    auto targetListView = std::dynamic_pointer_cast<ListView>(root->FindElementById("targetListView"));
    auto inputRowH = std::dynamic_pointer_cast<TextBox>(root->FindElementById("inputRowH"));
    auto inputLvWidth = std::dynamic_pointer_cast<TextBox>(root->FindElementById("inputLvWidth"));
    auto inputLvHeight = std::dynamic_pointer_cast<TextBox>(root->FindElementById("inputLvHeight"));
    auto inputLvMargin = std::dynamic_pointer_cast<TextBox>(root->FindElementById("inputLvMargin"));

    if (targetListView) {
        targetListView->AddColumn("#", 50.0f); targetListView->AddColumn("进程名称", 160.0f);
        targetListView->AddColumn("PID", 70.0f); targetListView->AddColumn("内存占用", 80.0f); targetListView->AddColumn("状态", 70.0f);

        struct VirtualData : ListViewDataSource {
            std::string GetCellText(int row, int col) override {
                switch (col) {
                case 0: return std::to_string(row);
                case 1: return "Process_" + std::to_string(row) + ".exe";
                case 2: return std::to_string(10000 + row);
                case 3: return std::to_string((row * 47) % 9999) + " MB";
                case 4: return (row % 2 == 0) ? "运行中" : "空闲";
                default: return "";
                }
            }
        };
        static VirtualData vs;
        targetListView->SetVirtualMode(100000, &vs);
    }
    if (targetListView && inputRowH) {
        inputRowH->OnTextChanged().Connect([targetListView, &window](TextBox*, const std::string& valStr) {
            float rh = static_cast<float>(atof(valStr.c_str()));
            if (rh > 10.0f) { targetListView->SetRowHeight(rh); NotifyPropertyUpdated(targetListView.get(), window); }
        });
    }
    if (targetListView && inputLvWidth) {
        inputLvWidth->OnTextChanged().Connect([targetListView, &window](TextBox*, const std::string& valStr) {
            float w = static_cast<float>(atof(valStr.c_str()));
            targetListView->SetProperty("width", Value(w)); NotifyPropertyUpdated(targetListView.get(), window);
        });
    }
    if (targetListView && inputLvHeight) {
        inputLvHeight->OnTextChanged().Connect([targetListView, &window](TextBox*, const std::string& valStr) {
            float h = static_cast<float>(atof(valStr.c_str()));
            targetListView->SetProperty("height", Value(h)); NotifyPropertyUpdated(targetListView.get(), window);
        });
    }
    if (targetListView && inputLvMargin) {
        inputLvMargin->OnTextChanged().Connect([targetListView, &window](TextBox*, const std::string& valStr) {
            targetListView->SetProperty("margin", Value(Thickness::Parse(valStr))); NotifyPropertyUpdated(targetListView.get(), window);
        });
    }

    // 8. Hyperlink Page
    auto targetLink = std::dynamic_pointer_cast<HyperlinkButton>(root->FindElementById("targetLink"));
    auto inputUrl = std::dynamic_pointer_cast<TextBox>(root->FindElementById("inputUrl"));
    auto inputLinkText = std::dynamic_pointer_cast<TextBox>(root->FindElementById("inputLinkText"));
    auto chkLinkEnabled = std::dynamic_pointer_cast<CheckBox>(root->FindElementById("chkLinkEnabled"));
    auto inputLinkFontSize = std::dynamic_pointer_cast<TextBox>(root->FindElementById("inputLinkFontSize"));
    auto inputLinkWidth = std::dynamic_pointer_cast<TextBox>(root->FindElementById("inputLinkWidth"));
    auto inputLinkHeight = std::dynamic_pointer_cast<TextBox>(root->FindElementById("inputLinkHeight"));
    auto inputLinkMargin = std::dynamic_pointer_cast<TextBox>(root->FindElementById("inputLinkMargin"));

    if (targetLink && inputUrl) {
        inputUrl->OnTextChanged().Connect([targetLink, &window](TextBox*, const std::string& url) {
            targetLink->SetNavigateUri(url); NotifyPropertyUpdated(targetLink.get(), window);
        });
    }
    if (targetLink && inputLinkText) {
        inputLinkText->OnTextChanged().Connect([targetLink, &window](TextBox*, const std::string& val) {
            targetLink->SetProperty("text", Value(val)); NotifyPropertyUpdated(targetLink.get(), window);
        });
    }
    if (targetLink && chkLinkEnabled) {
        chkLinkEnabled->OnCheckStateChanged().Connect([targetLink, &window](CheckBox*, CheckState st) {
            targetLink->SetIsEnabled(st == CheckState::Checked); NotifyPropertyUpdated(targetLink.get(), window);
        });
    }
    if (targetLink && inputLinkFontSize) {
        inputLinkFontSize->OnTextChanged().Connect([targetLink, &window](TextBox*, const std::string& valStr) {
            float fs = static_cast<float>(atof(valStr.c_str()));
            if (fs > 0.0f) { targetLink->SetProperty("fontSize", Value(fs)); NotifyPropertyUpdated(targetLink.get(), window); }
        });
    }
    if (targetLink && inputLinkWidth) {
        inputLinkWidth->OnTextChanged().Connect([targetLink, &window](TextBox*, const std::string& valStr) {
            float w = static_cast<float>(atof(valStr.c_str()));
            targetLink->SetProperty("width", Value(w)); NotifyPropertyUpdated(targetLink.get(), window);
        });
    }
    if (targetLink && inputLinkHeight) {
        inputLinkHeight->OnTextChanged().Connect([targetLink, &window](TextBox*, const std::string& valStr) {
            float h = static_cast<float>(atof(valStr.c_str()));
            targetLink->SetProperty("height", Value(h)); NotifyPropertyUpdated(targetLink.get(), window);
        });
    }
    if (targetLink && inputLinkMargin) {
        inputLinkMargin->OnTextChanged().Connect([targetLink, &window](TextBox*, const std::string& valStr) {
            targetLink->SetProperty("margin", Value(Thickness::Parse(valStr))); NotifyPropertyUpdated(targetLink.get(), window);
        });
    }

    // 9. Dynamic Stream Image Page
    g_streamImage = std::dynamic_pointer_cast<Image>(root->FindElementById("dynImageStream"));
    if (g_streamImage) {
        g_streamImage->InitDynamicBitmap(window.GetGraphicsContext().GetD2DContext(), 420, 240);
    }
    auto btnToggleStream = root->FindElementById("btnToggleStream");
    auto inputStreamWidth = std::dynamic_pointer_cast<TextBox>(root->FindElementById("inputStreamWidth"));
    auto inputStreamHeight = std::dynamic_pointer_cast<TextBox>(root->FindElementById("inputStreamHeight"));

    if (btnToggleStream) {
        btnToggleStream->OnClick().Connect([btnToggleStream, &window](UIElement*) {
            if (g_isStreaming) {
                StopStreamingThread(); btnToggleStream->SetProperty("background", Value("#10B981")); btnToggleStream->SetProperty("text", Value("恢复/启动推流"));
            } else {
                StartStreamingThread(); btnToggleStream->SetProperty("background", Value("#D13438")); btnToggleStream->SetProperty("text", Value("暂停推流播放"));
            }
            NotifyPropertyUpdated(btnToggleStream.get(), window);
        });
    }
    if (g_streamImage && inputStreamWidth) {
        inputStreamWidth->OnTextChanged().Connect([&window](TextBox*, const std::string& valStr) {
            float w = static_cast<float>(atof(valStr.c_str()));
            if (w > 50.0f && g_streamImage) { g_streamImage->SetProperty("width", Value(w)); NotifyPropertyUpdated(g_streamImage.get(), window); }
        });
    }
    if (g_streamImage && inputStreamHeight) {
        inputStreamHeight->OnTextChanged().Connect([&window](TextBox*, const std::string& valStr) {
            float h = static_cast<float>(atof(valStr.c_str()));
            if (h > 50.0f && g_streamImage) { g_streamImage->SetProperty("height", Value(h)); NotifyPropertyUpdated(g_streamImage.get(), window); }
        });
    }

    // 10. ContentDialog Page
    auto btnTriggerDialog = root->FindElementById("btnTriggerDialog");
    if (btnTriggerDialog) {
        btnTriggerDialog->OnClick().Connect([root](UIElement*) {
            ContentDialog::ShowMessageBox(root.get(), "未保存修改警告", "您在文档中有未保存的修改，退出前是否保存？");
        });
    }

    // 11. Grid Page
    auto targetGrid = std::dynamic_pointer_cast<Grid>(root->FindElementById("targetGrid"));
    auto inputCols = std::dynamic_pointer_cast<TextBox>(root->FindElementById("inputCols"));
    auto inputRows = std::dynamic_pointer_cast<TextBox>(root->FindElementById("inputRows"));

    if (targetGrid && inputCols) {
        inputCols->OnTextChanged().Connect([targetGrid, &window](TextBox*, const std::string& val) {
            targetGrid->SetColumnDefinitions(val); NotifyPropertyUpdated(targetGrid.get(), window);
        });
    }
    if (targetGrid && inputRows) {
        inputRows->OnTextChanged().Connect([targetGrid, &window](TextBox*, const std::string& val) {
            targetGrid->SetRowDefinitions(val); NotifyPropertyUpdated(targetGrid.get(), window);
        });
    }

    // 12. Canvas Page
    auto targetCanvasBtn = root->FindElementById("targetCanvasBtn");
    auto inputCanvasLeft = std::dynamic_pointer_cast<TextBox>(root->FindElementById("inputCanvasLeft"));
    auto inputCanvasTop = std::dynamic_pointer_cast<TextBox>(root->FindElementById("inputCanvasTop"));

    if (targetCanvasBtn && inputCanvasLeft) {
        inputCanvasLeft->OnTextChanged().Connect([targetCanvasBtn, &window](TextBox*, const std::string& valStr) {
            float l = static_cast<float>(atof(valStr.c_str()));
            targetCanvasBtn->SetProperty("Canvas.Left", Value(l)); NotifyPropertyUpdated(targetCanvasBtn.get(), window);
        });
    }
    if (targetCanvasBtn && inputCanvasTop) {
        inputCanvasTop->OnTextChanged().Connect([targetCanvasBtn, &window](TextBox*, const std::string& valStr) {
            float t = static_cast<float>(atof(valStr.c_str()));
            targetCanvasBtn->SetProperty("Canvas.Top", Value(t)); NotifyPropertyUpdated(targetCanvasBtn.get(), window);
        });
    }

    // 13. WrapPanel Page
    auto targetWrap = root->FindElementById("targetWrap");
    auto comboWrapOrient = std::dynamic_pointer_cast<ComboBox>(root->FindElementById("comboWrapOrient"));

    if (targetWrap && comboWrapOrient) {
        comboWrapOrient->AddItem("Horizontal (水平换行)"); comboWrapOrient->AddItem("Vertical (垂直换列)");
        comboWrapOrient->OnSelectionChanged().Connect([targetWrap, &window](ComboBox*, int idx, const std::string&) {
            targetWrap->SetProperty("orientation", Value(idx == 0 ? "Horizontal" : "Vertical"));
            NotifyPropertyUpdated(targetWrap.get(), window);
        });
    }

    // 14. DockPanel Page
    auto targetDock = root->FindElementById("targetDock");
    auto chkLastChildFill = std::dynamic_pointer_cast<CheckBox>(root->FindElementById("chkLastChildFill"));

    if (targetDock && chkLastChildFill) {
        chkLastChildFill->OnCheckStateChanged().Connect([targetDock, &window](CheckBox*, CheckState st) {
            targetDock->SetProperty("lastChildFill", Value(st == CheckState::Checked));
            NotifyPropertyUpdated(targetDock.get(), window);
        });
    }

    // 15. UniformGrid Page
    auto targetUg = root->FindElementById("targetUg");
    auto inputUgRows = std::dynamic_pointer_cast<TextBox>(root->FindElementById("inputUgRows"));
    auto inputUgCols = std::dynamic_pointer_cast<TextBox>(root->FindElementById("inputUgCols"));

    if (targetUg && inputUgRows) {
        inputUgRows->OnTextChanged().Connect([targetUg, &window](TextBox*, const std::string& valStr) {
            int r = atoi(valStr.c_str()); if (r >= 0) { targetUg->SetProperty("rows", Value(r)); NotifyPropertyUpdated(targetUg.get(), window); }
        });
    }
    if (targetUg && inputUgCols) {
        inputUgCols->OnTextChanged().Connect([targetUg, &window](TextBox*, const std::string& valStr) {
            int c = atoi(valStr.c_str()); if (c >= 0) { targetUg->SetProperty("columns", Value(c)); NotifyPropertyUpdated(targetUg.get(), window); }
        });
    }

    // 16. StackPanel Page
    auto targetSp = root->FindElementById("targetSp");
    auto comboOrient = std::dynamic_pointer_cast<ComboBox>(root->FindElementById("comboOrient"));
    auto inputGap = std::dynamic_pointer_cast<TextBox>(root->FindElementById("inputGap"));

    if (targetSp && comboOrient) {
        comboOrient->AddItem("Horizontal (水平)"); comboOrient->AddItem("Vertical (垂直)");
        comboOrient->OnSelectionChanged().Connect([targetSp, &window](ComboBox*, int idx, const std::string&) {
            targetSp->SetProperty("orientation", Value(idx == 0 ? "Horizontal" : "Vertical"));
            NotifyPropertyUpdated(targetSp.get(), window);
        });
    }
    if (targetSp && inputGap) {
        inputGap->OnTextChanged().Connect([targetSp, &window](TextBox*, const std::string& valStr) {
            float g = static_cast<float>(atof(valStr.c_str()));
            if (g >= 0.0f) { targetSp->SetProperty("gap", Value(g)); NotifyPropertyUpdated(targetSp.get(), window); }
        });
    }

    // 17. ScrollViewer Page
    auto targetSv = std::dynamic_pointer_cast<ScrollViewer>(root->FindElementById("targetSv"));
    auto inputSvOffset = std::dynamic_pointer_cast<TextBox>(root->FindElementById("inputSvOffset"));

    if (targetSv && inputSvOffset) {
        inputSvOffset->OnTextChanged().Connect([targetSv, &window](TextBox*, const std::string& valStr) {
            float off = static_cast<float>(atof(valStr.c_str()));
            if (off >= 0.0f) { targetSv->SetScrollOffsetY(off); NotifyPropertyUpdated(targetSv.get(), window); }
        });
    }

    // TabView Selection Event: Auto management for stream thread
    auto tabView = std::dynamic_pointer_cast<TabView>(root->FindElementById("mainTabView"));
    if (tabView) {
        tabView->OnSelectionChanged().Connect([&window](TabView*, int idx) {
            if (idx == 8) {
                StartStreamingThread();
            } else {
                StopStreamingThread();
            }
            if (window.GetHWND()) {
                InvalidateRect(window.GetHWND(), nullptr, FALSE);
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
    std::cout << "     CUI Control Gallery - Complete 17 Controls Showcase\n";
    std::cout << "========================================================\n";

    UIMarkupParser parser;
    std::string layoutPath = "assets/gallery_layout.xml";

    auto rootElement = parser.ParseXmlFile(layoutPath);
    if (!rootElement) {
        std::cerr << "Failed to load layout from " << layoutPath << std::endl;
        CoUninitialize();
        return -1;
    }

    Window window;
    if (!window.Create("CUI Control Gallery - 17 大控件嵌入式 XML 控制台", 1280, 780, true)) {
        std::cerr << "Failed to create Direct2D host window." << std::endl;
        CoUninitialize();
        return -1;
    }

    g_activeWindow = &window;
    BindParsedXmlInteractions(rootElement, window);

    window.SetRootElement(rootElement);
    window.Show();

    window.RunMessageLoop();

    StopStreamingThread();
    CoUninitialize();
    return 0;
}
