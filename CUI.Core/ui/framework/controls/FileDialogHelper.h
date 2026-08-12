#pragma once
#include <string>
#include <vector>
#include <windows.h>

namespace CUI {

struct FileDialogFilter {
    std::wstring name;
    std::wstring spec;
};

bool ShowOpenFileDialog(
    HWND owner,
    std::string& outPathUtf8,
    const std::wstring& title = L"选择文件",
    const std::vector<FileDialogFilter>& filters = {});

bool ShowOpenFolderDialog(
    HWND owner,
    std::string& outPathUtf8,
    const std::wstring& title = L"选择文件夹");

} // namespace CUI
