#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"
#include "framework/controls/AutoSuggestBox.h"
#include "framework/controls/Button.h"
#include "framework/controls/TextBlock.h"
#include <format>
#include <memory>
#include <string>
#include <vector>

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

Element BuildAutoSuggestBoxPage() {
    // ==========================================
    // 1. 静态数据目录与模糊匹配
    // ==========================================
    auto suggestA = AutoSuggestBoxWidget("输入城市名称 (如 Beijing, Shanghai, Tokyo...)")
        .Width(360.0f)
        .Height(32.0f);

    suggestA->SetSuggestionItems({
        "Beijing (北京)",
        "Shanghai (上海)",
        "Shenzhen (深圳)",
        "Guangzhou (广州)",
        "Hangzhou (杭州)",
        "Chengdu (成都)",
        "Wuhan (武汉)",
        "Nanjing (南京)",
        "Tokyo (东京)",
        "London (伦敦)",
        "New York (纽约)",
        "San Francisco (旧金山)",
        "Singapore (新加坡)",
        "Sydney (悉尼)"
    });

    auto statusA = MakeStatus("状态：就绪。尝试在上方输入框键入字母（如 Bei、hai、o）触发下拉联想。");

    suggestA->OnTextChanged().Connect([statusA](AutoSuggestBox*, const std::string& text) {
        statusA->Text = std::format("📝 [OnTextChanged] 正在输入：\"{}\"", text);
    });

    suggestA->OnSuggestionChosen().Connect([statusA](AutoSuggestBox*, const std::string& chosen) {
        statusA->Text = std::format("🎯 [OnSuggestionChosen] 已选中建议项：【{}】", chosen);
    });

    suggestA->OnQuerySubmitted().Connect([statusA](AutoSuggestBox*, const std::string& query) {
        statusA->Text = std::format("🚀 [OnQuerySubmitted] 已回车提交查询：\"{}\"", query);
    });

    auto btnClearA = Button("清空")
        .Width(80.0f)
        .Height(32.0f)
        .OnClick([suggestA, statusA](UIElement*) {
            suggestA->SetText("");
            statusA->Text = "已清空输入框内容。";
        });

    // ==========================================
    // 2. 动态语法与智能补全提供器 (Custom Provider)
    // ==========================================
    auto suggestB = AutoSuggestBoxWidget("输入 C++ 关键词 (如 std::、int、auto 或计算公式)...")
        .Width(420.0f)
        .Height(32.0f);

    suggestB->SetSuggestionProvider([](const std::string& query) -> std::vector<std::string> {
        if (query.empty()) {
            return {
                "std::vector<T>",
                "std::string",
                "std::shared_ptr<T>",
                "std::unique_ptr<T>",
                "std::make_shared<T>()",
                "std::format(...)",
                "std::clamp(val, min, max)"
            };
        }

        std::vector<std::string> results;
        // 动态生成补全与计算
        results.push_back(std::format("constexpr auto {}_val", query));
        results.push_back(std::format("std::vector<{}>", query));
        results.push_back(std::format("std::shared_ptr<{}>", query));
        results.push_back(std::format("std::make_unique<{}>()", query));
        results.push_back(std::format("#include <{}>", query));
        results.push_back(std::format("template <typename {}> class MyContainer;", query));
        return results;
    });

    auto statusB = MakeStatus("状态：动态 Provider 就绪。输入任意关键字即时生成智能补全列表。");

    suggestB->OnSuggestionChosen().Connect([statusB](AutoSuggestBox*, const std::string& chosen) {
        statusB->Text = std::format("💡 [代码补全落地] 已选定代码段：{}", chosen);
    });

    suggestB->OnQuerySubmitted().Connect([statusB](AutoSuggestBox*, const std::string& query) {
        statusB->Text = std::format("🔍 [提交代码查询] \"{}\"", query);
    });

    // ==========================================
    // 3. 最大展示条数与键盘导航
    // ==========================================
    auto suggestC = AutoSuggestBoxWidget("支持 ↑/↓ 方向键选择、Enter 确认、Esc 关闭...")
        .Width(360.0f)
        .Height(32.0f);
    suggestC->SetMaxVisibleSuggestions(4);
    suggestC->SetSuggestionItems({
        "01. Windows App SDK",
        "02. WinUI 3 现代控件",
        "03. Direct2D 硬件加速渲染",
        "04. DirectWrite 排版引擎",
        "05. CUI 响应式声明式 DSL",
        "06. DockManager 停靠系统",
        "07. DragDropService 拖放管线",
        "08. Fluent Design 亚克力与云母材质"
    });

    SamplePageSpec spec;
    spec.title = "AutoSuggestBox(自动建议框)";
    spec.subtitle = "基于文本输入即时提供下拉搜索建议与代码智能补全的控件，支持静态模糊过滤、动态生成 Provider、键盘全手势导航与自适应层级呈现。";
    spec.sections = {
        {
            "静态目录与模糊搜索 (Static Catalog)",
            "内置不区分大小写的子串智能匹配引擎，输入文字即时筛选候选列表，支持点击或键盘选择落地。",
            Column(12, {
                Row(8, { suggestA, btnClearA }).AlignVertical(Alignment::Center),
                statusA,
            }),
        },
        {
            "动态算法与智能补全生成器 (Dynamic Provider)",
            "通过 SetSuggestionProvider 注入自定义函数，可根据用户输入动态实时生成建议候选词或计算结果。",
            Column(12, {
                suggestB,
                statusB,
            }),
        },
        {
            "最大展示行数与键盘操作 (Keyboard Navigation)",
            "支持通过 SetMaxVisibleSuggestions 控制弹出菜单的最大显示条目数，内置自动滚动条与平滑显隐动画。",
            Column(12, {
                suggestC,
                MakeStatus("提示：支持按键盘方向键 ↑ / ↓ 在候选项之间高亮切换，按 Enter 键确认选中，按 Esc 键轻量关闭建议弹窗。"),
            }),
        },
    };

    spec.source = R"cpp(// 1. 静态数据集模式
auto box = AutoSuggestBoxWidget("搜索城市...");
box->SetSuggestionItems({ "Beijing", "Shanghai", "Tokyo", "London" });
box->OnSuggestionChosen().Connect([](AutoSuggestBox*, const std::string& item) {
    // 处理选中逻辑
});

// 2. 动态 Provider 模式
box->SetSuggestionProvider([](const std::string& query) -> std::vector<std::string> {
    return QueryDatabaseOrCalculate(query);
});
)cpp";

    return BuildSamplePage(spec);
}

} // namespace Gallery
