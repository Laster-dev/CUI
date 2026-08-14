#include "Value.h"
#include <windows.h>
#include <sstream>
#include <vector>
#include <algorithm>
#include <iomanip>

namespace CUI {

const Color Color::Transparent(0.0f, 0.0f, 0.0f, 0.0f);
const Color Color::Black(0.0f, 0.0f, 0.0f, 1.0f);
const Color Color::White(1.0f, 1.0f, 1.0f, 1.0f);
const Color Color::Gray(0.5f, 0.5f, 0.5f, 1.0f);
const Color Color::LightGray(0.8f, 0.8f, 0.8f, 1.0f);
const Color Color::Red(1.0f, 0.0f, 0.0f, 1.0f);
const Color Color::Green(0.0f, 1.0f, 0.0f, 1.0f);
const Color Color::Blue(0.0f, 0.0f, 1.0f, 1.0f);
const Color Color::Yellow(1.0f, 1.0f, 0.0f, 1.0f);
const Color Color::Cyan(0.0f, 1.0f, 1.0f, 1.0f);
const Color Color::Magenta(1.0f, 0.0f, 1.0f, 1.0f);
const Color Color::Orange(1.0f, 0.5f, 0.0f, 1.0f);
const Color Color::Purple(0.5f, 0.0f, 0.5f, 1.0f);

Color Color::Hex(const std::string& hexStr) {
    if (hexStr.empty()) return Color(0, 0, 0, 1);

    std::string s = hexStr;
    // Strip leading full-width (＃) or half-width (#) hash signs
    if (s.rfind("#", 0) == 0) {
        s = s.substr(1);
    } else if (s.rfind("\xef\xbc\x83", 0) == 0) {
        s = s.substr(3);
    }

    unsigned int rVal = 0, gVal = 0, bVal = 0, aVal = 255;
    if (s.length() == 3) {
        sscanf_s(s.c_str(), "%1x%1x%1x", &rVal, &gVal, &bVal);
        rVal = rVal * 17;
        gVal = gVal * 17;
        bVal = bVal * 17;
    } else if (s.length() == 4) {
        sscanf_s(s.c_str(), "%1x%1x%1x%1x", &rVal, &gVal, &bVal, &aVal);
        rVal = rVal * 17;
        gVal = gVal * 17;
        bVal = bVal * 17;
        aVal = aVal * 17;
    } else if (s.length() == 6) {
        sscanf_s(s.c_str(), "%2x%2x%2x", &rVal, &gVal, &bVal);
    } else if (s.length() >= 8) {
        sscanf_s(s.c_str(), "%2x%2x%2x%2x", &rVal, &gVal, &bVal, &aVal);
    }

    return Color(
        static_cast<float>(rVal) / 255.0f,
        static_cast<float>(gVal) / 255.0f,
        static_cast<float>(bVal) / 255.0f,
        static_cast<float>(aVal) / 255.0f
    );
}

std::wstring Utf8ToUtf16(const std::string& str) {
    if (str.empty()) return L"";
    int reqLen = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(str.length()), nullptr, 0);
    if (reqLen <= 0) {
        // Fallback to ANSI CP_ACP
        reqLen = MultiByteToWideChar(CP_ACP, 0, str.c_str(), static_cast<int>(str.length()), nullptr, 0);
        if (reqLen <= 0) return L"";
        std::wstring wstr(reqLen, 0);
        MultiByteToWideChar(CP_ACP, 0, str.c_str(), static_cast<int>(str.length()), &wstr[0], reqLen);
        return wstr;
    }
    std::wstring wstr(reqLen, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(str.length()), &wstr[0], reqLen);
    return wstr;
}

std::string Utf16ToUtf8(const std::wstring& wstr) {
    if (wstr.empty()) return "";
    int reqLen = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), static_cast<int>(wstr.length()), nullptr, 0, nullptr, nullptr);
    if (reqLen <= 0) return "";
    std::string str(reqLen, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), static_cast<int>(wstr.length()), &str[0], reqLen, nullptr, nullptr);
    return str;
}

Thickness Thickness::Parse(const std::string& str) {
    std::stringstream ss(str);
    std::vector<float> vals;
    float temp;
    while (ss >> temp) {
        vals.push_back(temp);
        if (ss.peek() == ',' || ss.peek() == ' ') ss.ignore();
    }
    if (vals.size() == 1) {
        return Thickness(vals[0]);
    } else if (vals.size() == 2) {
        return Thickness(vals[1], vals[0], vals[1], vals[0]); // lr, tb
    } else if (vals.size() >= 4) {
        return Thickness(vals[0], vals[1], vals[2], vals[3]);
    }
    return Thickness(0);
}

