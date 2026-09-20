#ifndef CUI_NO_DSL_SHORTCUTS
#define CUI_NO_DSL_SHORTCUTS   // 关闭「控件名即工厂」宏层，避免与 Widgets:: 句柄同名冲突
#endif
#include "Gallery.h"
#include "framework/core/Widgets.h"
#include <wincodec.h>
#include <wrl/client.h>
#include <windows.h>
#include <format>
#include <vector>

#pragma comment(lib, "windowscodecs.lib")

using namespace CUI;
using namespace CUI::DSL;
using Microsoft::WRL::ComPtr;

namespace Gallery {

namespace {

// ---------- 运行时生成示例 PNG（WIC 编码到临时目录，无需仓库资源） ----------
bool WritePng(const std::wstring& path, UINT width, UINT height, const std::vector<uint32_t>& bgra) {
    if (bgra.size() != static_cast<size_t>(width) * height) {
        return false;
    }
    ComPtr<IWICImagingFactory> factory;
    if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                                IID_PPV_ARGS(factory.ReleaseAndGetAddressOf())))) {
        return false;
    }
    ComPtr<IWICStream> stream;
    if (FAILED(factory->CreateStream(stream.ReleaseAndGetAddressOf()))) {
        return false;
    }
    if (FAILED(stream->InitializeFromFilename(path.c_str(), GENERIC_WRITE))) {
        return false;
    }
    ComPtr<IWICBitmapEncoder> encoder;
    if (FAILED(factory->CreateEncoder(GUID_ContainerFormatPng, nullptr, encoder.ReleaseAndGetAddressOf()))) {
        return false;
    }
    if (FAILED(encoder->Initialize(stream.Get(), WICBitmapEncoderNoCache))) {
        return false;
    }
    ComPtr<IWICBitmapFrameEncode> frame;
    ComPtr<IPropertyBag2> bag;
    if (FAILED(encoder->CreateNewFrame(frame.ReleaseAndGetAddressOf(), bag.ReleaseAndGetAddressOf()))) {
        return false;
    }
    if (FAILED(frame->Initialize(bag.Get()))) {
        return false;
    }
    if (FAILED(frame->SetSize(width, height))) {
        return false;
    }
    WICPixelFormatGUID format = GUID_WICPixelFormat32bppBGRA;
    if (FAILED(frame->SetPixelFormat(&format))) {
        return false;
    }
    const UINT stride = width * 4;
    if (FAILED(frame->WritePixels(height, stride, stride * height,
                                  reinterpret_cast<BYTE*>(const_cast<uint32_t*>(bgra.data()))))) {
        return false;
    }
    if (FAILED(frame->Commit()) || FAILED(encoder->Commit())) {
        return false;
    }
    return true;
}

std::string WideToUtf8(const std::wstring& w) {
    if (w.empty()) {
        return {};
    }
    const int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (n <= 1) {
        return {};
    }
    std::string out(static_cast<size_t>(n - 1), '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, out.data(), n, nullptr, nullptr);
    return out;
}

std::wstring TempPng(const wchar_t* name) {
    wchar_t dir[MAX_PATH]{};
    GetTempPathW(MAX_PATH, dir);
    return std::wstring(dir) + name;
}

std::string EnsureGradientPng() {
    const std::wstring path = TempPng(L"cui-gallery-gradient.png");
    constexpr UINT w = 320;
    constexpr UINT h = 180;
    std::vector<uint32_t> px(static_cast<size_t>(w) * h);
    for (UINT y = 0; y < h; ++y) {
        for (UINT x = 0; x < w; ++x) {
            const float u = static_cast<float>(x) / (w - 1);
            const float v = static_cast<float>(y) / (h - 1);
            const uint32_t r = static_cast<uint32_t>((0.15f + 0.55f * u) * 255.0f);
            const uint32_t g = static_cast<uint32_t>((0.35f + 0.40f * (1.0f - v)) * 255.0f);
            const uint32_t b = static_cast<uint32_t>((0.75f + 0.20f * v) * 255.0f);
            px[static_cast<size_t>(y) * w + x] = (255u << 24) | (r << 16) | (g << 8) | b;
        }
    }
    WritePng(path, w, h, px);
    return WideToUtf8(path);
}

std::string EnsureCheckerPng() {
    const std::wstring path = TempPng(L"cui-gallery-checker.png");
    constexpr UINT w = 128;
    constexpr UINT h = 128;
    std::vector<uint32_t> px(static_cast<size_t>(w) * h);
    for (UINT y = 0; y < h; ++y) {
        for (UINT x = 0; x < w; ++x) {
            const bool on = ((x / 16) + (y / 16)) % 2 == 0;
            const uint32_t c = on ? 0xFF2563EB : 0xFFF8FAFC;
            px[static_cast<size_t>(y) * w + x] = c;
        }
    }
    WritePng(path, w, h, px);
    return WideToUtf8(path);
}

std::shared_ptr<Image> MakePreview(float w, float h, Stretch stretch) {
    CUI::Widgets::Ref img = Widgets::Image().Shared();
    img.Width(w);
    img.Height(h);
    img.Stretch(stretch);
    img.CornerRadius(6.0f);
    img.BorderThickness(1.0f);
    img.BorderToken(ThemeTokenId::CardBorder);
    return img;
}

const char* FirstExisting(std::initializer_list<const char*> paths) {
    for (const char* p : paths) {
        const std::wstring w = [p]() {
            const int n = MultiByteToWideChar(CP_UTF8, 0, p, -1, nullptr, 0);
            std::wstring out(static_cast<size_t>(n > 0 ? n - 1 : 0), L'\0');
            MultiByteToWideChar(CP_UTF8, 0, p, -1, out.data(), n);
            return out;
        }();
        if (GetFileAttributesW(w.c_str()) != INVALID_FILE_ATTRIBUTES) {
            return p;
        }
    }
    return nullptr;
}

} // anonymous namespace

