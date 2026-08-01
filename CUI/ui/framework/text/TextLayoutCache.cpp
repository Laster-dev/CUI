#include "TextLayoutCache.h"
#include <functional>

namespace CUI {

bool TextLayoutCache::Key::operator==(const Key& other) const {
    return text == other.text
        && fontName == other.fontName
        && fontSize == other.fontSize
        && maxWidth == other.maxWidth
        && maxHeight == other.maxHeight
        && wrapping == other.wrapping
        && weight == other.weight
        && paragraphAlignment == other.paragraphAlignment
        && lineSpacing == other.lineSpacing
        && lineHeight == other.lineHeight;
}

size_t TextLayoutCache::KeyHasher::operator()(const Key& key) const {
    size_t seed = std::hash<std::wstring>{}(key.text);
    seed ^= std::hash<std::string>{}(key.fontName) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    seed ^= std::hash<float>{}(key.fontSize) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    seed ^= std::hash<float>{}(key.maxWidth) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    seed ^= std::hash<float>{}(key.maxHeight) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    seed ^= std::hash<int>{}(static_cast<int>(key.wrapping)) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    seed ^= std::hash<int>{}(static_cast<int>(key.weight)) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    seed ^= std::hash<int>{}(static_cast<int>(key.paragraphAlignment)) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    seed ^= std::hash<float>{}(key.lineSpacing) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    seed ^= std::hash<float>{}(key.lineHeight) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    return seed;
}

Microsoft::WRL::ComPtr<IDWriteTextLayout> TextLayoutCache::GetOrCreate(const Key& key) {
    auto found = m_cache.find(key);
    if (found != m_cache.end()) {
        return found->second;
    }

    GraphicsContext::TextLayoutOptions options;
    options.maxWidth = key.maxWidth;
    options.maxHeight = key.maxHeight;
    options.wrapping = key.wrapping;
    options.paragraphAlignment = key.paragraphAlignment;
    options.lineSpacing = key.lineSpacing;
    options.lineHeight = key.lineHeight;

    auto layout = GraphicsContext::CreateTextLayout(key.text, key.fontName, key.fontSize, options, key.weight);
    m_cache.emplace(key, layout);
    return layout;
}

void TextLayoutCache::Clear() {
    m_cache.clear();
}

} // namespace CUI
