#include "pages/BasicInput/Pages.h"
#include "pages/SamplePage.h"

#include "framework/core/CUIDsl.h"
#include "framework/core/Observable.h"
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
    auto selectAll = std::make_shared<CheckBox>("全选");
    selectAll->SetIsThreeState(true);

    auto wifiValue = std::make_shared<Observable<bool>>(true);
    auto bluetoothValue = std::make_shared<Observable<bool>>(true);
    auto airplaneValue = std::make_shared<Observable<bool>>(false);

    wifi->Bind(wifiValue);
    bluetooth->Bind(bluetoothValue);
    airplane->Bind(airplaneValue);

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
    selectAll->Bind(selectAllValue, false);
    auto applyingSelectAll = std::make_shared<bool>(false);

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
    status->BindText(statusValue);

    selectAll->OnCheckStateChanged().Connect(
        [wifiValue, bluetoothValue, airplaneValue, selectAllValue, applyingSelectAll](CheckBox* sender, CheckState) {
            if (sender->IsUpdatingFromBinding() || *applyingSelectAll) {
                return;
            }

            *applyingSelectAll = true;
            const bool selectEverything = selectAllValue->Get() != CheckState::Checked;
            wifiValue->Set(selectEverything);
            bluetoothValue->Set(selectEverything);
            airplaneValue->Set(selectEverything);
            *applyingSelectAll = false;
        });

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
        "auto wifiValue = std::make_shared<Observable<bool>>(true);\n"
        "wifi->Bind(wifiValue);\n"
        "auto allValue = MakeComputed<CheckState>(computeState, wifiValue, bluetoothValue, airplaneValue);\n"
        "selectAll->Bind(allValue, false);\n";
    return BuildSamplePage(spec);
}

} // namespace Gallery
