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

        auto wrap = std::make_shared<WrapPanel>(Orientation::Horizontal);
        CUI::DSL::Borrow(wrap).Gap(12.0f);
        CUI::DSL::Borrow(wrap).JustifyLines(true);
        CUI::DSL::Borrow(wrap).FillLastLine(true);
        CUI::DSL::Borrow(wrap).Align(Alignment::Stretch);

        for (const Entry* entry : items) {
            auto card = Column(6, {
                MakeLabel(entry->title, 15.0f, ThemeTokenId::TextPrimary, true),
                MakeLabel(entry->subtitle, 12.0f, ThemeTokenId::TextMuted, false),
            }).MinWidth(180).Padding(16).CornerRadius(6).Build();
            CUI::DSL::Borrow(card).BackgroundToken(ThemeTokenId::CardBackground);
            CUI::DSL::Borrow(card).BorderToken(ThemeTokenId::CardBorder);
            CUI::DSL::Borrow(card).BorderThickness(1.0f);
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
            CUI::DSL::Borrow(wrap).AddChild(card);
        }

        body.AddChild(Column(10, {
            MakeLabel(CategoryDisplayName(category), 16.0f, ThemeTokenId::TextSecondary, true),
            wrap,
        }));
    }

    auto column = body.Build();
    CUI::DSL::Borrow(column).BackgroundToken(ThemeTokenId::WindowBackground);

    auto scroll = std::make_shared<ScrollViewer>();
    CUI::DSL::Borrow(scroll).Align(Alignment::Stretch);
    CUI::DSL::Borrow(scroll).FlexGrow(1.0f);
    CUI::DSL::Borrow(scroll).BackgroundToken(ThemeTokenId::WindowBackground);
    CUI::DSL::Borrow(scroll).AddChild(column);
    return scroll;
}

} // namespace Gallery



