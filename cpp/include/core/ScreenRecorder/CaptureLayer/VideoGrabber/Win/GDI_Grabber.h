#pragma once

#include <windows.h>

#undef ERROR

#include <memory>

#include "../VideoGrabber.h"
#include "Infra/Log.h"

class GDIResourceGuard {
public:
    GDIResourceGuard(int width, int height) {
        screenHdc_ = GetDC(nullptr);
        if (!screenHdc_) {
            LOG_ERROR("GetDC failed");
            return;
        }

        memoryHDC_ = CreateCompatibleDC(screenHdc_);
        if (!memoryHDC_) {
            ReleaseDC(nullptr, screenHdc_);
            LOG_ERROR("CreateCompatibleDC failed");
            return;
        }

        ZeroMemory(&bitmapInfo_, sizeof(BITMAPINFO));
        bitmapInfo_.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bitmapInfo_.bmiHeader.biWidth = width;
        bitmapInfo_.bmiHeader.biHeight = -height;  // top-down bitmap
        bitmapInfo_.bmiHeader.biPlanes = 1;
        bitmapInfo_.bmiHeader.biBitCount = 32;
        bitmapInfo_.bmiHeader.biCompression = BI_RGB;

        hBitmap_ = CreateDIBSection(memoryHDC_, &bitmapInfo_, DIB_RGB_COLORS, &memoryBits_, nullptr, 0);
        if (!hBitmap_) {
            DeleteDC(memoryHDC_);
            ReleaseDC(nullptr, screenHdc_);
            LOG_ERROR("CreateDIBSection failed");
            return;
        }

        oldBitmap_ = static_cast<HBITMAP>(SelectObject(memoryHDC_, hBitmap_));
    }

    ~GDIResourceGuard() {
        if (memoryHDC_ && oldBitmap_) {
            SelectObject(memoryHDC_, oldBitmap_);
        }
        if (hBitmap_) {
            DeleteObject(hBitmap_);
        }
        if (memoryHDC_) {
            DeleteDC(memoryHDC_);
        }
        if (screenHdc_) {
            ReleaseDC(nullptr, screenHdc_);
        }
    }

    HDC getMemoryHDC() const { return memoryHDC_; }
    const BITMAPINFO& getBitmapInfo() const { return bitmapInfo_; }
    void* getMemoryBits() const { return memoryBits_; }

private:
    HDC screenHdc_ = nullptr;
    HDC memoryHDC_ = nullptr;
    HBITMAP hBitmap_ = nullptr;
    HBITMAP oldBitmap_ = nullptr;
    BITMAPINFO bitmapInfo_{};
    void* memoryBits_ = nullptr;
};

class GDI_Grabber : public VideoGrabber {
public:
    GDI_Grabber();
    ~GDI_Grabber();

    bool start() override;
    void stop() override;
    void pause() override;
    void resume() override;
    int getWidth() const override;
    int getHeight() const override;
    int getFps() const override;
    PixelFormat getPixelFormat() const override;
    bool isRunning() const override;
    bool isPaused() const override;
    FrameData CaptureFrame(int timeout_ms = 100) override;

private:
    void drawCursor(HDC hdc);

    std::unique_ptr<GDIResourceGuard> guard_;

    int width_ = 0;
    int height_ = 0;
    int fps_ = 0;
    bool isRunning_ = false;
    bool isPaused_ = false;
};
