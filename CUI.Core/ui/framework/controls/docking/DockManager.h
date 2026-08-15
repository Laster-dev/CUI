#pragma once

#include "../UIElement.h"
#include "../../animation/AnimationSystem.h"
#include "DockTypes.h"
#include "DockFloatWindow.h"
#include <memory>
#include <string>
#include <vector>
#include <windows.h>

namespace CUI {

class Window;

/**
 * @brief 自绘的 Visual Studio 风格高级停靠窗体面板管理器（DockManager）。
 * 本控件接管了所有的停靠逻辑：
 * 1. **分栏区域布局**：管理主界面左、右、顶、底四停靠边以及中央文档区。
 * 2. **选项卡标签渲染**：支持多个 Panel 在一个槽位成组（DockTabGroup）并通过 Tab 页签切换展示。
 * 3. **自由拖动与悬浮**：支持将任意 Pane 拖出拆分为独立的悬浮多子窗口（DockFloatWindow），并在边缘提供多向吸附箭头引导（Dock Guides）。
 * 4. **自动收拢隐藏（AutoHide）**：支持将 Pane 收叠为边缘窄条（AutoHide Strip），指针 Hover 时以动画形式平滑抽出。
 */
class DockManager : public UIElement {
public:
    DockManager();
    ~DockManager() override;

    const char* GetClassName() const override { return "DockManager"; } // 获取类名

    void SetOwnerWindow(Window* window) { m_ownerWindow = window; } // 注册所归属的主容器窗口
    Window* GetOwnerWindow() const { return m_ownerWindow; } // 获取所归属的主容器窗口

    int AddToolPane(const std::string& title, std::shared_ptr<UIElement> content, DockSide side); // 注入一个新的工具侧边栏卡片并指定初始停靠方位，返回唯一索引
    int AddDocument(const std::string& title, std::shared_ptr<UIElement> content); // 注入一个工作区文档卡片，并默认停靠在中央文档区
    void ClosePane(int paneIndex); // 彻底关闭销毁指定索引号的窗体卡片
    void FloatPane(int paneIndex, Point screenDipTopLeft = Point()); // 将指定卡片从排版树剥离，直接提升创建为一块原生悬浮子窗体
    void DockPane(int paneIndex, DockSide side); // 将特定卡片强制重新停靠收拢到指定侧沿
    void SetPaneAutoHide(int paneIndex, bool autoHide); // 设置是否让特定卡片在失去焦点后自动以缩折方式隐藏至边缘窄条

    int FindPaneIndexByTitle(const std::string& title) const; // 通过标题文本查找对应窗体卡片的队列索引位置（找不到返回-1）
    const DockPaneData* GetPane(int index) const; // 获得指定索引号对应的原始卡片状态数据明细
    int GetPaneCount() const { return static_cast<int>(m_panes.size()); } // 获取当前受托管理的总面板数

    void SetSideSize(DockSide side, float size); // 强制变更指定方位（左、右、顶、底）侧栏所分得的初始物理像素尺寸大小
    float GetSideSize(DockSide side) const; // 查询指定方位的侧栏物理占位像素尺寸大小

    bool SaveLayout(const std::wstring& path) const; // 将当前的停靠、折叠及悬浮分栏布局结构序列化保存至本地 XML 配置文件
    bool LoadLayout(const std::wstring& path); // 从本地配置文件中反序列化并还原停靠、折叠及悬浮的布局结构

