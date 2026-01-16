/**
 * @file bind_recorder_config.cpp
 * @brief RecorderConfig 及相关配置类的 Python 绑定
 */

#include <pybind11/attr.h>
#include <pybind11/cast.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <string>

#include "core/Config/UnifiedConfig.h"
#include "nlohmann/json_fwd.hpp"

namespace py = pybind11;

namespace Recorder::Bindings {

void bind_recorder_config(py::module& m) {
    using namespace Config;

    // 绑定 VideoEncoderConfig
    py::class_<VideoEncoderConfig>(m, "VideoEncoderConfig", "视频编码器配置")
        .def(py::init<>(), "默认构造函数")
        .def_readwrite("output_file_path", &VideoEncoderConfig::outputFilePath, "输出文件路径")
        .def_readwrite("width", &VideoEncoderConfig::width, "视频宽度")
        .def_readwrite("height", &VideoEncoderConfig::height, "视频高度")
        .def_readwrite("fps", &VideoEncoderConfig::fps, "帧率")
        .def_readwrite("bitrate", &VideoEncoderConfig::bitrate, "码率 (bps)")
        .def_readwrite("crf", &VideoEncoderConfig::crf, "质量参数 (CRF, 0-51)")
        .def_readwrite("preset", &VideoEncoderConfig::preset,
                       "编码预设 (ultrafast, superfast, veryfast, faster, fast, medium, slow, "
                       "slower, veryslow)")
        .def_readwrite("codec", &VideoEncoderConfig::codec, "编码器名称 (libx264, libx265, etc.)")
        .def("validate", &VideoEncoderConfig::validate,
             "验证配置有效性\n\n"
             "返回:\n"
             "    ValidationResult: 验证结果对象")
        .def("__repr__", [](const VideoEncoderConfig& c) {
            return "<VideoEncoderConfig " + std::to_string(c.width) + "x" +
                   std::to_string(c.height) + "@" + std::to_string(c.fps) + "fps " + c.codec + ">";
        });

    // 绑定 AudioEncoderConfig
    py::class_<AudioEncoderConfig>(m, "AudioEncoderConfig", "音频编码器配置")
        .def(py::init<>(), "默认构造函数")
        .def_readwrite("enabled", &AudioEncoderConfig::enabled, "是否启用音频")
        .def_readwrite("sample_rate", &AudioEncoderConfig::sampleRate, "采样率 (Hz)")
        .def_readwrite("channels", &AudioEncoderConfig::channels, "通道数 (1=单声道, 2=立体声)")
        .def_readwrite("bitrate", &AudioEncoderConfig::bitrate, "码率 (bps)")
        .def_readwrite("codec", &AudioEncoderConfig::codec, "编码器名称 (aac, mp3, etc.)")
        .def("validate", &AudioEncoderConfig::validate, "验证配置有效性")
        .def("__repr__", [](const AudioEncoderConfig& c) {
            return "<AudioEncoderConfig " + std::string(c.enabled ? "enabled" : "disabled") + " " +
                   std::to_string(c.sampleRate) + "Hz " + std::to_string(c.channels) + "ch " +
                   c.codec + ">";
        });

    // 绑定 ZMQConfig (使用 module_local 避免与 analyzer_module 冲突)
    py::class_<ZMQConfig>(m, "ZMQConfig", py::module_local(), "ZMQ 通信配置")
        .def(py::init<>(), "默认构造函数")
        .def_readwrite("endpoint", &ZMQConfig::endpoint, "ZMQ 端点地址 (如 tcp://*:5555)")
        .def_readwrite("timeout_ms", &ZMQConfig::timeoutMs, "超时时间 (毫秒)")
        .def_readwrite("io_threads", &ZMQConfig::ioThreads, "IO 线程数")
        .def("validate", &ZMQConfig::validate, "验证配置有效性")
        .def("__repr__", [](const ZMQConfig& c) {
            return "<ZMQConfig " + c.endpoint + " timeout=" + std::to_string(c.timeoutMs) +
                   "ms threads=" + std::to_string(c.ioThreads) + ">";
        });

    // 绑定 RecorderConfig (统一配置)
    py::class_<RecorderConfig>(m, "RecorderConfig", "录制器统一配置")
        .def(py::init<>(), "默认构造函数")
        .def_readwrite("zmq_publisher", &RecorderConfig::zmqPublisher, "ZMQ 发布器配置")
        .def_readwrite("video", &RecorderConfig::video, "视频编码配置")
        .def_readwrite("audio", &RecorderConfig::audio, "音频编码配置")
        .def("validate", &RecorderConfig::validate,
             "验证整体配置有效性\n\n"
             "返回:\n"
             "    ValidationResult: 验证结果对象")
        .def("load_from_file", &RecorderConfig::loadFromFile, py::arg("filepath"),
             "从 JSON 文件加载配置\n\n"
             "参数:\n"
             "    filepath (str): 配置文件路径\n\n"
             "异常:\n"
             "    RuntimeError: 文件不存在或格式错误")
        .def("save_to_file", &RecorderConfig::saveToFile, py::arg("filepath"),
             "保存配置到 JSON 文件\n\n"
             "参数:\n"
             "    filepath (str): 保存路径")
        .def(
            "to_json", [](const RecorderConfig& c) { return c.toJson().dump(); },
            "将配置转换为 JSON 字符串")
        .def(
            "from_json",
            [](RecorderConfig& c, const std::string& jsonStr) {
                c.fromJson(nlohmann::json::parse(jsonStr));
            },
            py::arg("json_str"),
            "从 JSON 字符串加载配置\n\n"
            "参数:\n"
            "    json_str (str): JSON 字符串")
        .def("__repr__", [](const RecorderConfig& c) {
            return "<RecorderConfig video=" + std::to_string(c.video.width) + "x" +
                   std::to_string(c.video.height) + "@" + std::to_string(c.video.fps) +
                   "fps audio=" + (c.audio.enabled ? "enabled" : "disabled") + ">";
        });

    // 工厂函数: 创建默认录制器配置
    m.def(
        "default_recorder_config",
        []() -> RecorderConfig {
            RecorderConfig config;

            // 视频配置默认值
            config.video.width = 1920;
            config.video.height = 1080;
            config.video.fps = 30;
            config.video.bitrate = 5000000;
            config.video.crf = 23;
            config.video.preset = "medium";
            config.video.codec = "libx264";

            // 音频配置默认值
            config.audio.enabled = true;
            config.audio.sampleRate = 48000;
            config.audio.channels = 2;
            config.audio.bitrate = 128000;
            config.audio.codec = "aac";

            // ZMQ 配置默认值
            config.zmqPublisher.endpoint = "tcp://*:5555";
            config.zmqPublisher.timeoutMs = 100;
            config.zmqPublisher.ioThreads = 1;

            return config;
        },
        "创建默认录制器配置\n\n"
        "返回:\n"
        "    RecorderConfig: 带有默认值的配置对象");
}

}  // namespace Recorder::Bindings
