#include "DXGI_Grabber.h"

#include "../VideoGrabber.h"

DXGI_Grabber::DXGI_Grabber() {}
DXGI_Grabber::~DXGI_Grabber() {}

bool DXGI_Grabber::start() {
    // Implementation for starting DXGI grabbing
    return true;
}

void DXGI_Grabber::stop() {
    // Implementation for stopping DXGI grabbing
}

void DXGI_Grabber::pause() {
    // Implementation for pausing DXGI grabbing
}

void DXGI_Grabber::resume() {
    // Implementation for resuming DXGI grabbing
}

int DXGI_Grabber::getWidth() const {
    return 1920;  // Example width
}

int DXGI_Grabber::getHeight() const {
    return 1080;  // Example height
}

int DXGI_Grabber::getFps() const {
    return 60;  // Example FPS
}

PixelFormat DXGI_Grabber::getPixelFormat() const {
    return PixelFormat::RGBA;
}

bool DXGI_Grabber::isRunning() const {
    return true;  // Example state
}

bool DXGI_Grabber::isPaused() const {
    return false;  // Example state
}

FrameData DXGI_Grabber::CaptureFrame(int timeout_ms) {
    FrameData frame;
    // Fill frame data here
    return frame;
}
