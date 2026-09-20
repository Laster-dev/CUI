#ifndef CUI_NO_DSL_SHORTCUTS
#define CUI_NO_DSL_SHORTCUTS   // 关闭「控件名即工厂」宏层，避免与 CUI::Widgets:: 句柄同名冲突
#endif
#include "Gallery.h"
#include "framework/core/Widgets.h"
using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

Element BuildContentDialogPage() {
    auto status = MakeStatus("点击按钮触发对话框，此处显示结果。");

    // ── 1. 信息确认对话框 ────────────────────────────────────────────────
    auto btnInfo = Button("打开信息提示框")
        .OnClick([status](UIElement* src) {
            CUI::Widgets::Ref dlg = CUI::Widgets::ContentDialog().Shared();
            dlg.Title("操作提示");
            dlg.Message("您确定要继续执行此操作吗？此操作不可撤销，请谨慎确认。");
            dlg.PrimaryButtonText("确定");
            dlg.CloseButtonText("取消");
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
        btnThree->SetBackgroundToken(ThemeTokenId::CardBackground);
    
        btnThree->SetBorderToken(ThemeTokenId::CardBorder);
        btnThree->SetBorderThickness(1.0f);
    btnThree->OnClick().Connect([status](UIElement* src) {
        CUI::Widgets::Ref dlg = CUI::Widgets::ContentDialog().Shared();
        dlg.Title("保存更改");
        dlg.Message("您有未保存的更改。是否要在关闭前保存？");
        dlg.PrimaryButtonText("保存");
        dlg.SecondaryButtonText("不保存");
        dlg.CloseButtonText("取消");
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
        btnInput->SetBackgroundToken(ThemeTokenId::CardBackground);
    
        btnInput->SetBorderToken(ThemeTokenId::CardBorder);
        btnInput->SetBorderThickness(1.0f);
    btnInput->OnClick().Connect([status](UIElement* src) {
        CUI::Widgets::Ref dlg = CUI::Widgets::ContentDialog().Shared();
        dlg.Title("新建文件夹");
        dlg.Message("请输入新文件夹的名称：");
        dlg.PrimaryButtonText("创建");
        dlg.CloseButtonText("取消");
        dlg.InputEnabled(true);
        dlg.InputText("新建文件夹");
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
    auto btnDanger = Button("危险操作确认")
        .Background(Color::Hex("#C62828"))
        .HoverBackground(Color::Hex("#B71C1C"))
        .PressedBackground(Color::Hex("#8E0000"))
        .Foreground(Color::White)
        .OnClick([status](UIElement* src) {
            CUI::Widgets::Ref dlg = CUI::Widgets::ContentDialog().Shared();
            dlg.Title("永久删除");
            dlg.Message("此操作将永久删除所选的 3 个文件，总计 128 MB。\n\n已删除的内容无法从回收站恢复，请确认操作。");
            dlg.PrimaryButtonText("永久删除");
            dlg.CloseButtonText("取消");
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
        "auto dlg = CUI::Widgets::ContentDialog().Shared();\n"
        "dlg.Title(\"标题\");\n"
        "dlg.Message(\"消息内容。\");\n"
        "dlg.PrimaryButtonText(\"确定\");\n"
        "dlg.CloseButtonText(\"取消\");\n"
        "parent->AddChild(dlg);\n"
        "dlg->Show([](DialogResult r) {\n"
        "    if (r == DialogResult::Primary) { /* 确认 */ }\n"
        "});\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery




