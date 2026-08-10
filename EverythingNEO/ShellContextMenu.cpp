#include "ShellContextMenu.h"

#include <shlobj.h>
#include <shobjidl.h>
#include <commctrl.h>
#include <vector>
#include <memory>
#include <string>
#include <cstring>

#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")

namespace EverythingNEO {
namespace {

struct ShellMenuSession {
    HWND owner = nullptr;
    IContextMenu* pcm = nullptr;
    IContextMenu2* pcm2 = nullptr;
    IContextMenu3* pcm3 = nullptr;
    POINT invokePt{ 0, 0 };
    bool comNeedUninit = false;

    ~ShellMenuSession() {
        if (pcm3) { pcm3->Release(); pcm3 = nullptr; }
        if (pcm2) { pcm2->Release(); pcm2 = nullptr; }
        if (pcm) { pcm->Release(); pcm = nullptr; }
        if (comNeedUninit) {
            CoUninitialize();
            comNeedUninit = false;
        }
    }

    void Invoke(UINT menuId) {
        if (!pcm || menuId == 0) return;
        CMINVOKECOMMANDINFOEX info{};
        info.cbSize = sizeof(info);
        info.fMask = CMIC_MASK_UNICODE | CMIC_MASK_PTINVOKE;
        if (GetKeyState(VK_CONTROL) < 0) info.fMask |= CMIC_MASK_CONTROL_DOWN;
        if (GetKeyState(VK_SHIFT) < 0) info.fMask |= CMIC_MASK_SHIFT_DOWN;
        info.hwnd = owner;
        info.lpVerb = MAKEINTRESOURCEA(menuId - 1);
        info.lpVerbW = MAKEINTRESOURCEW(menuId - 1);
        info.nShow = SW_SHOWNORMAL;
        info.ptInvoke = invokePt;
        pcm->InvokeCommand(reinterpret_cast<CMINVOKECOMMANDINFO*>(&info));
    }

