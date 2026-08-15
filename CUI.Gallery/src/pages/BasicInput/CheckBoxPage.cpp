#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"

#include "framework/core/CUIDsl.h"
#include "framework/core/State.h"
#include "framework/controls/CheckBox.h"
#include <memory>
#include <string>
#include <vector>

using namespace CUI;
using namespace CUI::DSL;

namespace Gallery {

std::shared_ptr<UIElement> BuildCheckBoxPage() {
    auto wifi = Make<CheckBox>("Wi-Fi");
    auto bluetooth = Make<CheckBox>("蓝牙");
    auto airplane = Make<CheckBox>("飞行模式");
    auto selectAll = Make<CheckBox>("全选");
    selectAll->SetIsThreeState(true);

    State<bool> wifiValue{ true };
    State<bool> bluetoothValue{ true };
    State<bool> airplaneValue{ false };

    wifi->Checked->Bind(wifiValue);
    bluetooth->Checked->Bind(bluetoothValue);
    airplane->Checked->Bind(airplaneValue);

    auto selectAllValue = MakeComputed<CheckState>(
        [](bool wifiEnabled, bool bluetoothEnabled, bool airplaneEnabled) {
            const bool allChecked = wifiEnabled && bluetoothEnabled && airplaneEnabled;
            const bool anyChecked = wifiEnabled || bluetoothEnabled || airplaneEnabled;
            return allChecked
                ? CheckState::Checked
                : (anyChecked ? CheckState::Indeterminate : CheckState::Unchecked);
        },
        wifiValue,
        bluetoothValue,
        airplaneValue);
    selectAll->State->Bind(selectAllValue, BindingMode::OneWay);
    State<bool> applyingSelectAll{ false };

    auto statusValue = MakeComputed<std::string>(
        [](bool wifiEnabled, bool bluetoothEnabled, bool airplaneEnabled) {
            const int checkedCount = static_cast<int>(wifiEnabled)
                + static_cast<int>(bluetoothEnabled)
                + static_cast<int>(airplaneEnabled);
            return "已选 " + std::to_string(checkedCount) + " / 3 项。";
        },
        wifiValue,
        bluetoothValue,
        airplaneValue);
    auto status = MakeStatus("");
    status->Text->Bind(statusValue);

    selectAll->OnCheckStateChanged().Connect(
        [wifiValue, bluetoothValue, airplaneValue, selectAllValue, applyingSelectAll](CheckBox* sender, CheckState) {
            if (sender->IsUpdatingFromBinding() || applyingSelectAll) {
                return;
            }

            applyingSelectAll = true;
            const bool selectEverything = selectAllValue->Get() != CheckState::Checked;
            wifiValue = selectEverything;
            bluetoothValue = selectEverything;
            airplaneValue = selectEverything;
            applyingSelectAll = false;
        });

    auto twoState = Make<CheckBox>("我同意条款");
    auto twoStatus = MakeStatus("未同意。");
    twoState->OnCheckStateChanged().Connect([twoStatus](CheckBox*, CheckState state) {
        twoStatus->Text = state == CheckState::Checked ? "已同意。" : "未同意。";
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
        "State<bool> wifiValue{ true };\n"
        "wifi->Checked->Bind(wifiValue);\n"
        "auto allValue = MakeComputed<CheckState>(computeState, wifiValue, bluetoothValue, airplaneValue);\n"
        "selectAll->State->Bind(allValue, BindingMode::OneWay);\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
