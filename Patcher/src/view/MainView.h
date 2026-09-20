#pragma once

#include "framework/core/Widget.h"

#include <memory>
#include <string>
#include "framework/core/CUIDsl.h"
#include "framework/window/Window.h"
#include "framework/controls/FilePicker.h"
#include "framework/controls/SegmentedControl.h"
#include "framework/controls/ToggleSwitch.h"
#include "framework/controls/AutoSuggestBox.h"
#include "framework/controls/LogView.h"
#include "../core/PePatcher.h"

namespace Patcher::View {

class MainView {
public:
    explicit MainView(CUI::Window* window = nullptr);
    ~MainView() = default;

    std::shared_ptr<CUI::UIElement> Build();

private:
    std::shared_ptr<CUI::UIElement> BuildHeader();
    std::shared_ptr<CUI::UIElement> BuildFilesArea();
    std::shared_ptr<CUI::UIElement> BuildOptionsArea();
    std::shared_ptr<CUI::UIElement> BuildActionsArea();

    void RunPatch();
    void ResetAll();
    void OpenOutputDir();
    void UpdateModeAvailability();
    void RefreshDllFunctionSuggestions();
    bool IsDllTarget();

    CUI::Window* m_window = nullptr;

    // FilePicker 控件引用
    CUI::Widgets::Ref<::CUI::FilePicker> m_fpWhite;
    CUI::Widgets::Ref<::CUI::FilePicker> m_fpPayload;

    // SegmentedControl 控件引用
    CUI::Widgets::Ref<::CUI::SegmentedControl> m_segTargetType;
    CUI::Widgets::Ref<::CUI::SegmentedControl> m_segPatchMode;
    CUI::Widgets::Ref<::CUI::SegmentedControl> m_segSubsystem;
    CUI::Widgets::Ref<::CUI::SegmentedControl> m_segUac;

    // ToggleSwitch 控件引用
    CUI::Widgets::Ref<::CUI::ToggleSwitch> m_swRemoveSig;
    CUI::Widgets::Ref<::CUI::ToggleSwitch> m_swWipeTimestamp;
    CUI::Widgets::Ref<::CUI::ToggleSwitch> m_swDisableCfg;

    // DLL 目标: 在目标类型行右侧显示“覆盖函数”输入框（自动识别/手动选择 DLL 时出现）
    CUI::Widgets::Ref<::CUI::AutoSuggestBox> m_asbDllFunc;
    std::shared_ptr<CUI::UIElement> m_dllFuncArea;  // 目标类型行右侧的“覆盖函数:”区域
    std::shared_ptr<CUI::UIElement> m_targetTypeRow; // 目标类型行（含右侧覆盖函数区域）
    bool m_lastTargetDll = false;                   // 上次生效的目标类型，用于去重日志

    std::wstring m_lastOutputPath;

    // 贴底日志框（所有消息统一走日志，不再使用 Toast）
    CUI::Widgets::Ref<::CUI::LogView> m_logView;

    std::unique_ptr<Core::PePatcher> m_pePatcher;
};

} // namespace Patcher::View
