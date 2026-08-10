#include "ShellContextMenu.h"

#include <shlobj.h>
#include <shobjidl.h>
#include <commctrl.h>
#include <vector>

#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "comctl32.lib")

namespace EverythingNEO {
namespace {

// Explorer forwards these to IContextMenu2/3 so cascading / owner-drawn items work.
struct MenuMsgState {
    IContextMenu2* cm2 = nullptr;
    IContextMenu3* cm3 = nullptr;
};

constexpr UINT_PTR kShellMenuSubclassId = 0xE4E1;

LRESULT CALLBACK ShellMenuSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
                                       UINT_PTR /*id*/, DWORD_PTR refData) {
    auto* state = reinterpret_cast<MenuMsgState*>(refData);
    if (state) {
        if (state->cm3) {
            LRESULT lr = 0;
            if (SUCCEEDED(state->cm3->HandleMenuMsg2(msg, wParam, lParam, &lr))) {
                return lr;
            }
        } else if (state->cm2) {
            if (SUCCEEDED(state->cm2->HandleMenuMsg(msg, wParam, lParam))) {
                return 0;
            }
        }
    }
    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

void FreePidls(std::vector<PIDLIST_ABSOLUTE>& pidls) {
    for (auto* p : pidls) {
        if (p) ILFree(p);
    }
    pidls.clear();
}

bool SameParentFolder(const std::vector<PIDLIST_ABSOLUTE>& pidls) {
    if (pidls.size() <= 1) return true;
    PIDLIST_ABSOLUTE parent0 = ILClone(pidls[0]);
    if (!parent0) return false;
    ILRemoveLastID(parent0);

    bool same = true;
    for (size_t i = 1; i < pidls.size(); ++i) {
        PIDLIST_ABSOLUTE parentI = ILClone(pidls[i]);
        if (!parentI) {
            same = false;
            break;
        }
        ILRemoveLastID(parentI);
        same = (ILIsEqual(parent0, parentI) != FALSE);
        ILFree(parentI);
        if (!same) break;
    }
    ILFree(parent0);
    return same;
}

// Classic Explorer path: IShellFolder::GetUIObjectOf — full verbs + extensions.
HRESULT CreateContextMenuFromFolder(HWND owner, const std::vector<PIDLIST_ABSOLUTE>& absPidls,
                                    IContextMenu** outMenu) {
    *outMenu = nullptr;
    if (absPidls.empty()) return E_INVALIDARG;

    IShellFolder* folder = nullptr;
    LPCITEMIDLIST child0 = nullptr;
    HRESULT hr = SHBindToParent(absPidls[0], IID_PPV_ARGS(&folder), &child0);
    if (FAILED(hr) || !folder) return hr;
    (void)child0;

    std::vector<LPCITEMIDLIST> children;
    children.reserve(absPidls.size());
    for (auto* abs : absPidls) {
        children.push_back(ILFindLastID(abs));
    }

    hr = folder->GetUIObjectOf(
        owner,
        static_cast<UINT>(children.size()),
        children.data(),
        IID_IContextMenu,
        nullptr,
        reinterpret_cast<void**>(outMenu));
    folder->Release();
    return hr;
}

HRESULT CreateContextMenuFromItemArray(const std::vector<PIDLIST_ABSOLUTE>& absPidls,
                                       IContextMenu** outMenu) {
    *outMenu = nullptr;
    IShellItemArray* array = nullptr;
    HRESULT hr = SHCreateShellItemArrayFromIDLists(
        static_cast<UINT>(absPidls.size()),
        const_cast<PCUIDLIST_ABSOLUTE*>(absPidls.data()),
        &array);
    if (FAILED(hr) || !array) return hr;

    hr = array->BindToHandler(nullptr, BHID_SFUIObject, IID_PPV_ARGS(outMenu));
    array->Release();
    return hr;
}

} // namespace

bool ShowShellContextMenu(HWND owner, const std::vector<std::wstring>& paths, int screenX, int screenY) {
    if (!owner || paths.empty()) return false;

    HRESULT hrInit = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    const bool needUninit = (hrInit == S_OK);

    std::vector<PIDLIST_ABSOLUTE> pidls;
    pidls.reserve(paths.size());
    for (const auto& path : paths) {
        PIDLIST_ABSOLUTE pidl = ILCreateFromPathW(path.c_str());
        if (!pidl) {
            // Fallback parse for paths that ILCreateFromPath rejects.
            SFGAOF sfgao = 0;
            if (FAILED(SHParseDisplayName(path.c_str(), nullptr, &pidl, 0, &sfgao)) || !pidl) {
                continue;
            }
        }
        pidls.push_back(pidl);
    }
    if (pidls.empty()) {
        if (needUninit) CoUninitialize();
        return false;
    }

    IContextMenu* pcm = nullptr;
    HRESULT hr = E_FAIL;
    if (SameParentFolder(pidls)) {
        hr = CreateContextMenuFromFolder(owner, pidls, &pcm);
    }
    if (FAILED(hr) || !pcm) {
        hr = CreateContextMenuFromItemArray(pidls, &pcm);
    }
    FreePidls(pidls);

    if (FAILED(hr) || !pcm) {
        if (needUninit) CoUninitialize();
        return false;
    }

    HMENU hMenu = CreatePopupMenu();
    if (!hMenu) {
        pcm->Release();
        if (needUninit) CoUninitialize();
        return false;
    }

    // Match Explorer: explore-style verbs. Shift adds extended verbs.
    UINT qcmFlags = CMF_NORMAL | CMF_EXPLORE;
    if (GetKeyState(VK_SHIFT) < 0) {
        qcmFlags |= CMF_EXTENDEDVERBS;
    }

    hr = pcm->QueryContextMenu(hMenu, 0, 1, 0x7FFF, qcmFlags);
    if (FAILED(hr)) {
        DestroyMenu(hMenu);
        pcm->Release();
        if (needUninit) CoUninitialize();
        return false;
    }

    MenuMsgState msgState{};
    pcm->QueryInterface(IID_PPV_ARGS(&msgState.cm3));
    if (!msgState.cm3) {
        pcm->QueryInterface(IID_PPV_ARGS(&msgState.cm2));
    }
    SetWindowSubclass(owner, ShellMenuSubclassProc, kShellMenuSubclassId,
                      reinterpret_cast<DWORD_PTR>(&msgState));

    // Allow menu to receive keyboard; don't steal with TPM_NONOTIFY — shell needs messages.
    const int cmd = TrackPopupMenuEx(
        hMenu,
        TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_HORIZONTAL | TPM_VERTICAL,
        screenX, screenY, owner, nullptr);

    RemoveWindowSubclass(owner, ShellMenuSubclassProc, kShellMenuSubclassId);
    if (msgState.cm3) msgState.cm3->Release();
    if (msgState.cm2) msgState.cm2->Release();

    if (cmd > 0) {
        CMINVOKECOMMANDINFOEX info{};
        info.cbSize = sizeof(info);
        info.fMask = CMIC_MASK_UNICODE | CMIC_MASK_PTINVOKE;
        if (GetKeyState(VK_CONTROL) < 0) info.fMask |= CMIC_MASK_CONTROL_DOWN;
        if (GetKeyState(VK_SHIFT) < 0) info.fMask |= CMIC_MASK_SHIFT_DOWN;
        info.hwnd = owner;
        info.lpVerb = MAKEINTRESOURCEA(cmd - 1);
        info.lpVerbW = MAKEINTRESOURCEW(cmd - 1);
        info.nShow = SW_SHOWNORMAL;
        info.ptInvoke = POINT{ screenX, screenY };
        pcm->InvokeCommand(reinterpret_cast<CMINVOKECOMMANDINFO*>(&info));
    }

    DestroyMenu(hMenu);
    pcm->Release();
    if (needUninit) CoUninitialize();
    return true;
}

} // namespace EverythingNEO
