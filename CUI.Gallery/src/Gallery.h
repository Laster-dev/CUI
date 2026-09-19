#pragma once
/**
 * @file Gallery.h
 * @brief Gallery 示例页的一站式入口头。
 *
 * 每个页面 cpp 只需：
 *      #include "Gallery.h"
 *
 * 即获得：CUI 全量控件与 DSL、主题、页面基类（SamplePage）、
 * 页面声明表（Pages.h）以及常用 STL 头。
 *
 * @note 这里主动定义 CUI_NO_USING_NAMESPACE / CUI_NO_DSL_SHORTCUTS，
 *       是为了不让 umbrella 把 `using namespace CUI;` 和「控件名即工厂」
 *       宏层注入每个页面；各页面仍按需自行书写 using 声明，行为与原先一致。
 */
#ifndef CUI_NO_USING_NAMESPACE
#define CUI_NO_USING_NAMESPACE
#endif
#ifndef CUI_NO_DSL_SHORTCUTS
#define CUI_NO_DSL_SHORTCUTS
#endif

#include "CUI.h"

#include "pages/SamplePage.h"
#include "pages/BasicInput/Pages.h"

#include <memory>
#include <string>
