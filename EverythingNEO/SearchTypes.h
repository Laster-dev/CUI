#pragma once

#include <cstdint>

namespace EverythingNEO {

struct SearchResultRef {
    uint32_t index = 0;
    bool is_folder = false;
};

enum class SearchResultKind : uint8_t {
    FilesAndFolders = 0,
    FilesOnly = 1,
    FoldersOnly = 2,
};

struct SearchOptions {
    bool use_regex = false;
    bool match_path = false;
    bool match_whole_word = false;
    bool match_case = false;
    SearchResultKind result_kind = SearchResultKind::FilesAndFolders;
};

} // namespace EverythingNEO
