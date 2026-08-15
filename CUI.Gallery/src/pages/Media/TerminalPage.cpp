#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"
#include "framework/controls/TerminalControl.h"
#include <algorithm>
#include <cctype>
#include <ctime>
#include <format>
#include <memory>
#include <string>

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

namespace {

std::string Trim(const std::string& s) {
    const size_t b = s.find_first_not_of(" \t\r\n");
    if (b == std::string::npos) return std::string();
    const size_t e = s.find_last_not_of(" \t\r\n");
    return s.substr(b, e - b + 1);
}

std::string ToLower(const std::string& s) {
    std::string out(s);
    std::transform(out.begin(), out.end(), out.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return out;
}

std::string NowString() {
    std::time_t t = std::time(nullptr);
    std::tm tm{};
    ::localtime_s(&tm, &t);
    char buf[64];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
    return std::string(buf);
}

std::string ColorBlock(int code, int repeat = 2) {
    std::string s = std::format("\x1b[48;5;{}m", code);
    s.append(static_cast<size_t>(repeat), ' ');
    s += "\x1b[0m";
    return s;
}

// ---------- 16 色 + 256 色板内容（主题切换区复用） ----------
std::string BuildPalette() {
    std::string s;
    s += "\x1b[1m标准 16 色 \x1b[0m(前景 / 背景):\r\n";
    for (int i = 0; i < 16; ++i) {
        const int fg = (i < 8) ? (30 + i) : (90 + i - 8);
        const int bg = (i < 8) ? (40 + i) : (100 + i - 8);
        s += std::format("\x1b[{}m\u2588\x1b[0m \x1b[{}m  \x1b[0m", fg, bg);
    }
    s += "\r\n\r\n";
    s += "\x1b[1m256 色渐变\x1b[0m (背景):\r\n";
    for (int base = 16; base < 256; base += 24) {
        for (int i = 0; i < 24; ++i) {
            s += ColorBlock(base + i);
        }
        s += "\r\n";
    }
    s += "\r\n";
    s += "\x1b[1m文字修饰\x1b[0m: \x1b[1m加粗\x1b[0m \x1b[2m暗淡\x1b[0m \x1b[3m斜体\x1b[0m \x1b[4m下划线\x1b[0m \x1b[7m反显\x1b[0m \x1b[9m删除线\x1b[0m\r\n";
    s += "\x1b[38;5;75mOSC 8 超链接\x1b[0m: \x1b]8;;https://github.com/microsoft/terminal\x1b\\https://github.com\x1b]8;;\x1b\\\x1b[0m (Ctrl+单击打开)\r\n";
    return s;
}

// ---------- 演示 Shell 后端：自包含迷你命令解释器（无真实进程） ----------
class DemoShellBackend : public Term::ITerminalBackend {
public:
    using Out = ITerminalBackend::OutputCallback;

    void SetOutputCallback(Out callback) override { m_out = std::move(callback); }

    bool Start(int, int) override {
        m_line.clear();
        if (m_out) {
            std::string boot;
            boot += "\x1b]0;CUI Demo Shell — 内置演示终端\x07";
            boot += "\x1b[2J\x1b[H";
            boot += "\x1b[1;38;5;39m  CUI Terminal Control\x1b[0m  \x1b[38;5;208mVirtual Terminal Demo v1.0\x1b[0m\r\n";
            boot += "\x1b[90m  --------------------------------------------------------\x1b[0m\r\n";
            boot += "\r\n";
            boot += "  欢迎！这是一个 \x1b[1mVT100/xterm 兼容\x1b[0m 的内嵌终端模拟器演示。\r\n";
            boot += "  输入 \x1b[1mhelp\x1b[0m 查看命令列表；试试 \x1b[1mneofetch\x1b[0m / \x1b[1mcolors\x1b[0m / \x1b[1mdir\x1b[0m。\r\n";
            boot += "\r\n";
            boot += "  \x1b[32m[ OK ]\x1b[0m 初始化 VT 解析器与 256 色板\r\n";
            boot += "  \x1b[32m[ OK ]\x1b[0m 挂载演示 Shell 后端 (ITerminalBackend)\r\n";
            boot += "  \x1b[32m[ OK ]\x1b[0m 启用光标闪烁 / 滚动条 / 搜索栏 / 右键菜单\r\n";
            boot += "\r\n";
            boot += "  \x1b[38;5;75mOSC 8 超链接\x1b[0m: \x1b]8;;https://github.com/microsoft/terminal\x1b\\https://github.com\x1b]8;;\x1b\\\x1b[0m  (Ctrl+单击打开)\r\n";
            boot += "\r\n";
            m_out(boot.data(), boot.size());
            PrintPrompt();
        }
        return true;
    }

    void Write(const char* data, size_t length) override {
        if (!m_out) return;
        for (size_t i = 0; i < length; ++i) {
            const char c = data[i];
            const unsigned char u = static_cast<unsigned char>(c);
            if (c == '\r' || c == '\n') {
                HandleEnter();
            } else if (c == '\x7f' || c == '\x08') {
                if (!m_line.empty()) {
                    m_line.pop_back();
                    static const char bs[] = "\x08 \x08";
                    m_out(bs, sizeof(bs) - 1);
                }
            } else if (c == '\x03') { // Ctrl+C
                m_line.clear();
                static const char cc[] = "^C\r\n";
                m_out(cc, sizeof(cc) - 1);
                PrintPrompt();
            } else if (u >= 0x20 && u != 0x7F) {
                m_line.push_back(c);
                m_out(&c, 1);
            }
        }
    }

    void Resize(int, int) override {}
    void Stop() override {}
    bool IsRunning() const override { return true; }

private:
    void PrintPrompt() {
        if (!m_out) return;
        static const char prompt[] = "\x1b[32mcui@demo\x1b[0m:\x1b[34m~\x1b[0m$ ";
        m_out(prompt, sizeof(prompt) - 1);
    }

    void HandleEnter() {
        static const char crlf[] = "\r\n";
        m_out(crlf, sizeof(crlf) - 1);
        const std::string cmd = Trim(m_line);
        m_line.clear();
        if (cmd.empty()) {
            PrintPrompt();
            return;
        }
        const std::string out = Execute(cmd);
        m_out(out.data(), out.size());
        PrintPrompt();
    }

    std::string HelpText() {
        return
            "  可用命令:\r\n"
            "    \x1b[1mhelp\x1b[0m        显示本帮助\r\n"
            "    \x1b[1mneofetch\x1b[0m    系统信息 (ANSI 图形)\r\n"
            "    \x1b[1mcolors\x1b[0m      16/256 色板演示\r\n"
            "    \x1b[1mdir\x1b[0m         彩色目录列表\r\n"
            "    \x1b[1mecho <text>\x1b[0m  回显文本\r\n"
            "    \x1b[1mdate\x1b[0m        当前时间\r\n"
            "    \x1b[1mpwd\x1b[0m / \x1b[1mwhoami\x1b[0m  用户信息\r\n"
            "    \x1b[1mclear\x1b[0m       清屏 (或 Ctrl+L)\r\n"
            "    \x1b[1mexit\x1b[0m        退出会话\r\n"
            "\r\n"
            "  终端快捷键: Ctrl+F 搜索 · Ctrl+滚轮 缩放 · Ctrl+Shift+C/V 复制粘贴\r\n"
            "  · 双击选中单词 · 三击选中整行 · 右键弹出上下文菜单\r\n";
    }

    std::string DirListing() {
        return
            "\x1b[1;34mdrwxr-xr-x\x1b[0m cui  cui  4.0K  08-14 18:30 \x1b[34m.\x1b[0m\r\n"
            "\x1b[1;34mdrwxr-xr-x\x1b[0m cui  cui  4.0K  08-14 18:30 \x1b[34m..\x1b[0m\r\n"
            "\x1b[1;34mdrwxr-xr-x\x1b[0m cui  cui  4.0K  08-14 18:30 \x1b[34msrc\x1b[0m\r\n"
            "\x1b[1;34mdrwxr-xr-x\x1b[0m cui  cui  4.0K  08-15 09:02 \x1b[34mbuild\x1b[0m\r\n"
            "\x1b[1;34mdrwxr-xr-x\x1b[0m cui  cui  4.0K  08-15 09:02 \x1b[34massets\x1b[0m\r\n"
            "-rw-r--r-- cui  cui  1.8K  08-15 09:12 main.cpp\r\n"
            "-rw-r--r-- cui  cui  3.4K  08-15 09:40 CMakeLists.txt\r\n"
            "-rw-r--r-- cui  cui   212  08-15 10:02 README.md\r\n"
            "-rw-r--r-- cui  cui  1.1K  08-15 10:12 LICENSE\r\n";
    }

    std::string ColorDemo() {
        std::string s;
        s += "\x1b[1m前景 16 色\x1b[0m:\r\n";
        for (int i = 0; i < 16; ++i) {
            const int code = (i < 8) ? (30 + i) : (90 + i - 8);
            s += std::format("\x1b[{}m\u2588\x1b[0m ", code);
        }
        s += "\r\n\r\n\x1b[1m背景 16 色\x1b[0m:\r\n";
        for (int i = 0; i < 16; ++i) {
            const int code = (i < 8) ? (40 + i) : (100 + i - 8);
            s += std::format("\x1b[{}m  \x1b[0m", code);
        }
        s += "\r\n\r\n\x1b[1m256 色渐变\x1b[0m:\r\n";
        for (int base = 16; base < 256; base += 24) {
            for (int i = 0; i < 24; ++i) {
                s += ColorBlock(base + i);
            }
            s += "\r\n";
        }
        return s;
    }

    std::string Neofetch() {
        return
            "\x1b[36m      \u2584\u2584\u2584\u2584\u2584\u2584\u2584\u2584      \x1b[0m\x1b[1m  cui@demo\x1b[0m\r\n"
            "\x1b[36m    \u2588\u2580\u2580\u2580\u2580\u2580\u2580\u2580\u2580\u2588    \x1b[0m\x1b[1m  -------------------\x1b[0m\r\n"
            "\x1b[36m    \u2588          \u2588    \x1b[0m\x1b[1m  OS\x1b[0m: CUI Terminal 1.0 (x86_64)\r\n"
            "\x1b[36m    \u2588          \u2588    \x1b[0m\x1b[1m  Host\x1b[0m: Demo VM\r\n"
            "\x1b[36m    \u2588          \u2588    \x1b[0m\x1b[1m  Shell\x1b[0m: cui-demo 1.0\r\n"
            "\x1b[36m    \u2580\u2584\u2584\u2584\u2584\u2584\u2584\u2584\u2584\u2580    \x1b[0m\x1b[1m  Theme\x1b[0m: CUI Terminal\r\n"
            "\x1b[36m      \u2580\u2580\u2580\u2580\u2580\u2580\u2580\u2580      \x1b[0m\x1b[1m  Colors\x1b[0m: \x1b[40m  \x1b[41m  \x1b[42m  \x1b[43m  \x1b[44m  \x1b[45m  \x1b[46m  \x1b[47m\x1b[0m\r\n"
            "\r\n"
            "  内存使用: \x1b[38;5;82m[##########------]\x1b[0m 48%  386 / 786 MB\r\n"
            "  CPU 负载: \x1b[38;5;220m[####----------------]\x1b[0m 12%\r\n";
    }

    std::string Execute(const std::string& cmd) {
        const size_t sp = cmd.find(' ');
        const std::string name = Trim(cmd.substr(0, sp == std::string::npos ? cmd.size() : sp));
        const std::string arg = (sp == std::string::npos) ? std::string() : Trim(cmd.substr(sp + 1));
        const std::string n = ToLower(name);

        if (n == "help") return HelpText();
        if (n == "clear" || n == "cls") return "\x1b[2J\x1b[H";
        if (n == "echo") return arg.empty() ? std::string("\r\n") : arg + "\r\n";
        if (n == "date" || n == "time") return NowString() + "\r\n";
        if (n == "pwd") return "/home/cui\r\n";
        if (n == "whoami") return "cui\r\n";
        if (n == "dir" || n == "ls") return DirListing();
        if (n == "colors" || n == "color") return ColorDemo();
        if (n == "neofetch") return Neofetch();
        if (n == "ver" || n == "uname") return "CUI Virtual Terminal 1.0.0 (build 20260815) [x86_64]\r\n";
        if (n == "exit" || n == "quit") return "再见！输入 help 重新查看命令。\r\n";
        return std::format("\x1b[31m{}\x1b[0m: command not found\r\n", name);
    }

    Out m_out;
    std::string m_line;
};

// ---------- 调色板后端：只输出静态色板内容，忽略输入 ----------
class PaletteBackend : public Term::ITerminalBackend {
public:
    using Out = ITerminalBackend::OutputCallback;

    void SetOutputCallback(Out callback) override { m_out = std::move(callback); }

    bool Start(int, int) override {
        if (m_out) {
            std::string s = "\x1b]0;CUI 调色板\x07\x1b[2J\x1b[H" + BuildPalette();
            m_out(s.data(), s.size());
        }
        return true;
    }

    void Write(const char*, size_t) override {}
    void Resize(int, int) override {}
    void Stop() override {}
    bool IsRunning() const override { return true; }

private:
    Out m_out;
};

} // anonymous namespace

Element BuildTerminalPage() {
    // ---------- 第 1 节：常规用法（内嵌终端 + 演示 Shell） ----------
    // 后端实例生命周期与进程一致，终端析构时可安全 Detach。
    static std::shared_ptr<DemoShellBackend> s_demoBackend = std::make_shared<DemoShellBackend>();

    auto demoTerm = std::make_shared<TerminalControl>();
    demoTerm->SetHeight(400.0f);
    demoTerm->AttachBackend(s_demoBackend.get());

    auto status1 = MakeStatus("点击终端聚焦后可直接输入；或使用下方按钮注入演示命令。输入 help 查看全部命令。");

    auto btnHelp = ElevatedButton("help", [demoTerm](UIElement*) { demoTerm->WriteInput("help\r"); }).Build();
    auto btnNeo = ElevatedButton("neofetch", [demoTerm](UIElement*) { demoTerm->WriteInput("neofetch\r"); }).Build();
    auto btnColors = ElevatedButton("colors", [demoTerm](UIElement*) { demoTerm->WriteInput("colors\r"); }).Build();
    auto btnDir = ElevatedButton("dir", [demoTerm](UIElement*) { demoTerm->WriteInput("dir\r"); }).Build();
    auto btnClear1 = ElevatedButton("清屏", [demoTerm](UIElement*) { demoTerm->WriteInput("clear\r"); }).Build();
    auto btnFind = ElevatedButton("搜索 Ctrl+F", [demoTerm](UIElement*) { demoTerm->ShowFind(true); }).Build();
    auto btnSelectAll = ElevatedButton("全选", [demoTerm](UIElement*) { demoTerm->SelectAll(); }).Build();
    auto btnCopy = ElevatedButton("复制选中", [demoTerm](UIElement*) { demoTerm->CopySelectionToClipboard(); }).Build();
    auto btnZoomIn = ElevatedButton("放大 +", [demoTerm](UIElement*) { demoTerm->Zoom(1); }).Build();
    auto btnZoomOut = ElevatedButton("缩小 -", [demoTerm](UIElement*) { demoTerm->Zoom(-1); }).Build();
    auto btnTitle1 = ElevatedButton("读取标题", [demoTerm, status1](UIElement*) {
        status1->Text = std::format("终端上报标题: “{}”", demoTerm->GetTerminalTitle());
    }).Build();

    // ---------- 第 2 节：真实 ConPty 对接（真实 Shell） ----------
    auto realTerm = std::make_shared<TerminalControl>("cmd.exe");
    realTerm->SetHeight(320.0f);
    realTerm->AttachBackend(nullptr); // 阻止自动拉起，由按钮显式启动

    auto status2 = MakeStatus("点击按钮拉起真实的 Windows 伪控制台 (ConPty) 子进程，可直接在终端内交互。");

    auto btnCmd = ElevatedButton("启动 cmd.exe", [realTerm, status2](UIElement*) {
        realTerm->StartShell("cmd.exe");
        status2->Text = "已启动 cmd.exe（ConPty 子进程），现在可以在终端内直接输入命令...";
    }).Build();
    auto btnPwsh = ElevatedButton("启动 PowerShell", [realTerm, status2](UIElement*) {
        realTerm->StartShell("powershell.exe");
        status2->Text = "已启动 PowerShell（ConPty 子进程），现在可以在终端内直接输入命令...";
    }).Build();
    auto btnStop = ElevatedButton("停止 Shell", [realTerm, status2](UIElement*) {
        realTerm->StopShell();
        status2->Text = "已停止并断开当前 Shell 子进程。";
    }).Build();
    auto btnClear2 = ElevatedButton("清屏", [realTerm](UIElement*) { realTerm->Terminal().Clear(); }).Build();
    auto btnTitle2 = ElevatedButton("读取标题", [realTerm, status2](UIElement*) {
        status2->Text = std::format("终端上报标题: “{}”", realTerm->GetTerminalTitle());
    }).Build();

    // ---------- 第 3 节：主题与外观 ----------
    static std::shared_ptr<PaletteBackend> s_paletteBackend = std::make_shared<PaletteBackend>();

    auto paletteTerm = std::make_shared<TerminalControl>();
    paletteTerm->SetHeight(280.0f);
    paletteTerm->AttachBackend(s_paletteBackend.get());

    auto status3 = MakeStatus("切换主题观察终端配色；Ctrl+滚轮或下方按钮可缩放字体。");

    auto btnDark = ElevatedButton("默认深色", [paletteTerm, status3](UIElement*) {
        paletteTerm->ApplyTheme(Term::TerminalTheme::Dark());
        status3->Text = "已应用默认深色主题 (bg #0C0C0C)。";
    }).Build();
    auto btnLight = ElevatedButton("浅色", [paletteTerm, status3](UIElement*) {
        paletteTerm->ApplyTheme(Term::TerminalTheme::Light());
        status3->Text = "已应用浅色主题 (bg #F3F3F3)。";
    }).Build();
    auto btnNord = ElevatedButton("Nord 自定义", [paletteTerm, status3](UIElement*) {
        Term::TerminalTheme nord;
        nord.Background = Term::TermColor::FromRgb(0x2E, 0x34, 0x40);
        nord.Foreground = Term::TermColor::FromRgb(0xD8, 0xDE, 0xE9);
        nord.Cursor = Term::TermColor::FromRgb(0x88, 0xC0, 0xD0);
        nord.Selection = Term::TermColor::FromArgb(0x99, 0x43, 0x4C, 0x5E);
        paletteTerm->ApplyTheme(nord);
        status3->Text = "已应用 Nord 风格自定义主题 (bg #2E3440)。";
    }).Build();
    auto btnZoomIn3 = ElevatedButton("放大 +", [paletteTerm](UIElement*) { paletteTerm->Zoom(1); }).Build();
    auto btnZoomOut3 = ElevatedButton("缩小 -", [paletteTerm](UIElement*) { paletteTerm->Zoom(-1); }).Build();

    SamplePageSpec spec;
    spec.title = "Terminal (终端控件)";
    spec.subtitle = "内嵌式虚拟终端模拟器：完整 VT100/xterm 协议解析、ConPty 真实 Shell 桥接、256 色渲染、搜索 / 划选 / 右键菜单 / 光标闪烁。";
    spec.sections = {
        {
            "常规用法 — 内嵌终端 + 演示 Shell",
            "1. 点击终端区域聚焦后可直接键盘输入，回车执行命令（help / neofetch / colors / dir / clear / echo…）；\n"
            "2. 后端通过可插拔的 ITerminalBackend 接口注入，本例为自包含的迷你命令解释器，无需真实进程；\n"
            "3. Ctrl+F 呼出右上角搜索栏、Ctrl+滚轮缩放字体、双击选中单词、右键弹出复制/粘贴菜单；\n"
            "4. 状态栏可读取由 OSC 0 上报的终端标题。",
            Column(12, {
                Row(8, { btnHelp, btnNeo, btnColors, btnDir, btnClear1 }),
                Row(8, { btnFind, btnSelectAll, btnCopy, btnZoomIn, btnZoomOut, btnTitle1 }),
                demoTerm,
                status1,
            }),
        },
        {
            "真实 ConPty 对接 — 拉起原生 Shell",
            "1. 点击“启动 cmd.exe / PowerShell”将创建 Windows Pseudo Console (ConPty) 管道，桥接真实子进程；\n"
            "2. 终端网格随控件尺寸自动调整，并把行列变化实时同步给 pty 进程；\n"
            "3. 输出采用背压队列异步冲刷，光标闪烁与滚动条自动淡入淡出。",
            Column(12, {
                Row(8, { btnCmd, btnPwsh, btnStop, btnClear2, btnTitle2 }),
                realTerm,
                status2,
            }),
        },
        {
            "主题与显示选项",
            "1. ApplyTheme 可整体替换背景 / 前景 / 光标 / 选区与 16 色调色板（含自定义 TermColor）；\n"
            "2. 演示了 16 色前景/背景、256 色渐变、文字修饰与 OSC 8 超链接渲染；\n"
            "3. Zoom 平滑缩放字体（8–48px），Ctrl+滚轮同样生效。",
            Column(12, {
                Row(8, { btnDark, btnLight, btnNord, btnZoomIn3, btnZoomOut3 }),
                paletteTerm,
                status3,
            }),
        },
    };

    spec.source = R"(
// 1) 创建终端控件，挂载自定义后端（或直接拉起真实 Shell）
auto terminal = std::make_shared<TerminalControl>();
terminal->AttachBackend(myBackend);          // ITerminalBackend 可插拔
// terminal->StartShell("powershell.exe");   // 或桥接真实 ConPty 进程

// 2) 编程注入输入 / 控制显示
terminal->WriteInput("help\r");
terminal->Zoom(+1);                          // Ctrl+滚轮 或编程缩放
terminal->ApplyTheme(Term::TerminalTheme::Light());
terminal->ShowFind(true);                    // 呼出搜索栏 (Ctrl+F)

// 3) 常用操作
terminal->SelectAll();
terminal->CopySelectionToClipboard();
std::string title = terminal->GetTerminalTitle(); // OSC 0 上报的标题
)";

    return BuildSamplePage(spec);
}

} // namespace Gallery
