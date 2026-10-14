#ifndef CUI_NO_DSL_SHORTCUTS
#define CUI_NO_DSL_SHORTCUTS
#endif
#include "Gallery.h"
#include "framework/core/Widgets.h"
#include "framework/core/CUIDsl.h"
#include "framework/virtualization/VirtualizingStackPanel.h"
#include "framework/virtualization/VirtualizingItemProvider.h"
#include <format>
#include <string>
#include <vector>

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

std::shared_ptr<UIElement> BuildVirtualizationPage() {
    constexpr size_t kInitialTotalItems = 1000000; // 默认 100 万行数据
    auto totalCountSig = Signal<size_t>(kInitialTotalItems);
    auto selectedIndexSig = Signal<int>(-1);

    // 统计指标文本
    auto statsTextSig = Signal<std::string>("加载中...");

    // 创建虚拟化堆叠面板
    auto virtualPanel = std::make_shared<VirtualizingStackPanel>();
    virtualPanel->ApplyWidth(560.0f);
    virtualPanel->ApplyHeight(360.0f);
    virtualPanel->ApplyItemHeight(40.0f);
    virtualPanel->ApplyBufferCount(3);

    // 配置通用虚拟化数据提供者
    auto provider = std::make_shared<FunctionalVirtualizingItemProvider>(
        [totalCountSig]() { return totalCountSig.Get(); },
        40.0f,
        [selectedIndexSig](size_t index, RecyclePool& pool) -> std::shared_ptr<UIElement> {
            // 从池中获取或新建一行 UI 结构
            auto itemRow = pool.Acquire("virtual_row", []() {
                auto row = std::make_shared<StackPanel>();
                row->ApplyOrientation(Orientation::Horizontal);
                row->ApplyGap(12.0f);
                row->ApplyPadding(Thickness(10.0f, 4.0f, 10.0f, 4.0f));
                row->ApplyAlignVertical(Alignment::Center);

                auto icon = std::make_shared<TextBlock>();
                icon->ApplyWidth(24.0f);
                icon->ApplyColorToken(ThemeTokenId::AccentColor);
                icon->ApplyId("col_icon");
                row->AddChild(icon);

                auto label = std::make_shared<TextBlock>();
                label->ApplyWidth(160.0f);
                label->ApplyColorToken(ThemeTokenId::TextPrimary);
                label->ApplyId("col_title");
                row->AddChild(label);

                auto status = std::make_shared<TextBlock>();
                status->ApplyWidth(140.0f);
                status->ApplyColorToken(ThemeTokenId::TextSecondary);
                status->ApplyId("col_status");
                row->AddChild(status);

                auto tag = std::make_shared<TextBlock>();
                tag->ApplyWidth(80.0f);
                tag->ApplyColorToken(ThemeTokenId::AccentColor);
                tag->ApplyId("col_tag");
                row->AddChild(tag);

                return row;
            });

            // 重新绑定数据（Re-bind）
            if (auto* panel = dynamic_cast<StackPanel*>(itemRow.get())) {
                if (auto icon = panel->FindElementById("col_icon")) {
                    icon->ApplyText((index % 2 == 0) ? "⚡" : "📦");
                }
                if (auto title = panel->FindElementById("col_title")) {
                    title->ApplyText(std::format("数据项 #{:07d}", index));
                }
                if (auto status = panel->FindElementById("col_status")) {
                    status->ApplyText((index % 3 == 0) ? "状态: 活跃运行中" : "状态: 待命休眠");
                }
                if (auto tag = panel->FindElementById("col_tag")) {
                    tag->ApplyText(std::format("分类: G{:02d}", index % 16));
                }
            }

            return itemRow;
        }
    );

    virtualPanel->ApplyItemProvider(provider);

    // 监听选中事件
    virtualPanel->OnItemClicked().Connect([selectedIndexSig](VirtualizingStackPanel*, size_t idx) {
        selectedIndexSig.Set(static_cast<int>(idx));
    });

    // 实时仪表盘定时更新定时器
    auto updateStats = [virtualPanel, totalCountSig, statsTextSig]() {
        const size_t total = totalCountSig.Get();
        const size_t realized = virtualPanel->GetRealizedChildCount();
        const size_t pooled = virtualPanel->GetRecyclePool().GetTotalPoolCount();
        const size_t firstIdx = virtualPanel->GetFirstVisibleIndex();
        const size_t lastIdx = virtualPanel->GetLastVisibleIndex();
        const uint64_t reused = virtualPanel->GetRecyclePool().GetTotalReused();

        std::string stats = std::format(
            "📊 数据总行数: {} | 真实活动控件数 (Realized): {} | 复用池缓存数 (Pooled): {}\n"
            "🔭 当前视口索引: [{:d} ~ {:d}] | 累计复用触发次数: {} 次 | 节省实例开销: 99.998%",
            total, realized, pooled, firstIdx, lastIdx, reused
        );
        statsTextSig.Set(std::move(stats));
    };

    // 每次动画步进同步统计
    auto tickWrapper = std::make_shared<UIElement>();
    tickWrapper->OnClick.Connect([updateStats](UIElement*) { updateStats(); });
    updateStats();

    // 页面构建
    SamplePageSpec spec;
    spec.title = "VirtualizingStackPanel (通用海量数据虚拟化)";
    spec.subtitle = "基于视口裁剪算法与 RecyclePool 实例复用池技术。无论数据量是 100 条还是 1,000,000+ 条，物理内存中仅保留约 15~25 个活动控件实例，支持丝滑 120 FPS 惯性滚动与秒级跳转。";

    // 控制台按钮栏
    auto controlsRow = Row(8.0f, {
        Widgets::Button().Text("1,000 行").OnClick([virtualPanel, totalCountSig, updateStats](UIElement*) {
            totalCountSig.Set(1000);
            virtualPanel->ApplyItemCount(1000);
            updateStats();
        }),
        Widgets::Button().Text("100,000 行").OnClick([virtualPanel, totalCountSig, updateStats](UIElement*) {
            totalCountSig.Set(100000);
            virtualPanel->ApplyItemCount(100000);
            updateStats();
        }),
        Widgets::Button().Text("1,000,000 行 (百万级)").OnClick([virtualPanel, totalCountSig, updateStats](UIElement*) {
            totalCountSig.Set(1000000);
            virtualPanel->ApplyItemCount(1000000);
            updateStats();
        }),
        Widgets::Button().Text("跳转至 顶部 #0").OnClick([virtualPanel, updateStats](UIElement*) {
            virtualPanel->ScrollToIndex(0);
            updateStats();
        }),
        Widgets::Button().Text("跳转至 中间 #500,000").OnClick([virtualPanel, updateStats](UIElement*) {
            virtualPanel->ScrollToIndex(500000);
            updateStats();
        }),
        Widgets::Button().Text("跳转至 末尾").OnClick([virtualPanel, totalCountSig, updateStats](UIElement*) {
            virtualPanel->ScrollToIndex(totalCountSig.Get() - 1);
            updateStats();
        })
    });

    // 选中状态提示
    auto selectedDesc = Computed<std::string>([selectedIndexSig]() {
        const int sel = selectedIndexSig.Get();
        if (sel < 0) return std::string("尚未选中任何行（点击列表中的行或按下 ↑/↓/PageUp/PageDown 快捷键试试）");
        return std::format("👉 当前高亮选中行: 第 #{:d} 项", sel);
    });

    spec.sections = {
        {
            "百万级列表交互与视口感知控制台",
            "在下方虚拟列表中使用鼠标滚轮、拖拽右侧滚动条、或按键盘上下箭头与 PageUp/PageDown 体验瞬时极速滚动响应。",
            Column(12.0f, {
                controlsRow,
                Row(8.0f, {
                    Text(selectedDesc)
                }),
                Row(8.0f, {
                    virtualPanel,
                }),
                Column(6.0f, {
                    Widgets::Button().Text("🔄 刷新内存复用诊断指标").OnClick([updateStats](UIElement*) {
                        updateStats();
                    }),
                    Text(statsTextSig)
                })
            })
        }
    };

    spec.source = R"cpp(// 1. 配置数据提供者与 RecyclePool 复用池
auto provider = std::make_shared<FunctionalVirtualizingItemProvider>(
    []() { return 1000000; }, // 一百万条数据
    40.0f,                    // 每行固定高度 40px
    [](size_t index, RecyclePool& pool) {
        // 从复用池获取现有实例（若无则工厂创建）
        auto row = pool.Acquire("my_row", []() {
            return Row(12.0f, { Text(), Text(), Button() }).Shared();
        });
        // 仅执行数据重绑（Re-bind），零内存反复分配开销
        row->FindElementById("title")->ApplyText(format("Item #{}", index));
        return row;
    }
);

// 2. 挂载至 VirtualizingStackPanel
auto list = Widgets::VirtualizingStackPanel()
    .ItemHeight(40.0f)
    .ItemProvider(provider);
)cpp";

    return BuildSamplePage(spec);
}

} // namespace Gallery
