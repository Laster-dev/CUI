#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>

namespace CUI {

enum class DragDropEffects : uint8_t {
    None = 0,
    Copy = 1,
    Move = 2,
    Link = 4
};

inline DragDropEffects operator|(DragDropEffects a, DragDropEffects b) {
    return static_cast<DragDropEffects>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}
inline DragDropEffects operator&(DragDropEffects a, DragDropEffects b) {
    return static_cast<DragDropEffects>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}
inline bool HasEffect(DragDropEffects mask, DragDropEffects bit) {
    return (static_cast<uint8_t>(mask) & static_cast<uint8_t>(bit)) != 0;
}

class DataPackage {
public:
    void SetText(std::string text);
    bool HasText() const { return m_hasText; }
    const std::string& GetText() const { return m_text; }

    void SetFiles(std::vector<std::string> paths);
    bool HasFiles() const { return !m_files.empty(); }
    const std::vector<std::string>& GetFiles() const { return m_files; }

    void SetFormat(const std::string& mime, std::string payload);
    bool HasFormat(const std::string& mime) const;
    std::string GetFormat(const std::string& mime) const;

    std::string PreviewLabel() const;

private:
    std::string m_text;
    bool m_hasText = false;
    std::vector<std::string> m_files;
    std::unordered_map<std::string, std::string> m_formats;
};

} // namespace CUI
