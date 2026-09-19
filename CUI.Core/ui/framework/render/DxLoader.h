#pragma once
// ============================================================================
// DxLoader.h — DirectX / DWM / WIC 运行时动态装载层
//
// CUI.Core 不再静态链接 d2d1.lib / dwrite.lib / d3d11.lib / dxgi.lib /
// dcomp.lib / dwmapi.lib / windowscodecs.lib。所有 DLL 入口点改为
// LoadLibraryW + GetProcAddress 在运行时解析，从而：
//
//   * MinGW / Clang 交叉编译时不需要任何 DirectX import library；
//   * 缺少某个组件的系统（如 Win7 无 dcomp.dll）可优雅降级而不是启动失败。
//
// 设计约束：
//   1. 本头只依赖 windows.h + unknwn.h，不引入任何 DirectX SDK 头；
//   2. 所有签名使用与原生 API ABI 等价的基础类型（枚举 -> UINT/UINT32，
//      接口指针 -> void*），因此调用方仍可直接传 D2D1_FACTORY_OPTIONS*、
//      D3D_FEATURE_LEVEL*、IDXGIDevice*、ComPtr<T> 的 & 取址结果等；
//   3. 内部使用 INIT_ONCE，首次调用时装载，线程安全且幂等。
//
// 注意：x64 上 __stdcall 不改名，直接用明文符号；若将来支持 x86，
// 需要额外处理 @N 后缀（可用 GetProcAddress 失败后回退尝试）。
// ============================================================================

#include <windows.h>
#include <unknwn.h>

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif
#ifndef DWMWA_WINDOW_CORNER_PREFERENCE
#define DWMWA_WINDOW_CORNER_PREFERENCE 33
#endif
#ifndef DWMWA_BORDER_COLOR
#define DWMWA_BORDER_COLOR 34
#endif
#ifndef DWMWA_SYSTEMBACKDROP_TYPE
#define DWMWA_SYSTEMBACKDROP_TYPE 38
#endif

namespace CUI {
namespace Dx {

// 与 uxtheme.h 的 MARGINS 布局完全一致，避免为了一个结构体引入 uxtheme.h。
struct Margins {
    int cxLeftWidth;
    int cxRightWidth;
    int cyTopHeight;
    int cyBottomHeight;
};

// 与 D2D1_POINT_2F 布局一致（8 字节 POD）。
// d2d1.h 里 D2D1MakeRotateMatrix / D2D1MakeSkewMatrix 按值传参，
// 8 字节结构体在 Windows x64 ABI 下走通用寄存器，因此这里必须同样按值传。
struct Point2F {
    float x;
    float y;
};

// 与 dwmapi.h 的 DWM_WINDOW_CORNER_PREFERENCE 取值一致。
enum DwmCornerPreference {
    DwmCornerDefault = 0,
    DwmCornerDoNotRound = 1,
    DwmCornerRound = 2,
    DwmCornerRoundSmall = 3
};

// CLSID_WICImagingFactory2（{317D06E8-5F24-433D-BDF7-79CE68D8ABC2}）。
// 自己定义一份，CoCreateInstance 时无需 windowscodecs.lib 提供 GUID 数据符号。
extern const CLSID kWicImagingFactory2;

// ---------------------------------------------------------------------------
// Direct2D（d2d1.dll）
// HRESULT D2D1CreateFactory(D2D1_FACTORY_TYPE, REFIID, const D2D1_FACTORY_OPTIONS*, void**)
// ---------------------------------------------------------------------------
HRESULT D2D1CreateFactory(UINT32 factoryType, REFIID riid, const void* options, void** factory);

// d2d1.h 中除 D2D1CreateFactory 外唯一从 d2d1.dll 导出的 4 个自由函数。
// D2D1::Matrix3x2F::Rotation() / Skew() / Invert() 等内联 helper 内部就是调它们，
// 所以这些 helper 也不能直接用（否则仍会引入 d2d1.lib）。
void D2D1MakeRotateMatrix(float angle, Point2F center, void* matrix);
void D2D1MakeSkewMatrix(float angleX, float angleY, Point2F center, void* matrix);
BOOL D2D1IsMatrixInvertible(const void* matrix);
BOOL D2D1InvertMatrix(void* matrix);

// ---------------------------------------------------------------------------
// DirectWrite（dwrite.dll）
// HRESULT DWriteCreateFactory(DWRITE_FACTORY_TYPE, REFIID, IUnknown**)
// ---------------------------------------------------------------------------
HRESULT DWriteCreateFactory(UINT32 factoryType, REFIID riid, void** factory);

// ---------------------------------------------------------------------------
// Direct3D 11（d3d11.dll）
// HRESULT D3D11CreateDevice(IDXGIAdapter*, D3D_DRIVER_TYPE, HMODULE, UINT,
//                           const D3D_FEATURE_LEVEL*, UINT, UINT,
//                           ID3D11Device**, D3D_FEATURE_LEVEL*, ID3D11DeviceContext**)
// featureLevels / featureLevel 用 void* 传递，调用方可直接传 D3D_FEATURE_LEVEL*。
// ---------------------------------------------------------------------------
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
    void** immediateContext);

// ---------------------------------------------------------------------------
// DXGI（dxgi.dll）
// ---------------------------------------------------------------------------
HRESULT CreateDXGIFactory1(REFIID riid, void** factory);
HRESULT CreateDXGIFactory2(UINT flags, REFIID riid, void** factory);

// ---------------------------------------------------------------------------
// DirectComposition（dcomp.dll）
// HRESULT DCompositionCreateDevice(IDXGIDevice*, REFIID, void**)
// ---------------------------------------------------------------------------
HRESULT DCompositionCreateDevice(void* dxgiDevice, REFIID riid, void** device);

// ---------------------------------------------------------------------------
// DWM（dwmapi.dll）
// ---------------------------------------------------------------------------
HRESULT DwmSetWindowAttribute(HWND hwnd, DWORD attribute, const void* value, DWORD size);
HRESULT DwmGetWindowAttribute(HWND hwnd, DWORD attribute, void* value, DWORD size);
HRESULT DwmExtendFrameIntoClientArea(HWND hwnd, const Margins* margins);
HRESULT DwmIsCompositionEnabled(BOOL* enabled);
HRESULT DwmFlush();
// 与原生一致：返回 TRUE 表示 DWM 已处理该消息（result 为返回值）。
BOOL DwmDefWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT* result);

// ---------------------------------------------------------------------------
// 装载状态
// ---------------------------------------------------------------------------
// 触发全部模块装载（幂等、线程安全）。返回 true 表示"至少核心模块可用"；
// 单个入口点是否可用请用下面的 Has*() 判断。
bool Ensure();
bool HasD2D1();
bool HasDWrite();
bool HasD3D11();
bool HasDXGI();
bool HasDComp();
bool HasDwm();

} // namespace Dx
} // namespace CUI
