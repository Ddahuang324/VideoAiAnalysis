#include "GDI_Grabber.h"

#include <opencv2/core/hal/interface.h>

#include <chrono>
#include <cstring>
#include <exception>
#include <memory>
#include <opencv2/core/mat.hpp>
#include <string>

#include "Infra/Log.h"
#include "VideoGrabber.h"
#include "windef.h"
#include "wingdi.h"
#include "winuser.h"


GDI_Grabber::GDI_Grabber() = default;
GDI_Grabber::~GDI_Grabber() {
    stop();
}

bool GDI_Grabber::start() {
    if (isRunning_) {
        return true;
    }

    SetProcessDPIAware();

    HDC hdcScreen = GetDC(nullptr);
    width_ = GetDeviceCaps(hdcScreen, DESKTOPHORZRES);
    height_ = GetDeviceCaps(hdcScreen, DESKTOPVERTRES);
    ReleaseDC(nullptr, hdcScreen);

    if (width_ <= 0 || height_ <= 0) {
        width_ = GetSystemMetrics(SM_CXSCREEN);
        height_ = GetSystemMetrics(SM_CYSCREEN);
    }
    fps_ = 30;

    try {
        guard_ = std::make_unique<GDIResourceGuard>(width_, height_);
    } catch (const std::exception& e) {
        LOG_ERROR(std::string("Failed to initialize GDI resources: ") + e.what());
        return false;
    }

    isRunning_ = true;
    return true;
}

void GDI_Grabber::stop() {
    if (!isRunning_) {
        return;
    }

    guard_.reset();
    isRunning_ = false;
}

void GDI_Grabber::pause() {
    isPaused_ = true;
    LOG_INFO("GDI_Grabber paused");
}

void GDI_Grabber::resume() {
    isPaused_ = false;
    LOG_INFO("GDI_Grabber resumed");
}

int GDI_Grabber::getWidth() const {
    return width_;
}

int GDI_Grabber::getHeight() const {
    return height_;
}

int GDI_Grabber::getFps() const {
    return fps_;
}

PixelFormat GDI_Grabber::getPixelFormat() const {
    return PixelFormat::BGRA;
}

bool GDI_Grabber::isRunning() const {
    return isRunning_;
}

bool GDI_Grabber::isPaused() const {
    return isPaused_;
}

FrameData GDI_Grabber::CaptureFrame(int timeout_ms) {
    (void)timeout_ms;  // Unused parameter
    if (!isRunning_ || isPaused_ || !guard_) {
        return FrameData();
    }

    HDC hScreen = GetDC(nullptr);
    if (!hScreen) {
        LOG_ERROR("GetDC failed in CaptureFrame");
        return FrameData();
    }

    bool success = BitBlt(guard_->getMemoryHDC(), 0, 0, width_, height_, hScreen, 0, 0, SRCCOPY);
    ReleaseDC(nullptr, hScreen);

    if (!success) {
        LOG_ERROR("BitBlt failed");
        return FrameData();
    }

    drawCursor(guard_->getMemoryHDC());

    FrameData frame = CreateFrameData(width_, height_);

    if (frame.data && guard_->getMemoryBits()) {
        size_t size = width_ * height_ * 4;
        std::memcpy(frame.data, guard_->getMemoryBits(), size);

        frame.format = PixelFormat::BGRA;
        frame.frame = cv::Mat(height_, width_, CV_8UC4, frame.data);
        frame.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                                 std::chrono::system_clock::now().time_since_epoch())
                                 .count();
    }

    return frame;
}

void GDI_Grabber::drawCursor(HDC hdc) {
    CURSORINFO cursorInfo = {0};
    cursorInfo.cbSize = sizeof(CURSORINFO);

    if (!GetCursorInfo(&cursorInfo)) {
        LOG_ERROR("GetCursorInfo failed");
        return;
    }

    if (cursorInfo.flags != CURSOR_SHOWING) {
        return;
    }

    ICONINFO iconInfo = {0};
    if (!GetIconInfo(cursorInfo.hCursor, &iconInfo)) {
        LOG_ERROR("GetIconInfo failed");
        return;
    }

    // Adjust position so the hotspot (tip) aligns with the screen coordinates
    int x = cursorInfo.ptScreenPos.x - iconInfo.xHotspot;
    int y = cursorInfo.ptScreenPos.y - iconInfo.yHotspot;

    DrawIconEx(hdc, x, y, cursorInfo.hCursor, 0, 0, 0, nullptr, DI_DEFAULTSIZE | DI_NORMAL);

    // GetIconInfo creates bitmaps that must be deleted
    if (iconInfo.hbmColor)
        DeleteObject(iconInfo.hbmColor);
    if (iconInfo.hbmMask)
        DeleteObject(iconInfo.hbmMask);
}
