#include "PageRegistry.h"
#include "../ShowcaseHelpers.h"
#include "framework/core/CUIDsl.h"
#include "framework/controls/Image.h"
#include "framework/controls/Button.h"
#include "framework/controls/FilePicker.h"
#include "framework/controls/TextBlock.h"
#include <wincodec.h>
#include <wrl/client.h>
#include <windows.h>
#include <vector>
#include <string>

#pragma comment(lib, "windowscodecs.lib")

using namespace CUI;
using namespace CUI::DSL;
using Microsoft::WRL::ComPtr;

namespace {

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
    const std::wstring path = TempPng(L"cui-demo-gradient.png");
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
    const std::wstring path = TempPng(L"cui-demo-checker.png");
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

const char* FirstExisting(std::initializer_list<const char*> paths) {
    for (const char* p : paths) {
        const std::wstring w = Utf8ToUtf16(p);
        if (GetFileAttributesW(w.c_str()) != INVALID_FILE_ATTRIBUTES) {
            return p;
        }
    }
    return nullptr;
}

std::shared_ptr<Image> MakePreview(float w, float h, Stretch stretch) {
    auto img = std::make_shared<Image>();
    img->SetWidth(w);
    img->SetHeight(h);
    img->SetStretch(stretch);
    img->SetCornerRadius(6.0f);
    return img;
}

} // namespace

ShowcasePage BuildImagePage(const ShowcaseContext& ctx) {
    const std::string gradient = EnsureGradientPng();
    const std::string checker = EnsureCheckerPng();
    const char* wallpaper = FirstExisting({
        "C:\\Windows\\Web\\Wallpaper\\Windows\\img0.jpg",
        "C:\\Windows\\Web\\4K\\Wallpaper\\Windows\\img0.jpg",
        "C:\\Windows\\Web\\Screen\\img100.jpg",
    });

    auto preview = MakePreview(280.0f, 160.0f, Stretch::Uniform);
    preview->SetSource(wallpaper ? wallpaper : gradient);

    auto uniform = MakePreview(160.0f, 100.0f, Stretch::Uniform);
    uniform->SetSource(gradient);
    auto fill = MakePreview(160.0f, 100.0f, Stretch::Fill);
    fill->SetSource(gradient);
    auto none = MakePreview(160.0f, 100.0f, Stretch::None);
    none->SetSource(checker);
    auto cover = MakePreview(160.0f, 100.0f, Stretch::UniformToFill);
    cover->SetSource(checker);

    auto avatar = std::make_shared<Image>(ImageType::Avatar, "CUI");
    avatar->SetWidth(48.0f);
    avatar->SetHeight(48.0f);
    auto fileIcon = std::make_shared<Image>(ImageType::FileIcon, "PNG");
    fileIcon->SetWidth(48.0f);
    fileIcon->SetHeight(48.0f);
    auto badge = std::make_shared<Image>(ImageType::StatusBadge, "");
    badge->SetWidth(18.0f);
    badge->SetHeight(18.0f);

    auto status = std::static_pointer_cast<TextBlock>(
        CreateShowcaseText("就绪", 12.0f, "#B5CEA8", false, "Consolas"));

    auto applySource = [preview, status](const std::string& path) {
        if (preview->SetSource(path)) {
            status->SetText("[Image] " + path + "  "
                + std::to_string(preview->GetPixelWidth()) + "x"
                + std::to_string(preview->GetPixelHeight()));
        } else {
            status->SetText("[Image] 加载失败: " + preview->GetLoadError() + "  " + path);
        }
    };

    auto picker = std::make_shared<FilePicker>();
    picker->SetWidth(320.0f);
    picker->SetHeight(32.0f);
    picker->SetFilter("图片", "*.png;*.jpg;*.jpeg;*.bmp;*.gif;*.tif;*.tiff;*.ico");
    picker->SetDialogTitle("选择图片");
    picker->OnPathChanged().Connect([applySource](FilePicker*, const std::string& path) {
        applySource(path);
    });

    auto btnGrad = std::make_shared<Button>("渐变 PNG");
    btnGrad->OnClick().Connect([applySource, gradient](UIElement*) { applySource(gradient); });
    auto btnCheck = std::make_shared<Button>("棋盘 PNG");
    btnCheck->OnClick().Connect([applySource, checker](UIElement*) { applySource(checker); });
    auto btnWall = std::make_shared<Button>("系统壁纸");
    btnWall->SetIsEnabled(wallpaper != nullptr);
    btnWall->OnClick().Connect([applySource, wallpaper](UIElement*) {
        if (wallpaper) {
            applySource(wallpaper);
        }
    });

    auto btnU = std::make_shared<Button>("Uniform");
    btnU->OnClick().Connect([preview, status](UIElement*) {
        preview->SetStretch(Stretch::Uniform);
        status->SetText("[Image] Stretch = Uniform");
    });
    auto btnF = std::make_shared<Button>("Fill");
    btnF->OnClick().Connect([preview, status](UIElement*) {
        preview->SetStretch(Stretch::Fill);
        status->SetText("[Image] Stretch = Fill");
    });
    auto btnN = std::make_shared<Button>("None");
    btnN->OnClick().Connect([preview, status](UIElement*) {
        preview->SetStretch(Stretch::None);
        status->SetText("[Image] Stretch = None");
    });
    auto btnC = std::make_shared<Button>("UniformToFill");
    btnC->OnClick().Connect([preview, status](UIElement*) {
        preview->SetStretch(Stretch::UniformToFill);
        status->SetText("[Image] Stretch = UniformToFill");
    });

    auto demo = Column(12).Children({
        CreateDemoSurface({
            CreateShowcaseText("文件图像（WIC → Direct2D）", 13.0f, "textPrimary", true),
            CreateShowcaseText("PNG / JPEG / BMP / GIF / TIFF / ICO。主题切换后从 WIC 缓存重建 D2D 位图。", 12.0f, "textSecondary", false),
            preview,
            Row(8).Children({ picker }).Build(),
            Row(8).Children({ btnGrad, btnCheck, btnWall }).Build(),
            Row(8).Children({ btnU, btnF, btnN, btnC }).Build(),
            status,
        }, 10.0f),
        CreateDemoSurface({
            CreateShowcaseText("Stretch", 13.0f, "textPrimary", true),
            Row(12).Children({
                Column(4).Children({ CreateShowcaseText("Uniform", 11.0f, "textMuted", false), uniform }).Build(),
                Column(4).Children({ CreateShowcaseText("Fill", 11.0f, "textMuted", false), fill }).Build(),
                Column(4).Children({ CreateShowcaseText("None", 11.0f, "textMuted", false), none }).Build(),
                Column(4).Children({ CreateShowcaseText("UniformToFill", 11.0f, "textMuted", false), cover }).Build(),
            }).Build(),
        }, 10.0f),
        CreateDemoSurface({
            CreateShowcaseText("占位绘制（无文件）", 13.0f, "textPrimary", true),
            Row(16).Children({ avatar, fileIcon, badge }).Build(),
        }, 10.0f),
    }).Build();

    return { "Image 图像", CreatePage(
        "Image 图像",
        "WIC 解码 PNG/JPEG 等到 Direct2D 位图；Stretch 四种适配；亮暗切换不丢源图。",
        demo) };
}
