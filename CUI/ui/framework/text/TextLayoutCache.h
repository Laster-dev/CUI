#pragma once

#include "../render/GraphicsContext.h"
#include <unordered_map>

namespace CUI {

class TextLayoutCache {
public:
    struct Key {
        std::wstring text;
        std::string fontName;
        float fontSize = 13.0f;
        float maxWidth = 10000.0f;
        float maxHeight = 10000.0f;
        DWRITE_WORD_WRAPPING wrapping = DWRITE_WORD_WRAPPING_NO_WRAP;
        DWRITE_FONT_WEIGHT weight = DWRITE_FONT_WEIGHT_NORMAL;
        DWRITE_PARAGRAPH_ALIGNMENT paragraphAlignment = DWRITE_PARAGRAPH_ALIGNMENT_NEAR;
        float lineSpacing = 1.0f;
        float lineHeight = 0.0f;

        bool operator==(const Key& other) const;
    };

    struct KeyHasher {
        size_t operator()(const Key& key) const;
    };

    Microsoft::WRL::ComPtr<IDWriteTextLayout> GetOrCreate(const Key& key);
    void Clear();

private:
    std::unordered_map<Key, Microsoft::WRL::ComPtr<IDWriteTextLayout>, KeyHasher> m_cache;
};

} // namespace CUI
