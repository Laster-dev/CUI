#include "CalculatorApp.h"

#include "framework/core/CUIDsl.h"
#include "framework/controls/Button.h"
#include "framework/controls/Panel.h"
#include "framework/controls/WindowTitleBar.h"
#include "framework/style/ThemeTokenId.h"
#include "framework/window/WindowBackdrop.h"

#include <cmath>
#include <iomanip>
#include <limits>
#include <sstream>

using namespace CUI;
using namespace CUI::DSL;

namespace Calc {

namespace {

struct ButtonSpec {
    const char* text;
    ThemeTokenId backgroundToken;
    ThemeTokenId textToken;
    int row;
    int column;
    int columnSpan;
};

} // namespace

int CalculatorApp::Run() {
    if (!m_window.Create("Calc", 380, 600, false)) {
        return -1;
    }

    m_window.SetThemeMode(ThemeMode::Dark);
    m_window.SetBackdropType(BackdropType::None);
    m_window.SetRenderStatsOverlayVisible(false);
    m_window.SetRootElement(BuildRoot());
    m_window.Show();
    m_window.RunMessageLoop();
    return 0;
}

std::shared_ptr<UIElement> CalculatorApp::BuildRoot() {
    auto root = Column(0)
        .BackgroundToken(ThemeTokenId::WindowBackground)
        .Build();
    root->SetColorToken(ThemeTokenId::TextPrimary);

    auto titleBar = std::make_shared<WindowTitleBar>();
    titleBar->SetTitle("Calc");
    titleBar->SetIconText("C");

    auto fileMenu = titleBar->GetMenuBar().AddMenu("File");
    fileMenu->AddItem("Clear", [this]() { ClearAll(); });
    fileMenu->AddSeparator();
    fileMenu->AddItem("Exit", [this]() {
       /* if (HWND hwnd = m_window.GetHWND()) {
            PostMessage(hwnd, WM_CLOSE, 0, 0);
        }*/
        ContentDialog::ShowMessageBox(m_window.GetRootElement().get(), "WinUI ContentDialog", "全盘 100% 纯 C++ 声明式 UI 完整回填生成。");
    });

    auto editMenu = titleBar->GetMenuBar().AddMenu("Edit");
    editMenu->AddItem("Toggle Sign", [this]() { ToggleSign(); });
    editMenu->AddItem("Percent", [this]() { ApplyPercent(); });

    auto helpMenu = titleBar->GetMenuBar().AddMenu("Help");
    helpMenu->AddItem("About Calc");

    auto content = Column(16)
        .Padding(18)
        .Build();
    content->SetFlexGrow(1.0f);

    auto title = Text("Calculator")
        .FontSize(28.0f)
        .ColorToken(ThemeTokenId::TextPrimary)
        .Build();

    auto subtitle = Text("Built with the CUI framework")
        .FontSize(12.0f)
        .ColorToken(ThemeTokenId::TextMuted)
        .Build();

    content->AddChild(title);
    content->AddChild(subtitle);
    content->AddChild(BuildDisplayPanel());

    auto grid = GridWidget().Build();
    grid->SetColumnDefinitions("1*,1*,1*,1*");
    grid->SetRowDefinitions("1*,1*,1*,1*,1*");
    grid->SetFlexGrow(1.0f);
    grid->SetGap(10.0f);
    BuildButtons(*grid);

    auto gridHost = Expanded(grid).Build();
    content->AddChild(gridHost);

    root->AddChild(titleBar);
    root->AddChild(content);

    UpdateDisplay();
    return root;
}

std::shared_ptr<UIElement> CalculatorApp::BuildDisplayPanel() {
    auto card = Container()
        .BackgroundToken(ThemeTokenId::CardBackground)
        .BorderToken(ThemeTokenId::CardBorder, 1.0f)
        .CornerRadius(18.0f)
        .Padding(18)
        .Build();

    auto column = Column(8).Build();
    column->SetMinHeight(130.0f);

    m_historyText = Text("")
        .FontSize(14.0f)
        .ColorToken(ThemeTokenId::TextMuted)
        .Build();
    m_historyText->SetTextAlign("Right");
    m_historyText->SetHeight(24.0f);

    m_displayText = Text("0")
        .FontSize(40.0f)
        .ColorToken(ThemeTokenId::TextPrimary)
        .Build();
    m_displayText->SetTextAlign("Right");
    m_displayText->SetVerticalAlign("Center");
    m_displayText->SetMinHeight(72.0f);

    column->AddChild(m_historyText);
    column->AddChild(m_displayText);
    card->AddChild(column);
    return card;
}

std::shared_ptr<Button> CalculatorApp::CreateButton(const std::string& text,
                                                    ThemeTokenId backgroundToken,
                                                    ThemeTokenId textToken,
                                                    float minHeight) {
    auto button = ElevatedButton(text).Build();
    button->SetBackgroundToken(backgroundToken);
    button->SetColorToken(textToken);
    button->SetBorderToken(ThemeTokenId::CardBorder);
    button->SetBorderThickness(1.0f);
    button->SetCornerRadius(16.0f);
    button->SetFontSize(20.0f);
    button->SetMinHeight(minHeight);
    return button;
}

void CalculatorApp::BuildButtons(Grid& grid) {
    const ButtonSpec specs[] = {
        { "C", ThemeTokenId::DangerColor, ThemeTokenId::AccentForeground, 0, 0, 1 },
        { "+/-", ThemeTokenId::PaneBackground, ThemeTokenId::TextPrimary, 0, 1, 1 },
        { "%", ThemeTokenId::PaneBackground, ThemeTokenId::TextPrimary, 0, 2, 1 },
        { "/", ThemeTokenId::AccentColor, ThemeTokenId::AccentForeground, 0, 3, 1 },
        { "7", ThemeTokenId::CardBackground, ThemeTokenId::TextPrimary, 1, 0, 1 },
        { "8", ThemeTokenId::CardBackground, ThemeTokenId::TextPrimary, 1, 1, 1 },
        { "9", ThemeTokenId::CardBackground, ThemeTokenId::TextPrimary, 1, 2, 1 },
        { "*", ThemeTokenId::AccentColor, ThemeTokenId::AccentForeground, 1, 3, 1 },
        { "4", ThemeTokenId::CardBackground, ThemeTokenId::TextPrimary, 2, 0, 1 },
        { "5", ThemeTokenId::CardBackground, ThemeTokenId::TextPrimary, 2, 1, 1 },
        { "6", ThemeTokenId::CardBackground, ThemeTokenId::TextPrimary, 2, 2, 1 },
        { "-", ThemeTokenId::AccentColor, ThemeTokenId::AccentForeground, 2, 3, 1 },
        { "1", ThemeTokenId::CardBackground, ThemeTokenId::TextPrimary, 3, 0, 1 },
        { "2", ThemeTokenId::CardBackground, ThemeTokenId::TextPrimary, 3, 1, 1 },
        { "3", ThemeTokenId::CardBackground, ThemeTokenId::TextPrimary, 3, 2, 1 },
        { "+", ThemeTokenId::AccentColor, ThemeTokenId::AccentForeground, 3, 3, 1 },
        { "0", ThemeTokenId::InputBackground, ThemeTokenId::TextPrimary, 4, 0, 2 },
        { ".", ThemeTokenId::InputBackground, ThemeTokenId::TextPrimary, 4, 2, 1 },
        { "=", ThemeTokenId::AccentColor, ThemeTokenId::AccentForeground, 4, 3, 1 },
    };

    for (const ButtonSpec& spec : specs) {
        auto button = CreateButton(spec.text, spec.backgroundToken, spec.textToken, 64.0f);
        button->SetGridRow(spec.row);
        button->SetGridColumn(spec.column);
        button->SetGridColumnSpan(spec.columnSpan);

        const std::string label = spec.text;
        button->OnClick().Connect([this, label](UIElement*) {
            if (label == "C") {
                ClearAll();
            } else if (label == "+/-") {
                ToggleSign();
            } else if (label == "%") {
                ApplyPercent();
            } else if (label == ".") {
                AppendDecimalPoint();
            } else if (label == "=") {
                Evaluate();
            } else if (label == "+" || label == "-" || label == "*" || label == "/") {
                ApplyOperator(label);
            } else {
                AppendDigit(label);
            }
        });

        grid.AddChild(button);
    }
}

void CalculatorApp::AppendDigit(const std::string& digit) {
    if (m_hasError) {
        ClearAll();
    }

    if (m_startNewEntry) {
        m_input = digit;
        m_startNewEntry = false;
    } else if (m_input == "0") {
        m_input = digit;
    } else {
        m_input += digit;
    }

    UpdateDisplay();
}

void CalculatorApp::AppendDecimalPoint() {
    if (m_hasError) {
        ClearAll();
    }

    if (m_startNewEntry) {
        m_input = "0.";
        m_startNewEntry = false;
    } else if (m_input.find('.') == std::string::npos) {
        m_input += '.';
    }

    UpdateDisplay();
}

void CalculatorApp::ApplyOperator(const std::string& op) {
    if (m_hasError) {
        return;
    }

    const double value = CurrentValue();
    if (m_hasStoredValue && !m_pendingOperator.empty() && !m_startNewEntry) {
        if (!CommitPendingOperation(value)) {
            return;
        }
        m_input = FormatNumber(m_storedValue);
    } else if (!m_hasStoredValue) {
        m_storedValue = value;
        m_hasStoredValue = true;
    }

    m_pendingOperator = op;
    m_history = FormatNumber(m_storedValue) + " " + op;
    m_startNewEntry = true;
    UpdateDisplay();
}

void CalculatorApp::ToggleSign() {
    if (m_hasError) {
        ClearAll();
    }

    const double value = -CurrentValue();
    m_input = FormatNumber(value);
    m_startNewEntry = false;
    UpdateDisplay();
}

void CalculatorApp::ApplyPercent() {
    if (m_hasError) {
        ClearAll();
    }

    const double value = CurrentValue() / 100.0;
    m_input = FormatNumber(value);
    m_startNewEntry = false;
    UpdateDisplay();
}

void CalculatorApp::ClearAll() {
    m_input = "0";
    m_history.clear();
    m_pendingOperator.clear();
    m_storedValue = 0.0;
    m_hasStoredValue = false;
    m_startNewEntry = true;
    m_hasError = false;
    UpdateDisplay();
}

void CalculatorApp::Evaluate() {
    if (m_hasError || !m_hasStoredValue || m_pendingOperator.empty()) {
        return;
    }

    const double rhs = CurrentValue();
    const std::string expression = FormatNumber(m_storedValue) + " " + m_pendingOperator + " " + FormatNumber(rhs);
    if (!CommitPendingOperation(rhs)) {
        return;
    }

    m_input = FormatNumber(m_storedValue);
    m_history = expression + " =";
    m_pendingOperator.clear();
    m_hasStoredValue = false;
    m_startNewEntry = true;
    UpdateDisplay();
}

void CalculatorApp::SetError(const std::string& message) {
    m_input = message;
    m_history.clear();
    m_pendingOperator.clear();
    m_hasStoredValue = false;
    m_startNewEntry = true;
    m_hasError = true;
    UpdateDisplay();
}

void CalculatorApp::UpdateDisplay() {
    if (m_historyText) {
        m_historyText->SetText(m_history);
    }
    if (m_displayText) {
        m_displayText->SetText(m_input);
    }
}

bool CalculatorApp::CommitPendingOperation(double rhs) {
    if (m_pendingOperator == "+") {
        m_storedValue += rhs;
    } else if (m_pendingOperator == "-") {
        m_storedValue -= rhs;
    } else if (m_pendingOperator == "*") {
        m_storedValue *= rhs;
    } else if (m_pendingOperator == "/") {
        if (std::fabs(rhs) <= std::numeric_limits<double>::epsilon()) {
            SetError("Cannot divide by zero");
            return false;
        }
        m_storedValue /= rhs;
    }

    if (!std::isfinite(m_storedValue)) {
        SetError("Overflow");
        return false;
    }

    return true;
}

double CalculatorApp::CurrentValue() const {
    try {
        return std::stod(m_input);
    } catch (...) {
        return 0.0;
    }
}

std::string CalculatorApp::FormatNumber(double value) {
    if (!std::isfinite(value)) {
        return "Overflow";
    }

    if (std::fabs(value) <= std::numeric_limits<double>::epsilon()) {
        value = 0.0;
    }

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(12) << value;
    std::string text = oss.str();

    const std::size_t dotPos = text.find('.');
    if (dotPos != std::string::npos) {
        while (!text.empty() && text.back() == '0') {
            text.pop_back();
        }
        if (!text.empty() && text.back() == '.') {
            text.pop_back();
        }
    }

    if (text.empty() || text == "-0") {
        return "0";
    }

    return text;
}

} // namespace Calc