    Size Measure(Size availableSize) override; // 进行多区域综合拆分和子控件流排版测量
    void Arrange(Rect finalRect) override; // 依次对左、顶、右、底及文档各面板区进行对齐，并将子级窗体摆放到位
    void OnRender(GraphicsContext& ctx) override; // 绘制边框线、选项夹背景、分隔条抓手以及折叠窄条外观
    void RenderOverlay(GraphicsContext& ctx) override; // 绘制处于最顶层的九宫格吸附定位罗盘 Guides 以及拖拽半透明幻影 Ghost 框
    UIElement* HitTest(float x, float y) override; // 命中碰撞测试
    UIElement* HitTestOverlay(float x, float y) override; // 悬浮吸附层命中碰撞测试
    void OnMouseDown(Point pt) override; // 鼠标按下，开始拖动分隔分割线或准备剥离选项夹进行 Float 悬浮
    void OnMouseMove(Point pt) override; // 鼠标移动，更新吸附预览、更改调节边栏大小或悬停选项夹
    void OnMouseUp(Point pt) override; // 鼠标松开，确定完成吸附停靠重排或释放拖拽
    void OnMouseLeave() override; // 鼠标离去，清空各项卡片 Hover 焦点标记
    HCURSOR GetCursor() const override; // 获取悬浮交互鼠标样式（依据所滑过的分隔条或按键返回对应鼠标指针）
    bool OnAnimationTick() override; // 驱动各边栏隐藏滑出、罗盘 Guides 淡入淡出动画帧步进
    bool HasSelfAnimation() const override; // 检查是否依然有面板过渡动画在活动中
    void OnMouseWheel(float delta) override; // 响应滚轮，可在选项标签条超出长限制时进行横向横向平移滚动

    void NotifyFloatClosed(DockFloatWindow* wnd); // 悬浮子窗口被销毁时回调，注销释放对应子窗口句柄
    Point LocalToScreenDip(Point local) const; // 将局部坐标转化为 DPI 缩放后的屏幕绝对位置点坐标
    Point ScreenDipToLocal(Point screenDip) const; // 将高精 DPI 屏幕绝对坐标反转为局部二维坐标点

    void BeginFloatRedock(int paneIndex); // 启动拖拽中的悬浮窗体边缘重新吸附（Redock）测试流程
    void UpdateFloatRedock(Point screenDip); // 拖动高频更新边缘吸附测试，判定鼠标落入哪个吸附罗盘热区
    bool CompleteFloatRedock(Point screenDip); // 拖拽释放，将卡片顺利合入罗盘所指定的那个物理边侧
    void CancelFloatRedock(); // 取消拖拽吸附操作
    ::HWND OwnerHwnd() const; // 获取宿主主窗口的 ::HWND 句柄
    void InvalidateOwner(); // 强行让主窗口失效，发出画面重绘请求

private:
    friend class DockLayoutSerializer;
    friend class DockFloatWindow;

    /**
     * @brief 标记点击或 Hover 在停靠栏中的具体分部细小区域。
     */
    enum class HitPart : uint8_t {
        None = 0,         // 无
        Tab,              // 页签选项标签
        Pin,              // 自动折叠隐藏大头针钉纽
        Close,            // 关闭叉叉按钮
        Header,           // 窗体头装饰栏
        Splitter,         // 可拖拽比例切分条
        AutoHide,         // 自动收起窄边条
        Content,          // 窗体正文内容
        TabScrollLeft,    // 标签条左翻页微调小箭头
        TabScrollRight    // 标签条右翻页微调小箭头
    };

    struct HitResult {
        HitPart part = HitPart::None; // 命中的细部零件
        DockSide side = DockSide::None; // 命中零件所在的停靠大边侧（左、右、顶、底、中）
        int groupPaneLocal = -1;       // 在该页签组内的相对行号索引
        int paneIndex = -1;            // 在主面板管理器队列中的绝对全局索引
        int splitter = -1;             // 命中的分割条编号
    };

    struct SlotGeom {
        Rect outer;                 // 当前侧位占用的最外围绝对矩形边界
        Rect header;                // 侧位面板头矩形边界
        Rect content;               // 侧位面板正文有效摆放区域矩形边界
        Rect tabStrip;              // 侧位底部或顶部选项标签条占位矩形
        Rect pinBtn;                // 钉扣小按钮的逻辑区域
        Rect closeBtn;              // 关闭小按钮的逻辑区域
        Rect scrollLeft;            // 标签组左平移按钮的逻辑区域
        Rect scrollRight;           // 标签组右平移按钮的逻辑区域
        std::vector<Rect> tabs;     // 各个页签标签分别占用的逻辑矩形区域序列
        float totalTabsWidth = 0.0f; // 标签全尺寸渲染累计总像素宽度
        bool showScroll = false;     // 指示当前标签选项条是否太长需要展现平移微调左右按钮
        bool visible = false;        // 指示当前侧位是否真的包含有显示内容并有处于激活状态的 Pane
    };

