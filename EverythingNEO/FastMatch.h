#pragma once



#include <cstdint>

#include <cstring>

#include <string>

#include <string_view>

#include <regex>



namespace EverythingNEO {



struct CaseFold {

    unsigned char lower[256]{};

    CaseFold() {

        for (int i = 0; i < 256; ++i) lower[i] = static_cast<unsigned char>(i);

        for (int i = 'A'; i <= 'Z'; ++i) lower[i] = static_cast<unsigned char>(i - 'A' + 'a');

    }

};



inline const CaseFold& GetCaseFold() {

    static const CaseFold kFold;

    return kFold;

}



inline void ToLowerAsciiInPlace(std::string& s) {

    const auto& fold = GetCaseFold().lower;

    for (char& c : s) c = static_cast<char>(fold[static_cast<unsigned char>(c)]);

}



inline char FoldChar(unsigned char c) {

    return static_cast<char>(GetCaseFold().lower[c]);

}



inline bool MatchLowerQuery(std::string_view name, std::string_view lowerQuery) {

    const size_t n = name.size();

    const size_t qn = lowerQuery.size();

    if (qn == 0) return true;

    if (n < qn) return false;



    const auto& fold = GetCaseFold().lower;

    const unsigned char* s = reinterpret_cast<const unsigned char*>(name.data());

    const unsigned char* q = reinterpret_cast<const unsigned char*>(lowerQuery.data());

    const unsigned char q0 = q[0];

    const size_t last = n - qn;

    for (size_t i = 0; i <= last; ++i) {

        if (fold[s[i]] != q0) continue;

        size_t j = 1;

        for (; j < qn; ++j) {

            if (fold[s[i + j]] != q[j]) break;

        }

        if (j == qn) return true;

    }

    return false;

}



inline bool MatchWholeWord(std::string_view name, std::string_view lowerQuery) {

    const size_t n = name.size();

    const size_t qn = lowerQuery.size();

    if (qn == 0) return true;

    if (n < qn) return false;



    const auto& fold = GetCaseFold().lower;

    const unsigned char* s = reinterpret_cast<const unsigned char*>(name.data());

    const unsigned char* q = reinterpret_cast<const unsigned char*>(lowerQuery.data());



    for (size_t i = 0; i + qn <= n; ++i) {

        if ((i > 0 && s[i - 1] != '.' && s[i - 1] != '\\' && s[i - 1] != '/'

             && s[i - 1] != '_' && s[i - 1] != '-' && s[i - 1] != ' ')

            && !((i > 0) && fold[s[i - 1]] == fold[s[i]])) {

            // allow word start at boundary chars

        }

        bool match = true;

        for (size_t j = 0; j < qn; ++j) {

            if (fold[s[i + j]] != q[j]) { match = false; break; }

        }

        if (!match) continue;

        size_t end = i + qn;

        if (end < n) {

            unsigned char next = s[end];

            if (next != '.' && next != '\\' && next != '/' && next != '_' && next != ' ') {

                continue;

            }

        }

        return true;

    }

    return false;

}



bool MatchWildcard(std::string_view name, std::string_view pattern, bool caseInsensitive);

bool MatchRegex(std::string_view text, const std::regex& re);

bool QueryLooksLikeWildcard(std::string_view query);



struct QueryMatcher {

    bool use_regex = false;

    bool match_whole_word = false;

    bool match_case = false;



    void Prepare(std::string_view query);

    bool Matches(std::string_view text) const;



private:

    std::string m_query;

    std::string m_lowerQuery;

    bool m_isWildcard = false;

    bool m_regexReady = false;

    std::regex m_regex;

};



} // namespace EverythingNEO

