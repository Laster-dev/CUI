#pragma once
#include "Control.h"
#include "ScrollbarAutoHide.h"
#include "TextBox.h"
#include "terminal/ConPtyBackend.h"
#include "terminal/Terminal.h"
#include "terminal/TerminalRenderer.h"
#include <atomic>
#include <chrono>
#include <memory>
#include <string>
#include <vector>

namespace CUI {

// Embeddable VT terminal view: grid surface + scrollbar + find bar + context menu.
// Call AttachConPty (or AttachBackend) after the control is in the visual tree.
class TerminalControl : public Control {
public:
    TerminalControl();
    explicit TerminalControl(const std::string& shellPath);
    ~TerminalControl() override;

    const char* GetClassName() const override { return "TerminalControl"; }
    Value GetProperty(PropertyId id) const override;
    bool HasProperty(PropertyId id) const override;
    void SetProperty(PropertyId id, const Value& val) override;
    HCURSOR GetCursor() const override;

    Term::Terminal& Terminal() { return *m_terminal; }
    Term::TerminalRenderer& Renderer() { return *m_renderer; }

    // Backend lifetime is owned by the caller unless AttachConPty is used.
    void AttachBackend(Term::ITerminalBackend* backend);
    void AttachConPty(const std::string& shellPath, const std::string& arguments = std::string());
    void DetachBackend();

    // Legacy helpers kept so existing showcase/host code keeps working.
    void StartShell(const std::string& shellPath = "cmd.exe");
    void StopShell();
    void WriteInput(const std::string& text);

    void ApplyTheme(const Term::TerminalTheme& theme);
    void Zoom(int deltaSteps);

    void ShowFind(bool show = true);
    bool IsFindVisible() const { return m_findVisible; }

    const std::string& GetShell() const { return m_pendingShell; }
    void SetShell(const std::string& shellPath) {
        m_pendingShell = shellPath;
        MarkRenderContentDirty();
    }

    const std::string& GetTerminalTitle() const { return m_terminalTitle; }

    void CopySelectionToClipboard();
    void PasteFromClipboard();
    void SelectAll();

    Size Measure(Size availableSize) override;
    void Arrange(Rect finalRect) override;
    void OnRender(GraphicsContext& ctx) override;

    bool OnAnimationTick() override;
    bool HasSelfAnimation() const override;

    bool OnKeyDown(int vkCode) override;
    void OnCharInput(wchar_t ch) override;
    void OnMouseDown(Point pt) override;
    void OnMouseDblClick(Point pt) override;
    void OnMouseMove(Point pt) override;
    void OnMouseUp(Point pt) override;
    void OnMouseWheel(float delta) override;
    void OnMouseLeave() override;
    void OnFocus() override;
    void OnBlur() override;

private:
    class FindBox;

    void InitTerminal(const std::string& shellPath);
    void BuildContextMenu();
    void BuildFindBar();

    Rect GetFindBarRect() const;
    Rect GetSurfaceRect() const;
    Rect GetScrollBarRect() const;
    Rect GetScrollThumbRect() const;
    Rect GetFindButtonRect(int index) const;
    Rect GetRowRect(int row) const;

    void RecalculateSize(GraphicsContext& ctx);
    void SyncScrollFromThumb(float y);
    void MarkViewportDirty();
    void MarkDirtyRows();
    void QueueRedraw();
    void RequestWindowRepaint();

    bool HitTestCell(Point pt, int& col, int& row) const;
    int AbsoluteRow(int viewportRow) const;
    static unsigned CurrentModifiers();

    void DoFind(bool forward);
    void UpdateFindStatus();
    void MaybeCopyOnSelect();
    void SendKeySequence(const std::string& seq);

    std::unique_ptr<Term::Terminal> m_terminal;
    std::unique_ptr<Term::TerminalRenderer> m_renderer;
    std::unique_ptr<Term::ConPtyBackend> m_ownedBackend;
    Term::ITerminalBackend* m_backend = nullptr;
    std::string m_pendingShell;
    std::string m_terminalTitle;
    bool m_backendStartAttempted = false;

    std::shared_ptr<FindBox> m_findBox;
    bool m_findVisible = false;
    int m_findRow = 0;
    int m_findCol = 0;
    std::string m_findStatus;
    int m_hoveredFindButton = -1;

    bool m_cursorOn = true;
    float m_blinkAccumMs = 0.0f;
    float m_flushAccumMs = 0.0f;
    bool m_redrawQueued = false;
    std::atomic<bool> m_outputPending{ false };
    HWND m_hwnd = nullptr;

    int m_lastCols = -1;
    int m_lastRows = -1;
    int m_lastYDisp = 0;
    std::vector<Term::BufferLine*> m_boundLines;

    bool m_mouseReporting = false;
    int m_pressedButton = -1;
    int m_lastMouseCol = -1;
    int m_lastMouseRow = -1;
    bool m_draggingScrollbar = false;
    float m_scrollGrabOffset = 0.0f;
    ScrollbarAutoHide m_scrollbarAutoHide;

    int m_clickCount = 0;
    std::chrono::steady_clock::time_point m_lastClickTime;
    int m_lastClickCol = -1;
    int m_lastClickRow = -1;

    int m_suppressCharCount = 0;
    std::wstring m_imePreedit;
};

} // namespace CUI