    struct LayoutGeom {
        SlotGeom left, right, top, bottom, center; // 五大分区的具体几何测算参数
        Rect splitL, splitR, splitT, splitB;       // 四边对应的虚拟切分调拖拽热区矩形
        Rect visSplitL, visSplitR, visSplitT, visSplitB; // 四边分割线实际屏幕可视线段的矩形边界
        float strip = 24.0f;                       // 隐藏窄收纳带默认的分配厚度 (px)
    };

    struct SideChromeAnim {
        AnimatedScalar underlineX{ 0.0f }; // 选中 Tab 页签底部高亮蓝色下滑指示线的平移动画 X 轴位置
        AnimatedScalar underlineW{ 0.0f }; // 选中 Tab 下划线的长度伸展过渡动画
        AnimatedScalar contentFade{ 1.0f }; // 切换面板时的正文淡入淡出动画透明度 (0.0f - 1.0f)
        bool underlineInited = false;       // 标识下划线的初始定位参数是否已载入就绪
    };

    static constexpr float kHeaderH = 28.0f;         // 窗体头部最小限定高度值 (px)
    static constexpr float kSplitHit = 5.0f;         // 拖拽分隔条的命中热区粗细范围 (px)
    static constexpr float kDragThreshold = 6.0f;    // 触发鼠标拖拽起飞进入悬浮状态的最小像素拖移死区 (px)
    static constexpr float kMinSide = 80.0f;         // 侧边分栏所允许拉伸调节的最小极限像素尺寸
    static constexpr float kMinCenter = 120.0f;      // 中央文档区所被迫保证保留的最小像素尺寸
    static constexpr float kTabMinW = 64.0f;         // 页签选项标签渲染的最小限定宽度值 (px)
    static constexpr float kTabMaxW = 160.0f;        // 页签选项标签渲染的最大限制宽度值 (px)
    static constexpr float kTabPadX = 10.0f;         // 页签文字两侧留白 (px)
    static constexpr float kChromeBtn = 20.0f;       // 窗体头右上角小叉、钉扣等小按钮的像素物理大小
    static constexpr float kScrollBtn = 18.0f;       // 选项条平移微调按钮的像素物理大小
    static constexpr float kAutoHideStrip = 24.0f;   // 侧边隐藏状态窄条分配占位 (px)

