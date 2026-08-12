#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "FileDialogHelper.h"
#include "../core/Value.h"
#include <shobjidl.h>

#pragma comment(lib, "ole32.lib")

namespace CUI {

namespace {
std::wstring BuildFilterSpec(const std::vector<FileDialogFilter>& filters) {
    std::wstring combined;
    for (const auto& filter : filters) {
        combined += filter.name;
        combined.push_back(L'\0');
        combined += filter.spec;
        combined.push_back(L'\0');
    }
    combined.push_back(L'\0');
    return combined;
}
} // namespace

bool ShowOpenFileDialog(
    HWND owner,
    std::string& outPathUtf8,
    const std::wstring& title,
    const std::vector<FileDialogFilter>& filters) {
    outPathUtf8.clear();

    IFileOpenDialog* dialog = nullptr;
    HRESULT hr = CoCreateInstance(
        CLSID_FileOpenDialog,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&dialog));
    if (FAILED(hr) || !dialog) {
        return false;
    }

    if (!title.empty()) {
        dialog->SetTitle(title.c_str());
    }

    DWORD options = 0;
    dialog->GetOptions(&options);
    dialog->SetOptions(options | FOS_FILEMUSTEXIST | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST);

    std::vector<FileDialogFilter> effectiveFilters = filters;
    if (effectiveFilters.empty()) {
        effectiveFilters.push_back({ L"所有文件", L"*.*" });
    }

    std::wstring specBlob = BuildFilterSpec(effectiveFilters);
    std::vector<COMDLG_FILTERSPEC> specs;
    specs.reserve(effectiveFilters.size());
    size_t offset = 0;
    for (const auto& filter : effectiveFilters) {
        COMDLG_FILTERSPEC spec{};
        spec.pszName = specBlob.c_str() + offset;
        offset += filter.name.size() + 1;
        spec.pszSpec = specBlob.c_str() + offset;
        offset += filter.spec.size() + 1;
        specs.push_back(spec);
    }
    dialog->SetFileTypes(static_cast<UINT>(specs.size()), specs.data());
    dialog->SetFileTypeIndex(1);

    hr = dialog->Show(owner);
    if (FAILED(hr)) {
        dialog->Release();
        return false;
    }

    IShellItem* item = nullptr;
    hr = dialog->GetResult(&item);
    dialog->Release();
    if (FAILED(hr) || !item) {
        return false;
    }

    PWSTR path = nullptr;
    hr = item->GetDisplayName(SIGDN_FILESYSPATH, &path);
    item->Release();
    if (FAILED(hr) || !path) {
        return false;
    }

    outPathUtf8 = Utf16ToUtf8(path);
    CoTaskMemFree(path);
    return !outPathUtf8.empty();
}

bool ShowOpenFolderDialog(HWND owner, std::string& outPathUtf8, const std::wstring& title) {
    outPathUtf8.clear();

    IFileOpenDialog* dialog = nullptr;
    HRESULT hr = CoCreateInstance(
        CLSID_FileOpenDialog,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&dialog));
    if (FAILED(hr) || !dialog) {
        return false;
    }

    if (!title.empty()) {
        dialog->SetTitle(title.c_str());
    }

    DWORD options = 0;
    dialog->GetOptions(&options);
    dialog->SetOptions(
        options | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST | FOS_FILEMUSTEXIST);

    hr = dialog->Show(owner);
    if (FAILED(hr)) {
        dialog->Release();
        return false;
    }

    IShellItem* item = nullptr;
    hr = dialog->GetResult(&item);
    dialog->Release();
    if (FAILED(hr) || !item) {
        return false;
    }

    PWSTR path = nullptr;
    hr = item->GetDisplayName(SIGDN_FILESYSPATH, &path);
    item->Release();
    if (FAILED(hr) || !path) {
        return false;
    }

    outPathUtf8 = Utf16ToUtf8(path);
    CoTaskMemFree(path);
    return !outPathUtf8.empty();
}

} // namespace CUI
