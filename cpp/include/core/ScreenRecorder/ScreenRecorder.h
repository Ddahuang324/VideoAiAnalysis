#pragma once

#include <memory>
#include <string>

#include "FFmpegWrapper.h"
#include "FrameEncoder.h"
#include "IScreenGrabber.h"

class ScreenRecorder {
public:
    ScreenRecorder();
    ~ScreenRecorder();

    bool startRecording(std::string path);
    void stopRecording();

private:
    std::unique_ptr<IScreenGrabber> m_grabber;
    std::unique_ptr<FFmpegWrapper> m_ffmpeg;
    std::unique_ptr<FrameEncoder> m_encoder;
};