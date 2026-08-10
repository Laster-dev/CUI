#include "FastMatch.h"

#include <vector>



namespace EverythingNEO {



namespace {



bool WildcardMatchImpl(const char* s, const char* p, bool ci) {

    auto eq = [ci](char a, char b) {

        if (!ci) return a == b;

        return FoldChar(static_cast<unsigned char>(a)) == FoldChar(static_cast<unsigned char>(b));

    };



    const char* star = nullptr;

    const char* ss = s;

    while (*s) {

        if (*p == '?' || eq(*s, *p)) {

            ++s;

            ++p;

            continue;

        }

        if (*p == '*') {

            star = p++;

            ss = s;

            continue;

        }

        if (star) {

            s = ++ss;

            p = star + 1;

            continue;

        }

        return false;

    }

    while (*p == '*') ++p;

    return *p == '\0';

}



} // namespace



bool MatchWildcard(std::string_view name, std::string_view pattern, bool caseInsensitive) {

    if (pattern.empty()) return true;

    std::string n(name);

    std::string p(pattern);

    return WildcardMatchImpl(n.c_str(), p.c_str(), caseInsensitive);

}



bool MatchRegex(std::string_view text, const std::regex& re) {

    return std::regex_search(text.begin(), text.end(), re);

}



bool QueryLooksLikeWildcard(std::string_view query) {

    return query.find('*') != std::string_view::npos || query.find('?') != std::string_view::npos;

}



void QueryMatcher::Prepare(std::string_view query) {

    m_query.assign(query.data(), query.size());

    m_lowerQuery.clear();

    m_isWildcard = false;

    m_regexReady = false;



    if (m_query.empty()) return;



    // Everything-style: *.dat / test?.txt always use wildcard, even if regex mode is on.

    if (QueryLooksLikeWildcard(m_query)) {

        m_isWildcard = true;

        m_lowerQuery = m_query;

        if (!match_case) ToLowerAsciiInPlace(m_lowerQuery);

        return;

    }



    if (use_regex) {

        try {

            auto flags = std::regex_constants::ECMAScript | std::regex_constants::optimize;

            if (!match_case) flags |= std::regex_constants::icase;

            m_regex = std::regex(m_query, flags);

            m_regexReady = true;

        } catch (...) {

            m_regexReady = false;

        }

        return;

    }



    m_lowerQuery = m_query;

    if (!match_case) ToLowerAsciiInPlace(m_lowerQuery);

}



bool QueryMatcher::Matches(std::string_view text) const {

    if (m_query.empty()) return true;



    if (m_isWildcard) {

        const std::string& pattern = (!match_case && !m_lowerQuery.empty()) ? m_lowerQuery : m_query;

        return MatchWildcard(text, pattern, !match_case);

    }



    if (use_regex) {

        if (!m_regexReady) return false;

        return MatchRegex(text, m_regex);

    }



    if (match_case) {

        return text.find(m_query) != std::string_view::npos;

    }



    if (match_whole_word) {

        return MatchWholeWord(text, m_lowerQuery);

    }

    return MatchLowerQuery(text, m_lowerQuery);

}



} // namespace EverythingNEO