bool Value::AsBool(bool def) const {
    if (m_type == Type::Bool) return std::get<bool>(m_val);
    if (m_type == Type::Int) return std::get<int>(m_val) != 0;
    if (m_type == Type::String) {
        std::string s = std::get<std::string>(m_val);
        std::transform(s.begin(), s.end(), s.begin(), ::tolower);
        return s == "true" || s == "1" || s == "yes";
    }
    return def;
}

int Value::AsInt(int def) const {
    if (m_type == Type::Int) return std::get<int>(m_val);
    if (m_type == Type::Float) return static_cast<int>(std::get<float>(m_val));
    if (m_type == Type::Bool) return std::get<bool>(m_val) ? 1 : 0;
    if (m_type == Type::String) {
        try { return std::stoi(std::get<std::string>(m_val)); } catch (...) {}
    }
    return def;
}

float Value::AsFloat(float def) const {
    if (m_type == Type::Float) return std::get<float>(m_val);
    if (m_type == Type::Int) return static_cast<float>(std::get<int>(m_val));
    if (m_type == Type::String) {
        try { return std::stof(std::get<std::string>(m_val)); } catch (...) {}
    }
    return def;
}

std::string Value::AsString(const std::string& def) const {
    if (m_type == Type::String) return std::get<std::string>(m_val);
    if (m_type == Type::Bool) return std::get<bool>(m_val) ? "true" : "false";
    if (m_type == Type::Int) return std::to_string(std::get<int>(m_val));
    if (m_type == Type::Float) return std::to_string(std::get<float>(m_val));
    if (m_type == Type::Color) {
        D2D1_COLOR_F c = std::get<D2D1_COLOR_F>(m_val);
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(4)
            << c.r << "," << c.g << "," << c.b << "," << c.a;
        return oss.str();
    }
    if (m_type == Type::Thickness) {
        Thickness t = std::get<Thickness>(m_val);
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(3)
            << t.left << "," << t.top << "," << t.right << "," << t.bottom;
        return oss.str();
    }
    return def;
}

D2D1_COLOR_F Value::AsColor(D2D1_COLOR_F def) const {
    if (m_type == Type::Color) return std::get<D2D1_COLOR_F>(m_val);
    if (m_type == Type::String) return ParseColor(std::get<std::string>(m_val));
    return def;
}

Thickness Value::AsThickness(Thickness def) const {
    if (m_type == Type::Thickness) return std::get<Thickness>(m_val);
    if (m_type == Type::String) return Thickness::Parse(std::get<std::string>(m_val));
    return def;
}

void* Value::AsPointer(void* def) const {
    if (m_type == Type::Pointer) return std::get<void*>(m_val);
    return def;
}

D2D1_COLOR_F Value::ParseColor(const std::string& str) {
    if (str.empty()) return D2D1::ColorF(0, 0, 0, 0);

    std::string s = str;
    if (s[0] == '#') s = s.substr(1);

    unsigned int val = 0;
    std::stringstream ss;
    ss << std::hex << s;
    ss >> val;

    float r = 0.0f, g = 0.0f, b = 0.0f, a = 1.0f;
    if (s.length() == 6) {
        r = ((val >> 16) & 0xFF) / 255.0f;
        g = ((val >> 8) & 0xFF) / 255.0f;
        b = (val & 0xFF) / 255.0f;
    } else if (s.length() == 8) {
        r = ((val >> 24) & 0xFF) / 255.0f;
        g = ((val >> 16) & 0xFF) / 255.0f;
        b = ((val >> 8) & 0xFF) / 255.0f;
        a = (val & 0xFF) / 255.0f;
    }

    return D2D1::ColorF(r, g, b, a);
}

Value Value::ParseAuto(const std::string& str) {
    if (str == "true" || str == "false" || str == "True" || str == "False") {
        std::string lower = str;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        return Value(lower == "true");
    }
    if (!str.empty() && str[0] == '#') {
        return Value(ParseColor(str));
    }
    // Try int / float
    try {
        size_t idx = 0;
        int iv = std::stoi(str, &idx);
        if (idx == str.length()) return Value(iv);
    } catch (...) {}

    try {
        size_t idx = 0;
        float fv = std::stof(str, &idx);
        if (idx == str.length()) return Value(fv);
    } catch (...) {}

    return Value(str);
}

} // namespace CUI
