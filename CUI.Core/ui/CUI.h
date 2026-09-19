#pragma once

// ============================================================================
//  CUI.h —— CUI.Core 唯一入口头（Umbrella Header）
//
//  用法（附加包含目录需包含 CUI.Core\ui）：
//      #include "CUI.h"
//
//  这一个头会带出：核心对象模型与属性系统、全部控件与 DSL、布局、渲染上下文、
//  主题、文本、动画、输入路由、拖放、窗口宿主。
//
//  默认把 `CUI` 与 `CUI::DSL` 注入全局命名空间，可直接写 Column() / Text() / Window。
//  若不想污染全局，在 include 之前定义：
//      #define CUI_NO_USING_NAMESPACE
//      #include "CUI.h"
// ============================================================================

// ---------------------------------------------------------------- 核心对象模型
#include "framework/core/Object.h"
#include "framework/core/Value.h"
#include "framework/core/PropertyId.h"
#include "framework/core/Property.h"
#include "framework/core/PropertyDesc.h"
#include "framework/core/BindableProperty.h"
#include "framework/core/Binding.h"
#include "framework/core/ValueConverter.h"
#include "framework/core/Event.h"
#include "framework/core/Observable.h"
#include "framework/core/State.h"

// ------------------------------------------------------------ 控件与 DSL（全量）
#include "framework/core/CUIDsl.h"

// -------------------------------------------------------------- 控件基类（显式）
#include "framework/controls/UIElement.h"
#include "framework/controls/Control.h"
#include "framework/controls/Panel.h"

// -------------------------------------------------------------------- 布局 / 渲染
#include "framework/layout/Layout.h"
#include "framework/render/GraphicsContext.h"
#include "framework/render/RenderNode.h"
#include "framework/render/RenderLayer.h"
#include "framework/render/DirtyRegion.h"
#include "framework/render/CompositionContext.h"

// ---------------------------------------------------------------- 主题 / 文本
#include "framework/style/ThemeManager.h"
#include "framework/style/ThemeTokenId.h"
#include "framework/text/TextLayoutCache.h"

// -------------------------------------------------------------------- 动画系统
#include "framework/animation/AnimationService.h"
#include "framework/animation/AnimationManager.h"
#include "framework/animation/FrameScheduler.h"
#include "framework/animation/AnimationSystem.h"

// ------------------------------------------------------------ 输入 / 命令 / 拖放
#include "framework/input/RoutedEvent.h"
#include "framework/input/Command.h"
#include "framework/dnd/DragDropService.h"

// -------------------------------------------------------------------- 窗口与宿主
#include "framework/window/Dpi.h"
#include "framework/window/Window.h"
#include "framework/window/CUIWindow.h"
#include "framework/window/PopupHost.h"
#include "framework/window/PopupPlacement.h"
#include "framework/window/MenuPopupWindow.h"
#include "framework/window/WindowBackdrop.h"
#include "framework/window/IWindowChrome.h"

// ---------------------------------------------------------------- 命名空间快捷方式
#ifndef CUI_NO_USING_NAMESPACE
using namespace CUI;
using namespace CUI::DSL;
#endif