    std::string MakePaneId(); // 唯一生成字符串 PaneID 的流水线助手
    DockTabGroup& SlotGroup(DockSide side); // 辅助读取对应方位的页签选项组引用
    const DockTabGroup& SlotGroup(DockSide side) const;
    SideChromeAnim& SideAnim(DockSide side); // 读取对应方位的平移动画状态器
    const SideChromeAnim& SideAnim(DockSide side) const;
    DockSide SideOfPane(int paneIndex) const; // 查询指定卡片当前的归宿停靠方位（悬浮返回 None）
    void RemovePaneFromAllSlots(int paneIndex); // 从所有的侧边和页签组中移除释放此卡片
    void AddPaneToSlot(int paneIndex, DockSide side, bool select = true); // 强制往指定的侧位组里插入一张卡片
    void SyncContentChildren(); // 同步并重建 UIElement 基类的 m_children 以和目前的显示卡片严格保持一致
    void RelayoutContents(); // 使布局变脏，迫使下一次 Relayout 重新计算
    void ApplyLayoutNow(); // 立即执行全分区布局位置更新
    void EnsureTabVisible(DockSide side); // 确保当前方位的页签激活选项能够显示在视口中（自动微调滑动偏移）
    void SyncSideUnderline(DockSide side, bool jump); // 同步指定方位页签底部的蓝色滑动指示线的动画目标值
    void JumpAllUnderlines(); // 瞬移各侧边下划线，免去在初始页面生成时的长位移动效
    void BeginContentFade(DockSide side); // 在更换卡片选项页时触发正文淡入动效
    void ApplyContentFadeOpacities(); // 应用淡入渐变透明度参数至渲染节点树
    float MeasureTabWidth(const std::string& title) const; // 测算单个标签在指定标题文本下的理想渲染大小
    bool HasAutoHideOn(DockSide side) const; // 验证该方位当前是否确实注册有折缩隐藏的 AutoHide 面板
    void AutoHideStripInsets(float& left, float& top, float& right, float& bottom) const; // 计算因为隐藏窄边条存在造成的界面客户区内缩值
    float MeasureAutoHideTabExtent(const std::string& title) const; // 测算窄条上的标签尺寸
    float AutoHideTabOrigin(DockSide side, int indexOnSide) const; // 计算窄条上第 index 个标签的相对位置起点坐标
    Rect AutoHideTabRect(DockSide side, int indexOnSide, const std::string& title) const; // 计算窄条上标签的逻辑绘制矩形
    void DrawAutoHideLabel(GraphicsContext& ctx, const Rect& btn, const std::string& title,
                           DockSide side, D2D1_COLOR_F color) const; // 在侧边隐藏窄带上自绘出垂直旋转后的页签说明文字
    DockSide PeekSide() const; // 读取当前临时滑出展开的侧边方位
    Rect PeekOuterRect() const; // 读取临时滑出的卡片最外围绝对矩形边界
    SlotGeom MakePeekGeom() const; // 估算临时滑出卡片的排版数据
    void ShowPeek(int paneIndex); // 临时将处于 AutoHide 收缩态的指定卡片平滑滑出展现
    void HidePeek(); // 将滑出展开的侧卡平滑收回隐藏
    void DrawPeek(GraphicsContext& ctx) const; // 绘制临时滑出卡片的背景板及阴影
    LayoutGeom ComputeGeom(const Rect& bounds); // 综合运算并确定左、顶、右、底及文档等五大分区的全部尺寸与包络范围
    void FillSlotGeom(SlotGeom& slot, DockTabGroup& group, DockSide side, Rect outer); // 测算特定侧边分区在剩余空间中的具体排版详情参数
    HitResult HitTestChrome(float x, float y) const; // 碰撞测试定位坐标落在哪个分部的标签、关闭叉、钉子或分割条上
    Rect GuideHot(DockDropKind kind, const Rect& host) const; // 罗盘定位上对应具体定位向热区位置 (px)
    DockDropKind HitTestDrop(float x, float y) const; // 计算拖拽中的高精鼠标指针落在吸附罗盘的哪个方向箭头上
    void BeginDrag(int paneIndex, Point pt); // 对某张卡片开启拖拽吸附状态
    void UpdateDrag(Point pt); // 拖拽中，平移 Ghost 幻影框或弹出吸附罗盘
    void EndDrag(Point pt); // 结束拖拽并重新停靠或创建为悬浮窗口
    void CancelDrag(); // 取消当前的拖动吸附动作
    void DrawSlot(GraphicsContext& ctx, DockSide side, const SlotGeom& g) const; // 自绘出特定分区的页签、关闭和图层背景
    void DrawGuides(GraphicsContext& ctx) const; // 自绘九宫格边缘吸附罗盘图标
    void DrawDragGhost(GraphicsContext& ctx) const; // 自绘淡蓝色的拖拽预览吸附幻影 Ghost 框
    void DrawAutoHideStrips(GraphicsContext& ctx) const; // 自绘四边缩折窄条
    void DrawChromeButtonBg(GraphicsContext& ctx, const Rect& r, float hoverT, bool danger) const; // 自绘关闭或图钉小按钮的悬浮高亮圆圈背景
    void DrawCloseGlyph(GraphicsContext& ctx, const Rect& r, D2D1_COLOR_F color) const; // 自绘关闭叉图标
    void DrawPinGlyph(GraphicsContext& ctx, const Rect& r, D2D1_COLOR_F color, bool autoHide) const; // 自绘大头针图钉图标
    int SelectedPaneOf(const DockTabGroup& g) const; // 查找该组内目前被激活显示的内容绝对行号索引
    void SelectInGroup(DockSide side, int localIndex); // 切换该停靠边组内的活跃标签页项
    void CloseFloatForPane(int paneIndex); // 销毁与特定卡片绑定的悬浮窗体实例
    const SlotGeom* SlotGeomFor(DockSide side) const; // 获取对应停靠边的排版结果参数

