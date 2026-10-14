#ifndef CUI_NO_DSL_SHORTCUTS
#define CUI_NO_DSL_SHORTCUTS
#endif
#include "Gallery.h"
#include "framework/core/Widgets.h"
#include "framework/core/CUIDsl.h"
#include "framework/reactive/Signal.h"
#include <format>
#include <string>
#include <vector>

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

std::shared_ptr<UIElement> BuildReactivityPage() {
    // ----------------------------------------------------
    // 1. 基础信号与计算属性状态
    // ----------------------------------------------------
    Signal<int> count(0);
    Computed<std::string> countDescription([count]() {
        return std::format("当前计数值: {} (两倍值: {})", count.Get(), count.Get() * 2);
    });

    // ----------------------------------------------------
    // 2. 条件渲染状态
    // ----------------------------------------------------
    Signal<bool> showAdvanced(false);

    // ----------------------------------------------------
    // 3. 动态列表渲染状态
    // ----------------------------------------------------
    struct TodoItem {
        int id;
        std::string title;
        bool done;
        bool operator==(const TodoItem&) const = default;
    };
    Signal<std::vector<TodoItem>> todoList(std::vector<TodoItem>{
        { 1, "探索 CUI 响应式信号机制 (Signal)", true },
        { 2, "体验惰性衍生计算属性 (Computed)", true },
        { 3, "使用 If / ForEach 声明式控制流", false },
        { 4, "构建现代高性能原生桌面 UI", false }
    });
    Signal<int> nextTodoId(5);

    // ----------------------------------------------------
    // 4. 多分支选择状态 (Switch)
    // ----------------------------------------------------
    Signal<int> statusTab(0); // 0: 全部, 1: 进行中, 2: 已完成

    // ----------------------------------------------------
    // 界面搭建
    // ----------------------------------------------------
    SamplePageSpec spec;
    spec.title = "Reactivity & Signal (响应式与控制流)";
    spec.subtitle = "基于原子状态图的细粒度响应式系统（Signal / Computed / Effect / Batch），结合现代声明式控制流原语（If / ForEach / Switch），实现零虚拟 DOM、无冗余测算的极致性能响应式体验。";

    // 计数器交互面板
    auto counterSection = Column(12.0f, {
        Row(12.0f, {
            Widgets::Button().Text("递增 +1").OnClick([count](UIElement*) {
                count.Set(count.Get() + 1);
            }),
            Widgets::Button().Text("递减 -1").OnClick([count](UIElement*) {
                count.Set(count.Get() - 1);
            }),
            Widgets::Button().Text("批量递增 +10 (Batch)").OnClick([count](UIElement*) {
                Batch([count]() {
                    for (int i = 0; i < 10; ++i) {
                        count.Set(count.Get() + 1);
                    }
                });
            }),
            Widgets::Button().Text("重置为 0").OnClick([count](UIElement*) {
                count.Set(0);
            })
        }),
        Row(8.0f, {
            Text("【响应式文本绑定】: "),
            Text(countDescription)
        })
    });

    // 条件渲染展示面板
    auto ifSection = Column(12.0f, {
        Row(12.0f, {
            Widgets::Button().Text("切换高级选项显示/隐藏").OnClick([showAdvanced](UIElement*) {
                showAdvanced.Set(!showAdvanced.Get());
            })
        }),
        If(showAdvanced,
            []() {
                return Column(8.0f, {
                    Text("💡 恭喜展开高级选项！当前使用的是 If(Signal<bool>, ThenFn, ElseFn) 动态挂载机制。"),
                    Text("当条件为 true 时才会动态构建当前分支，条件为 false 时子树完全解离并释放绑定的 Effect，无内存泄漏与静默运行代价。")
                });
            },
            []() {
                return Text("（高级选项当前处于折叠状态，点击上方按钮即可展开）");
            }
        )
    });

    // 动态列表渲染展示面板
    auto forEachSection = Column(12.0f, {
        Row(12.0f, {
            Widgets::Button().Text("添加待办任务").OnClick([todoList, nextTodoId](UIElement*) {
                int id = nextTodoId.Get();
                nextTodoId.Set(id + 1);
                auto items = todoList.Get();
                items.push_back({ id, std::format("新任务 #{}", id), false });
                todoList.Set(std::move(items));
            }),
            Widgets::Button().Text("清除最后一项").OnClick([todoList](UIElement*) {
                auto items = todoList.Get();
                if (!items.empty()) {
                    items.pop_back();
                    todoList.Set(std::move(items));
                }
            })
        }),
        ForEach(todoList, [todoList](const TodoItem& item, size_t index) {
            return Row(10.0f, {
                Text(std::format("#{}: {}", item.id, item.title)),
                Text(item.done ? " [✓ 已完成]" : " [⏳ 待处理]"),
                Widgets::Button().Text(item.done ? "标为未完成" : "标为已完成").OnClick([todoList, item](UIElement*) {
                    auto items = todoList.Get();
                    for (auto& it : items) {
                        if (it.id == item.id) {
                            it.done = !it.done;
                            break;
                        }
                    }
                    todoList.Set(std::move(items));
                }),
                Widgets::Button().Text("删除").OnClick([todoList, item](UIElement*) {
                    auto items = todoList.Get();
                    std::erase_if(items, [&](const TodoItem& it) { return it.id == item.id; });
                    todoList.Set(std::move(items));
                })
            });
        }, Orientation::Vertical, 8.0f)
    });

    // 多分支展示面板
    auto switchSection = Column(12.0f, {
        Row(12.0f, {
            Widgets::Button().Text("分支 A").OnClick([statusTab](UIElement*) { statusTab.Set(0); }),
            Widgets::Button().Text("分支 B").OnClick([statusTab](UIElement*) { statusTab.Set(1); }),
            Widgets::Button().Text("分支 C").OnClick([statusTab](UIElement*) { statusTab.Set(2); })
        }),
        Switch(statusTab)
            .Case(0, []() { return Text("当前激活：【分支 A】 - 实时监控核心系统状态"); })
            .Case(1, []() { return Text("当前激活：【分支 B】 - 性能剖析与帧率追踪"); })
            .Case(2, []() { return Text("当前激活：【分支 C】 - 调试控制台与日志归档"); })
            .Default([]() { return Text("未知分支"); })
    });

    spec.sections = {
        {
            "1. 细粒度信号与计算属性 (Signal & Computed)",
            "无需手动触发 Invalidate 或调用 setState，读取自动收集依赖，写入精确通知对应观察者；支持 Batch 事务批处理合并通知。",
            counterSection
        },
        {
            "2. 响应式条件渲染 (If)",
            "DSL 流控制原语 If(cond, thenFn, elseFn)，按需惰性构建激活子树，条件切换时自动析构非活动子树并释放 Effect 资源。",
            ifSection
        },
        {
            "3. 响应式列表渲染 (ForEach)",
            "监听 Signal<std::vector<T>> 集合变化，自动根据新数据重构子项序列，支持双向增删改查交互。",
            forEachSection
        },
        {
            "4. 多分支模式匹配 (Switch)",
            "Switch(key).Case(val, builder).Default(builder) 实现声明式多态分支切换。",
            switchSection
        }
    };

    spec.source = R"cpp(// 1. 定义细粒度响应式状态与计算属性
Signal<int> count(0);
Computed<std::string> desc([count]() {
    return std::format("计数值: {}", count.Get());
});

// 2. 在控件中直接绑定 Signal 或 Computed
Text(desc);

// 3. 条件控制流
If(showDetails,
    []() { return Text("详细配置内容"); },
    []() { return Text("无详细内容"); }
);

// 4. 列表渲染
ForEach(items, [](const Item& item, size_t index) {
    return Row(8.0f, {
        Text(item.name),
        Button("删除")
    });
});
)cpp";

    return BuildSamplePage(spec);
}

} // namespace Gallery
