#include "WasapiAudioGrabber.h"

#include <chrono>
#include <cstddef>
#include <functional>
#include <thread>

#include "AudioData.h"
#include "AudioSessionTypes.h"
#include "audioclient.h"
#include "basetsd.h"
#include "combaseapi.h"
#include "minwindef.h"
#include "mmdeviceapi.h"
#include "objbase.h"
#include "winerror.h"
#include "winnt.h"

#pragma comment(lib, "Ole32.lib")

namespace {
    constexpr auto SLEEP_DURATION_MS = std::chrono::milliseconds(1);
}

WasapiAudioGrabber::WasapiAudioGrabber() {
    CoInitialize(nullptr);
}

WasapiAudioGrabber::~WasapiAudioGrabber() {
    stop();
    cleanupWasapi();
    CoUninitialize();
}

bool WasapiAudioGrabber::start() {
    if (isRunning_) {
        return true;
    }

    if (!initializeWasapi()) {
        return false;
    }

    isRunning_ = true;
    captureThread_ = std::thread(&WasapiAudioGrabber::captureThreadProc, this);
    return true;
}

void WasapiAudioGrabber::stop() {
    isRunning_ = false;
    if (captureThread_.joinable()) {
        captureThread_.join();
    }
    cleanupWasapi();
}

void WasapiAudioGrabber::setCallback(std::function<void(const AudioData&)> callback) {
    callback_ = std::move(callback);
}

bool WasapiAudioGrabber::initializeWasapi() {
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                                  __uuidof(IMMDeviceEnumerator),
                                  reinterpret_cast<void**>(&enumerator_));
    if (FAILED(hr)) {
        return false;
    }

    hr = enumerator_->GetDefaultAudioEndpoint(eRender, eConsole, &device_);
    if (FAILED(hr)) {
        return false;
    }

    hr = device_->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr,
                           reinterpret_cast<void**>(&audioClient_));
    if (FAILED(hr)) {
        return false;
    }

    hr = audioClient_->GetMixFormat(&waveFormat_);
    if (FAILED(hr)) {
        return false;
    }

    sampleRate_ = static_cast<int>(waveFormat_->nSamplesPerSec);
    channels_ = static_cast<int>(waveFormat_->nChannels);
    bitsPerSample_ = static_cast<int>(waveFormat_->wBitsPerSample);

    hr = audioClient_->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK, 0, 0,
                                  waveFormat_, nullptr);
    if (FAILED(hr)) {
        return false;
    }

    hr = audioClient_->GetService(__uuidof(IAudioCaptureClient),
                                  reinterpret_cast<void**>(&captureClient_));
    if (FAILED(hr)) {
        return false;
    }

    hr = audioClient_->Start();
    return SUCCEEDED(hr);
}

void WasapiAudioGrabber::cleanupWasapi() {
    if (audioClient_) {
        audioClient_->Stop();
        audioClient_->Release();
        audioClient_ = nullptr;
    }
    if (captureClient_) {
        captureClient_->Release();
        captureClient_ = nullptr;
    }
    if (device_) {
        device_->Release();
        device_ = nullptr;
    }
    if (enumerator_) {
        enumerator_->Release();
        enumerator_ = nullptr;
    }
    if (waveFormat_) {
        CoTaskMemFree(waveFormat_);
        waveFormat_ = nullptr;
    }
}

void WasapiAudioGrabber::captureThreadProc() {
    UINT32 packetLength = 0;

    while (isRunning_) {
        HRESULT hr = captureClient_->GetNextPacketSize(&packetLength);
        if (FAILED(hr)) {
            break;
        }

        if (packetLength == 0) {
            std::this_thread::sleep_for(SLEEP_DURATION_MS);
            continue;
        }

        BYTE* pData = nullptr;
        UINT32 numFramesAvailable = 0;
        DWORD flags = 0;

        hr = captureClient_->GetBuffer(&pData, &numFramesAvailable, &flags, nullptr, nullptr);
        if (SUCCEEDED(hr) && callback_) {
            AudioData audioData;
            const int bytesPerFrame = waveFormat_->nBlockAlign;
            const size_t dataSize = numFramesAvailable * bytesPerFrame;

            audioData.data.assign(pData, pData + dataSize);
            audioData.sampleRate = sampleRate_;
            audioData.channels = channels_;
            audioData.samplesPerChannel = static_cast<int>(numFramesAvailable);
            audioData.bitsPerSample = bitsPerSample_;
            audioData.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                                         std::chrono::system_clock::now().time_since_epoch())
                                         .count();

            callback_(audioData);
            captureClient_->ReleaseBuffer(numFramesAvailable);
        } else {
            captureClient_->ReleaseBuffer(numFramesAvailable);
            std::this_thread::sleep_for(SLEEP_DURATION_MS);
        }
    }
}