    Window* m_ownerWindow = nullptr;                                    // 指向归属主窗口 Window 的指针
    std::vector<DockPaneData> m_panes;                                  // 所有拖入管理的 Pane 静态数据明细队列
    DockTabGroup m_left, m_right, m_top, m_bottom, m_center;            // 五大分区对应的页签组成员
    std::vector<DockAutoHideItem> m_autoHide;                           // 四边折缩隐藏窄边上的条目队列
    std::vector<std::unique_ptr<DockFloatWindow>> m_floats;             // 包含的所有悬浮子窗口句柄强引用

    float m_leftSize = 220.0f;                                          // 左侧栏占用物理像素尺寸宽 (px)
    float m_rightSize = 240.0f;                                         // 右侧栏占用物理像素尺寸宽 (px)
    float m_topSize = 140.0f;                                           // 顶部栏占用物理像素尺寸高 (px)
    float m_bottomSize = 150.0f;                                        // 底部栏占用物理像素尺寸高 (px)

    LayoutGeom m_geom{};                                                // 包含所有的分块排版结果快照
    HitResult m_hover{};                                                // 鼠标当前高亮悬停的零件位置快照

    bool m_dragArmed = false;                                           // 拖拽起飞前的准备阶段标记
    bool m_dragging = false;                                            // 正处于拖拽调节或剥离悬浮状态中
    int m_dragPane = -1;                                                // 拖拽的目标卡片队列索引
    Point m_dragStart{};                                                // 拖动开始时鼠标的高精物理点击点
    Point m_dragPt{};                                                   // 拖拽中鼠标当前移动的位置
    DockDropKind m_dropHighlight = DockDropKind::None;                  // 鼠标已滑入吸附罗盘的目标方位指示
    AnimatedScalar m_guideOpacity{ 0.0f };                              // 吸附罗盘淡入淡出的动画时钟
    AnimatedScalar m_dropPulse{ 0.0f };                                 // 吸附指示虚影框的边缘脉冲渐变动画时钟

    SideChromeAnim m_animLeft, m_animRight, m_animTop, m_animBottom, m_animCenter; // 各自页签下划线滑动状态动画器
    AnimatedScalar m_hoverPin{ 0.0f };                                  // 图钉小按钮高亮淡入动画进度
    AnimatedScalar m_hoverClose{ 0.0f };                                // 关闭小按钮高亮淡入动画进度
    AnimatedScalar m_hoverScrollL{ 0.0f };                              // 左滑动按钮高亮淡入动画进度
    AnimatedScalar m_hoverScrollR{ 0.0f };                              // 右滑动按钮高亮淡入动画进度
    DockSide m_hoverBtnSide = DockSide::None;                           // 指示当前悬停的高亮小按钮是在哪个停靠大边上

    int m_activeSplitter = -1;                                          // 当前正被拖动尺寸微调的分隔条编号
    float m_splitStartSize = 0.0f;                                      // 拖拽开始时该分割条所调节的分区初始大小 (px)
    Point m_splitStartPt{};                                             // 分割条拖动开始时的鼠标坐标
    int m_peekPane = -1;                                                // 当前平滑抽出来、展出预览中的折叠卡片队列索引

    unsigned long long m_nextPaneId = 1;                                // 下一个要分配卡片的递增唯一 ID 流水号
};

} // namespace CUI
