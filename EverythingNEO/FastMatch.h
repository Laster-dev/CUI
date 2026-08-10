#pragma once

#include <cstdint>
#include <cstring>
#include <string_view>

namespace EverythingNEO {

// Everything-style case fold: one table lookup, no locale/CRT per character.
struct CaseFold {
    unsigned char lower[256]{};
    CaseFold() {
        for (int i = 0; i < 256; ++i) {
            lower[i] = static_cast<unsigned char>(i);
        }
        for (int i = 'A'; i <= 'Z'; ++i) {
            lower[i] = static_cast<unsigned char>(i - 'A' + 'a');
        }
    }
};

inline const CaseFold& GetCaseFold() {
    static const CaseFold kFold;
    return kFold;
}

// Substring match against already-lowercased ASCII query.
// Non-ASCII bytes compared as-is (UTF-8 filenames still work for ASCII queries).
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

inline void ToLowerAsciiInPlace(std::string& s) {
    const auto& fold = GetCaseFold().lower;
    for (char& c : s) {
        c = static_cast<char>(fold[static_cast<unsigned char>(c)]);
    }
}

} // namespace EverythingNEO
