#include "core/ScreenRecorder/CaptureLayer/GrabberFactory.h"

#include <memory>
#include <vector>

// 抓屏接口
#include "core/ScreenRecorder/CaptureLayer/IScreenGrabber.h"

// Windows 平台具体抓屏实现
#include "core/ScreenRecorder/CaptureLayer/SpecificGrabber/Win/DXGI_Grabber.h"
#include "core/ScreenRecorder/CaptureLayer/SpecificGrabber/Win/GDI_Grabber.h"

std::shared_ptr<IScreenGrabber> GrabberFactory::createGrabber(GrabberType type) {
    if (type == GrabberType::AUTO) {
        type = detectBestGrabber();
    }

    switch (type) {
#ifdef _WIN32
        case GrabberType::GDI:
            return std::make_shared<GDI_Grabber>();
        case GrabberType::DXGI:
            return std::make_shared<DXGI_Grabber>();
#endif

#ifdef __linux__
        case GrabberType::X11:
            return std::make_shared<X11_Grabber>();
        case GrabberType::PIPEWIRE:
            return std::make_shared<PipeWire_Grabber>();
#endif

#ifdef __APPLE__
        case GrabberType::AUTO:

#endif

        default:
            return nullptr;
    }
}

std::vector<GrabberType> GrabberFactory::getAvailableGrabbers() {
    std::vector<GrabberType> availableGrabbers;

#ifdef _WIN32
    availableGrabbers.push_back(GrabberType::GDI);
    availableGrabbers.push_back(GrabberType::DXGI);
#endif

#ifdef __linux__
    availableGrabbers.push_back(GrabberType::X11);
    availableGrabbers.push_back(GrabberType::PIPEWIRE);
#endif

    return availableGrabbers;
}

GrabberType GrabberFactory::detectBestGrabber() {
#ifdef _WIN32
    return GrabberType::GDI;

#elif __linux__
    return GrabberType::X11;

#elif __APPLE__
    return GrabberType::AUTO;

#endif
    return GrabberType::AUTO;
}
