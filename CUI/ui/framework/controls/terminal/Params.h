#pragma once
#include <algorithm>
#include <vector>

namespace CUI {
namespace Term {

enum class ParserState {
    Ground,
    Escape,
    EscapeIntermediate,
    CsiEntry,
    CsiParam,
    CsiIntermediate,
    CsiIgnore,
    DcsEntry,
    DcsParam,
    DcsIntermediate,
    DcsPassthrough,
    DcsIgnore,
    OscString,
    SosPmApcString
};

class Params {
public:
    Params() { m_params.reserve(16); }

    int Length() const { return static_cast<int>(m_params.size()); }

    int operator[](int index) const {
        return index < static_cast<int>(m_params.size()) ? m_params[static_cast<size_t>(index)] : 0;
    }

    void Reset() {
        m_params.clear();
        m_empty = true;
    }

    void AddParam(int value) {
        m_params.push_back(value);
        m_empty = false;
    }

    void AddDigit(int digit) {
        if (m_empty) {
            m_params.push_back(digit);
            m_empty = false;
        } else {
            int& last = m_params.back();
            int v = last;
            if (v < 0) {
                v = 0;
            }
            last = (std::min)(v * 10 + digit, 9999);
        }
    }

    void FinalizeParam() {
        if (m_empty) {
            m_params.push_back(-1); // omitted
        }
        m_empty = true;
    }

    int Get(int index, int defaultValue = 0) const {
        if (index >= static_cast<int>(m_params.size()) || index < 0) {
            return defaultValue;
        }
        const int v = m_params[static_cast<size_t>(index)];
        return v < 0 ? defaultValue : v;
    }

    int GetNonZero(int index, int defaultValue = 1) const {
        const int v = Get(index, defaultValue);
        return v == 0 ? defaultValue : v;
    }

private:
    std::vector<int> m_params;
    bool m_empty = true;
};

} // namespace Term
} // namespace CUI
