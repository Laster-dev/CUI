#pragma once

#include <cstdint>

namespace EverythingNEO {

struct SearchResultRef {
    uint32_t index = 0;
    bool is_folder = false;
};

struct SearchOptions {
    bool use_regex = false;
    bool match_path = false;
    bool match_whole_word = false;
    bool match_case = false;
};

} // namespace EverythingNEO
