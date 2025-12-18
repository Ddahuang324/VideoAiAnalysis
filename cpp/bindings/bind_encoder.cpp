#include <pybind11/attr.h>
#include <pybind11/cast.h>
#include <pybind11/functional.h>
#include <pybind11/gil.h>
#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>
#include <pybind11/stl.h>

#include <cstdint>
#include <memory>
#include <string>

#include "FFmpegWrapper.h"
#include "FrameEncoder.h"
#include "IScreenGrabber.h"
#include "ThreadSafetyQueue.h"
#include "bindings.h"

void bind_encoder(py::module& m) {
    // 绑定 EncoderConfig 结构体
    py::class_<EncoderConfig>(m, "EncoderConfig", "Video encoder configuration")
        .def(py::init<>(), "Create default encoder config")
        .def_readwrite("output_file_path", &EncoderConfig::outputFilePath, "Output video file path")
        .def_readwrite("width", &EncoderConfig::width, "Video width in pixels")
        .def_readwrite("height", &EncoderConfig::height, "Video height in pixels")
        .def_readwrite("fps", &EncoderConfig::fps, "Frames per second")
        .def_readwrite("bitrate", &EncoderConfig::bitrate, "Video bitrate")
        .def_readwrite("crf", &EncoderConfig::crf, "Constant Rate Factor (quality)")
        .def_readwrite("preset", &EncoderConfig::preset, "Encoder preset (e.g., 'fast', 'medium')")
        .def_readwrite("codec", &EncoderConfig::codec, "Video codec (e.g., 'libx264')");

    // 添加默认配置工厂函数
    m.def("default_encoder_config", &defaultEncoderConfig, "Get default encoder configuration");

    // 绑定 FFmpegWrapper 类
    py::class_<FFmpegWrapper>(m, "FFmpegWrapper", "FFmpeg video encoder wrapper")
        .def(py::init<>(), "Initialize FFmpegWrapper")

        // 核心方法
        .def("initialize", &FFmpegWrapper::initialize, py::arg("config"),
             "Initialize encoder with configuration")
        .def("encoder_frame", &FFmpegWrapper::encoderFrame, py::arg("frame_data"),
             "Encode a single frame")
        .def("finalize", &FFmpegWrapper::finalize, "Finalize encoding and close output file")

        // 状态查询
        .def("get_output_file_size", &FFmpegWrapper::getOutputFileSize,
             "Get current output file size in bytes")
        .def("get_last_error", &FFmpegWrapper::getLastError, "Get last error message")
        .def("is_initialized", &FFmpegWrapper::isInitialized, "Check if encoder is initialized")
        .def("get_encoded_frame_count", &FFmpegWrapper::getEncodedFrameCount,
             "Get number of encoded frames");

    // 绑定 FrameEncoder 类
    py::class_<FrameEncoder>(m, "FrameEncoder", "Frame encoder with threading support")
        .def(py::init<std::shared_ptr<ThreadSafetyQueue<FrameData>>, const EncoderConfig&>(),
             py::arg("queue"), py::arg("config"), "Initialize FrameEncoder with queue and config")

        // 控制方法
        .def("start", &FrameEncoder::start, py::call_guard<py::gil_scoped_release>(),
             "Start the encoding thread")
        .def("stop", &FrameEncoder::stop, py::call_guard<py::gil_scoped_release>(),
             "Stop the encoding thread")

        // 状态查询
        .def("is_running", &FrameEncoder::isRunning, "Check if encoder is running")
        .def("get_encoded_frame_count", &FrameEncoder::getEncodedFrameCount,
             "Get number of encoded frames")
        .def("get_output_file_size", &FrameEncoder::getOutputFileSize,
             "Get current output file size in bytes")

        // 回调设置方法(支持 GIL 安全的 Python 回调)
        .def(
            "set_progress_callback",
            [](FrameEncoder& self, py::function callback) {
                // 包装 Python 回调,在调用时自动获取 GIL
                self.setProgressCallback([callback](int64_t frames, int64_t size) {
                    py::gil_scoped_acquire gil;
                    callback(frames, size);
                });
            },
            py::arg("callback"), "Set progress callback: callback(frames, file_size)")
        .def(
            "set_finished_callback",
            [](FrameEncoder& self, py::function callback) {
                // 包装 Python 回调,在调用时自动获取 GIL
                self.setFinishedCallback([callback](int64_t total_frames, const std::string& path) {
                    py::gil_scoped_acquire gil;
                    callback(total_frames, path);
                });
            },
            py::arg("callback"), "Set finished callback: callback(total_frames, output_path)")
        .def(
            "set_error_callback",
            [](FrameEncoder& self, py::function callback) {
                // 包装 Python 回调,在调用时自动获取 GIL
                self.setErrorCallback([callback](const std::string& error_message) {
                    py::gil_scoped_acquire gil;
                    callback(error_message);
                });
            },
            py::arg("callback"), "Set error callback: callback(error_message)");
}
