#include "pages/BasicInput/Pages.h"
#include "page/SamplePage.h"

#include "framework/core/CUIDsl.h"
#include "framework/controls/Button.h"
#include <memory>
#include <string>

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

std::shared_ptr<UIElement> BuildButtonPage() {
    auto save = std::make_shared<Button>("保存");
    auto cancel = std::make_shared<Button>("取消");
    auto formStatus = MakeStatus("尚未更改。");
    save->SetToolTip("保存当前文档。");
    cancel->SetToolTip("放弃未保存的更改。");
    save->OnClick().Connect([formStatus](UIElement*) {
        formStatus->SetText("已保存。");
    });
    cancel->OnClick().Connect([formStatus](UIElement*) {
        formStatus->SetText("已放弃。");
    });

    auto start = std::make_shared<Button>("开始");
    auto stop = std::make_shared<Button>("停止");
    stop->SetIsEnabled(false);
    auto runStatus = MakeStatus("已停止。");
    start->OnClick().Connect([start, stop, runStatus](UIElement*) {
        start->SetIsEnabled(false);
        stop->SetIsEnabled(true);
        runStatus->SetText("正在运行。");
    });
    stop->OnClick().Connect([start, stop, runStatus](UIElement*) {
        start->SetIsEnabled(true);
        stop->SetIsEnabled(false);
        runStatus->SetText("已停止。");
    });

    auto add = std::make_shared<Button>("添加项");
    add->SetIcon("＋");
    auto countStatus = MakeStatus("0 项。");
    auto count = std::make_shared<int>(0);
    add->OnClick().Connect([count, countStatus](UIElement*) {
        ++(*count);
        countStatus->SetText(std::to_string(*count) + " 项。");
    });

    SamplePageSpec spec;
    spec.title = "Button(按钮)";
    spec.subtitle = "按钮用于触发操作。单击，或聚焦后按空格或 Enter。";
    spec.sections = {
        {
            "对话框操作",
            "保存和取消是表单或对话框中的常见组合。",
            Column(10).Children({
                Row(12).Children({ save, cancel }).Build(),
                formStatus,
            }).Build(),
        },
        {
            "启用与禁用",
            "开始任务后，「停止」可用。运行期间「开始」不可用。",
            Column(10).Children({
                Row(12).Children({ start, stop }).Build(),
                runStatus,
            }).Build(),
        },
        {
            "重复操作",
            "可多次单击的图标按钮。",
            Column(10).Children({
                add,
                countStatus,
            }).Build(),
        },
    };
    spec.source =
        "auto save = std::make_shared<Button>(\"Save\");\n"
        "save->OnClick().Connect([](UIElement*) {\n"
        "    // persist\n"
        "});\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
