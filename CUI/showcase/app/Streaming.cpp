#include "Streaming.h"
#include <Windows.h>
#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

using namespace CUI;

namespace {
std::atomic<bool> g_isStreaming{ false };
std::thread g_streamThread;
Window* g_activeWindow = nullptr;
std::shared_ptr<Image> g_streamImage = nullptr;
}

std::shared_ptr<Image> CreateStreamImage() {
    auto image = std::make_shared<Image>();
    image->SetWidth(420.0f);
    image->SetHeight(240.0f);
    image->SetStretch(Stretch::Fill);
    image->SetImageType(ImageType::DynamicBitmap);
    image->SetClipToBounds(true);
    return image;
}

void StartStreamingThread(Window* window, const std::shared_ptr<Image>& image) {
    if (g_isStreaming) return;
    if (!window || !image) return;

    g_activeWindow = window;
    g_streamImage = image;
    image->SetStretch(Stretch::Fill);
    image->SetImageType(ImageType::DynamicBitmap);
    image->RequestAnimationTicks();
    g_isStreaming = true;
    g_streamThread = std::thread([]() {
        uint32_t width = 640;
        uint32_t height = 360;
        std::vector<uint32_t> pixels(width * height);

        HDC hdcScreen = GetDC(nullptr);
        HDC hdcMem = CreateCompatibleDC(hdcScreen);
        HBITMAP hbm = CreateCompatibleBitmap(hdcScreen, width, height);
        SelectObject(hdcMem, hbm);

        BITMAPINFO bmi = {};
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = width;
        bmi.bmiHeader.biHeight = -static_cast<int32_t>(height);
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;

        int screenW = GetSystemMetrics(SM_CXSCREEN);
        int screenH = GetSystemMetrics(SM_CYSCREEN);

        while (g_isStreaming) {
            SetStretchBltMode(hdcMem, HALFTONE);
            StretchBlt(hdcMem, 0, 0, width, height, hdcScreen, 0, 0, screenW, screenH, SRCCOPY);
            GetDIBits(hdcMem, hbm, 0, height, pixels.data(), &bmi, DIB_RGB_COLORS);

            if (g_streamImage) {
                g_streamImage->UpdatePixelBuffer(pixels.data(), width, height);
            }
            if (g_activeWindow && g_activeWindow->GetHWND()) {
                InvalidateRect(g_activeWindow->GetHWND(), nullptr, FALSE);
                PostMessage(g_activeWindow->GetHWND(), WM_NULL, 0, 0);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        DeleteObject(hbm);
        DeleteDC(hdcMem);
        ReleaseDC(nullptr, hdcScreen);
    });
}

void StopStreamingThread() {
    g_isStreaming = false;
    if (g_streamThread.joinable()) {
        g_streamThread.join();
    }
}
