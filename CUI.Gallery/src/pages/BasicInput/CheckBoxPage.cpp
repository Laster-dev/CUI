#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"

#include "framework/core/CUIDsl.h"
#include "framework/controls/CheckBox.h"
#include <memory>
#include <string>
#include <vector>

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

std::shared_ptr<UIElement> BuildCheckBoxPage() {
    auto wifi = std::make_shared<CheckBox>("Wi-Fi");
    auto bluetooth = std::make_shared<CheckBox>("蓝牙");
    auto airplane = std::make_shared<CheckBox>("飞行模式");
    wifi->SetState(CheckState::Checked);
    bluetooth->SetState(CheckState::Checked);

    auto selectAll = std::make_shared<CheckBox>("全选");
    selectAll->SetIsThreeState(true);

    auto status = MakeStatus("");
    auto options = std::make_shared<std::vector<std::shared_ptr<CheckBox>>>(
        std::vector<std::shared_ptr<CheckBox>>{ wifi, bluetooth, airplane });
    auto syncing = std::make_shared<bool>(false);

    auto updateStatus = [options, status]() {
        int n = 0;
        for (const auto& cb : *options) {
            if (cb->GetState() == CheckState::Checked) {
                ++n;
            }
        }
        status->SetText("已选 " + std::to_string(n) + " / " + std::to_string(options->size()) + " 项。");
    };

    auto syncFromItems = [selectAll, options, syncing, updateStatus]() {
        if (*syncing) {
            return;
        }
        int n = 0;
        for (const auto& cb : *options) {
            if (cb->GetState() == CheckState::Checked) {
                ++n;
            }
        }
        *syncing = true;
        if (n == 0) {
            selectAll->SetState(CheckState::Unchecked);
        } else if (n == static_cast<int>(options->size())) {
            selectAll->SetState(CheckState::Checked);
        } else {
            selectAll->SetState(CheckState::Indeterminate);
        }
        *syncing = false;
        updateStatus();
    };

    selectAll->OnCheckStateChanged().Connect(
        [selectAll, options, syncing, updateStatus](CheckBox*, CheckState state) {
            if (*syncing) {
                return;
            }
            *syncing = true;
            if (state == CheckState::Indeterminate) {
                state = CheckState::Unchecked;
                selectAll->SetState(CheckState::Unchecked);
            }
            const CheckState itemState = (state == CheckState::Checked)
                ? CheckState::Checked
                : CheckState::Unchecked;
            for (const auto& cb : *options) {
                cb->SetState(itemState);
            }
            *syncing = false;
            updateStatus();
        });

    for (const auto& cb : *options) {
        cb->OnCheckStateChanged().Connect([syncFromItems](CheckBox*, CheckState) {
            syncFromItems();
        });
    }
    syncFromItems();

    auto twoState = std::make_shared<CheckBox>("我同意条款");
    auto twoStatus = MakeStatus("未同意。");
    twoState->OnCheckStateChanged().Connect([twoStatus](CheckBox*, CheckState state) {
        twoStatus->SetText(state == CheckState::Checked ? "已同意。" : "未同意。");
    });

    SamplePageSpec spec;
    spec.title = "CheckBox(复选框)";
    spec.subtitle = "复选框用于打开或关闭选项。三态还可表示部分选中。";
    spec.sections = {
        {
            "全选",
            "标题跟随列表。部分选中显示为不确定状态。单击可全选或全清。",
            Column(8).Children({
                selectAll,
                wifi,
                bluetooth,
                airplane,
                status,
            }).Build(),
        },
        {
            "两态",
            "单个是/否选项。",
            Column(8).Children({ twoState, twoStatus }).Build(),
        },
    };
    spec.source =
        "auto all = std::make_shared<CheckBox>(\"Select all\");\n"
        "all->SetIsThreeState(true);\n"
        "all->OnCheckStateChanged().Connect([](CheckBox*, CheckState state) {\n"
        "    // sync children\n"
        "});\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
