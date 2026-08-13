#include "PageRegistry.h"
#include "../ShowcaseHelpers.h"
#include "framework/core/CUIDsl.h"
#include "framework/controls/MarkdownView.h"
#include "framework/controls/Button.h"
#include "framework/controls/TextBlock.h"
#include "framework/controls/Toast.h"

using namespace CUI;
using namespace CUI::DSL;

namespace {

const char* kSampleDoc = R"MD(# MarkdownView

只读 **Markdown → 自绘** 富文本。解析是框架内子集实现（无 WebView）；排版与绘制走 `GraphicsContext`。

## 行内样式

普通段落里可以有 **加粗**、*斜体*、***粗斜体***、行内代码 `LayoutMarkdown()`，以及 [链接](https://learn.microsoft.com/windows/apps/design/controls/rich-text-block)。

## 列表

- 无序一项
- 嵌套也可以
  - 第二层
- 主题色跟随 Accent

1. 有序列表
2. 选中后 `Ctrl+C` 复制纯文本
3. `Ctrl+A` 全选

## 引用

> 验收：Showcase 加载内置样例；滚动流畅；选中复制纯文本；主题切换后代码块仍然可读。

## 代码块

```cpp
MdLayoutResult layout = LayoutMarkdown(ctx, blocks, width, true);
view->SetMarkdown(source);
```

## 表格

| 块 | 绘制 | 说明 |
| --- | :---: | ---: |
| 标题 h1–h6 | 字号阶梯 | 下划线 |
| 围栏代码 | Consolas + 底板 | 可选行号 |
| 管道表 | 网格 | 表头加粗 |

---

拖动选择文字，双击选词。Home / End 跳到文档两端。
)MD";

const char* kSampleCompact = R"MD(# 紧凑样例

一段 *emphasis* 和 **strong**，还有 `code`。

- Alpha
- Beta

```json
{ "ok": true }
```
)MD";

} // namespace

ShowcasePage BuildMarkdownPage(const ShowcaseContext& ctx) {
    auto view = std::make_shared<MarkdownView>();
    view->SetWidth(-1.0f);
    view->SetHeight(520.0f);
    view->SetShowCodeLineNumbers(true);
    view->SetMarkdown(kSampleDoc);

    auto log = std::static_pointer_cast<TextBlock>(
        CreateShowcaseText("提示：拖选复制；点击链接会打开浏览器。", 12.0f, "#B5CEA8", false, "Consolas"));

    view->OnLinkClicked().Connect([window = ctx.windowRef, log](MarkdownView*, const std::string& url) {
        log->SetText("[链接] " + url);
        if (window) {
            Toast::Show(window->GetRootElement().get(), "Markdown", url, ToastCorner::BottomRight, 1800);
        }
    });

    auto fullBtn = std::make_shared<Button>("完整样例");
    fullBtn->SetWidth(100.0f);
    fullBtn->SetHeight(32.0f);
    fullBtn->OnClick().Connect([view, log](UIElement*) {
        view->SetMarkdown(kSampleDoc);
        log->SetText("已加载完整样例");
    });
    auto shortBtn = std::make_shared<Button>("紧凑样例");
    shortBtn->SetWidth(100.0f);
    shortBtn->SetHeight(32.0f);
    shortBtn->OnClick().Connect([view, log](UIElement*) {
        view->SetMarkdown(kSampleCompact);
        log->SetText("已加载紧凑样例");
    });
    auto copyBtn = std::make_shared<Button>("复制选区");
    copyBtn->SetWidth(100.0f);
    copyBtn->SetHeight(32.0f);
    copyBtn->OnClick().Connect([view, log](UIElement*) {
        view->CopySelection();
        const auto sel = view->GetSelectedText();
        log->SetText(sel.empty() ? "已复制全文" : ("已复制 " + std::to_string(sel.size()) + " 字符"));
    });
    auto linesBtn = std::make_shared<Button>("切换行号");
    linesBtn->SetWidth(100.0f);
    linesBtn->SetHeight(32.0f);
    linesBtn->OnClick().Connect([view, log](UIElement*) {
        view->SetShowCodeLineNumbers(!view->GetShowCodeLineNumbers());
        log->SetText(view->GetShowCodeLineNumbers() ? "代码行号：开" : "代码行号：关");
    });

    auto demo = Column(12).Children({
        CreateDemoSurface({
            CreateShowcaseText("1. 自绘 Markdown 视图", 12.0f, "textSecondary", false),
            CreateShowcaseText("标题 / 段落 / 加粗斜体 / 链接 / 列表 / 引用 / 代码块 / 表格。无 WebView。", 12.0f, "textMuted", false),
            Row(8).Children({ fullBtn, shortBtn, copyBtn, linesBtn }).Build(),
            view,
            log,
        }, 10.0f),
    }).Build();

    return { "Markdown", CreatePage(
        "Markdown 渲染器",
        "MarkdownView：自研子集解析 + Direct2D 排版绘制。只读，支持选区复制与链接。",
        demo,
        CreatePropertyGrid(ctx, view), view) };
}
