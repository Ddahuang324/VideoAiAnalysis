#pragma once

#include <memory>
#include <vector>

#include "VideoGrabber.h"

enum class GrabberType {
    AUTO,
    GDI,
    DXGI,
    X11,
    PIPEWIRE,
};

class VideoGrabberFactory {
public:
    static std::shared_ptr<VideoGrabber> createGrabber(GrabberType type);

    static std::vector<GrabberType> getAvailableGrabbers();

private:
    static GrabberType detectBestGrabber();
};
