#pragma once

#include <windows.h>

#undef ERROR

#include <memory>

#include "Infra/Log.h"
#include "core/ScreenRecorder/CaptureLayer/IScreenGrabber.h"

class GDIResourceGuard {
public:
    GDIResourceGuard(int width, int height) {
        m_screenHdc = GetDC(nullptr);
        if (!m_screenHdc) {
            LOG_ERROR("GetDC failed");
            return;
        }

        m_memoryHDC = CreateCompatibleDC(m_screenHdc);
        if (!m_memoryHDC) {
            ReleaseDC(nullptr, m_screenHdc);
            LOG_ERROR("CreateCompatibleDC failed");
            return;
        }

        ZeroMemory(&m_bitmapinfo, sizeof(BITMAPINFO));
        m_bitmapinfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        m_bitmapinfo.bmiHeader.biWidth = width;
        m_bitmapinfo.bmiHeader.biHeight = -height;  // top-down bitmap
        m_bitmapinfo.bmiHeader.biPlanes = 1;
        m_bitmapinfo.bmiHeader.biBitCount = 32;
        m_bitmapinfo.bmiHeader.biCompression = BI_RGB;

        // Use CreateDIBSection to get direct access to memory bits
        m_hBitmap =
            CreateDIBSection(m_memoryHDC, &m_bitmapinfo, DIB_RGB_COLORS, &m_memoryBits, nullptr, 0);
        if (!m_hBitmap) {
            DeleteDC(m_memoryHDC);
            ReleaseDC(nullptr, m_screenHdc);
            LOG_ERROR("CreateDIBSection failed");
            return;
        }

        m_oldBitmap = static_cast<HBITMAP>(SelectObject(m_memoryHDC, m_hBitmap));
    }

    ~GDIResourceGuard() {
        if (m_memoryHDC && m_oldBitmap) {
            SelectObject(m_memoryHDC, m_oldBitmap);
        }
        if (m_hBitmap) {
            DeleteObject(m_hBitmap);
        }
        if (m_memoryHDC) {
            DeleteDC(m_memoryHDC);
        }
        if (m_screenHdc) {
            ReleaseDC(nullptr, m_screenHdc);
        }
    }

    HDC getMemoryHDC() const { return m_memoryHDC; }
    const BITMAPINFO& getBitmapInfo() const { return m_bitmapinfo; }
    void* getMemoryBits() const { return m_memoryBits; }

private:
    HDC m_screenHdc{};
    HDC m_memoryHDC{};
    HBITMAP m_hBitmap{};
    HBITMAP m_oldBitmap{};
    BITMAPINFO m_bitmapinfo{};
    void* m_memoryBits{};
};

class GDI_Grabber : public IScreenGrabber {
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

    HDC m_screenHdc{};
    HDC m_memoryHDC{};
    HBITMAP m_hBitmap{};
    HBITMAP m_oldBitmap{};
    BITMAPINFO m_bitmapinfo{};
    std::unique_ptr<GDIResourceGuard> m_guard;

    int m_width{};
    int m_height{};
    int m_fps{};
    bool m_isRunning{false};
    bool m_isPaused{false};
};