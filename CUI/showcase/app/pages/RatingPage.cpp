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
    CUI::Widgets::Ref interactive = CUI::Widgets::RatingControl().Shared();
        interactive.Value(3.5f);
        interactive.MaxRating(5);
        interactive.Step(0.5f);
        interactive.IsClearEnabled(true);
        interactive.ToolTip("拖动或点击星星评分，支持半星；同一星再点或左侧 × 清除。");

    CUI::Widgets::Ref valueLabel =std::static_pointer_cast<TextBlock>(
        CreateShowcaseText(FormatRating(interactive->GetValue(), interactive->GetMaxRating()), 13.0f, "textPrimary", true));
    interactive->OnValueChanged().Connect([valueLabel, interactive](RatingControl*, float v) {
                valueLabel.Text(FormatRating(v, interactive->GetMaxRating()));
    });

    CUI::Widgets::Ref btn0 = CUI::Widgets::Button("清除 0").Shared();
    btn0->OnClick().Connect([interactive](UIElement*) {     interactive.Value(0.0f); });
    CUI::Widgets::Ref btnHalf = CUI::Widgets::Button("2.5").Shared();
    btnHalf->OnClick().Connect([interactive](UIElement*) {     interactive.Value(2.5f); });
    CUI::Widgets::Ref btnFull = CUI::Widgets::Button("满分 5").Shared();
    btnFull->OnClick().Connect([interactive](UIElement*) {     interactive.Value(5.0f); });

    CUI::Widgets::Ref chkReadOnly = CUI::Widgets::CheckBox("只读").Shared();
    chkReadOnly->OnCheckStateChanged().Connect([interactive](CheckBox*, CheckState st) {
                interactive.IsReadOnly(st == CheckState::Checked);
    });
    CUI::Widgets::Ref chkClear = CUI::Widgets::CheckBox("允许清除").Shared();
        chkClear.State(CheckState::Checked);
    chkClear->OnCheckStateChanged().Connect([interactive](CheckBox*, CheckState st) {
                interactive.IsClearEnabled(st == CheckState::Checked);
    });

    CUI::Widgets::Ref readonly = CUI::Widgets::RatingControl().Shared();
        readonly.Value(4.5f);
        readonly.IsReadOnly(true);
        readonly.IsClearEnabled(false);

    CUI::Widgets::Ref ten = CUI::Widgets::RatingControl().Shared();
        ten.MaxRating(10);
        ten.Step(1.0f);
        ten.Value(7.0f);
        ten.IsClearEnabled(true);

    CUI::Widgets::Ref tenLabel =std::static_pointer_cast<TextBlock>(
        CreateShowcaseText(FormatRating(ten->GetValue(), ten->GetMaxRating()), 12.0f, "textSecondary", false));
    ten->OnValueChanged().Connect([tenLabel, ten](RatingControl*, float v) {
                tenLabel.Text(FormatRating(v, ten->GetMaxRating()));
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

