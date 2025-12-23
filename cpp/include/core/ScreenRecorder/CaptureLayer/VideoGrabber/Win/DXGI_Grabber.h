#pragma once

#include "../VideoGrabber.h"

class DXGI_Grabber : public VideoGrabber {
public:
    DXGI_Grabber();
    ~DXGI_Grabber();

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
};
