#include "Value.h"
#include <windows.h>
#include <sstream>
#include <vector>
#include <algorithm>
#include <iomanip>

namespace CUI {

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
    if (str == "true" || str == "false") {
        return Value(str == "true");
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
