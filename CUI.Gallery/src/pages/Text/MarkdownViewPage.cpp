#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"

#include "framework/controls/MarkdownView.h"

using namespace CUI;

namespace Gallery {

std::shared_ptr<UIElement> BuildMarkdownViewPage() {
    auto markdown = std::make_shared<MarkdownView>(R"markdown(
# MarkdownView

支持 **粗体**、*斜体*、`行内代码`、~~删除线~~ 和 [链接](https://example.com)。

> 引用块可包含多行内容，正文会根据可用宽度自动换行。

## 任务列表

- [x] 标题、段落、列表和引用
- [x] 表格单元格按内容自动增高
- [x] C++、Python、JSON、XML 代码高亮
- [ ] 后续可继续扩展图片和嵌入式媒体

## 表格

| 类型 | 说明 |
| --- | --- |
| 自动换行 | 长文本按单元格宽度换行，行高随最高单元格自动增加。 |
| 代码块 | 关键字、类型、字符串、数字、注释和预处理指令会使用不同颜色。 |

## C++

```cpp
#include <memory>

constexpr int RetryCount = 3;
State<std::string> title{ "CUI Gallery" };
if (RetryCount > 0) {
    // 绑定属性会自动刷新 UI。
    text->Text->Bind(title);
}
```

## JSON

```json
{
  "theme": "dark",
  "animations": true,
  "retries": 3
}
```
)markdown");
    markdown->SetHeight(560.0f);

    SamplePageSpec spec;
    spec.title = "MarkdownView(Markdown 视图)";
    spec.subtitle = "渲染常用 Markdown 结构，表格自动换行，代码块按语言高亮。";
    spec.sections = {
        { "渲染预览", "单击链接、滚动、选择文本与复制代码均可直接体验。", markdown },
    };
    spec.source =
        "auto markdown = std::make_shared<MarkdownView>(R\"markdown(... )markdown\");\n"
        "markdown->SetHeight(560.0f);\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
