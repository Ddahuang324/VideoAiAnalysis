#include "core/ScreenRecorder/CaptureLayer/SpecificGrabber/Win/GDI_Grabber.h"

#include <chrono>
#include <cstring>
#include <exception>
#include <string>

#include "Infra/Log.h"

GDI_Grabber::GDI_Grabber() = default;
GDI_Grabber::~GDI_Grabber() {
    stop();
}

bool GDI_Grabber::start() {
    if (m_isRunning)
        return true;

    // 尝试启用 DPI 感知，确保截取到物理像素分辨率
    // 注意：SetProcessDPIAware 在较新 Windows 版本中建议使用 SetProcessDpiAwareness，
    // 但为了兼容性和简便性，且作为 Demo/Test 这是一个快速方案。
    // 如果已经设置过（如通过 Manifest），这行可能失败但无害。
    SetProcessDPIAware();

    HDC hdcScreen = GetDC(nullptr);
    m_width = GetDeviceCaps(hdcScreen, DESKTOPHORZRES);
    m_height = GetDeviceCaps(hdcScreen, DESKTOPVERTRES);
    ReleaseDC(nullptr, hdcScreen);

    // 如果获取失败（极少情况），回退到 SystemMetrics
    if (m_width <= 0 || m_height <= 0) {
        m_width = GetSystemMetrics(SM_CXSCREEN);
        m_height = GetSystemMetrics(SM_CYSCREEN);
    }
    m_fps = 30;  // Default to 30 FPS for GDI

    try {
        m_guard = std::make_unique<GDIResourceGuard>(m_width, m_height);
    } catch (const std::exception& e) {
        LOG_ERROR(std::string("Failed to initialize GDI resources: ") + e.what());
        return false;
    }

    m_isRunning = true;
    return true;
}

void GDI_Grabber::stop() {
    if (!m_isRunning)
        return;

    m_guard.reset();
    m_isRunning = false;
}

void GDI_Grabber::pause() {
    m_isPaused = true;
    LOG_INFO("GDI_Grabber paused");
}

void GDI_Grabber::resume() {
    m_isPaused = false;
    LOG_INFO("GDI_Grabber resumed");
}

int GDI_Grabber::getWidth() const {
    return m_width;
}

int GDI_Grabber::getHeight() const {
    return m_height;
}

int GDI_Grabber::getFps() const {
    return m_fps;
}

PixelFormat GDI_Grabber::getPixelFormat() const {
    return PixelFormat::BGRA;
}

bool GDI_Grabber::isRunning() const {
    return m_isRunning;
}

bool GDI_Grabber::isPaused() const {
    return m_isPaused;
}

FrameData GDI_Grabber::CaptureFrame(int timeout_ms) {
    if (!m_isRunning || m_isPaused || !m_guard)
        return FrameData();

    // Get a temporary DC for the screen to capture from
    HDC hScreen = GetDC(nullptr);
    if (!hScreen) {
        LOG_ERROR("GetDC failed in CaptureFrame");
        return FrameData();
    }

    // Capture screen to our memory DC (backed by DIBSection)
    bool success = BitBlt(m_guard->getMemoryHDC(), 0, 0, m_width, m_height, hScreen, 0, 0, SRCCOPY);

    ReleaseDC(nullptr, hScreen);

    if (!success) {
        LOG_ERROR("BitBlt failed");
        return FrameData();
    }

    drawCursor(m_guard->getMemoryHDC());

    // Create a new FrameData with its own buffer
    FrameData frame = ::CaptureFrame(m_width, m_height);

    // Deep copy the bits from the DIBSection to the FrameData buffer
    if (frame.data && m_guard->getMemoryBits()) {
        size_t size = m_width * m_height * 4;
        std::memcpy(frame.data, m_guard->getMemoryBits(), size);

        frame.format = PixelFormat::BGRA;
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
