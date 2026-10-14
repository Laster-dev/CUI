#pragma once
#include <cstdint>
#include <string>

namespace CUI {

/**
 * @brief 文本水平对齐方式枚举
 */
enum class TextAlignment : uint8_t {
    Left = 0,
    Center = 1,
    Right = 2,
    Justified = 3,
    Leading = Left,
    Trailing = Right
};

/**
 * @brief 文本垂直对齐方式枚举
 */
enum class TextVerticalAlignment : uint8_t {
    Top = 0,
    Center = 1,
    Bottom = 2,
    Near = Top,
    Far = Bottom
};

using ParagraphAlignment = TextVerticalAlignment;


/**
 * @brief 字体粗细/字重等级枚举（标准 OpenType/CSS 数值）
 */
enum class FontWeight : uint16_t {
    Thin = 100,
    ExtraLight = 200,
    Light = 300,
    SemiLight = 350,
    Normal = 400,
    Regular = 400,
    Medium = 500,
    DemiBold = 600,
    SemiBold = 600,
    Bold = 700,
    ExtraBold = 800,
    Black = 900,
    ExtraBlack = 950
};

/**
 * @brief 字体倾斜样式枚举
 */
enum class FontStyle : uint8_t {
    Normal = 0,
    Oblique = 1,
    Italic = 2
};

/**
 * @brief 字体水平拉伸宽度枚举
 */
enum class FontStretch : uint8_t {
    Undefined = 0,
    UltraCondensed = 1,
    ExtraCondensed = 2,
    Condensed = 3,
    SemiCondensed = 4,
    Normal = 5,
    Medium = 5,
    SemiExpanded = 6,
    Expanded = 7,
    ExtraExpanded = 8,
    UltraExpanded = 9
};

/**
 * @brief 文本折行模式枚举
 */
enum class TextWrapping : uint8_t {
    NoWrap = 0,
    Wrap = 1,
    EmergencyBreak = 2
};

/**
 * @brief 文本溢出修剪模式枚举
 */
enum class TextTrimming : uint8_t {
    None = 0,
    Character = 1,
    Word = 2,
    Ellipsis = 3
};

/**
 * @brief 统一文本样式复合结构体
 */
struct TextStyle {
    std::string fontFamily = "微软雅黑";
    float fontSize = 13.0f;
    FontWeight weight = FontWeight::Normal;
    FontStyle style = FontStyle::Normal;
    FontStretch stretch = FontStretch::Normal;
    TextAlignment alignment = TextAlignment::Left;
    TextVerticalAlignment verticalAlignment = TextVerticalAlignment::Center;
    TextWrapping wrapping = TextWrapping::NoWrap;
    bool underline = false;
    bool strikethrough = false;
    bool truncateWithEllipsis = false;
    float lineSpacing = 1.0f;
    float lineHeight = 0.0f;
};

// 字符串转换辅助函数
const char* FontWeightToString(FontWeight value);
FontWeight FontWeightFromString(const std::string& value);
const char* FontStyleToString(FontStyle value);
FontStyle FontStyleFromString(const std::string& value);
const char* FontStretchToString(FontStretch value);
FontStretch FontStretchFromString(const std::string& value);
const char* TextAlignmentToString(TextAlignment value);
TextAlignment TextAlignmentFromString(const std::string& value);

} // namespace CUI
