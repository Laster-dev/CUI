#pragma once

#include "framework/window/Window.h"
#include "framework/controls/TextBlock.h"
#include "framework/style/ThemeTokenId.h"

#include <memory>
#include <string>

namespace CUI {
class Button;
class Grid;
class UIElement;
}

namespace Calc {

class CalculatorApp {
public:
    int Run();

private:
    std::shared_ptr<CUI::UIElement> BuildRoot();
    std::shared_ptr<CUI::UIElement> BuildDisplayPanel();
    std::shared_ptr<CUI::Button> CreateButton(const std::string& text,
                                              CUI::ThemeTokenId backgroundToken,
                                              CUI::ThemeTokenId textToken,
                                              float minHeight);
    void BuildButtons(CUI::Grid& grid);

    void AppendDigit(const std::string& digit);
    void AppendDecimalPoint();
    void ApplyOperator(const std::string& op);
    void ToggleSign();
    void ApplyPercent();
    void ClearAll();
    void Evaluate();
    void SetError(const std::string& message);
    void UpdateDisplay();
    bool CommitPendingOperation(double rhs);
    double CurrentValue() const;
    static std::string FormatNumber(double value);

    CUI::Window m_window;
    std::shared_ptr<CUI::TextBlock> m_historyText;
    std::shared_ptr<CUI::TextBlock> m_displayText;
    std::string m_input = "0";
    std::string m_history;
    std::string m_pendingOperator;
    double m_storedValue = 0.0;
    bool m_hasStoredValue = false;
    bool m_startNewEntry = true;
    bool m_hasError = false;
};

} // namespace Calc
