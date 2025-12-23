#include <pybind11/attr.h>
#include <pybind11/cast.h>
#include <pybind11/functional.h>
#include <pybind11/gil.h>
#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>
#include <pybind11/stl.h>
#include <pyerrors.h>

#include <cstdint>
#include <optional>
#include <string>

#include "bindings.h"
#include "core/ScreenRecorder/CaptureLayer/IScreenGrabber.h"
#include "core/ScreenRecorder/ProcessLayer/FFmpegWrapper.h"
#include "core/ScreenRecorder/ScreenRecorder.h"

namespace py = pybind11;

// ============================================================================
// GIL 管理辅助函数
// ============================================================================

/**
 * @brief 包装 Python 回调函数,确保在 C++ 线程中调用时正确获取 GIL
 *
 * 【GIL 管理原理】
 * Python 的全局解释器锁(GIL)确保同一时刻只有一个线程执行 Python 字节码。
 * 当 C++ 线程(如编码线程)需要调用 Python 回调时,必须先获取 GIL,
 * 否则会导致 Python 解释器崩溃或数据竞争。
 *
 * 【使用场景】
 * - 进度回调: 从编码线程调用 Python 进度回调
 * - 错误回调: 从编码线程调用 Python 错误处理函数
 * - 完成回调: 从编码线程通知 Python 录制完成
 *
 * @tparam Func 回调函数类型
 * @param callback Python 回调函数
 * @return 包装后的回调函数,自动管理 GIL
 */
template <typename Func>
Func wrapCallbackWithGIL(Func callback) {
    if (!callback) {
        return nullptr;
    }

    // 返回一个 lambda,在调用 Python 回调前获取 GIL
    return [callback](auto&&... args) {
        // 【关键步骤1】获取 GIL - 这对于从 C++ 线程调用 Python 代码至关重要
        // py::gil_scoped_acquire 是 RAII 风格的 GIL 获取器,
        // 在作用域结束时自动释放 GIL
        py::gil_scoped_acquire gil;

        try {
            // 【关键步骤2】调用 Python 回调
            // 此时 GIL 已被持有,可以安全地调用 Python 代码
            callback(std::forward<decltype(args)>(args)...);
        } catch (const py::error_already_set& e) {
            // 【关键步骤3】捕获 Python 异常,避免传播到 C++ 代码
            // Python 异常不应该跨越 C++/Python 边界,否则会导致未定义行为
            // 在实际应用中,可以记录日志或通过其他方式通知用户
            PyErr_Clear();  // 清除 Python 错误状态
            // TODO: 添加日志记录
        }
        // 【关键步骤4】GIL 自动释放(gil 对象析构)
    };
}