    void HandleMenuMsg(UINT msg, WPARAM wParam, LPARAM lParam) {
        if (pcm3) {
            LRESULT lr = 0;
            pcm3->HandleMenuMsg2(msg, wParam, lParam, &lr);
        } else if (pcm2) {
            pcm2->HandleMenuMsg(msg, wParam, lParam);
        }
    }
};

std::string WideToUtf8(const std::wstring& w) {
    if (w.empty()) return {};
    int n = WideCharToMultiByte(CP_UTF8, 0, w.data(), static_cast<int>(w.size()), nullptr, 0, nullptr, nullptr);
    if (n <= 0) return {};
    std::string out(static_cast<size_t>(n), '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.data(), static_cast<int>(w.size()), out.data(), n, nullptr, nullptr);
    return out;
}

std::string StripMenuAmpersands(const std::wstring& in) {
    std::wstring out;
    out.reserve(in.size());
    for (size_t i = 0; i < in.size(); ++i) {
        if (in[i] == L'&') {
            if (i + 1 < in.size() && in[i + 1] == L'&') {
                out.push_back(L'&');
                ++i;
            }
            continue;
        }
        out.push_back(in[i]);
    }
    while (!out.empty() && (out.back() == L' ' || out.back() == L'\t')) {
        out.pop_back();
    }
    return WideToUtf8(out);
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

bool IsRealMenuBitmap(HBITMAP hbm) {
    if (!hbm || hbm == HBMMENU_CALLBACK) return false;
    // HBMMENU_* system constants are small integers (1..11 etc.).
    const auto v = reinterpret_cast<ULONG_PTR>(hbm);
    return v > 16;
}

HICON IconFromHBitmap(HBITMAP hbmSrc) {
    if (!IsRealMenuBitmap(hbmSrc)) return nullptr;

    BITMAP bm{};
    if (!GetObject(hbmSrc, sizeof(bm), &bm) || bm.bmWidth <= 0 || bm.bmHeight <= 0) {
        return nullptr;
    }

    HBITMAP hbmColor = static_cast<HBITMAP>(CopyImage(hbmSrc, IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION));
    if (!hbmColor) return nullptr;

    HBITMAP hbmMask = CreateBitmap(bm.bmWidth, bm.bmHeight, 1, 1, nullptr);
    ICONINFO ii{};
    ii.fIcon = TRUE;
    ii.hbmColor = hbmColor;
    ii.hbmMask = hbmMask ? hbmMask : hbmColor;
    HICON icon = CreateIconIndirect(&ii);
    if (hbmMask) DeleteObject(hbmMask);
    DeleteObject(hbmColor);
    return icon;
}

HICON IconFromOwnerDraw(ShellMenuSession& session, HMENU hMenu, UINT menuId, ULONG_PTR itemData) {
    if (!session.pcm2 && !session.pcm3) return nullptr;

    constexpr int kW = 32;
    constexpr int kH = 16;

    BITMAPINFO bmi{};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = kW;
    bmi.bmiHeader.biHeight = -kH; // top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* bits = nullptr;
    HDC screenDc = GetDC(nullptr);
    HDC memDc = CreateCompatibleDC(screenDc);
    HBITMAP dib = CreateDIBSection(memDc, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (!dib || !bits) {
        if (dib) DeleteObject(dib);
        DeleteDC(memDc);
        ReleaseDC(nullptr, screenDc);
        return nullptr;
    }
    std::memset(bits, 0, static_cast<size_t>(kW * kH * 4));
    HGDIOBJ old = SelectObject(memDc, dib);

    MEASUREITEMSTRUCT mis{};
    mis.CtlType = ODT_MENU;
    mis.itemID = menuId;
    mis.itemWidth = kW;
    mis.itemHeight = kH;
    mis.itemData = itemData;
    session.HandleMenuMsg(WM_MEASUREITEM, 0, reinterpret_cast<LPARAM>(&mis));

    DRAWITEMSTRUCT dis{};
    dis.CtlType = ODT_MENU;
    dis.itemID = menuId;
    dis.itemAction = ODA_DRAWENTIRE;
    dis.itemState = ODS_NOFOCUSRECT;
    dis.hwndItem = reinterpret_cast<HWND>(hMenu);
    dis.hDC = memDc;
    SetRect(&dis.rcItem, 0, 0, kW, kH);
    dis.itemData = itemData;
    session.HandleMenuMsg(WM_DRAWITEM, 0, reinterpret_cast<LPARAM>(&dis));

    // Crop left 16x16 (standard menu glyph gutter) into an icon.
    constexpr int kIcon = 16;
    BITMAPINFO cropBi{};
    cropBi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    cropBi.bmiHeader.biWidth = kIcon;
    cropBi.bmiHeader.biHeight = -kIcon;
    cropBi.bmiHeader.biPlanes = 1;
    cropBi.bmiHeader.biBitCount = 32;
    cropBi.bmiHeader.biCompression = BI_RGB;
    void* cropBits = nullptr;
    HBITMAP crop = CreateDIBSection(memDc, &cropBi, DIB_RGB_COLORS, &cropBits, nullptr, 0);
    HICON icon = nullptr;
    if (crop && cropBits) {
        auto* src = static_cast<const UINT32*>(bits);
        auto* dst = static_cast<UINT32*>(cropBits);
        bool any = false;
        for (int y = 0; y < kIcon; ++y) {
            for (int x = 0; x < kIcon; ++x) {
                const UINT32 px = src[y * kW + x];
                dst[y * kIcon + x] = px;
                if (px & 0xFF000000u) any = true;
                else if ((px & 0x00FFFFFFu) != 0) any = true;
            }
        }
        if (any) {
            icon = IconFromHBitmap(crop);
        }
        DeleteObject(crop);
    }

    SelectObject(memDc, old);
    DeleteObject(dib);
    DeleteDC(memDc);
    ReleaseDC(nullptr, screenDc);
    return icon;
}

HICON ExtractMenuItemIcon(ShellMenuSession& session, HMENU hMenu, int index) {
    MENUITEMINFOW mii{};
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_BITMAP | MIIM_CHECKMARKS | MIIM_ID | MIIM_DATA | MIIM_STATE | MIIM_FTYPE;
    if (!GetMenuItemInfoW(hMenu, index, TRUE, &mii)) return nullptr;

    if (IsRealMenuBitmap(mii.hbmpItem)) {
        if (HICON icon = IconFromHBitmap(mii.hbmpItem)) return icon;
    }
    if (IsRealMenuBitmap(mii.hbmpUnchecked)) {
        if (HICON icon = IconFromHBitmap(mii.hbmpUnchecked)) return icon;
    }
    if (mii.hbmpItem == HBMMENU_CALLBACK || (mii.fType & MFT_OWNERDRAW)) {
        if (HICON icon = IconFromOwnerDraw(session, hMenu, mii.wID, mii.dwItemData)) {
            return icon;
        }
    }
    return nullptr;
}

HICON LoadStockIcon(SHSTOCKICONID id) {
    SHSTOCKICONINFO sii{};
    sii.cbSize = sizeof(sii);
    if (FAILED(SHGetStockIconInfo(id, SHGSI_ICON | SHGSI_SMALLICON, &sii))) {
        return nullptr;
    }
    return sii.hIcon;
}

std::wstring GetMenuItemLabel(HMENU hMenu, int index, IContextMenu* pcm, UINT idCmdFirst) {
    MENUITEMINFOW mii{};
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_STRING | MIIM_ID | MIIM_FTYPE;
    mii.dwTypeData = nullptr;
    mii.cch = 0;
    if (!GetMenuItemInfoW(hMenu, index, TRUE, &mii)) return {};

    if (mii.cch > 0) {
        std::wstring buf(mii.cch + 1, L'\0');
        mii.dwTypeData = buf.data();
        mii.cch = static_cast<UINT>(buf.size());
        if (GetMenuItemInfoW(hMenu, index, TRUE, &mii) && mii.dwTypeData) {
            return std::wstring(mii.dwTypeData);
        }
    }

    if (!pcm || mii.wID < idCmdFirst) return {};
    const UINT offset = mii.wID - idCmdFirst;
    wchar_t verb[256]{};
    if (SUCCEEDED(pcm->GetCommandString(offset, GCS_HELPTEXTW, nullptr, reinterpret_cast<char*>(verb), 255))
        && verb[0]) {
        return verb;
    }
    verb[0] = 0;
    if (SUCCEEDED(pcm->GetCommandString(offset, GCS_VERBW, nullptr, reinterpret_cast<char*>(verb), 255))
        && verb[0]) {
        return verb;
    }
    return {};
}

void PopulateMenuFromHMenu(CUI::ContextMenu& menu, HMENU hMenu,
                           const std::shared_ptr<ShellMenuSession>& session,
                           UINT idCmdFirst);

void AddShellItem(CUI::ContextMenu& menu, HMENU hMenu, int index,
                  const std::shared_ptr<ShellMenuSession>& session, UINT idCmdFirst) {
    MENUITEMINFOW mii{};
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_FTYPE | MIIM_ID | MIIM_STATE | MIIM_SUBMENU | MIIM_BITMAP | MIIM_DATA;
    if (!GetMenuItemInfoW(hMenu, index, TRUE, &mii)) return;

    if (mii.fType & MFT_SEPARATOR) {
        menu.AddSeparator();
        return;
    }

    const std::wstring labelW = GetMenuItemLabel(hMenu, index, session ? session->pcm : nullptr, idCmdFirst);
    const std::string label = StripMenuAmpersands(labelW);
    if (label.empty() && !mii.hSubMenu) return;

    const bool disabled = (mii.fState & (MFS_DISABLED | MFS_GRAYED)) != 0;
    const bool checked = (mii.fState & MFS_CHECKED) != 0;
    const UINT menuId = mii.wID;
    HICON icon = session ? ExtractMenuItemIcon(*session, hMenu, index) : nullptr;

    if (mii.hSubMenu) {
        auto item = menu.AddSubMenuItem(label.empty() ? "..." : label);
        if (icon) item->SetNativeIcon(icon, true);
        if (disabled) item->SetIsEnabled(false);
        if (checked) item->SetChecked(true);
        PopulateMenuFromHMenu(*item->GetSubMenu(), mii.hSubMenu, session, idCmdFirst);
        return;
    }

    auto item = menu.AddItem(label.empty() ? "..." : label, [session, menuId]() {
        if (session) session->Invoke(menuId);
    });
    if (icon) item->SetNativeIcon(icon, true);
    if (disabled) item->SetIsEnabled(false);
    if (checked) item->SetChecked(true);
}

void PopulateMenuFromHMenu(CUI::ContextMenu& menu, HMENU hMenu,
                           const std::shared_ptr<ShellMenuSession>& session,
                           UINT idCmdFirst) {
    if (!hMenu) return;
    const int count = GetMenuItemCount(hMenu);
    for (int i = 0; i < count; ++i) {
        AddShellItem(menu, hMenu, i, session, idCmdFirst);
    }
}

} // namespace

std::shared_ptr<CUI::ContextMenu> BuildShellContextMenu(
    HWND owner,
    const std::vector<std::wstring>& paths,
    const ShellMenuActions& actions) {
    if (!owner || paths.empty()) return nullptr;

    HRESULT hrInit = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    const bool needUninit = (hrInit == S_OK);

    std::vector<PIDLIST_ABSOLUTE> pidls;
    pidls.reserve(paths.size());
    for (const auto& path : paths) {
        PIDLIST_ABSOLUTE pidl = ILCreateFromPathW(path.c_str());
        if (!pidl) {
            SFGAOF sfgao = 0;
            if (FAILED(SHParseDisplayName(path.c_str(), nullptr, &pidl, 0, &sfgao)) || !pidl) {
                continue;
            }
        }
        pidls.push_back(pidl);
    }
    if (pidls.empty()) {
        if (needUninit) CoUninitialize();
        return nullptr;
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
        return nullptr;
    }

    HMENU hMenu = CreatePopupMenu();
    if (!hMenu) {
        pcm->Release();
        if (needUninit) CoUninitialize();
        return nullptr;
    }

    constexpr UINT kIdCmdFirst = 1;
    UINT qcmFlags = CMF_NORMAL | CMF_EXPLORE;
    if (GetKeyState(VK_SHIFT) < 0) {
        qcmFlags |= CMF_EXTENDEDVERBS;
    }
    hr = pcm->QueryContextMenu(hMenu, 0, kIdCmdFirst, 0x7FFF, qcmFlags);
    if (FAILED(hr)) {
        DestroyMenu(hMenu);
        pcm->Release();
        if (needUninit) CoUninitialize();
        return nullptr;
    }

    // Many handlers only attach bitmaps after WM_INITMENUPOPUP.
    {
        IContextMenu3* cm3 = nullptr;
        IContextMenu2* cm2 = nullptr;
        pcm->QueryInterface(IID_PPV_ARGS(&cm3));
        if (!cm3) pcm->QueryInterface(IID_PPV_ARGS(&cm2));
        if (cm3) {
            LRESULT lr = 0;
            cm3->HandleMenuMsg2(WM_INITMENUPOPUP, reinterpret_cast<WPARAM>(hMenu), 0, &lr);
            cm3->Release();
        } else if (cm2) {
            cm2->HandleMenuMsg(WM_INITMENUPOPUP, reinterpret_cast<WPARAM>(hMenu), 0);
            cm2->Release();
        }
    }

    auto session = std::make_shared<ShellMenuSession>();
    session->owner = owner;
    session->pcm = pcm;
    pcm->QueryInterface(IID_PPV_ARGS(&session->pcm3));
    if (!session->pcm3) {
        pcm->QueryInterface(IID_PPV_ARGS(&session->pcm2));
    }
    session->comNeedUninit = needUninit;
    GetCursorPos(&session->invokePt);

    auto menu = std::make_shared<CUI::ContextMenu>();

    const int count = GetMenuItemCount(hMenu);
    bool injected = false;
    auto injectExtras = [&]() {
        if (injected) return;
        injected = true;
        if (actions.showOpenPath && actions.openPath) {
            auto item = menu->AddItem("打开路径(P)", actions.openPath);
            if (HICON ic = LoadStockIcon(SIID_FOLDEROPEN)) item->SetNativeIcon(ic, true);
        }
        if (actions.showCopyFullPath && actions.copyFullPath) {
            auto item = menu->AddItem("复制完整路径(C)", actions.copyFullPath);
            if (HICON ic = LoadStockIcon(SIID_DOCASSOC)) item->SetNativeIcon(ic, true);
        }
        if (actions.showRename && actions.rename) {
            auto item = menu->AddItem("重命名(M)", actions.rename);
            if (HICON ic = LoadStockIcon(SIID_RENAME)) item->SetNativeIcon(ic, true);
        }
    };

    for (int i = 0; i < count; ++i) {
        MENUITEMINFOW mii{};
        mii.cbSize = sizeof(mii);
        mii.fMask = MIIM_FTYPE | MIIM_STATE | MIIM_SUBMENU;
        if (!GetMenuItemInfoW(hMenu, i, TRUE, &mii)) continue;

        const bool isSep = (mii.fType & MFT_SEPARATOR) != 0;
        AddShellItem(*menu, hMenu, i, session, kIdCmdFirst);

        if (!injected && !isSep && !mii.hSubMenu) {
            injectExtras();
        }
    }
    if (!injected) {
        injectExtras();
    }

    DestroyMenu(hMenu);
    return menu;
}

} // namespace EverythingNEO
