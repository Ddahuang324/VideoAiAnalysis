#pragma once

#include <memory>
#include <string>

#include "FFmpegWrapper.h"
#include "IScreenGrabber.h"

class ScreenRecorder {
public:
    ScreenRecorder();
    ~ScreenRecorder();

    bool startRecording(string path);
    void stopRecording();

private:
    std::unique_ptr<IScreenGrabber> m_grabber;
    std::unique_ptr<FFmpegWrapper> m_ffmpeg;
};