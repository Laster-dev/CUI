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

/**
 * @brief 内嵌式虚拟终端模拟器控件（TerminalControl / Terminal）。
 * 包含了完整的 VT100 / xterm 协议兼容终端模拟，用于在 CUI 应用内嵌入原生的命令行或 Shell：
 * 1. **ConPty 后端连接**：支持通过 Windows Pseudo Console (ConPty) 桥接底层 cmd.exe / powershell.exe / wsl.exe 进程物理管道。
 * 2. **硬件渲染单元**：通过 D2D1 字符图表集缓存和高频渲染器 (TerminalRenderer) 提供零延迟输入响应。
 * 3. **终端增强组件**：自带淡入淡出滚动条、搜索栏 (FindBox)、右键上下文复制粘贴菜单、以及终端字符双击划选。
 */
class TerminalControl : public Control {
public:
    TerminalControl();
    explicit TerminalControl(const std::string& shellPath);
    ~TerminalControl() override;

    const char* GetClassName() const override { return "TerminalControl"; } // 获取类名
    Value GetProperty(PropertyId id) const override; // 反射获取属性值
    bool HasProperty(PropertyId id) const override; // 检查是否存在对应属性
    void SetProperty(PropertyId id, const Value& val) override; // 反射设定属性值
    HCURSOR GetCursor() const override; // 获取终端网格表面上特有的 Windows I-Beam 文本鼠标指针

    Term::Terminal& Terminal() { return *m_terminal; } // 获取内部终端虚拟缓冲区对象
    Term::TerminalRenderer& Renderer() { return *m_renderer; } // 获取终端物理渲染核心对象

    void AttachBackend(Term::ITerminalBackend* backend); // 挂载与绑定一个外部实现的原生 VT 终端输出后端通道
    void AttachConPty(const std::string& shellPath, const std::string& arguments = std::string()); // 异步拉起本机的 Windows 伪控制台子进程管道（cmd.exe等）并挂接读写后端
    void DetachBackend(); // 强行注销并断开与当前后端的连接

    void StartShell(const std::string& shellPath = "cmd.exe"); // 快捷助手：拉起 cmd.exe 进程启动终端
    void StopShell(); // 快捷助手：强行杀死当前拉起的终端进程，停止命令管道
    void WriteInput(const std::string& text); // 向虚拟终端直接注入发送原始的 UTF-8 输入字节序列

    void ApplyTheme(const Term::TerminalTheme& theme); // 为终端重新应用色彩和光标的规范皮肤
    void Zoom(int deltaSteps); // 对终端内的英文字体字号进行平滑缩放调节 (px)

    void ShowFind(bool show = true); // 呼出或收折隐藏终端右上角的搜索查找匹配框 (FindBox)
    bool IsFindVisible() const { return m_findVisible; } // 检查搜索栏目前是否处于可见状态

    const std::string& GetShell() const { return m_pendingShell; } // 读取待启动的目标 Shell 文件路径
    void SetShell(const std::string& shellPath) { // 预备设置要启动的 Shell 并标记局部重绘
        m_pendingShell = shellPath;
        MarkRenderContentDirty();
    }

    const std::string& GetTerminalTitle() const { return m_terminalTitle; } // 读取终端上报的最新标题文本（如当前执行的程序）

    void CopySelectionToClipboard(); // 将用户拉框选中的终端高亮文本复制到系统剪贴板中
    void PasteFromClipboard(); // 从系统剪贴板中读取文本并以虚拟按键字节流形式直接写入终端中
    void SelectAll(); // 快速全选当前缓冲区内的全部字符内容

    Size Measure(Size availableSize) override; // 依据终端行高列宽字体测算最适的静态占位大小
    void Arrange(Rect finalRect) override; // 编排界面并实时通知 ConPty 后端调整 pty 物理终端的行列网格大小
    void OnRender(GraphicsContext& ctx) override; // 绘制终端字符网格、闪烁光标、选择辅助高亮、滚动条以及右上角搜索卡片

    bool OnAnimationTick() override; // 驱动光标持续规律性闪烁以及搜索框展开折叠动画
    bool HasSelfAnimation() const override; // 检查是否存在正在播放的提示与光标闪烁时钟

    bool OnKeyDown(int vkCode) override; // 键盘物理扫描按键压下，翻译并组装成 VT 转义序列发送给 PTY 管道
    void OnCharInput(wchar_t ch) override; // 键盘字元符号字符输入翻译并写入
    void OnMouseDown(Point pt) override; // 鼠标按下，处理右键菜单弹出或开始左键拉框滑动字符选择
    void OnMouseDblClick(Point pt) override; // 鼠标双击，自动分词选中当前滑过的完整单词
    void OnMouseMove(Point pt) override; // 鼠标移动，更新划选范围大小或滚动条拖拽偏移量
    void OnMouseUp(Point pt) override; // 鼠标松开释放拉框捕获
    void OnMouseWheel(float delta) override; // 响应滚轮，垂直滚动浏览历史缓冲区日志行
    void OnMouseLeave() override; // 鼠标指针离开，重置 Hover 标识并淡出滚动条
    void OnFocus() override; // 获焦时，标记启动光标闪烁定时器并刷新光标形态
    void OnBlur() override; // 失去焦点时，停止光标闪烁并重绘光标为虚框

private:
    class FindBox; // 声明右上角嵌入的搜索栏具体自绘包装类

    void InitTerminal(const std::string& shellPath); // 内部执行终端缓冲及渲染器的核心初始化建构
    void BuildContextMenu(); // 初始化组装鼠标右键“复制/粘贴/全选”弹出式快捷上下文菜单
    void BuildFindBar(); // 初始化组装搜索输入文本框及搜索功能按键

