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
    void CollectIntermediate(uint8_t b);
    void AbortStringSequence();

    ParserState m_state = ParserState::Ground;
    Params m_params;
    int m_collect = 0;
    std::vector<uint8_t> m_osc;
    std::vector<uint8_t> m_dcs;
    int m_utf8Expected = 0;
    int m_utf8CodePoint = 0;

    // UTF-8 合法性校验：第一个续字节的允许范围（拒绝 overlong / 代理区 / > U+10FFFF）。
    uint8_t m_utf8SecondMin = 0x80;
    uint8_t m_utf8SecondMax = 0; // 0 表示“无需检查第一个续字节”
    uint32_t m_utf8UpperBound = 0x10FFFFu;

    // 字符串型序列的长度保护：OSC/DCS 会一直缓冲到终止符，恶意输出可无限吃内存。
    static constexpr size_t kMaxStringLength = 64u * 1024u;
    size_t m_stringLength = 0; // 仅用于不保存内容的 SOS/PM/APC 序列计数
};

} // namespace Term
} // namespace CUI
