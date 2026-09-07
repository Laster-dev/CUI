#pragma once

#include <memory>
#include <string>
#include "framework/core/CUIDsl.h"
#include "framework/window/Window.h"
#include "framework/controls/FilePicker.h"
#include "framework/controls/SegmentedControl.h"
#include "framework/controls/ToggleSwitch.h"
#include "framework/controls/LogView.h"
#include "framework/controls/ToastCenter.h"
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

    CUI::Window* m_window = nullptr;

    // FilePicker 控件引用
    std::shared_ptr<CUI::FilePicker> m_fpWhite;
    std::shared_ptr<CUI::FilePicker> m_fpPayload;

    // SegmentedControl 控件引用
    std::shared_ptr<CUI::SegmentedControl> m_segPatchMode;
    std::shared_ptr<CUI::SegmentedControl> m_segSubsystem;
    std::shared_ptr<CUI::SegmentedControl> m_segUac;

    // ToggleSwitch 控件引用
    std::shared_ptr<CUI::ToggleSwitch> m_swRemoveSig;
    std::shared_ptr<CUI::ToggleSwitch> m_swWipeTimestamp;

    std::wstring m_lastOutputPath;

    // 贴底日志框与通知
    std::shared_ptr<CUI::LogView> m_logView;
    float m_lastLogHeight = 0.0f;
    std::shared_ptr<CUI::ToastCenter> m_toastCenter;

    std::unique_ptr<Core::PePatcher> m_pePatcher;
};

} // namespace Patcher::View
