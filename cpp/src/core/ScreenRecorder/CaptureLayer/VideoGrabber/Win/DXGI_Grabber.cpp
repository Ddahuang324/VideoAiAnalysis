#include "DXGI_Grabber.h"

#include "../VideoGrabber.h"

DXGI_Grabber::DXGI_Grabber() = default;
DXGI_Grabber::~DXGI_Grabber() = default;

bool DXGI_Grabber::start() {
    return true;
}

void DXGI_Grabber::stop() {}

void DXGI_Grabber::pause() {}

void DXGI_Grabber::resume() {}

int DXGI_Grabber::getWidth() const {
    return 1920;
}

int DXGI_Grabber::getHeight() const {
    return 1080;
}

int DXGI_Grabber::getFps() const {
    return 60;
}

PixelFormat DXGI_Grabber::getPixelFormat() const {
    return PixelFormat::RGBA;
}

bool DXGI_Grabber::isRunning() const {
    return true;
}

bool DXGI_Grabber::isPaused() const {
    return false;
}

FrameData DXGI_Grabber::CaptureFrame(int timeout_ms) {
    (void)timeout_ms;
    return FrameData();
}
