#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"
#include "framework/core/CUIDsl.h"
#include "framework/controls/Button.h"
#include "framework/controls/TextBox.h"
#include "framework/controls/TextBlock.h"
#include "framework/controls/MessageBox.h"

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

Element BuildContentDialogPage() {
    auto status = MakeStatus("点击按钮触发对话框，此处显示结果。");

    // ── 1. 信息确认对话框 ────────────────────────────────────────────────
    auto btnInfo = Button("打开信息提示框");
    btnInfo->OnClick().Connect([status](UIElement* src) {
        auto dlg = ContentDialogWidget();
        dlg->SetTitle("操作提示");
        dlg->SetMessage("您确定要继续执行此操作吗？此操作不可撤销，请谨慎确认。");
        dlg->SetPrimaryButtonText("确定");
        dlg->SetCloseButtonText("取消");
        src->AddChild(dlg);
        dlg->Show([status, dlg](DialogResult r) {
            if (r == DialogResult::Primary)
                status->Text = "结果：已点击【确定】，操作继续执行。";
            else
                status->Text = "结果：已点击【取消】，操作已中止。";
        });
    });

    // ── 2. 三按钮对话框 ─────────────────────────────────────────────────
    auto btnThree = Button("三个按钮的对话框");
    btnThree->BackgroundToken = ThemeTokenId::CardBackground;
    btnThree->ColorToken = ThemeTokenId::TextPrimary;
    btnThree->BorderToken = ThemeTokenId::CardBorder;
    btnThree->BorderThickness = 1.0f;
    btnThree->OnClick().Connect([status](UIElement* src) {
        auto dlg = ContentDialogWidget();
        dlg->SetTitle("保存更改");
        dlg->SetMessage("您有未保存的更改。是否要在关闭前保存？");
        dlg->SetPrimaryButtonText("保存");
        dlg->SetSecondaryButtonText("不保存");
        dlg->SetCloseButtonText("取消");
        src->AddChild(dlg);
        dlg->Show([status, dlg](DialogResult r) {
            if (r == DialogResult::Primary)
                status->Text = "结果：已选择【保存】，文件已写入磁盘。";
            else if (r == DialogResult::Secondary)
                status->Text = "结果：已选择【不保存】，更改已丢弃。";
            else
                status->Text = "结果：已取消，继续编辑。";
        });
    });

    // ── 3. 输入对话框 ───────────────────────────────────────────────────
    auto btnInput = Button("带文本输入的对话框");
    btnInput->BackgroundToken = ThemeTokenId::CardBackground;
    btnInput->ColorToken = ThemeTokenId::TextPrimary;
    btnInput->BorderToken = ThemeTokenId::CardBorder;
    btnInput->BorderThickness = 1.0f;
    btnInput->OnClick().Connect([status](UIElement* src) {
        auto dlg = ContentDialogWidget();
        dlg->SetTitle("新建文件夹");
        dlg->SetMessage("请输入新文件夹的名称：");
        dlg->SetPrimaryButtonText("创建");
        dlg->SetCloseButtonText("取消");
        dlg->SetInputEnabled(true);
        dlg->SetInputText("新建文件夹");
        src->AddChild(dlg);
        dlg->Show([status, dlg](DialogResult r) {
            if (r == DialogResult::Primary) {
                std::string name = dlg->GetInputText();
                status->Text = "结果：已创建文件夹「" + (name.empty() ? "（无名称）" : name) + "」。";
            } else {
                status->Text = "结果：已取消创建。";
            }
        });
    });

    // ── 4. 危险操作对话框 ───────────────────────────────────────────────
    auto btnDanger = Button("危险操作确认");
    btnDanger->Background = Color::Hex("#C62828");
    btnDanger->HoverBackground = Color::Hex("#B71C1C");
    btnDanger->PressedBackground = Color::Hex("#8E0000");
    btnDanger->Foreground = Color::White;
    btnDanger->OnClick().Connect([status](UIElement* src) {
        auto dlg = ContentDialogWidget();
        dlg->SetTitle("永久删除");
        dlg->SetMessage("此操作将永久删除所选的 3 个文件，总计 128 MB。\n\n已删除的内容无法从回收站恢复，请确认操作。");
        dlg->SetPrimaryButtonText("永久删除");
        dlg->SetCloseButtonText("取消");
        src->AddChild(dlg);
        dlg->Show([status, dlg](DialogResult r) {
            if (r == DialogResult::Primary)
                status->Text = "结果：已执行永久删除，文件已清除。";
            else
                status->Text = "结果：已取消删除操作。";
        });
    });

    SamplePageSpec spec;
    spec.title    = "ContentDialog（内容对话框）";
    spec.subtitle = "以模态遮罩的方式弹出对话框，阻断背景操作，用于确认、提示或收集用户输入。";
    spec.sections = {
        {
            "基本用法",
            "单击下方按钮触发对应的对话框样式。主按钮（Primary）、副按钮（Secondary）和关闭按钮均可独立配置。",
            Column(12, {
                Row(10, {btnInfo, btnThree }),
                Row(10, {btnInput, btnDanger }),
                status,
            }),
        },
        {
            "按钮结果回调",
            "Show() 方法接受一个 std::function<void(DialogResult)> 回调，回调参数为枚举值 Primary / Secondary / Cancel。\n"
            "在回调内可通过 GetInputText() 读取用户在输入框中填写的内容。",
            Column(8, {
                MakeLabel("DialogResult 枚举：", 12.0f, ThemeTokenId::TextMuted),
                MakeLabel("  Primary   — 主确认按钮", 12.0f, ThemeTokenId::TextSecondary),
                MakeLabel("  Secondary — 副辅助按钮", 12.0f, ThemeTokenId::TextSecondary),
                MakeLabel("  Cancel    — 取消/关闭按钮", 12.0f, ThemeTokenId::TextSecondary),
            }),
        },
    };
    spec.source =
        "auto dlg = ContentDialogWidget();\n"
        "dlg->SetTitle(\"标题\");\n"
        "dlg->SetMessage(\"消息内容。\");\n"
        "dlg->SetPrimaryButtonText(\"确定\");\n"
        "dlg->SetCloseButtonText(\"取消\");\n"
        "parent->AddChild(dlg);\n"
        "dlg->Show([](DialogResult r) {\n"
        "    if (r == DialogResult::Primary) { /* 确认 */ }\n"
        "});\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
