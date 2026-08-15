#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"
#include "framework/controls/ScrollViewer.h"
#include "framework/controls/Button.h"
#include "framework/controls/ToggleButton.h"
#include "framework/controls/TextBox.h"
#include <format>
#include <memory>
#include <vector>

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

/**
 * @brief 构建 ScrollViewer (滚动视图) 展示页面。
 * 遵循 Fluent 规范，全弹性流式布局，不硬编码固定宽度。
 */
Element BuildScrollViewerPage() {
    auto statusLabel = MakeStatus("提示：使用鼠标滚轮体验 Chromium 级贝塞尔丝滑减速阻尼；或单击鼠标中键启用自动漫游滚动。");

    // ==========================================
    // 示例 1: 经典长列表平滑滚动容器
    // ==========================================
    auto scrollViewer1 = ScrollViewerWidget()
        .Height(280.0f)
        .Background(D2D1::ColorF(0x141416, 0.4f))
        .Border(D2D1::ColorF(0x3F3F46, 0.5f), 1.0f)
        .CornerRadius(8.0f)
        .Build();

    // 构造 15 张长图文卡片
    auto listContent = Column(8);
    listContent.Padding(12.0f);

    struct CardData {
        const char* icon;
        const char* title;
        const char* desc;
        const char* tag;
    };

    std::vector<CardData> cards = {
        { "⚡", "Chromium 物理平滑滚动", "内置 EaseInOut 三次贝塞尔曲线与高精度 Newton-Raphson 步进求解器，杜绝卡顿与画面撕裂。", "物理动画" },
        { "🖱️", "鼠标中键自动平移 (Auto-Scroll)", "按下鼠标滚轮中键即可在视口出现方向罗盘指示器，随光标距离自动加减速漫游滚动。", "高级交互" },
        { "🎨", "自动收起与悬浮滚动条 (AutoHide)", "滚动条常态下自动透明渐隐，鼠标靠近轨道或滚轮滑动时平滑淡入，最大化利用内容宽度。", "Fluent 动效" },
        { "🚀", "零卡顿局部脏矩形剪裁", "针对超长列表仅渲染视口内可见图元，极低 CPU 与 GPU 显存开销。", "高性能" },
        { "📐", "全弹性流式排布 (Flex Layout)", "子内容自适应父级容器可用宽度，不硬编码固定像素值。", "响应式" },
        { "🛡️", "边界弹性碰撞阻尼与回弹", "滚到最顶或最底时平滑阻尼收敛，提供舒适的触觉视觉反馈。", "物理反馈" },
        { "📄", "文档与富文本无缝集成", "完美承载大型代码编辑器、Markdown 查看器或海量日志监视流。", "通用容器" },
        { "🧩", "嵌套滚动与事件链穿透", "支持在内部嵌套图表、拓扑图或滑块控件，滚动事件智能路由分发。", "事件管线" },
        { "🌙", "深浅色主题自适应", "滚动条滑块与轨道自动拾取系统高对比度色彩 Token，契合暗色/浅色模式。", "现代主题" },
        { "⌨️", "键盘焦点与 PageUp/PageDown", "支持键盘上下箭头、Home、End 与空格键快速翻页。", "无障碍" },
        { "🖥️", "高分屏 DPI 亚像素吸附", "在 4K 150%/200% 缩放比例下，滚动条边框与滑块保持像素完美锐利对齐。", "HiDPI" },
        { "📊", "复杂数据流看板", "可无缝承载数十列表格与实时动态图表矩阵。", "数据看板" },
        { "⚙️", "可配置浮层覆盖模式 (Overlay)", "切换滚动条是否挤占内容宽度，保障左右对齐元素绝对居中。", "排版模式" },
        { "🏷️", "版本历程与更新日志", "记录框架各迭代周期的特性与性能演进。", "版本管理" },
        { "🏁", "已到达内容底部", "您已完整浏览所有滚动列表项。", "完成" },
    };

    for (size_t i = 0; i < cards.size(); ++i) {
        const auto& c = cards[i];
        auto itemCard = Row(10, {
            Text(c.icon).FontSize(16.0f),
            Column(2, {
                Row(6, {
                    Text(std::format("{}. {}", i + 1, c.title)).FontWeight(FontWeight::SemiBold).FontSize(12.0f),
                    Text(std::format("[{}]", c.tag)).FontSize(10.0f).Foreground(D2D1::ColorF(0x38BDF8, 1.0f)),
                }),
                Text(c.desc).FontSize(11.0f).Foreground(D2D1::ColorF(0x94A3B8, 1.0f)),
            }),
        })
        .Padding(8.0f)
        .Background(D2D1::ColorF(0x1F1F23, 0.6f))
        .Border(D2D1::ColorF(0x333338, 0.4f), 1.0f)
        .CornerRadius(6.0f);

        listContent->AddChild(itemCard.Build());
    }

    scrollViewer1->AddChild(listContent.Build());

    // 控制按钮：滚动至顶部 / 中间 / 底部
    auto btnScrollTop = Button("⬆️ 滚至顶部 (Top)")
        .OnClick([scrollViewer1, statusLabel](UIElement*) {
            scrollViewer1->SetScrollOffsetY(0.0f);
            statusLabel->Text = "已平滑滚动至容器【最顶部】。";
        });

    auto btnScrollMid = Button("↕️ 滚至中间 (50%)")
        .OnClick([scrollViewer1, statusLabel](UIElement*) {
            scrollViewer1->SetScrollOffsetY(400.0f);
            statusLabel->Text = "已平滑滚动至内容【中间位置】(Offset: 400px)。";
        });

    auto btnScrollBottom = Button("⬇️ 滚至底部 (Bottom)")
        .OnClick([scrollViewer1, statusLabel](UIElement*) {
            scrollViewer1->SetScrollOffsetY(2000.0f);
            statusLabel->Text = "已平滑滚动至容器【最底部】。";
        });

    auto toggleOverlay = ToggleButtonWidget("浮层滚动条模式 (Overlay Scrollbar)");
    toggleOverlay->OnClick.Connect([scrollViewer1, statusLabel](UIElement* sender) {
        auto btn = dynamic_cast<ToggleButton*>(sender);
        bool isOverlay = btn && btn->IsChecked();
        scrollViewer1->SetOverlayScrollbar(isOverlay);
        statusLabel->Text = isOverlay
            ? "浮层滚动条模式已【开启】：滚动条浮在内容上方，不挤占排版宽度。"
            : "浮层滚动条模式已【关闭】：滚动条保留专属轨道空间。";
    });

    SamplePageSpec spec;
    spec.title = "ScrollViewer (滚动视图)";
    spec.subtitle = "高帧率丝滑滚动容器，内置 Chromium 贝塞尔缓动物理阻尼、鼠标中键漫游、滚动条自动淡隐与浮层排版模式。";
    spec.sections = {
        {
            "长图文内容平滑滚动 (Chromium 缓动 · 鼠标中键自动漫游)",
            "在下方视口内滚动滚轮体验减速阻尼感；或单击滚轮中键拖移光标体验自动漫游。点击上方按钮可执行代码控制的平滑滚动跳转。",
            Column(12, {
                Row(8, { btnScrollTop, btnScrollMid, btnScrollBottom, toggleOverlay }),
                scrollViewer1,
                statusLabel,
            }),
        },
    };

    spec.source = R"cpp(// 1. 创建滚动视图并设定视口高度
auto scrollViewer = ScrollViewerWidget()
    .Height(280.0f)
    .Build();

// 2. 构造任意长图文、列表或代码内容
auto content = Column(8);
for (int i = 0; i < 20; ++i) {
    content->AddChild(CreateItemCard(i));
}

// 3. 将超长内容放入滚动容器中
scrollViewer->AddChild(content.Build());

// 4. 代码控制滚动位置
scrollViewer->SetScrollOffsetY(0.0f);      // 滚至顶部
scrollViewer->SetOverlayScrollbar(true);    // 浮层滚动条模式
)cpp";

    return BuildSamplePage(spec);
}

} // namespace Gallery
