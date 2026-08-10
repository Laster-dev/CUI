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
        if ((b & 0xC0) != 0x80) {
            m_utf8Expected = 0;
            if (OnPrint) OnPrint(0xFFFD);
            Ground(b);
            return;
        }

        m_utf8CodePoint = (m_utf8CodePoint << 6) | (b & 0x3F);
        m_utf8Expected--;
        if (m_utf8Expected == 0 && OnPrint) {
            OnPrint(m_utf8CodePoint);
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

    if ((b & 0xE0) == 0xC0) {
        m_utf8CodePoint = b & 0x1F;
        m_utf8Expected = 1;
    } else if ((b & 0xF0) == 0xE0) {
        m_utf8CodePoint = b & 0x0F;
        m_utf8Expected = 2;
    } else if ((b & 0xF8) == 0xF0) {
        m_utf8CodePoint = b & 0x07;
        m_utf8Expected = 3;
    } else {
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
        m_collect = (m_collect << 8) | b;
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
        m_collect = (m_collect << 8) | b;
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
        m_collect = (m_collect << 8) | b;
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
        m_collect = (m_collect << 8) | b;
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
        m_collect = (m_collect << 8) | b;
        m_state = ParserState::DcsIntermediate;
        return;
    }

    if (b >= 0x40 && b <= 0x7E) {
        m_params.FinalizeParam();
        m_collect = (m_collect << 8) | b;
        m_dcs.clear();
        m_state = ParserState::DcsPassthrough;
        return;
    }

    m_state = ParserState::DcsIgnore;
}

void EscapeSequenceParser::DcsIntermediate(uint8_t b) {
    if (b >= 0x20 && b <= 0x2F) {
        m_collect = (m_collect << 8) | b;
        return;
    }

    if (b >= 0x40 && b <= 0x7E) {
        m_collect = (m_collect << 8) | b;
        m_dcs.clear();
        m_state = ParserState::DcsPassthrough;
        return;
    }

    m_state = ParserState::DcsIgnore;
}

void EscapeSequenceParser::DcsPassthrough(uint8_t b) {
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
        // wait for backslash
        return;
    }

    if (b == 0x5C || b == 0x07) {
        m_state = ParserState::Ground;
    }
}

} // namespace Term
} // namespace CUI
