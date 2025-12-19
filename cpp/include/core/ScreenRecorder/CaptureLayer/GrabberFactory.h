#pragma once

#include <memory>
#include <vector>

#include "IScreenGrabber.h"

enum class GrabberType {
    AUTO,
    GDI,
    DXGI,
    X11,
    PIPEWIRE,
};

class GrabberFactory {
public:
    static std::shared_ptr<IScreenGrabber> createGrabber(GrabberType type);

    static std::vector<GrabberType> getAvailableGrabbers();

private:
    static GrabberType detectBestGrabber();
};