    Rect GetFindBarRect() const; // 计算搜索小面板在终端内的局部放置坐标矩形
    Rect GetSurfaceRect() const; // 计算终端字符网格绘制表面占用的局部实际矩形
    Rect GetScrollBarRect() const; // 计算垂直滚动条轨道局域矩形
    Rect GetScrollThumbRect() const; // 计算垂直滚动条滑块手柄局域矩形
    Rect GetFindButtonRect(int index) const; // 获得搜索栏上第 index 个功能按键物理矩形
    Rect GetRowRect(int row) const; // 计算第 row 行字符网格对应的局部物理矩形

    void RecalculateSize(GraphicsContext& ctx); // 依据最新边界尺寸, 重新推算终端能够容纳的最大列数和行数
    void SyncScrollFromThumb(float y); // 根据滚动条手柄移动高度, 反向同步更新终端的视口 Y 轴偏移行号
    void MarkViewportDirty(); // 脏化当前显示的整个终端字符网格视口区域
    void MarkDirtyRows(); // 分析终端行更新脏标志, 仅失效重绘发生内容变动的那几行
    void QueueRedraw(); // 发出异步安全的高频重绘申请
    void RequestWindowRepaint(); // 指示窗口重新进行 Paint 刷新

    bool HitTestCell(Point pt, int& col, int& row) const; // 点命中测试反向定位坐标落入的字符网格第几列、第几行
    int AbsoluteRow(int viewportRow) const; // 将相对当前视口高度的行号 viewportRow 转换为历史缓冲区内的绝对行索引号
    static unsigned CurrentModifiers(); // 静态助手：获取当前 Windows 系统 Ctrl/Shift/Alt 物理按键的修饰状态掩码

    void DoFind(bool forward); // 在缓冲区中执行具体正向或反向的字符串关键词查询
    void UpdateFindStatus(); // 更新搜索栏文字标签（如“没有找到”或“第 3/10 个”）
    void MaybeCopyOnSelect(); // 判断是否配置了“划选完即自动复制到剪贴板”的策略
    void SendKeySequence(const std::string& seq); // 注入发送一组转义序列

    std::unique_ptr<Term::Terminal> m_terminal;                         // 虚拟终端控制台控制缓冲区对象
    std::unique_ptr<Term::TerminalRenderer> m_renderer;                 // 字符集高速光栅化渲染器
    std::unique_ptr<Term::ConPtyBackend> m_ownedBackend;                // 控件独占持有的 Windows PTY 进程管道读写器
    Term::ITerminalBackend* m_backend = nullptr;                        // 当前绑定的后端通道抽象接口弱指针
    std::string m_pendingShell;                                         // 待拉起执行的目标 Shell 文件路径
    std::string m_terminalTitle;                                        // 上报的终端窗口当前标题
    bool m_backendStartAttempted = false;                               // 标记是否尝试拉起过 PTY 子进程以防无限重试崩溃

    std::shared_ptr<FindBox> m_findBox;                                 // 搜索栏组件实例引用
    bool m_findVisible = false;                                         // 搜索栏可见状态
    int m_findRow = 0;                                                  // 搜索到匹配条目的绝对行号索引
    int m_findCol = 0;                                                  // 搜索到匹配条目的列索引
    std::string m_findStatus;                                           // 搜索结果提示标签内容
    int m_hoveredFindButton = -1;                                       // 鼠标当前悬浮在搜索面板的哪项功能按键上

    bool m_cursorOn = true;                                             // 终端光标物理闪烁状态（开或关）
    float m_blinkAccumMs = 0.0f;                                        // 闪烁计时累计毫秒数，控制 500ms 周期闪烁
    float m_flushAccumMs = 0.0f;                                        // 帧率及脏状态冲刷累计毫秒数
    bool m_redrawQueued = false;                                        // 标志是否已有重绘请求在队列中等待调度
    std::atomic<bool> m_outputPending{ false };                         // 标记是否有新接收到的终端命令行回显数据等待冲刷显示
    ::HWND m_hwnd = nullptr;                                              // 主窗口句柄

    int m_lastCols = -1;                                                // 终端前一帧的网格列数限制
    int m_lastRows = -1;                                                // 终端前一帧的网格行数限制
    int m_lastYDisp = 0;                                                // 终端前一帧的视口滚动位移量
    std::vector<Term::BufferLine*> m_boundLines;                        // 绑定并参与本帧布局绘制的历史缓冲区缓存行指针

    bool m_mouseReporting = false;                                      // 是否向后台进程上报终端内的鼠标滑动事件
    int m_pressedButton = -1;                                           // 按下的鼠标键标识
    int m_lastMouseCol = -1;                                            // 指针前一刻所在的网格列索引
    int m_lastMouseRow = -1;                                            // 指针前一刻所在的网格行索引
    bool m_draggingScrollbar = false;                                   // 指示当前是否正在拖拽垂直滚动条的手柄
    float m_scrollGrabOffset = 0.0f;                                    // 拽住手柄拖动时的垂直起动像素位移偏移量
    ScrollbarAutoHide m_scrollbarAutoHide;                              // 终端右边缘滚动条

    int m_clickCount = 0;                                               // 用于判定双击或三击的计数器
    std::chrono::steady_clock::time_point m_lastClickTime;              // 记录上一次鼠标点击的时间戳
    int m_lastClickCol = -1;                                            // 上一次点击事件所命中的列号
    int m_lastClickRow = -1;                                            // 上一次点击事件所命中的行号

    int m_suppressCharCount = 0;                                        // 过滤/拦截物理输入的计数器
    std::wstring m_imePreedit;                                          // 存储正被 Windows IME 输入法联想编辑中的中文字元暂存串
};

} // namespace CUI
