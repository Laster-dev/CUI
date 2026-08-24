#include "PageRegistry.h"
#include "../ShowcaseHelpers.h"
#include "framework/core/CUIDsl.h"
#include "framework/controls/RatingControl.h"
#include "framework/controls/Button.h"
#include "framework/controls/TextBlock.h"
#include "framework/controls/CheckBox.h"
#include <sstream>
#include <iomanip>

using namespace CUI;
using namespace CUI::DSL;

namespace {
std::string FormatRating(float value, int maxRating) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << value << " / " << maxRating;
    return oss.str();
}
} // namespace

ShowcasePage BuildRatingPage(const ShowcaseContext& ctx) {
    auto interactive = std::make_shared<RatingControl>();
    CUI::DSL::Borrow(interactive).Value(3.5f);
    DSL::Borrow(interactive).MaxRating(5);
    DSL::Borrow(interactive).Step(0.5f);
    DSL::Borrow(interactive).IsClearEnabled(true);
    CUI::DSL::Borrow(interactive).ToolTip("拖动或点击星星评分，支持半星；同一星再点或左侧 × 清除。");

    auto valueLabel = std::static_pointer_cast<TextBlock>(
        CreateShowcaseText(FormatRating(interactive->GetValue(), interactive->GetMaxRating()), 13.0f, "textPrimary", true));
    interactive->OnValueChanged().Connect([valueLabel, interactive](RatingControl*, float v) {
        CUI::DSL::Borrow(valueLabel).Text(FormatRating(v, interactive->GetMaxRating()));
    });

    auto btn0 = std::make_shared<Button>("清除 0");
    btn0->OnClick().Connect([interactive](UIElement*) { DSL::Borrow(interactive).Value(0.0f); });
    auto btnHalf = std::make_shared<Button>("2.5");
    btnHalf->OnClick().Connect([interactive](UIElement*) { DSL::Borrow(interactive).Value(2.5f); });
    auto btnFull = std::make_shared<Button>("满分 5");
    btnFull->OnClick().Connect([interactive](UIElement*) { DSL::Borrow(interactive).Value(5.0f); });

    auto chkReadOnly = std::make_shared<CheckBox>("只读");
    chkReadOnly->OnCheckStateChanged().Connect([interactive](CheckBox*, CheckState st) {
        CUI::DSL::Borrow(interactive).IsReadOnly(st == CheckState::Checked);
    });
    auto chkClear = std::make_shared<CheckBox>("允许清除");
    CUI::DSL::Borrow(chkClear).State(CheckState::Checked);
    chkClear->OnCheckStateChanged().Connect([interactive](CheckBox*, CheckState st) {
        DSL::Borrow(interactive).IsClearEnabled(st == CheckState::Checked);
    });

    auto readonly = std::make_shared<RatingControl>();
    CUI::DSL::Borrow(readonly).Value(4.5f);
    CUI::DSL::Borrow(readonly).IsReadOnly(true);
    DSL::Borrow(readonly).IsClearEnabled(false);

    auto ten = std::make_shared<RatingControl>();
    DSL::Borrow(ten).MaxRating(10);
    DSL::Borrow(ten).Step(1.0f);
    CUI::DSL::Borrow(ten).Value(7.0f);
    DSL::Borrow(ten).IsClearEnabled(true);

    auto tenLabel = std::static_pointer_cast<TextBlock>(
        CreateShowcaseText(FormatRating(ten->GetValue(), ten->GetMaxRating()), 12.0f, "textSecondary", false));
    ten->OnValueChanged().Connect([tenLabel, ten](RatingControl*, float v) {
        CUI::DSL::Borrow(tenLabel).Text(FormatRating(v, ten->GetMaxRating()));
    });

    auto demo = Column(12).Children({
        CreateDemoSurface({
            CreateShowcaseText("1. 交互评分（半星、左侧清除、键盘 ←→ / Home / End）", 12.0f, "textSecondary", false),
            Row(12).Children({ interactive, valueLabel }).Build(),
            Row(8).Children({ btn0, btnHalf, btnFull, chkReadOnly, chkClear }).Build(),
        }, 10.0f),
        CreateDemoSurface({
            CreateShowcaseText("2. 只读 4.5 / 5（无 hover / 键盘）", 12.0f, "textSecondary", false),
            readonly,
        }, 10.0f),
        CreateDemoSurface({
            CreateShowcaseText("3. 十分制（MaxRating = 10，Step = 1）", 12.0f, "textSecondary", false),
            Row(12).Children({ ten, tenLabel }).Build(),
        }, 10.0f),
    }).Build();

    return { "RatingControl 评分", CreatePage(
        "RatingControl 星级评分",
        "纯自绘五角星：半星填充、hover 预览、点击定值、点同一星或 × 清除；主题色跟随 Accent。",
        demo) };
}

