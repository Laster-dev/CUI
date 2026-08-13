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
    list.SetItems(rows);
}
} // namespace

ShowcasePage BuildRangeSliderPage(const ShowcaseContext& ctx) {
    auto price = std::make_shared<RangeSlider>();
    price->SetMinimum(0.0f);
    price->SetMaximum(5000.0f);
    price->SetStep(50.0f);
    price->SetMinimumRange(50.0f);
    price->SetRange(150.0f, 1500.0f);
    price->SetWidth(360.0f);
    price->SetHeight(48.0f);
    price->SetToolTip("拖任一端筛选价格；两滑块不可交叉。");

    auto priceLabel = std::static_pointer_cast<TextBlock>(
        CreateShowcaseText(FormatPriceRange(price->GetLowerValue(), price->GetUpperValue()), 14.0f, "textPrimary", true));

    auto catalog = std::make_shared<ListBox>();
    catalog->SetHeight(148.0f);
    catalog->SetWidth(360.0f);
    FillCatalog(*catalog, price->GetLowerValue(), price->GetUpperValue());

    price->OnValueChanged().Connect([priceLabel, catalog](RangeSlider*, float lo, float hi) {
        priceLabel->SetText(FormatPriceRange(lo, hi));
        FillCatalog(*catalog, lo, hi);
    });

    auto btnBudget = std::make_shared<Button>("百元档");
    btnBudget->OnClick().Connect([price](UIElement*) { price->SetRange(0.0f, 300.0f); });
    auto btnMid = std::make_shared<Button>("中端");
    btnMid->OnClick().Connect([price](UIElement*) { price->SetRange(200.0f, 1500.0f); });
    auto btnAll = std::make_shared<Button>("全部");
    btnAll->OnClick().Connect([price](UIElement*) { price->SetRange(0.0f, 5000.0f); });

    auto vertical = std::make_shared<RangeSlider>();
    vertical->SetOrientation(Orientation::Vertical);
    vertical->SetMinimum(0.0f);
    vertical->SetMaximum(100.0f);
    vertical->SetStep(1.0f);
    vertical->SetMinimumRange(5.0f);
    vertical->SetRange(20.0f, 80.0f);
    vertical->SetWidth(80.0f);
    vertical->SetHeight(200.0f);

    auto vertLabel = std::static_pointer_cast<TextBlock>(
        CreateShowcaseText(FormatPlainRange(20.0f, 80.0f, "%"), 13.0f, "textPrimary", true));
    vertical->OnValueChanged().Connect([vertLabel](RangeSlider*, float lo, float hi) {
        vertLabel->SetText(FormatPlainRange(lo, hi, "%"));
    });

    auto gapSlider = std::make_shared<RangeSlider>();
    gapSlider->SetMinimum(0.0f);
    gapSlider->SetMaximum(100.0f);
    gapSlider->SetStep(1.0f);
    gapSlider->SetMinimumRange(20.0f);
    gapSlider->SetRange(25.0f, 70.0f);
    gapSlider->SetWidth(320.0f);
    gapSlider->SetHeight(48.0f);

    auto gapLabel = std::static_pointer_cast<TextBlock>(
        CreateShowcaseText("跨度至少 20  ·  " + FormatPlainRange(25.0f, 70.0f, ""), 12.0f, "textSecondary", false));
    gapSlider->OnValueChanged().Connect([gapLabel](RangeSlider*, float lo, float hi) {
        gapLabel->SetText("跨度至少 20  ·  " + FormatPlainRange(lo, hi, ""));
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
