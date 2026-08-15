#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"
#include "framework/input/Command.h"
#include "framework/controls/Control.h"
#include "framework/controls/Button.h"
#include "framework/controls/ToggleButton.h"
#include "framework/window/Window.h"
#include "framework/style/ThemeManager.h"
#include <format>
#include <memory>
#include <vector>

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

namespace {

/**
 * @brief 实时快捷键捕获与命令执行控制台控件
 */
class CommandConsoleControl : public Control {
public:
    CommandConsoleControl() {
        SetHeight(120.0f);
        SetCornerRadius(8.0f);
        SetToolTip("点击此处聚焦后，按下键盘快捷键 (如 Ctrl+N, Ctrl+S, F5, Ctrl+F, Ctrl+R) 即可实时触发命令");

        InitCommands();
    }

    ~CommandConsoleControl() override {
        UnregisterGlobalCommands();
    }

    const char* GetClassName() const override { return "CommandConsoleControl"; }
    bool AcceptsTabFocus() const override { return true; }

    void InitCommands() {
        m_canSave = true;

        m_cmdNew = std::make_shared<Command>(
            [this]() { LogAction("📄 [File.New] 触发【新建文档】命令 (Ctrl + N)"); }
        );
        m_cmdNew->SetId("File.New");
        m_cmdNew->SetLabel("新建文档");
        m_cmdNew->SetGesture("Ctrl+N");

        m_cmdSave = std::make_shared<Command>(
            [this]() { LogAction("💾 [File.Save] 触发【保存文件】命令 (Ctrl + S)"); },
            [this]() { return m_canSave; }
        );
        m_cmdSave->SetId("File.Save");
        m_cmdSave->SetLabel("保存文件");
        m_cmdSave->SetGesture("Ctrl+S");

        m_cmdBuild = std::make_shared<Command>(
            [this]() { LogAction("⚡ [Build.Run] 触发【一键构建调试】命令 (F5)"); }
        );
        m_cmdBuild->SetId("Build.Run");
        m_cmdBuild->SetLabel("启动调试");
        m_cmdBuild->SetGesture("F5");

        m_cmdFind = std::make_shared<Command>(
            [this]() { LogAction("🔍 [Edit.Find] 触发【全局搜索查找】命令 (Ctrl + F)"); }
        );
        m_cmdFind->SetId("Edit.Find");
        m_cmdFind->SetLabel("查找替换");
        m_cmdFind->SetGesture("Ctrl+F");

        m_manager.Register(m_cmdNew);
        m_manager.Register(m_cmdSave);
        m_manager.Register(m_cmdBuild);
        m_manager.Register(m_cmdFind);

        RegisterGlobalCommands();
    }

    void RegisterGlobalCommands() {
        if (auto* win = Window::Current()) {
            win->GetCommands().Register(m_cmdNew);
            win->GetCommands().Register(m_cmdSave);
            win->GetCommands().Register(m_cmdBuild);
            win->GetCommands().Register(m_cmdFind);
        }
    }

    void UnregisterGlobalCommands() {
        if (auto* win = Window::Current()) {
            win->GetCommands().Unregister(m_cmdNew.get());
            win->GetCommands().Unregister(m_cmdSave.get());
            win->GetCommands().Unregister(m_cmdBuild.get());
            win->GetCommands().Unregister(m_cmdFind.get());
        }
    }

    void OnNavigatedFrom() override {
        UnregisterGlobalCommands();
        UIElement::OnNavigatedFrom();
    }

    Size Measure(Size availableSize) override {
        float w = GetWidth() > 0.0f ? GetWidth() : (availableSize.width > 0.0f ? availableSize.width : 420.0f);
        m_desiredSize = Size(w, 120.0f);
        return m_desiredSize;
    }

