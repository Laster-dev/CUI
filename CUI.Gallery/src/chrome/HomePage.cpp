#include "Gallery.h"
#include "chrome/HomePage.h"
#include "catalog/Catalog.h"
#include "GalleryHost.h"


using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

Element BuildHomePage() {
    auto body = Column(20).Padding(24);
    body.AddChild(Column(6, {
        MakeLabel("CUI Gallery", 28.0f, ThemeTokenId::TextPrimary, true),
        MakeLabel(
            "以 WinUI 风格展示控件。从左侧导航、主页卡片或搜索打开示例。",
            14.0f,
            ThemeTokenId::TextMuted,
            false),
    }));

    for (Category category : CategoryOrder()) {
        auto items = EntriesIn(category);
        if (items.empty()) {
            continue;
        }

        auto wrap = std::make_shared<WrapPanel>();
        wrap->SetOrientation(Orientation::Horizontal);
                wrap->SetGap(12.0f);
                wrap->SetJustifyLines(true);
                wrap->SetFillLastLine(true);
                wrap->SetAlign(Alignment::Stretch);

        for (const Entry* entry : items) {
            auto card = Column(6, {
                MakeLabel(entry->title, 15.0f, ThemeTokenId::TextPrimary, true),
                MakeLabel(entry->subtitle, 12.0f, ThemeTokenId::TextMuted, false),
            }).MinWidth(180).Padding(16).CornerRadius(6).Build();
                        card->SetBackgroundToken(ThemeTokenId::CardBackground);
                        card->SetBorderToken(ThemeTokenId::CardBorder);
                        card->SetBorderThickness(1.0f);
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

        body.AddChild(Column(10, {
            MakeLabel(CategoryDisplayName(category), 16.0f, ThemeTokenId::TextSecondary, true),
            wrap,
        }));
    }

    auto column = body.Build();
        column->SetBackgroundToken(ThemeTokenId::WindowBackground);

    auto scroll = std::make_shared<ScrollViewer>();
        scroll->SetAlign(Alignment::Stretch);
        scroll->SetFlexGrow(1.0f);
        scroll->SetBackgroundToken(ThemeTokenId::WindowBackground);
        scroll->AddChild(column);
    return scroll;
}

} // namespace Gallery



