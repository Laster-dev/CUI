#pragma once
#include "Params.h"
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace CUI {
namespace Term {

// VT/xterm escape sequence parser state machine (aligned with xterm.js EscapeSequenceParser).
class EscapeSequenceParser {
public:
    std::function<void(int)> OnPrint;
    std::function<void(uint8_t)> OnExecute;
    std::function<void(uint8_t, const Params&, int)> OnCsi;
    std::function<void(uint8_t, int)> OnEsc;
    std::function<void(const std::string&, const std::string&)> OnOsc;
    std::function<void(uint8_t, const Params&, int, const std::string&)> OnDcs;

    void Reset();
    void Parse(const uint8_t* data, size_t length);
    void Parse(const std::string& data) {
        Parse(reinterpret_cast<const uint8_t*>(data.data()), data.size());
    }

private:
    void Ground(uint8_t b);
    void Escape(uint8_t b);
    void EscapeIntermediate(uint8_t b);
    void CsiEntry(uint8_t b);
    void CsiParam(uint8_t b);
    void CsiIntermediate(uint8_t b);
    void CsiIgnore(uint8_t b);
    void DispatchCsi(uint8_t final);
    void OscString(uint8_t b);
    void FinishOsc();
    void DcsEntry(uint8_t b);
    void DcsParam(uint8_t b);
    void DcsIntermediate(uint8_t b);
    void DcsPassthrough(uint8_t b);
    void DcsIgnore(uint8_t b);
    void SosPmApcString(uint8_t b);

    ParserState m_state = ParserState::Ground;
    Params m_params;
    int m_collect = 0;
    std::vector<uint8_t> m_osc;
    std::vector<uint8_t> m_dcs;
    int m_utf8Expected = 0;
    int m_utf8CodePoint = 0;
};

} // namespace Term
} // namespace CUI
