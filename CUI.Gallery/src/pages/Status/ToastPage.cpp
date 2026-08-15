#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"
#include "framework/controls/Toast.h"
#include "framework/controls/Button.h"
#include <format>
#include <memory>

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

/**
 * @brief 构建 Toast (全局应用内通知) 展示页面。
 * 遵循 Fluent 规范，全弹性流式布局，支持多角落定位与多状态类型。
 */
Element BuildToastPage() {
    auto statusLabel = MakeStatus("提示：点击下方按钮触发系统级应用内浮层通知，自带平滑入场/退场动效与鼠标悬停暂停计时。");

    static ToastCorner s_corner = ToastCorner::TopRight;

    auto btnSuccess = Button("✅ 成功提示 (Success)")
        .OnClick([statusLabel](UIElement* sender) {
            Toast::Show(sender, "保存成功", "项目工作区与依赖项已成功同步至本地缓存。", ToastType::Success, s_corner, 3000);
            statusLabel->Text = "已触发【成功通知】(Success Toast)";
        });

    auto btnInfo = Button("ℹ️ 信息通知 (Info)")
        .OnClick([statusLabel](UIElement* sender) {
            Toast::Show(sender, "系统通知", "Direct2D 硬件加速图层已自动启用，视口帧率锁定 60 FPS。", ToastType::Info, s_corner, 3000);
            statusLabel->Text = "已触发【信息通知】(Info Toast)";
        });

    auto btnWarning = Button("⚠️ 警告提示 (Warning)")
        .OnClick([statusLabel](UIElement* sender) {
            Toast::Show(sender, "磁盘容量告警", "检测到临时缓存目录剩余空间不足 2GB，请及时清理。", ToastType::Warning, s_corner, 3500);
            statusLabel->Text = "已触发【警告通知】(Warning Toast)";
        });

    auto btnError = Button("❌ 错误报告 (Error)")
        .OnClick([statusLabel](UIElement* sender) {
            Toast::Show(sender, "连接超时", "无法建立与远程服务节点的 WebSocket 握手，错误代码: 0x80070005。", ToastType::Error, s_corner, 4000);
            statusLabel->Text = "已触发【错误通知】(Error Toast)";
        });

    // 方位切换
    auto btnTR = Button("右上角 (TopRight)")
        .OnClick([statusLabel](UIElement*) {
            s_corner = ToastCorner::TopRight;
            statusLabel->Text = "弹出方位已设为：【右上角 (TopRight)】";
        });

    auto btnBR = Button("右下角 (BottomRight)")
        .OnClick([statusLabel](UIElement*) {
            s_corner = ToastCorner::BottomRight;
            statusLabel->Text = "弹出方位已设为：【右下角 (BottomRight)】";
        });

    auto btnTL = Button("左上角 (TopLeft)")
        .OnClick([statusLabel](UIElement*) {
            s_corner = ToastCorner::TopLeft;
            statusLabel->Text = "弹出方位已设为：【左上角 (TopLeft)】";
        });

    auto btnBL = Button("左下角 (BottomLeft)")
        .OnClick([statusLabel](UIElement*) {
            s_corner = ToastCorner::BottomLeft;
            statusLabel->Text = "弹出方位已设为：【左下角 (BottomLeft)】";
        });

    SamplePageSpec spec;
    spec.title = "Toast (应用内全局通知)";
    spec.subtitle = "轻量级非模态浮动消息通知系统，支持四方位角吸附、多通知自动垂直排队层叠、倒计时自动淡出与鼠标悬停暂停。";
    spec.sections = {
        {
            "预设通知类型演示",
            "点击下方各类型按钮，在当前选定的屏幕角落触发带有 Fluent 亚克力光效与主题配色的浮动通知卡片。",
            Column(12, {
                Row(8, { btnSuccess, btnInfo, btnWarning, btnError }),
            }),
        },
        {
            "通知弹出方位角配置",
            "支持将全局 Toast 通知投递到主视口的四个不同角落，同侧多条通知将自动计算间距并垂直平滑层叠。",
            Column(12, {
                Row(8, { btnTR, btnBR, btnTL, btnBL }),
                statusLabel,
            }),
        },
    };

    spec.source = R"cpp(// 静态方法一键触发全局应用内通知
Toast::Show(
    targetElement,
    "构建成功",
    "所有模块已通过静态分析与单元测试。",
    ToastType::Success,
    ToastCorner::TopRight,
    3000 // 自动显示 3000ms 后平滑淡出
);
)cpp";

    return BuildSamplePage(spec);
}

} // namespace Gallery