Element BuildImagePage() {
    const std::string gradient = EnsureGradientPng();
    const std::string checker = EnsureCheckerPng();
    const char* wallpaper = FirstExisting({
        "C:\\Windows\\Web\\Wallpaper\\Windows\\img0.jpg",
        "C:\\Windows\\Web\\4K\\Wallpaper\\Windows\\img0.jpg",
        "C:\\Windows\\Web\\Screen\\img100.jpg",
    });

    // ---------- 1. 文件图像加载 ----------
    CUI::Widgets::Ref preview =MakePreview(300.0f, 170.0f, Stretch::Uniform);
        preview.Source(wallpaper ? wallpaper : gradient);

    auto status = MakeStatus("就绪。点击按钮或选择本地图片文件加载预览。");

    auto applySource = [preview, status](const std::string& path) {
        if (preview.Source(path)) {
            status->Text = std::format("[Image] 已加载: {}", path);
        } else {
            status->Text = std::format("[Image] 加载失败: {}  ({})", preview->GetLoadError(), path);
        }
    };

    CUI::Widgets::Ref picker = Widgets::FilePicker().Shared();
    picker.Width(320.0f);
    picker.Height(32.0f);
    picker.Filter("图片", "*.png;*.jpg;*.jpeg;*.bmp;*.gif;*.tif;*.tiff;*.ico");
    picker.DialogTitle("选择图片");
    picker->OnPathChanged().Connect([applySource](FilePicker*, const std::string& path) {
        applySource(path);
    });

    auto btnGrad = ElevatedButton("渐变 PNG", [applySource, gradient](UIElement*) { applySource(gradient); }).Build();
    auto btnCheck = ElevatedButton("棋盘 PNG", [applySource, checker](UIElement*) { applySource(checker); }).Build();
    auto btnWall = ElevatedButton("系统壁纸", [applySource, wallpaper](UIElement*) {
        if (wallpaper) applySource(wallpaper);
    }).Build();
    ElementBuilder<CUI::Button>(btnWall).IsEnabled(wallpaper != nullptr);

    auto stretchLabel = MakeStatus("当前拉伸: Uniform（等比缩放，完整显示）");

    auto btnU = ElevatedButton("Uniform", [preview, stretchLabel](UIElement*) {
        preview.Stretch(Stretch::Uniform);
        stretchLabel->Text = "当前拉伸: Uniform — 等比缩放，完整显示（两侧留白）";
    }).Build();
    auto btnF = ElevatedButton("Fill", [preview, stretchLabel](UIElement*) {
        preview.Stretch(Stretch::Fill);
        stretchLabel->Text = "当前拉伸: Fill — 强行拉伸充满，可能变形";
    }).Build();
    auto btnN = ElevatedButton("None", [preview, stretchLabel](UIElement*) {
        preview.Stretch(Stretch::None);
        stretchLabel->Text = "当前拉伸: None — 保持原始像素大小";
    }).Build();
    auto btnC = ElevatedButton("UniformToFill", [preview, stretchLabel](UIElement*) {
        preview.Stretch(Stretch::UniformToFill);
        stretchLabel->Text = "当前拉伸: UniformToFill — 等比放大填满，多余裁剪";
    }).Build();

    // ---------- 2. 四种拉伸模式对比 ----------
    CUI::Widgets::Ref uniform =MakePreview(160.0f, 100.0f, Stretch::Uniform);
    uniform.Source(gradient);
    CUI::Widgets::Ref fill =MakePreview(160.0f, 100.0f, Stretch::Fill);
    fill.Source(gradient);
    CUI::Widgets::Ref none =MakePreview(160.0f, 100.0f, Stretch::None);
    none.Source(checker);
    CUI::Widgets::Ref cover =MakePreview(160.0f, 100.0f, Stretch::UniformToFill);
    cover.Source(checker);

    // ---------- 3. 占位绘制模式（无需文件） ----------
    CUI::Widgets::Ref avatar1 = Widgets::Image(ImageType::Avatar, "CUI", Rgb(0x007ACC)).Shared();
    avatar1.Width(56.0f);
    avatar1.Height(56.0f);
    CUI::Widgets::Ref avatar2 = Widgets::Image(ImageType::Avatar, "BU", Rgb(0x13A10E)).Shared();
    avatar2.Width(56.0f);
    avatar2.Height(56.0f);
    CUI::Widgets::Ref avatar3 = Widgets::Image(ImageType::Avatar, "FF", Rgb(0xD83B01)).Shared();
    avatar3.Width(56.0f);
    avatar3.Height(56.0f);

    CUI::Widgets::Ref iconPng = Widgets::Image(ImageType::FileIcon, "PNG").Shared();
    iconPng.Width(52.0f);
    iconPng.Height(52.0f);
    CUI::Widgets::Ref iconDoc = Widgets::Image(ImageType::FileIcon, "DOC").Shared();
    iconDoc.Width(52.0f);
    iconDoc.Height(52.0f);
    CUI::Widgets::Ref iconXls = Widgets::Image(ImageType::FileIcon, "XLS").Shared();
    iconXls.Width(52.0f);
    iconXls.Height(52.0f);

    CUI::Widgets::Ref badgeOnline = Widgets::Image(ImageType::StatusBadge, "").Shared();
    badgeOnline.Width(18.0f);
    badgeOnline.Height(18.0f);
    badgeOnline.BadgeColor(Rgb(0x13A10E));
    badgeOnline.BadgeText("●");
    CUI::Widgets::Ref badgeWarn = Widgets::Image(ImageType::StatusBadge, "").Shared();
    badgeWarn.Width(18.0f);
    badgeWarn.Height(18.0f);
    badgeWarn.BadgeColor(Rgb(0xFFB900));
    badgeWarn.BadgeText("!");
    CUI::Widgets::Ref badgeErr = Widgets::Image(ImageType::StatusBadge, "").Shared();
    badgeErr.Width(18.0f);
    badgeErr.Height(18.0f);
    badgeErr.BadgeColor(Rgb(0xE74856));
    badgeErr.BadgeText("✕");

    SamplePageSpec spec;
    spec.title = "Image (图像)";
    spec.subtitle = "WIC 异步解码 PNG / JPEG / BMP / GIF / TIFF 到 Direct2D 硬件位图，内置头像 / 文件图标 / 状态徽章占位绘制，四种拉伸规则。";
    spec.sections = {
        {
            "文件图像加载（WIC → Direct2D）",
            "1. 通过 SetSource 传入磁盘路径，WIC 异步解码、渲染时缓存为 D2D 硬件位图；\n"
            "2. 主题切换后自动从 WIC 源重建位图，不失真不丢失；\n"
            "3. 支持 FilePicker 选择任意图片，也可点击右侧按钮载入运行时生成的示例图。",
            Column(12, {
                Row(8, { picker, btnGrad, btnCheck, btnWall }),
                preview,
                Row(8, { btnU, btnF, btnN, btnC }),
                stretchLabel,
                status,
            }),
        },
        {
            "四种拉伸模式对比",
            "同一张图片在 160×100 的固定画布上，分别应用 Uniform / Fill / None / UniformToFill：\n"
            "None 保持原始像素；Fill 拉伸变形；Uniform 等比留白；UniformToFill 等比裁剪。",
            Row(12, {
                Column(8, { Text("Uniform"), uniform }),
                Column(8, { Text("Fill"), fill }),
                Column(8, { Text("None"), none }),
                Column(8, { Text("UniformToFill"), cover }),
            }),
        },
        {
            "占位绘制模式（无需文件）",
            "Image 提供三种内建绘制样式：圆形头像（文字 + 背景色）、文件类型图标（扩展名标识）、状态徽章（小圆点 + 徽标文字）。",
            Column(12, {
                Row(16, {
                    Column(8, { Text("头像 Avatar"), Row(12, { avatar1, avatar2, avatar3 }) }),
                    Column(8, { Text("文件图标 FileIcon"), Row(12, { iconPng, iconDoc, iconXls }) }),
                    Column(8, { Text("状态徽章 StatusBadge"), Row(12, { badgeOnline, badgeWarn, badgeErr }) }),
                }),
            }),
        },
    };

    spec.source = R"(
// 1) 加载磁盘图片文件（WIC 异步解码）
CUI::Widgets::Ref image = Widgets::Image()\n    .Width(300.0f)\n    .Height(170.0f)\n    .StretchMode(Stretch::Uniform)\n    .Source("C:\\path\\to\\photo.png")\n    .Shared();
// Source 已在 Fluent Builder 中配置。

// 2) 内建占位绘制（无需文件）
CUI::Widgets::Ref avatar = Widgets::Image(ImageType::Avatar, "CUI", Rgb(0x007ACC)).Shared();
CUI::Widgets::Ref icon   = Widgets::Image(ImageType::FileIcon, "PNG").Shared();
CUI::Widgets::Ref badge  = Widgets::Image(ImageType::StatusBadge, "").Shared();

// 3) 拉伸规则
image.Stretch(Stretch::Fill); // None / Fill / Uniform / UniformToFill

// 4) 像素信息与错误查询
int w = image->GetPixelWidth();
int h = image->GetPixelHeight();
const std::string& err = image->GetLoadError();
)";

    return BuildSamplePage(spec);
}

} // namespace Gallery



