#include "DxLoader.h"

// DirectX / DWM 全部走运行时解析，本翻译单元不链接任何 import library。

namespace CUI {
namespace Dx {

// {317D06E8-5F24-433D-BDF7-79CE68D8ABC2}
const CLSID kWicImagingFactory2 = {
    0x317d06e8, 0x5f24, 0x433d, { 0xbd, 0xf7, 0x79, 0xce, 0x68, 0xd8, 0xab, 0xc2 }
};

namespace {

// ---- 函数指针类型（签名与 DxLoader.h 中的包装保持一致）----
using FnD2D1CreateFactory = HRESULT(WINAPI*)(UINT32, REFIID, const void*, void**);
using FnD2D1MakeRotateMatrix = void(WINAPI*)(float, Point2F, void*);
using FnD2D1MakeSkewMatrix = void(WINAPI*)(float, float, Point2F, void*);
using FnD2D1IsMatrixInvertible = BOOL(WINAPI*)(const void*);
using FnD2D1InvertMatrix = BOOL(WINAPI*)(void*);
using FnDWriteCreateFactory = HRESULT(WINAPI*)(UINT32, REFIID, void**);
using FnD3D11CreateDevice = HRESULT(WINAPI*)(
    void*, UINT32, HMODULE, UINT, const void*, UINT, UINT, void**, void*, void**);
using FnCreateDXGIFactory1 = HRESULT(WINAPI*)(REFIID, void**);
using FnCreateDXGIFactory2 = HRESULT(WINAPI*)(UINT, REFIID, void**);
using FnDCompositionCreateDevice = HRESULT(WINAPI*)(void*, REFIID, void**);

using FnDwmSetWindowAttribute = HRESULT(WINAPI*)(HWND, DWORD, const void*, DWORD);
using FnDwmGetWindowAttribute = HRESULT(WINAPI*)(HWND, DWORD, void*, DWORD);
using FnDwmExtendFrameIntoClientArea = HRESULT(WINAPI*)(HWND, const Margins*);
using FnDwmIsCompositionEnabled = HRESULT(WINAPI*)(BOOL*);
using FnDwmFlush = HRESULT(WINAPI*)(void);
using FnDwmDefWindowProc = BOOL(WINAPI*)(HWND, UINT, WPARAM, LPARAM, LRESULT*);

// 保持平凡可默认构造（无 = nullptr 初始化器），使其只走静态零初始化，
// 这样即便别的翻译单元的静态初始化器先跑，也不会覆盖已解析好的指针。
struct FunctionTable {
    FnD2D1CreateFactory D2D1CreateFactory;
    FnD2D1MakeRotateMatrix D2D1MakeRotateMatrix;
    FnD2D1MakeSkewMatrix D2D1MakeSkewMatrix;
    FnD2D1IsMatrixInvertible D2D1IsMatrixInvertible;
    FnD2D1InvertMatrix D2D1InvertMatrix;
    FnDWriteCreateFactory DWriteCreateFactory;
    FnD3D11CreateDevice D3D11CreateDevice;
    FnCreateDXGIFactory1 CreateDXGIFactory1;
    FnCreateDXGIFactory2 CreateDXGIFactory2;
    FnDCompositionCreateDevice DCompositionCreateDevice;

