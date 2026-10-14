#include "TextTypes.h"
#include <algorithm>

namespace CUI {

const char* FontWeightToString(FontWeight value) {
    switch (value) {
    case FontWeight::Thin: return "Thin";
    case FontWeight::ExtraLight: return "ExtraLight";
    case FontWeight::Light: return "Light";
    case FontWeight::SemiLight: return "SemiLight";
    case FontWeight::Normal: return "Normal";
    case FontWeight::Medium: return "Medium";
    case FontWeight::SemiBold: return "SemiBold";
    case FontWeight::Bold: return "Bold";
    case FontWeight::ExtraBold: return "ExtraBold";
    case FontWeight::Black: return "Black";
    case FontWeight::ExtraBlack: return "ExtraBlack";
    default: return "Normal";
    }
}

FontWeight FontWeightFromString(const std::string& value) {
    if (value == "Thin") return FontWeight::Thin;
    if (value == "ExtraLight") return FontWeight::ExtraLight;
    if (value == "Light") return FontWeight::Light;
    if (value == "SemiLight") return FontWeight::SemiLight;
    if (value == "Medium") return FontWeight::Medium;
    if (value == "SemiBold" || value == "DemiBold") return FontWeight::SemiBold;
    if (value == "Bold") return FontWeight::Bold;
    if (value == "ExtraBold") return FontWeight::ExtraBold;
    if (value == "Black") return FontWeight::Black;
    if (value == "ExtraBlack") return FontWeight::ExtraBlack;
    return FontWeight::Normal;
}

const char* FontStyleToString(FontStyle value) {
    switch (value) {
    case FontStyle::Italic: return "Italic";
    case FontStyle::Oblique: return "Oblique";
    default: return "Normal";
    }
}

FontStyle FontStyleFromString(const std::string& value) {
    if (value == "Italic") return FontStyle::Italic;
    if (value == "Oblique") return FontStyle::Oblique;
    return FontStyle::Normal;
}

const char* FontStretchToString(FontStretch value) {
    switch (value) {
    case FontStretch::UltraCondensed: return "UltraCondensed";
    case FontStretch::ExtraCondensed: return "ExtraCondensed";
    case FontStretch::Condensed: return "Condensed";
    case FontStretch::SemiCondensed: return "SemiCondensed";
    case FontStretch::SemiExpanded: return "SemiExpanded";
    case FontStretch::Expanded: return "Expanded";
    case FontStretch::ExtraExpanded: return "ExtraExpanded";
    case FontStretch::UltraExpanded: return "UltraExpanded";
    default: return "Normal";
    }
}

FontStretch FontStretchFromString(const std::string& value) {
    if (value == "UltraCondensed") return FontStretch::UltraCondensed;
    if (value == "ExtraCondensed") return FontStretch::ExtraCondensed;
    if (value == "Condensed") return FontStretch::Condensed;
    if (value == "SemiCondensed") return FontStretch::SemiCondensed;
    if (value == "SemiExpanded") return FontStretch::SemiExpanded;
    if (value == "Expanded") return FontStretch::Expanded;
    if (value == "ExtraExpanded") return FontStretch::ExtraExpanded;
    if (value == "UltraExpanded") return FontStretch::UltraExpanded;
    return FontStretch::Normal;
}

const char* TextAlignmentToString(TextAlignment value) {
    switch (value) {
    case TextAlignment::Center: return "Center";
    case TextAlignment::Right: return "Right";
    case TextAlignment::Justified: return "Justified";
    default: return "Left";
    }
}

TextAlignment TextAlignmentFromString(const std::string& value) {
    if (value == "Center") return TextAlignment::Center;
    if (value == "Right" || value == "Trailing") return TextAlignment::Right;
    if (value == "Justified") return TextAlignment::Justified;
    return TextAlignment::Left;
}

} // namespace CUI
