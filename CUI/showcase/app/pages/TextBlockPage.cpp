#include "PageRegistry.h"
#include "../ShowcaseHelpers.h"

ShowcasePage BuildTextBlockPage(const ShowcaseContext& ctx) {
    auto target = CreateShowcaseText("DirectWrite 高性能矢量文本渲染", 18.0f, "#4EC9B0", true);
    return {
        "TextBlock 文本",
        CreatePage(
        "TextBlock 文本控件全属性控制台",
        "支持 DirectWrite 高级排版、行间距、固定行高、颜色与字体调节。",
        CreateDemoSurface(
            {
                target
            },
            0.0f
        ))
    };
}