    FnDwmSetWindowAttribute DwmSetWindowAttribute;
    FnDwmGetWindowAttribute DwmGetWindowAttribute;
    FnDwmExtendFrameIntoClientArea DwmExtendFrameIntoClientArea;
    FnDwmIsCompositionEnabled DwmIsCompositionEnabled;
    FnDwmFlush DwmFlush;
    FnDwmDefWindowProc DwmDefWindowProc;
};

FunctionTable g_fn;
INIT_ONCE g_once = INIT_ONCE_STATIC_INIT;

HMODULE LoadSystemLibrary(const wchar_t* name) {
    // 先查已加载模块，避免重复 LoadLibrary 造成的额外引用计数。
    if (HMODULE existing = GetModuleHandleW(name)) {
        return existing;
    }
    return LoadLibraryW(name);
}

template <class T>
T Resolve(HMODULE module, const char* name) {
    if (!module) {
        return nullptr;
    }
    FARPROC proc = GetProcAddress(module, name);
    if (!proc) {
        return nullptr;
    }
    // 函数指针 -> 函数指针，标准允许的 reinterpret_cast。
    return reinterpret_cast<T>(proc);
}

BOOL CALLBACK ResolveAll(PINIT_ONCE, PVOID, PVOID*) {
    const HMODULE d2d1 = LoadSystemLibrary(L"d2d1.dll");
    const HMODULE dwrite = LoadSystemLibrary(L"dwrite.dll");
    const HMODULE d3d11 = LoadSystemLibrary(L"d3d11.dll");
    const HMODULE dxgi = LoadSystemLibrary(L"dxgi.dll");
    const HMODULE dcomp = LoadSystemLibrary(L"dcomp.dll");
    const HMODULE dwm = LoadSystemLibrary(L"dwmapi.dll");

    g_fn.D2D1CreateFactory = Resolve<FnD2D1CreateFactory>(d2d1, "D2D1CreateFactory");
    g_fn.D2D1MakeRotateMatrix = Resolve<FnD2D1MakeRotateMatrix>(d2d1, "D2D1MakeRotateMatrix");
    g_fn.D2D1MakeSkewMatrix = Resolve<FnD2D1MakeSkewMatrix>(d2d1, "D2D1MakeSkewMatrix");
    g_fn.D2D1IsMatrixInvertible = Resolve<FnD2D1IsMatrixInvertible>(d2d1, "D2D1IsMatrixInvertible");
    g_fn.D2D1InvertMatrix = Resolve<FnD2D1InvertMatrix>(d2d1, "D2D1InvertMatrix");
    g_fn.DWriteCreateFactory = Resolve<FnDWriteCreateFactory>(dwrite, "DWriteCreateFactory");
    g_fn.D3D11CreateDevice = Resolve<FnD3D11CreateDevice>(d3d11, "D3D11CreateDevice");
    g_fn.CreateDXGIFactory1 = Resolve<FnCreateDXGIFactory1>(dxgi, "CreateDXGIFactory1");
    g_fn.CreateDXGIFactory2 = Resolve<FnCreateDXGIFactory2>(dxgi, "CreateDXGIFactory2");
    g_fn.DCompositionCreateDevice =
        Resolve<FnDCompositionCreateDevice>(dcomp, "DCompositionCreateDevice");

    g_fn.DwmSetWindowAttribute = Resolve<FnDwmSetWindowAttribute>(dwm, "DwmSetWindowAttribute");
    g_fn.DwmGetWindowAttribute = Resolve<FnDwmGetWindowAttribute>(dwm, "DwmGetWindowAttribute");
    g_fn.DwmExtendFrameIntoClientArea =
        Resolve<FnDwmExtendFrameIntoClientArea>(dwm, "DwmExtendFrameIntoClientArea");
    g_fn.DwmIsCompositionEnabled = Resolve<FnDwmIsCompositionEnabled>(dwm, "DwmIsCompositionEnabled");
    g_fn.DwmFlush = Resolve<FnDwmFlush>(dwm, "DwmFlush");
    g_fn.DwmDefWindowProc = Resolve<FnDwmDefWindowProc>(dwm, "DwmDefWindowProc");

    return TRUE;
}

inline const FunctionTable& Table() {
    InitOnceExecuteOnce(&g_once, &ResolveAll, nullptr, nullptr);
    return g_fn;
}

// 入口点缺失时统一返回的错误码，便于定位"某个 DLL 缺少导出"。
inline HRESULT NotAvailable() {
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

} // namespace

bool Ensure() {
    (void)Table();
    return g_fn.D2D1CreateFactory != nullptr;
}

bool HasD2D1() { return Table().D2D1CreateFactory != nullptr; }
bool HasDWrite() { return Table().DWriteCreateFactory != nullptr; }
bool HasD3D11() { return Table().D3D11CreateDevice != nullptr; }
bool HasDXGI() { return Table().CreateDXGIFactory1 != nullptr; }
bool HasDComp() { return Table().DCompositionCreateDevice != nullptr; }
bool HasDwm() { return Table().DwmSetWindowAttribute != nullptr; }

HRESULT D2D1CreateFactory(UINT32 factoryType, REFIID riid, const void* options, void** factory) {
    const FunctionTable& fn = Table();
    if (!fn.D2D1CreateFactory) {
        return NotAvailable();
    }
    return fn.D2D1CreateFactory(factoryType, riid, options, factory);
}

void D2D1MakeRotateMatrix(float angle, Point2F center, void* matrix) {
    const FunctionTable& fn = Table();
    if (!fn.D2D1MakeRotateMatrix) {
        return;
    }
    fn.D2D1MakeRotateMatrix(angle, center, matrix);
}

void D2D1MakeSkewMatrix(float angleX, float angleY, Point2F center, void* matrix) {
    const FunctionTable& fn = Table();
    if (!fn.D2D1MakeSkewMatrix) {
        return;
    }
    fn.D2D1MakeSkewMatrix(angleX, angleY, center, matrix);
}

BOOL D2D1IsMatrixInvertible(const void* matrix) {
    const FunctionTable& fn = Table();
    if (!fn.D2D1IsMatrixInvertible) {
        return FALSE;
    }
    return fn.D2D1IsMatrixInvertible(matrix);
}

BOOL D2D1InvertMatrix(void* matrix) {
    const FunctionTable& fn = Table();
    if (!fn.D2D1InvertMatrix) {
        return FALSE;
    }
    return fn.D2D1InvertMatrix(matrix);
}

HRESULT DWriteCreateFactory(UINT32 factoryType, REFIID riid, void** factory) {
    const FunctionTable& fn = Table();
    if (!fn.DWriteCreateFactory) {
        return NotAvailable();
    }
    return fn.DWriteCreateFactory(factoryType, riid, factory);
}

HRESULT D3D11CreateDevice(
    void* adapter,
    UINT32 driverType,
    HMODULE software,
    UINT flags,
    const void* featureLevels,
    UINT featureLevelCount,
    UINT sdkVersion,
    void** device,
    void* featureLevel,
    void** immediateContext) {
    const FunctionTable& fn = Table();
    if (!fn.D3D11CreateDevice) {
        return NotAvailable();
    }
    return fn.D3D11CreateDevice(
        adapter,
        driverType,
        software,
        flags,
        featureLevels,
        featureLevelCount,
        sdkVersion,
        device,
        featureLevel,
        immediateContext);
}

HRESULT CreateDXGIFactory1(REFIID riid, void** factory) {
    const FunctionTable& fn = Table();
    if (!fn.CreateDXGIFactory1) {
        return NotAvailable();
    }
    return fn.CreateDXGIFactory1(riid, factory);
}

HRESULT CreateDXGIFactory2(UINT flags, REFIID riid, void** factory) {
    const FunctionTable& fn = Table();
    if (!fn.CreateDXGIFactory2) {
        return NotAvailable();
    }
    return fn.CreateDXGIFactory2(flags, riid, factory);
}

HRESULT DCompositionCreateDevice(void* dxgiDevice, REFIID riid, void** device) {
    const FunctionTable& fn = Table();
    if (!fn.DCompositionCreateDevice) {
        return NotAvailable();
    }
    return fn.DCompositionCreateDevice(dxgiDevice, riid, device);
}

HRESULT DwmSetWindowAttribute(HWND hwnd, DWORD attribute, const void* value, DWORD size) {
    const FunctionTable& fn = Table();
    if (!fn.DwmSetWindowAttribute) {
        return NotAvailable();
    }
    return fn.DwmSetWindowAttribute(hwnd, attribute, value, size);
}

HRESULT DwmGetWindowAttribute(HWND hwnd, DWORD attribute, void* value, DWORD size) {
    const FunctionTable& fn = Table();
    if (!fn.DwmGetWindowAttribute) {
        return NotAvailable();
    }
    return fn.DwmGetWindowAttribute(hwnd, attribute, value, size);
}

HRESULT DwmExtendFrameIntoClientArea(HWND hwnd, const Margins* margins) {
    const FunctionTable& fn = Table();
    if (!fn.DwmExtendFrameIntoClientArea) {
        return NotAvailable();
    }
    return fn.DwmExtendFrameIntoClientArea(hwnd, margins);
}

HRESULT DwmIsCompositionEnabled(BOOL* enabled) {
    const FunctionTable& fn = Table();
    if (!fn.DwmIsCompositionEnabled) {
        return NotAvailable();
    }
    return fn.DwmIsCompositionEnabled(enabled);
}

HRESULT DwmFlush() {
    const FunctionTable& fn = Table();
    if (!fn.DwmFlush) {
        return NotAvailable();
    }
    return fn.DwmFlush();
}

BOOL DwmDefWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT* result) {
    const FunctionTable& fn = Table();
    if (!fn.DwmDefWindowProc) {
        return FALSE;
    }
    return fn.DwmDefWindowProc(hwnd, msg, wParam, lParam, result);
}

} // namespace Dx
} // namespace CUI