    void OnRender(GraphicsContext& ctx) override {
        const auto& tokens = ThemeManager::Instance().GetTokens();
        const float radius = 8.0f;
        const bool focused = IsFocused();

        ctx.FillRoundedRect(m_bounds, radius, tokens.cardBackground);
        ctx.DrawRoundedRect(m_bounds, radius, focused ? tokens.focusedBorder : tokens.cardBorder, focused ? 1.5f : 1.0f);

        // 头部标题与聚焦状态
        std::string header = focused
            ? "⌨️ 快捷键测试区 (已聚焦 - 直接按下键盘按键即可触发)"
            : "⌨️ 快捷键测试区 (点击此处获取键盘焦点)";
        ctx.DrawText(
            header,
            Rect(m_bounds.x + 16.0f, m_bounds.y + 12.0f, m_bounds.width - 32.0f, 20.0f),
            focused ? tokens.accentColor : tokens.textSecondary,
            "微软雅黑",
            12.0f,
            DWRITE_TEXT_ALIGNMENT_LEADING,
            DWRITE_PARAGRAPH_ALIGNMENT_CENTER,
            focused ? DWRITE_FONT_WEIGHT_SEMI_BOLD : DWRITE_FONT_WEIGHT_NORMAL);

        // 实时最近捕获的命令执行状态
        ctx.DrawText(
            m_lastLog,
            Rect(m_bounds.x + 16.0f, m_bounds.y + 42.0f, m_bounds.width - 32.0f, 60.0f),
            tokens.textPrimary,
            "Consolas",
            12.0f,
            DWRITE_TEXT_ALIGNMENT_LEADING,
            DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
    }

    void OnMouseDown(Point pt) override {
        Control::OnMouseDown(pt);
        Focus(FocusState::Pointer);
        RegisterGlobalCommands();
        MarkRenderRectDirty(m_bounds);
    }

    bool OnKeyDown(int vkCode) override {
        const bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
        const bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
        const bool alt = (GetKeyState(VK_MENU) & 0x8000) != 0;

        if (m_manager.TryExecute(vkCode, ctrl, shift, alt)) {
            return true;
        }

        // 记录普通按键
        std::string keyCombo;
        if (ctrl) keyCombo += "Ctrl + ";
        if (shift) keyCombo += "Shift + ";
        if (alt) keyCombo += "Alt + ";
        keyCombo += std::format("VK_{}", vkCode);
        LogAction(std::format("按下了未绑定命令的组合键：[{}]", keyCombo));
        return false;
    }

    void LogAction(const std::string& text) {
        m_lastLog = text;
        MarkRenderRectDirty(m_bounds);
        if (m_onStatusChanged) {
            m_onStatusChanged(text);
        }
    }

    void SetStatusHandler(std::function<void(const std::string&)> handler) {
        m_onStatusChanged = std::move(handler);
    }

    void SetCanSave(bool canSave) {
        m_canSave = canSave;
        m_cmdSave->RaiseCanExecuteChanged();
    }

    std::shared_ptr<Command> GetCmdNew() const { return m_cmdNew; }
    std::shared_ptr<Command> GetCmdSave() const { return m_cmdSave; }
    std::shared_ptr<Command> GetCmdBuild() const { return m_cmdBuild; }
    std::shared_ptr<Command> GetCmdFind() const { return m_cmdFind; }

private:
    CommandManager m_manager;
    std::shared_ptr<Command> m_cmdNew;
    std::shared_ptr<Command> m_cmdSave;
    std::shared_ptr<Command> m_cmdBuild;
    std::shared_ptr<Command> m_cmdFind;
    bool m_canSave = true;
    std::string m_lastLog = "就绪。支持快捷键：Ctrl+N (新建), Ctrl+S (保存), F5 (调试), Ctrl+F (查找)";
    std::function<void(const std::string&)> m_onStatusChanged;
};

} // namespace

/**
 * @brief 构建 Commands (命令与快捷键系统) 展示页面。
 * 遵循 Fluent 规范，全弹性流式布局，不硬编码固定宽度。
 */
Element BuildCommandsPage() {
    auto statusLabel = MakeStatus("提示：按下键盘快捷键 (Ctrl+N, Ctrl+S, F5, Ctrl+F) 或点击下方按钮均可触发命令。");

    auto console = std::make_shared<CommandConsoleControl>();
    console->SetStatusHandler([statusLabel](const std::string& msg) {
        statusLabel->Text = msg;
    });

    // ==========================================
    // 1. 命令按钮触发与状态切换
    // ==========================================
    auto btnExecNew = Button("📄 触发新建 (Ctrl+N)")
        .OnClick([console](UIElement*) {
            console->GetCmdNew()->Execute();
        });

    auto btnExecSave = Button("💾 触发保存 (Ctrl+S)")
        .OnClick([console, statusLabel](UIElement*) {
            auto cmd = console->GetCmdSave();
            if (cmd->CanExecute()) {
                cmd->Execute();
            } else {
                statusLabel->Text = "⚠️ 保存命令当前被禁用 (CanExecute = false)！";
            }
        });

    auto btnExecBuild = Button("⚡ 触发调试 (F5)")
        .OnClick([console](UIElement*) {
            console->GetCmdBuild()->Execute();
        });

    auto btnExecFind = Button("🔍 触发查找 (Ctrl+F)")
        .OnClick([console](UIElement*) {
            console->GetCmdFind()->Execute();
        });

    auto toggleCanSave = ToggleButtonWidget("允许保存 (CanExecute = true)");
    toggleCanSave->SetIsChecked(true);
    toggleCanSave->OnClick.Connect([console, statusLabel](UIElement* sender) {
        auto btn = dynamic_cast<ToggleButton*>(sender);
        bool canSave = btn && btn->IsChecked();
        console->SetCanSave(canSave);
        statusLabel->Text = canSave
            ? "已【启用】保存命令 (Ctrl+S 可正常触发)"
            : "已【禁用】保存命令 (Ctrl+S 将被拦截阻断)";
    });

    SamplePageSpec spec;
    spec.title = "Commands (命令与快捷键)";
    spec.subtitle = "解耦 UI 表现与核心业务逻辑的应用级命令调度系统，内置 KeyGesture 快捷键智能匹配、全局路由分发与 CanExecute 动态条件判定。";
    spec.sections = {
        {
            "快捷键实时捕获与命令执行控制台",
            "在下方控制台区域按下快捷键（或在窗口任意位置按下 Ctrl+N、Ctrl+S、F5、Ctrl+F）即可实时观察命令触发。",
            Column(12, {
                console,
            }),
        },
        {
            "按钮委托触发与 CanExecute 动态拦截",
            "除键盘快捷键外，Command 亦可直接绑定至 Button 或 MenuBar；可通过开关切换 CanExecute 状态测试执行阻断。",
            Column(12, {
                Row(8, { btnExecNew, btnExecSave, btnExecBuild, btnExecFind }),
                Row(8, { toggleCanSave }),
                statusLabel,
            }),
        },
    };

    spec.source = R"cpp(// 1. 创建 Command 并指定快捷键手势与判定函数
auto cmdSave = std::make_shared<Command>(
    []() { /* 保存文件逻辑 */ },
    []() { return isModified(); } // CanExecute 判定
);
cmdSave->SetId("File.Save");
cmdSave->SetGesture("Ctrl+S");

// 2. 注册到 Window 全局命令管理器
Window::Current()->GetCommands().Register(cmdSave);

// 3. 动态触发与状态通知
cmdSave->Execute();
cmdSave->RaiseCanExecuteChanged();
)cpp";

    return BuildSamplePage(spec);
}

} // namespace Gallery
