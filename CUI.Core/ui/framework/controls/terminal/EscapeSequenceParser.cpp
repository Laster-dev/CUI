#include "EscapeSequenceParser.h"

namespace CUI {
namespace Term {

void EscapeSequenceParser::Reset() {
    m_state = ParserState::Ground;
    m_params.Reset();
    m_collect = 0;
    m_osc.clear();
    m_dcs.clear();
    m_utf8Expected = 0;
    m_utf8CodePoint = 0;
    m_utf8SecondMin = 0x80;
    m_utf8SecondMax = 0;
    m_utf8UpperBound = 0x10FFFFu;
    m_stringLength = 0;
}

void EscapeSequenceParser::CollectIntermediate(uint8_t b) {
    // m_collect 是 32 位整型，累积过多 intermediate 字节后继续左移会溢出（未定义行为）。
    if (m_collect >= 0x01000000) {
        return;
    }
    m_collect = (m_collect << 8) | b;
}

void EscapeSequenceParser::AbortStringSequence() {
    // 超长或未终止的字符串序列：直接丢弃，回到 Ground，避免内存被吃光。
    m_osc.clear();
    m_dcs.clear();
    m_stringLength = 0;
    m_collect = 0;
    m_state = ParserState::Ground;
}

void EscapeSequenceParser::Parse(const uint8_t* data, size_t length) {
    if (!data || length == 0) {
        return;
    }

    for (size_t i = 0; i < length; ++i) {
        const uint8_t b = data[i];
        switch (m_state) {
        case ParserState::Ground:            Ground(b); break;
        case ParserState::Escape:             Escape(b); break;
        case ParserState::EscapeIntermediate: EscapeIntermediate(b); break;
        case ParserState::CsiEntry:           CsiEntry(b); break;
        case ParserState::CsiParam:           CsiParam(b); break;
        case ParserState::CsiIntermediate:    CsiIntermediate(b); break;
        case ParserState::CsiIgnore:          CsiIgnore(b); break;
        case ParserState::OscString:          OscString(b); break;
        case ParserState::DcsEntry:           DcsEntry(b); break;
        case ParserState::DcsParam:           DcsParam(b); break;
        case ParserState::DcsIntermediate:    DcsIntermediate(b); break;
        case ParserState::DcsPassthrough:     DcsPassthrough(b); break;
        case ParserState::DcsIgnore:          DcsIgnore(b); break;
        case ParserState::SosPmApcString:     SosPmApcString(b); break;
        }
    }
}

void EscapeSequenceParser::Ground(uint8_t b) {
    if (m_utf8Expected > 0) {
        const bool checkSecond = (m_utf8SecondMax != 0);
        const bool badContinuation =
            (b & 0xC0) != 0x80 ||
            (checkSecond && (b < m_utf8SecondMin || b > m_utf8SecondMax));

        if (badContinuation) {
            // 非法续字节：只输出一个替换字符并丢弃该字节。
            // （不再递归处理，避免把垃圾字节当成一个新字符继续解析。）
            m_utf8Expected = 0;
            m_utf8SecondMax = 0;
            if (OnPrint) OnPrint(0xFFFD);
            return;
        }

        m_utf8SecondMax = 0;
        m_utf8CodePoint = (m_utf8CodePoint << 6) | (b & 0x3F);
        m_utf8Expected--;
        if (m_utf8Expected == 0) {
            const uint32_t cp = static_cast<uint32_t>(m_utf8CodePoint);
            if (OnPrint) {
                OnPrint(cp > m_utf8UpperBound ? 0xFFFD : static_cast<int>(cp));
            }
        }
        return;
    }

    if (b == 0x1B) {
        m_state = ParserState::Escape;
        return;
    }

    if (b < 0x20) {
        if (OnExecute) OnExecute(b);
        return;
    }

    if (b < 0x80) {
        if (OnPrint) OnPrint(b);
        return;
    }

    // UTF-8 首字节：同时记录第一个续字节的合法范围，用于拒绝
    // overlong 编码（C0/C1、E0 80..9F、F0 80..8F）、UTF-16 代理区（ED A0..BF）
    // 以及超出 U+10FFFF 的码点（F4 90..）。
    if ((b & 0xE0) == 0xC0) {
        m_utf8CodePoint = b & 0x1F;
        m_utf8Expected = 1;
        m_utf8SecondMin = 0x80;
        m_utf8SecondMax = 0xBF;
        m_utf8UpperBound = 0x7FFu;
    } else if ((b & 0xF0) == 0xE0) {
        m_utf8CodePoint = b & 0x0F;
        m_utf8Expected = 2;
        m_utf8UpperBound = 0xFFFFu;
        if (b == 0xE0) {
            m_utf8SecondMin = 0xA0; // E0 80..9F 为 overlong
            m_utf8SecondMax = 0xBF;
        } else if (b == 0xED) {
            m_utf8SecondMin = 0x80;
            m_utf8SecondMax = 0x9F; // ED A0..BF 落在 UTF-16 代理区
        } else {
            m_utf8SecondMin = 0x80;
            m_utf8SecondMax = 0xBF;
        }
    } else if ((b & 0xF8) == 0xF0) {
        m_utf8CodePoint = b & 0x07;
        m_utf8Expected = 3;
        m_utf8UpperBound = 0x10FFFFu;
        if (b == 0xF0) {
            m_utf8SecondMin = 0x90; // F0 80..8F 为 overlong
            m_utf8SecondMax = 0xBF;
        } else if (b == 0xF4) {
            m_utf8SecondMin = 0x80;
            m_utf8SecondMax = 0x8F; // F4 90.. 超过 U+10FFFF
        } else {
            m_utf8SecondMin = 0x80;
            m_utf8SecondMax = 0xBF;
        }
    } else {
        // 0x80..0xBF 的游离续字节，以及 0xF8..0xFF 的非法首字节
        if (OnPrint) OnPrint(0xFFFD);
    }
}

void EscapeSequenceParser::Escape(uint8_t b) {
    if (b == 0x1B) {
        return;
    }

    if (b < 0x20) {
        if (OnExecute) OnExecute(b);
        return;
    }

    switch (b) {
    case '[':
        m_params.Reset();
        m_collect = 0;
        m_state = ParserState::CsiEntry;
        return;
    case ']':
        m_osc.clear();
        m_state = ParserState::OscString;
        return;
    case 'P':
        m_params.Reset();
        m_collect = 0;
        m_dcs.clear();
        m_state = ParserState::DcsEntry;
        return;
    case 'X':
    case '^':
    case '_':
        m_stringLength = 0;
        m_state = ParserState::SosPmApcString;
        return;
    default:
        break;
    }

    if (b >= 0x20 && b <= 0x2F) {
        m_collect = b;
        m_state = ParserState::EscapeIntermediate;
        return;
    }

    if (OnEsc) OnEsc(b, m_collect);
    m_collect = 0;
    m_state = ParserState::Ground;
}

void EscapeSequenceParser::EscapeIntermediate(uint8_t b) {
    if (b < 0x20) {
        if (OnExecute) OnExecute(b);
        return;
    }

    if (b >= 0x20 && b <= 0x2F) {
        CollectIntermediate(b);
        return;
    }

    if (OnEsc) OnEsc(b, m_collect);
    m_collect = 0;
    m_state = ParserState::Ground;
}

void EscapeSequenceParser::CsiEntry(uint8_t b) {
    if (b < 0x20) {
        if (OnExecute) OnExecute(b);
        return;
    }

    if (b >= 0x20 && b <= 0x2F) {
        m_collect = b;
        m_state = ParserState::CsiIntermediate;
        return;
    }

    if (b >= 0x30 && b <= 0x39) {
        m_params.AddDigit(b - '0');
        m_state = ParserState::CsiParam;
        return;
    }

    if (b == ';') {
        m_params.FinalizeParam();
        m_state = ParserState::CsiParam;
        return;
    }

    if (b >= 0x3C && b <= 0x3F) {
        m_collect = b;
        m_state = ParserState::CsiParam;
        return;
    }

    if (b >= 0x40 && b <= 0x7E) {
        DispatchCsi(b);
        return;
    }

    m_state = ParserState::CsiIgnore;
}

void EscapeSequenceParser::CsiParam(uint8_t b) {
    if (b < 0x20) {
        if (OnExecute) OnExecute(b);
        return;
    }

    if (b >= 0x30 && b <= 0x39) {
        m_params.AddDigit(b - '0');
        return;
    }

    if (b == ';') {
        m_params.FinalizeParam();
        return;
    }

    if (b == ':') {
        // subparams - treat as separator for now
        m_params.FinalizeParam();
        return;
    }

    if (b >= 0x20 && b <= 0x2F) {
        m_params.FinalizeParam();
        CollectIntermediate(b);
        m_state = ParserState::CsiIntermediate;
        return;
    }

    if (b >= 0x40 && b <= 0x7E) {
        m_params.FinalizeParam();
        DispatchCsi(b);
        return;
    }

    m_state = ParserState::CsiIgnore;
}

void EscapeSequenceParser::CsiIntermediate(uint8_t b) {
    if (b < 0x20) {
        if (OnExecute) OnExecute(b);
        return;
    }

    if (b >= 0x20 && b <= 0x2F) {
        CollectIntermediate(b);
        return;
    }

    if (b >= 0x40 && b <= 0x7E) {
        DispatchCsi(b);
        return;
    }

    m_state = ParserState::CsiIgnore;
}

void EscapeSequenceParser::CsiIgnore(uint8_t b) {
    if (b < 0x20) {
        if (OnExecute) OnExecute(b);
        return;
    }

    if (b >= 0x40 && b <= 0x7E) {
        m_state = ParserState::Ground;
    }
}

void EscapeSequenceParser::DispatchCsi(uint8_t final) {
    if (OnCsi) OnCsi(final, m_params, m_collect);
    m_collect = 0;
    m_state = ParserState::Ground;
}

void EscapeSequenceParser::OscString(uint8_t b) {
    if (b == 0x07 || (b == 0x5C && !m_osc.empty() && m_osc.back() == 0x1B)) {
        if (b == 0x5C && !m_osc.empty()) {
            m_osc.pop_back();
        }
        FinishOsc();
        return;
    }

    // OSC 会一直缓冲到 BEL / ST：恶意或畸形输出可以无限堆积内存，必须有上限。
    if (m_osc.size() >= kMaxStringLength) {
        AbortStringSequence();
        return;
    }

    if (b == 0x1B) {
        m_osc.push_back(b);
        return;
    }

    if (b < 0x20 && b != 0x07) {
        // ignore other C0 in OSC body except BEL
        return;
    }

    m_osc.push_back(b);
}

void EscapeSequenceParser::FinishOsc() {
    const std::string text(reinterpret_cast<const char*>(m_osc.data()), m_osc.size());
    const size_t semi = text.find(';');
    if (OnOsc) {
        if (semi != std::string::npos) {
            OnOsc(text.substr(0, semi), text.substr(semi + 1));
        } else {
            OnOsc(text, std::string());
        }
    }
    m_osc.clear();
    m_state = ParserState::Ground;
}

void EscapeSequenceParser::DcsEntry(uint8_t b) {
    if (b < 0x20) {
        if (OnExecute) OnExecute(b);
        return;
    }

    if (b >= 0x20 && b <= 0x2F) {
        m_collect = b;
        m_state = ParserState::DcsIntermediate;
        return;
    }

    if ((b >= 0x30 && b <= 0x39) || b == ';' || (b >= 0x3C && b <= 0x3F)) {
        if (b >= 0x30 && b <= 0x39) {
            m_params.AddDigit(b - '0');
        } else if (b == ';') {
            m_params.FinalizeParam();
        } else {
            m_collect = b;
        }
        m_state = ParserState::DcsParam;
        return;
    }

    if (b >= 0x40 && b <= 0x7E) {
        m_state = ParserState::DcsPassthrough;
        m_dcs.clear();
        // final already consumed - xterm uses hook; we store final in collect high
        CollectIntermediate(b);
        return;
    }

    m_state = ParserState::DcsIgnore;
}

void EscapeSequenceParser::DcsParam(uint8_t b) {
    if (b >= 0x30 && b <= 0x39) {
        m_params.AddDigit(b - '0');
        return;
    }

    if (b == ';') {
        m_params.FinalizeParam();
        return;
    }

    if (b >= 0x20 && b <= 0x2F) {
        m_params.FinalizeParam();
        CollectIntermediate(b);
        m_state = ParserState::DcsIntermediate;
        return;
    }

    if (b >= 0x40 && b <= 0x7E) {
        m_params.FinalizeParam();
        CollectIntermediate(b);
        m_dcs.clear();
        m_state = ParserState::DcsPassthrough;
        return;
    }

    m_state = ParserState::DcsIgnore;
}

void EscapeSequenceParser::DcsIntermediate(uint8_t b) {
    if (b >= 0x20 && b <= 0x2F) {
        CollectIntermediate(b);
        return;
    }

    if (b >= 0x40 && b <= 0x7E) {
        CollectIntermediate(b);
        m_dcs.clear();
        m_state = ParserState::DcsPassthrough;
        return;
    }

    m_state = ParserState::DcsIgnore;
}

void EscapeSequenceParser::DcsPassthrough(uint8_t b) {
    // DCS 数据同样会缓冲到 ST / BEL，需要长度上限防止内存被恶意输出耗尽。
    if (m_dcs.size() >= kMaxStringLength) {
        AbortStringSequence();
        return;
    }

    if (b == 0x1B) {
        // ST may arrive as the two-byte form: ESC followed by backslash.
        m_dcs.push_back(b);
        return;
    }

    if (b == 0x5C && !m_dcs.empty() && m_dcs.back() == 0x1B) {
        m_dcs.pop_back();
        const uint8_t final = static_cast<uint8_t>(m_collect & 0xFF);
        const std::string data(reinterpret_cast<const char*>(m_dcs.data()), m_dcs.size());
        if (OnDcs) OnDcs(final, m_params, m_collect >> 8, data);
        m_dcs.clear();
        m_collect = 0;
        m_state = ParserState::Ground;
        return;
    }

    if (b == 0x07) {
        const uint8_t final = static_cast<uint8_t>(m_collect & 0xFF);
        const std::string data(reinterpret_cast<const char*>(m_dcs.data()), m_dcs.size());
        if (OnDcs) OnDcs(final, m_params, m_collect >> 8, data);
        m_dcs.clear();
        m_collect = 0;
        m_state = ParserState::Ground;
        return;
    }

    m_dcs.push_back(b);
}

void EscapeSequenceParser::DcsIgnore(uint8_t b) {
    if (b == 0x1B) {
        m_state = ParserState::Escape;
    } else if (b == 0x07) {
        m_state = ParserState::Ground;
    }
}

void EscapeSequenceParser::SosPmApcString(uint8_t b) {
    if (b == 0x1B) {
        // wait for backslash (ST 的两字节形式 ESC '\')
        return;
    }

    if (b == 0x5C || b == 0x07) {
        m_stringLength = 0;
        m_state = ParserState::Ground;
        return;
    }

    // SOS/PM/APC 会一直吞掉后续所有字节直到终止符。若输出里根本没有终止符，
    // 之后的所有内容都会被静默丢弃——累计到上限后强制回到 Ground。
    if (++m_stringLength >= kMaxStringLength) {
        m_stringLength = 0;
        m_state = ParserState::Ground;
    }
}

} // namespace Term
} // namespace CUI
