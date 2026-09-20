#include "PageRegistry.h"
#include "../ShowcaseHelpers.h"
#include "framework/core/CUIDsl.h"
#include "framework/controls/RangeSlider.h"
#include "framework/controls/Button.h"
#include "framework/controls/TextBlock.h"
#include "framework/controls/ListBox.h"
#include <sstream>
#include <vector>

using namespace CUI;
using namespace CUI::DSL;

namespace {
struct CatalogItem {
    const char* name;
    float price;
};

constexpr CatalogItem kCatalog[] = {
    { "入门耳机", 89.0f },
    { "机械键盘", 329.0f },
    { "显示器 27\"", 1299.0f },
    { "轻薄本", 4599.0f },
    { "无线鼠标", 159.0f },
    { "移动硬盘 1T", 449.0f },
    { "摄像头", 219.0f },
    { "扩展坞", 399.0f },
};

std::string FormatPriceRange(float lo, float hi) {
    std::ostringstream oss;
    oss << "¥" << static_cast<int>(lo) << " – ¥" << static_cast<int>(hi);
    return oss.str();
}

std::string FormatPlainRange(float lo, float hi, const char* unit) {
    std::ostringstream oss;
    oss << static_cast<int>(lo) << unit << " – " << static_cast<int>(hi) << unit;
    return oss.str();
}

void FillCatalog(ListBox& list, float lo, float hi) {
    std::vector<std::string> rows;
    rows.reserve(8);
    for (const auto& item : kCatalog) {
        if (item.price >= lo && item.price <= hi) {
            std::ostringstream oss;
            oss << item.name << "    ¥" << static_cast<int>(item.price);
            rows.push_back(oss.str());
        }
    }
    if (rows.empty()) {
        rows.emplace_back("（当前区间没有商品）");
    }
        list.ApplyItems(rows);
}
} // namespace

ShowcasePage BuildRangeSliderPage(const ShowcaseContext& ctx) {
    CUI::Widgets::Ref price = CUI::Widgets::RangeSlider().Shared();
        price.Minimum(0.0f);
        price.Maximum(5000.0f);
        price.Step(50.0f);
        price.MinimumRange(50.0f);
        price.Range(150.0f, 1500.0f);
        price.Width(360.0f);
        price.Height(48.0f);
        price.ToolTip("拖任一端筛选价格；两滑块不可交叉。");

    CUI::Widgets::Ref priceLabel =std::static_pointer_cast<TextBlock>(
        CreateShowcaseText(FormatPriceRange(price->GetLowerValue(), price->GetUpperValue()), 14.0f, "textPrimary", true));

    CUI::Widgets::Ref catalog = CUI::Widgets::ListBox().Shared();
        catalog.Height(148.0f);
        catalog.Width(360.0f);
    FillCatalog(*catalog, price->GetLowerValue(), price->GetUpperValue());

    price->OnValueChanged().Connect([priceLabel, catalog](RangeSlider*, float lo, float hi) {
                priceLabel.Text(FormatPriceRange(lo, hi));
        FillCatalog(*catalog, lo, hi);
    });

    CUI::Widgets::Ref btnBudget = CUI::Widgets::Button("百元档").Shared();
    btnBudget->OnClick().Connect([price](UIElement*) {     price.Range(0.0f, 300.0f); });
    CUI::Widgets::Ref btnMid = CUI::Widgets::Button("中端").Shared();
    btnMid->OnClick().Connect([price](UIElement*) {     price.Range(200.0f, 1500.0f); });
    CUI::Widgets::Ref btnAll = CUI::Widgets::Button("全部").Shared();
    btnAll->OnClick().Connect([price](UIElement*) {     price.Range(0.0f, 5000.0f); });

    CUI::Widgets::Ref vertical = CUI::Widgets::RangeSlider().Shared();
        vertical.Orientation(Orientation::Vertical);
        vertical.Minimum(0.0f);
        vertical.Maximum(100.0f);
        vertical.Step(1.0f);
        vertical.MinimumRange(5.0f);
        vertical.Range(20.0f, 80.0f);
        vertical.Width(80.0f);
        vertical.Height(200.0f);

    CUI::Widgets::Ref vertLabel =std::static_pointer_cast<TextBlock>(
        CreateShowcaseText(FormatPlainRange(20.0f, 80.0f, "%"), 13.0f, "textPrimary", true));
    vertical->OnValueChanged().Connect([vertLabel](RangeSlider*, float lo, float hi) {
                vertLabel.Text(FormatPlainRange(lo, hi, "%"));
    });

    CUI::Widgets::Ref gapSlider = CUI::Widgets::RangeSlider().Shared();
        gapSlider.Minimum(0.0f);
        gapSlider.Maximum(100.0f);
        gapSlider.Step(1.0f);
        gapSlider.MinimumRange(20.0f);
        gapSlider.Range(25.0f, 70.0f);
        gapSlider.Width(320.0f);
        gapSlider.Height(48.0f);

    CUI::Widgets::Ref gapLabel =std::static_pointer_cast<TextBlock>(
        CreateShowcaseText("跨度至少 20  ·  " + FormatPlainRange(25.0f, 70.0f, ""), 12.0f, "textSecondary", false));
    gapSlider->OnValueChanged().Connect([gapLabel](RangeSlider*, float lo, float hi) {
                gapLabel.Text("跨度至少 20  ·  " + FormatPlainRange(lo, hi, ""));
    });

    auto demo = Column(12).Children({
        CreateDemoSurface({
            CreateShowcaseText("1. 价格筛选（Step 50，最小跨度 50）", 12.0f, "textSecondary", false),
            Row(16).Children({ priceLabel }).Build(),
            price,
            Row(8).Children({ btnBudget, btnMid, btnAll }).Build(),
            catalog,
        }, 10.0f),
        CreateDemoSurface({
            CreateShowcaseText("2. 垂直区间（最小跨度 5）", 12.0f, "textSecondary", false),
            Row(16).Children({ vertical, vertLabel }).Build(),
        }, 10.0f),
        CreateDemoSurface({
            CreateShowcaseText("3. 强制最小跨度 20 · 键盘 ←→ 微调，L/U 切换活动端", 12.0f, "textSecondary", false),
            gapSlider,
            gapLabel,
        }, 10.0f),
    }).Build();

    return { "RangeSlider 区间滑块", CreatePage(
        "RangeSlider 双滑块区间",
        "纯自绘双拇指：不可交叉、步长吸附、拖动数值气泡；筛选场景可直接绑下限/上限。",
        demo) };
}
