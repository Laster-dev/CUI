#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"

#include "framework/core/CUIDsl.h"
#include "framework/controls/HyperlinkButton.h"
#include "framework/core/Value.h"
#include <windows.h>
#include <shellapi.h>
#include <GalleryHost.h>

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

std::shared_ptr<UIElement> BuildHyperlinkButtonPage() {
    
    auto docs = std::make_shared<HyperlinkButton>("打开文档", "https://learn.microsoft.com/windows/apps/design/controls/hyperlink-button");
    auto inApp = std::make_shared<HyperlinkButton>("打开设置页面");
    auto status = MakeStatus("链接看起来像文本，行为像按钮。");
    docs->OnClick().Connect([docs](UIElement*) {
        const std::wstring uri = Utf8ToUtf16(docs->GetNavigateUri());
        if (!uri.empty()) {
            ShellExecuteW(nullptr, L"open", uri.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        }
    });
    inApp->OnClick().Connect([status](UIElement*) {
        status->SetText("应用内操作：将打开详情面板。");
        //直接切换到设置页面
        Gallery::Host::Instance().Navigate("settings");
    });

    auto disabled = std::make_shared<HyperlinkButton>("不可用链接");
    disabled->SetIsEnabled(false);

    SamplePageSpec spec;
    spec.title = "HyperlinkButton(超链接按钮)";
    spec.subtitle = "用于导航的文本样式按钮。可打开 URI 或执行应用内命令。";
    spec.sections = {
        {
            "导航与应用内操作",
            "第一个链接在浏览器中打开。第二个留在应用内。",
            Column(10).Children({
                Column(8).Children({ docs, inApp, disabled }).Build(),
                status,
            }).Build(),
        },
    };
    spec.source =
        "auto link = std::make_shared<HyperlinkButton>(\"Open docs\", \"https://example.com\");\n"
        "link->OnClick().Connect([](UIElement*) {\n"
        "    // ShellExecute or in-app navigate\n"
        "});\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