void bind_Screen_Recorder(py::module& m) {
    // 绑定 PixelFormat 枚举
    py::enum_<PixelFormat>(m, "PixelFormat", py::arithmetic(), "像素格式枚举")
        .value("UNKNOWN", PixelFormat::UNKNOWN, "未知格式")
        .value("BGRA", PixelFormat::BGRA, "BGRA 格式")
        .value("RGBA", PixelFormat::RGBA, "RGBA 格式")
        .value("RGB24", PixelFormat::RGB24, "RGB24 格式")
        .export_values();

    // 绑定 RecorderMode 枚举
    py::enum_<RecorderMode>(m, "RecorderMode", "录制模式")
        .value("VIDEO", RecorderMode::VIDEO, "视频模式 (高帧率)")
        .value("SNAPSHOT", RecorderMode::SNAPSHOT, "快照模式 (1fps)")
        .export_values();

    // 绑定 FrameData 结构体
    py::class_<FrameData>(m, "FrameData", "帧数据结构")
        .def(py::init<>(), "默认构造函数")
        .def_readwrite("width", &FrameData::width, "帧宽度")
        .def_readwrite("height", &FrameData::height, "帧高度")
        .def_readwrite("format", &FrameData::format, "像素格式")
        .def_readwrite("timestamp_ms", &FrameData::timestamp_ms, "时间戳(毫秒)")
        .def_property_readonly(
            "data_ptr",
            [](const FrameData& frame) -> uintptr_t {
                return reinterpret_cast<uintptr_t>(frame.data);
            },
            "数据指针地址(只读)")
        .def("__repr__", [](const FrameData& frame) {
            return "<FrameData width=" + std::to_string(frame.width) +
                   " height=" + std::to_string(frame.height) +
                   " format=" + std::to_string(static_cast<int>(frame.format)) +
                   " timestamp=" + std::to_string(frame.timestamp_ms) + "ms>";
        });

    // 绑定 EncoderConfig 结构体
    py::class_<EncoderConfig>(m, "EncoderConfig", "编码器配置")
        .def(py::init<>(), "默认构造函数")
        .def_readwrite("outputFilePath", &EncoderConfig::outputFilePath, "输出文件路径")
        .def_readwrite("width", &EncoderConfig::width, "视频宽度")
        .def_readwrite("height", &EncoderConfig::height, "视频高度")
        .def_readwrite("fps", &EncoderConfig::fps, "帧率")
        .def_readwrite("bitrate", &EncoderConfig::bitrate, "码率")
        .def_readwrite("crf", &EncoderConfig::crf, "质量参数(CRF)")
        .def_readwrite("preset", &EncoderConfig::preset, "编码预设")
        .def_readwrite("codec", &EncoderConfig::codec, "编码器名称")
        .def("__repr__", [](const EncoderConfig& config) {
            return "<EncoderConfig " + std::to_string(config.width) + "x" +
                   std::to_string(config.height) + "@" + std::to_string(config.fps) +
                   "fps codec=" + config.codec + " preset=" + config.preset + ">";
        });

    // 绑定默认配置函数
    m.def("default_encoder_config", &defaultEncoderConfig, py::arg("width") = 1920,
          py::arg("height") = 1080, "创建默认编码器配置");

    // 绑定 ScreenRecorder 类
    py::class_<ScreenRecorder>(m, "ScreenRecorder", "屏幕录制器主类")
        .def(py::init<>(), "构造函数")

        // 录制控制方法
        .def(
            "start_recording",
            [](ScreenRecorder& recorder, const std::string& path,
               std::optional<RecorderMode> mode) {
                if (mode.has_value()) {
                    recorder.setRecorderMode(mode.value());
                }
                std::string p = path;
                return recorder.startRecording(p);
            },
            py::arg("path"), py::arg("mode") = py::none(), py::call_guard<py::gil_scoped_release>(),
            "开始录制\n\n"
            "参数:\n"
            "    path (str): 输出文件路径\n"
            "    mode (RecorderMode, optional): 录制模式 (VIDEO/SNAPSHOT)\n\n"
            "返回:\n"
            "    bool: 成功返回 True,失败返回 False")

        .def("stop_recording", &ScreenRecorder::stopRecording,
             py::call_guard<py::gil_scoped_release>(), "停止录制")

        .def("pause_recording", &ScreenRecorder::pauseRecording,
             py::call_guard<py::gil_scoped_release>(), "暂停录制")

        .def("resume_recording", &ScreenRecorder::resumeRecording,
             py::call_guard<py::gil_scoped_release>(), "恢复录制")

        // 状态查询方法
        .def("get_frame_count", &ScreenRecorder::getFrameCount, "获取已捕获的帧数")

        .def("get_encoded_count", &ScreenRecorder::getEncodedCount, "获取已编码的帧数")

        .def("get_dropped_count", &ScreenRecorder::getDroppedCount, "获取丢弃的帧数")

        .def("get_output_file_size", &ScreenRecorder::getOutputFileSize, "获取输出文件大小(字节)")

        .def("get_current_fps", &ScreenRecorder::getCurrentFps, "获取当前帧率")

        .def("is_recording", &ScreenRecorder::is_Recording, "检查是否正在录制")

        // ====================================================================
        // 回调设置方法 - 带 GIL 管理
        // ====================================================================
        // 【重要说明】
        // 这些回调函数会在 C++ 编码线程中被调用,因此必须使用 GIL 包装器
        // 来确保线程安全。不使用包装器会导致 Python 解释器崩溃。

        .def(
            "set_progress_callback",
            [](ScreenRecorder& recorder, py::function callback) {
                // 使用 GIL 包装器包装 Python 回调
                // 这确保了当编码线程调用回调时,会先获取 GIL
                auto wrapped = wrapCallbackWithGIL<ScreenRecorder::ProgressCallback>(
                    [callback](int64_t frames, int64_t size) { callback(frames, size); });
                recorder.setProgressCallback(wrapped);
            },
            py::arg("callback"),
            "设置进度回调函数\n\n"
            "【线程安全】回调会在 C++ 编码线程中调用,已自动处理 GIL\n\n"
            "回调函数签名: callback(frames: int, size: int)\n"
            "    frames: 已编码的帧数\n"
            "    size: 输出文件大小(字节)\n\n"
            "示例:\n"
            "    def on_progress(frames, size):\n"
            "        print(f'已编码 {frames} 帧, 文件大小 {size} 字节')\n"
            "    recorder.set_progress_callback(on_progress)")

        .def(
            "set_error_callback",
            [](ScreenRecorder& recorder, py::function callback) {
                // 使用 GIL 包装器包装 Python 回调
                auto wrapped = wrapCallbackWithGIL<ScreenRecorder::ErrorCallback>(
                    [callback](const std::string& errorMessage) { callback(errorMessage); });
                recorder.setErrorCallback(wrapped);
            },
            py::arg("callback"),
            "设置错误回调函数\n\n"
            "【线程安全】回调会在 C++ 编码线程中调用,已自动处理 GIL\n\n"
            "回调函数签名: callback(error_message: str)\n"
            "    error_message: 错误信息\n\n"
            "示例:\n"
            "    def on_error(error_msg):\n"
            "        print(f'录制错误: {error_msg}')\n"
            "    recorder.set_error_callback(on_error)")

        // Python 属性风格的访问器
        .def_property_readonly("frame_count", &ScreenRecorder::getFrameCount,
                               "已捕获的帧数(只读属性)")
        .def_property_readonly("encoded_count", &ScreenRecorder::getEncodedCount,
                               "已编码的帧数(只读属性)")
        .def_property_readonly("dropped_count", &ScreenRecorder::getDroppedCount,
                               "丢弃的帧数(只读属性)")
        .def_property_readonly("output_file_size", &ScreenRecorder::getOutputFileSize,
                               "输出文件大小(只读属性)")
        .def_property_readonly("current_fps", &ScreenRecorder::getCurrentFps, "当前帧率(只读属性)")
        .def_property_readonly("is_recording", &ScreenRecorder::is_Recording,
                               "是否正在录制(只读属性)")

        .def_property("recorder_mode", &ScreenRecorder::getRecorderMode,
                      &ScreenRecorder::setRecorderMode, "录制模式 (VIDEO/SNAPSHOT)")

        // 字符串表示
        .def("__repr__",
             [](const ScreenRecorder& recorder) {
                 return "<ScreenRecorder recording=" +
                        std::string(recorder.is_Recording() ? "True" : "False") +
                        " frames=" + std::to_string(recorder.getFrameCount()) +
                        " encoded=" + std::to_string(recorder.getEncodedCount()) +
                        " fps=" + std::to_string(recorder.getCurrentFps()) + ">";
             })

        // ====================================================================
        // 上下文管理器支持 - 带 GIL 管理
        // ====================================================================
        .def("__enter__", [](ScreenRecorder& recorder) -> ScreenRecorder& { return recorder; })
        .def(
            "__exit__",
            [](ScreenRecorder& recorder, py::object exc_type, py::object exc_value,
               py::object traceback) {
                // 在退出上下文时停止录制
                // 这里释放 GIL,因为 stopRecording 可能需要等待线程结束
                if (recorder.is_Recording()) {
                    py::gil_scoped_release release;
                    recorder.stopRecording();
                }
                return false;  // 不抑制异常
            },
            "上下文管理器退出\n\n"
            "【GIL 管理】在停止录制时会释放 GIL,避免阻塞其他 Python 线程");

    // ========================================================================
    // 模块级文档
    // ========================================================================
    m.attr("__doc__") = "屏幕录制核心模块\n\n"
                        "提供高性能的屏幕录制功能,包括:\n"
                        "- 实时屏幕捕获\n"
                        "- H.264/H.265 视频编码\n"
                        "- 多线程处理架构\n"
                        "- 进度和错误回调\n"
                        "- 自动 GIL 管理,确保线程安全\n\n"
                        "【GIL 管理说明】\n"
                        "本模块在以下场景自动管理 GIL:\n"
                        "1. 录制控制方法(start/stop/pause/resume)会释放 GIL,避免阻塞\n"
                        "2. 回调函数会在获取 GIL 后调用,确保线程安全\n"
                        "3. 上下文管理器在清理时会释放 GIL\n\n"
                        "示例:\n"
                        "    # 基本使用\n"
                        "    recorder = ScreenRecorder()\n"
                        "    recorder.set_progress_callback(lambda f, s: print(f'Frames: {f}'))\n"
                        "    recorder.start_recording('output.mp4')\n"
                        "    # ... 录制中 ...\n"
                        "    recorder.stop_recording()\n\n"
                        "    # 使用上下文管理器(推荐)\n"
                        "    with ScreenRecorder() as recorder:\n"
                        "        recorder.set_error_callback(lambda e: print(f'Error: {e}'))\n"
                        "        recorder.start_recording('output.mp4')\n"
                        "        # 自动调用 stop_recording()";
}
