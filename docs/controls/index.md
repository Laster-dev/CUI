---
layout: page
title: 控件参考
nav_order: 2
---

# 控件参考

逐控件详解。每篇统一结构：控件定位 → 类信息 → 构造函数与出厂默认值 → 属性 → 方法 → 运行时行为 → 事件 → 示例 → 主题与自定义 → 注意事项。

## 按钮

| 控件 | 一句话 |
|---|---|
| [Button](Button.html) | 触发一次即时动作，带 Hover/Pressed 过渡与水波纹 |
| [ToggleButton](ToggleButton.html) | 可保持按下 / 弹起状态的按钮 |
| [DropDownButton](DropDownButton.html) | 点任意位置弹出自绘下拉菜单 |
| [SplitButton](SplitButton.html) | 左半执行默认动作、右半弹菜单 |
| [HyperlinkButton](HyperlinkButton.html) | 带下划线的文字链接，携带 `NavigateUri` |

## 选择与开关

| 控件 | 一句话 |
|---|---|
| [CheckBox](CheckBox.html) | 两态 / 三态复选，支持绑定到 `Observable` |
| [RadioButton](RadioButton.html) | 同 `GroupName` 互斥的单选，圆形外观 |
| [ToggleSwitch](ToggleSwitch.html) | 滑道 + 圆钮的开关，可带说明文字 |

## 输入

| 控件 | 一句话 |
|---|---|
| [TextBox](TextBox.html) | 单行 / 多行文本输入，选区、撤销、IME、拖放 |
| [PasswordBox](PasswordBox.html) | 遮罩显示的密码输入，内置"显示密码"按钮 |
| [NumberBox](NumberBox.html) | 带微调箭头的数值输入，提交时支持表达式求值 |
| [AutoSuggestBox](AutoSuggestBox.html) | 输入即弹候选列表，支持自定义 Provider |

## 编写说明

- 所有 API、默认值均取自 `CUI.Core/ui/framework/controls/*.h|*.cpp` 源码，并标注了源码位置。
- 示例中统一使用伞形头 `#include "CUI.h"`。
- 属性表分两档：控件自身属性与继承自基类且常用到的属性；基类通用属性见 [04 · 属性与绑定](../CUI.Core/04-属性与绑定.html)。
