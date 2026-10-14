#pragma once

#include "TextLayout.h"
#include <unordered_map>
#include <string>

namespace CUI {

class TextLayoutCache {
public:
    struct Key {
        std::wstring text;
        std::string fontName;
        float fontSize = 13.0f;
        float maxWidth = 10000.0f;
        float maxHeight = 10000.0f;
        TextWrapping wrapping = TextWrapping::NoWrap;
        FontWeight weight = FontWeight::Normal;
        FontStyle style = FontStyle::Normal;
        FontStretch stretch = FontStretch::Normal;
        TextVerticalAlignment paragraphAlignment = TextVerticalAlignment::Top;
        float lineSpacing = 1.0f;
        float lineHeight = 0.0f;

        bool operator==(const Key& other) const;
    };

    struct KeyHasher {
        size_t operator()(const Key& key) const;
    };

    TextLayout GetOrCreate(const Key& key);
    void Clear();

private:
    std::unordered_map<Key, TextLayout, KeyHasher> m_cache;
};

} // namespace CUI
