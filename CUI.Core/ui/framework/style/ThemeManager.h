#pragma once

#include "../core/Value.h"
#include "../window/WindowBackdrop.h"
#include "ThemeTokenId.h"
#include <string>
#include <unordered_map>
#include <vector>

namespace CUI {

/**
 * @brief 云母/亚克力物理背景合成时的材质分级层色。
 * Chrome: 高透明度导航条或标题栏。
 * Surface: 中透明度主工作区内容背景。
 * Solid: 不透明层（下拉框、浮出提示气泡）。
 */
enum class MaterialRole {
    Chrome,  // 铬黄/透明标题栏：具有最高的通透感与磨砂虚化特征
    Surface, // 主板区表面：中等半透率的主窗体正文内容承载背景
    Solid    // 固体：完全不透明的主色，用作下拉弹出菜单与文字提示框以防字迹被背景干扰
};

/**
 * @brief 核心主题色彩标记 Token 解构明细表。
 */
struct ThemeTokens {
    D2D1_COLOR_F windowBackground;  // 窗口默认主背景色
    D2D1_COLOR_F cardBackground;    // 卡片板块容器的背景色
    D2D1_COLOR_F cardBorder;        // 卡片板块外包围线框的描边色
    D2D1_COLOR_F textPrimary;       // 主正文字元前景文本色
    D2D1_COLOR_F textSecondary;     // 次要辅助副标题的前景文本色
    D2D1_COLOR_F textMuted;         // 被遮蔽/禁用的灰色字元前景文本色
    D2D1_COLOR_F accentColor;       // 全局强调高亮高瞩目的主标志色彩
    D2D1_COLOR_F accentForeground;  // 高亮条目上的前景反衬反差文本色
    D2D1_COLOR_F dangerColor;       // 警告/危险类操作专属高亮色彩
    D2D1_COLOR_F paneBackground;     // 侧边栏大抽屉专用的底色背景
    D2D1_COLOR_F inputBackground;    // 输入框底盘背景色
    D2D1_COLOR_F inputBorder;        // 输入框正常状态边框线圈色
    D2D1_COLOR_F hoverBackground;    // 控件被鼠标滑过悬浮时的过渡背景色
    D2D1_COLOR_F pressedBackground;  // 控件被鼠标左键按下交互时的过渡背景色
    D2D1_COLOR_F selectedBackground; // 选项在被选中激活时的背景底盘色
    D2D1_COLOR_F focusedBorder;      // 获得键盘聚焦后激活的突出高亮边框色
};

/**
 * @brief 主题配色方案的获取来源方式。
 * System: 跟随 Windows 操作系统全局主题更改。
 * Light: 强制锁定使用浅色亮色主题。
 * Dark: 强制锁定使用深色暗色主题。
 */
enum class ThemeSource {
    System, // 自动同步并跟随操作系统的深浅模式更改
    Light,  // 锁定并应用明亮色系样式模板
    Dark    // 锁定并应用深黑色系样式模板
};

/**
 * @brief 主题配色方案生命周期与资源配置管理器单例类。
 * ThemeManager 托管全局样式规范，提供：
 * 1. **深浅两色皮肤切换**：支持跟随 Windows 注册表 `AppsUseLightTheme` 广播自动变更。
 * 2. **后置毛玻璃毛刺渲染降噪与 Alpha 重组**：当 Backdrop 开启时，根据 MaterialRole（Chrome/Surface）动态下发材质半透度。
 * 3. **统一配色 Token 提供者**：控件只管查询 Token ID（ThemeTokenId），具体 RGB RGBA 色值在此处分发。
 */
class ThemeManager {
public:
    static ThemeManager& Instance(); // 获取 ThemeManager 全局单例的唯一引用

    static ThemeMode DetectSystemThemeMode(); // 静态调用读取 Win32 注册表以检测系统当前实际处于深色还是浅色模式

    ThemeSource GetThemeSource() const { return m_source; } // 查询当前使用的主题来源方式
    void SetThemeSource(ThemeSource source); // 设定主题来源（若设定为 System 则会立刻发起一次系统深浅检测）

    ThemeMode GetThemeMode() const { return m_mode; } // 获取当前生效的主题类型 (Light / Dark)
    void SetThemeMode(ThemeMode mode); // 手动设定主题模式，此操作将使得主题来源 ThemeSource 强制变更为 Light 或 Dark 锁定

    bool CheckAndUpdateSystemTheme(); // 响应 WM_SETTINGCHANGE，检测系统主题是否发生过物理改变并刷新，有改变返回 true

    bool IsBackdropActive() const { return m_backdropActive; } // 查询窗口云母/亚克力半透明特效当前是否被激活
    void SetBackdropActive(bool active); // 设定是否启用半透明特效（在启用时会适当弱化 windowBackground 的 alpha 权重）
    void SetBackdropType(BackdropType type); // 更改窗口毛玻璃的后置背景合成材质
    BackdropType GetBackdropType() const { return m_backdropType; } // 获取后置背景材质类型

    const ThemeTokens& GetTokens() const { return m_tokens; } // 获得解构后的明细色盘结构

    D2D1_COLOR_F GetColor(ThemeTokenId id) const; // 获得指定 Token ID 对应的实际渲染 RGBA 颜色（在云母启用下自带材质半透混合）
    D2D1_COLOR_F GetColor(const std::string& tokenName) const; // 从字符串格式 Token 名称查找渲染 RGBA 颜色
    D2D1_COLOR_F GetFlatColor(ThemeTokenId id) const; // 获得指定 Token ID 绝对纯实的、永远不附带毛玻璃半透度的扁平 RGBA 颜色
    D2D1_COLOR_F GetFlatColor(const std::string& tokenName) const; // 从字符串 Token 名称获取绝对纯实的扁平 RGBA 颜色
    MaterialRole GetMaterialRole(const std::string& tokenName) const; // 从字符串获取材质作用级别分级
    MaterialRole GetMaterialRole(ThemeTokenId id) const; // 获取对应 Token ID 的材质作用级别分级

    std::string GetColorHex(const std::string& tokenName) const; // 将指定 Token 配色值转化为 "#RRGGBBAA" 格式 Hex 字符串
    static const std::vector<std::string>& GetTokenNames(); // 静态获取注册的所有颜色 Token 名称字符串列表

private:
    ThemeManager();
    void UpdateTokens(); // 彻底重新加载并刷新当前的 Token 配色板缓存数据
    D2D1_COLOR_F LookupBaseColor(const std::string& tokenName) const; // 查找未发生半透转换的基础色值
    D2D1_COLOR_F LookupBaseColor(ThemeTokenId id) const; // 查找未发生半透转换的基础色值
    D2D1_COLOR_F ApplyMaterialRole(const std::string& tokenName, D2D1_COLOR_F base) const; // 融合 Backdrop 材质的半透度修饰
    D2D1_COLOR_F ApplyMaterialRole(ThemeTokenId id, D2D1_COLOR_F base) const; // 融合 Backdrop 材质的半透度修饰

    ThemeSource m_source = ThemeSource::System;          // 配色方案来源，默认设为跟随操作系统
    ThemeMode m_mode = ThemeMode::Dark;                  // 实际所应用的主题皮肤，默认设为 Dark
    ThemeTokens m_tokens{};                              // 缓存的明细色盘数据
    bool m_backdropActive = false;                       // 系统云母毛玻璃后置特效目前是否正处于有效使能状态
    BackdropType m_backdropType = BackdropType::None;    // 激活的后置背景材质类型，默认设为 None
};

} // namespace CUI
