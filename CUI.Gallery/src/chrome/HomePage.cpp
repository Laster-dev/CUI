#include "chrome/HomePage.h"
#include "catalog/Catalog.h"
#include "GalleryHost.h"
#include "pages/SamplePage.h"

#include "framework/core/CUIDsl.h"
#include "framework/controls/Panel.h"
#include "framework/controls/ScrollViewer.h"
#include "framework/style/ThemeTokenId.h"

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

Element BuildHomePage() {
    auto body = Column(20).Padding(24);
    body.Add(Column(6).Children({
        MakeLabel("CUI Gallery", 28.0f, ThemeTokenId::TextPrimary, true),
        MakeLabel(
            "以 WinUI 风格展示控件。从左侧导航、主页卡片或搜索打开示例。",
            14.0f,
            ThemeTokenId::TextMuted,
            false),
    }).Build());

    for (Category category : CategoryOrder()) {
        auto items = EntriesIn(category);
        if (items.empty()) {
            continue;
        }

        auto wrap = std::make_shared<WrapPanel>(Orientation::Horizontal);
        wrap->Gap = 12.0f;
        wrap->JustifyLines = true;
        wrap->FillLastLine = true;
        wrap->Align = Alignment::Stretch;

        for (const Entry* entry : items) {
            auto card = Column(6).MinWidth(180).Padding(16).CornerRadius(6).Children({
                MakeLabel(entry->title, 15.0f, ThemeTokenId::TextPrimary, true),
                MakeLabel(entry->subtitle, 12.0f, ThemeTokenId::TextMuted, false),
            }).Build();
            card->BackgroundToken = ThemeTokenId::CardBackground;
            card->BorderToken = ThemeTokenId::CardBorder;
            card->BorderThickness = 1.0f;
            const std::string tag = entry->tag;
            auto go = [tag](UIElement*) {
                Host::Instance().Navigate(tag);
            };
            card->OnClick().Connect(go);
            for (const auto& child : card->GetChildren()) {
                if (child) {
                    child->OnClick().Connect(go);
                }
            }
            wrap->AddChild(card);
        }

        body.Add(Column(10).Children({
            MakeLabel(CategoryDisplayName(category), 16.0f, ThemeTokenId::TextSecondary, true),
            wrap,
        }).Build());
    }

    auto column = body.Build();
    column->BackgroundToken = ThemeTokenId::WindowBackground;

    auto scroll = std::make_shared<ScrollViewer>();
    scroll->Align = Alignment::Stretch;
    scroll->FlexGrow = 1.0f;
    scroll->BackgroundToken = ThemeTokenId::WindowBackground;
    scroll->AddChild(column);
    return scroll;
}

} // namespace Gallery
