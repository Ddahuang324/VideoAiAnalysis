/**
 * @file recorder_module.cpp
 * @brief recorder_module Python 模块入口点
 */

#include <pybind11/pybind11.h>

namespace py = pybind11;

// 前向声明
namespace Recorder::Bindings {
void bind_recorder_types(py::module& m);
void bind_recorder_config(py::module& m);
void bind_recorder_api(py::module& m);
}  // namespace Recorder::Bindings

PYBIND11_MODULE(recorder_module, m) {
    using namespace Recorder::Bindings;

    m.doc() = "录制进程 Python 绑定模块\n\n"
              "提供 RecorderAPI 的完整 Python 接口，用于控制视频录制进程。\n\n"
              "主要类:\n"
              "    - RecorderAPI: 录制器主类\n"
              "    - RecorderConfig: 录制配置\n"
              "    - RecordingStatus: 状态枚举\n"
              "    - RecordingStats: 统计信息\n\n"
              "示例:\n"
              "    import recorder_module as rec\n\n"
              "    # 创建配置\n"
              "    config = rec.default_recorder_config()\n"
              "    config.video.output_file_path = 'output.mp4'\n\n"
              "    # 使用录制器\n"
              "    with rec.RecorderAPI() as api:\n"
              "        api.initialize(config)\n"
              "        api.start()\n"
              "        # ... 录制中 ...\n"
              "        api.stop()\n\n"
              "    # 使用回调\n"
              "    def on_status_change(status):\n"
              "        print(f'状态变更: {status}')\n\n"
              "    api = rec.RecorderAPI()\n"
              "    api.set_status_callback(on_status_change)";

    // 绑定顺序很重要：先绑定类型，再绑定配置，最后绑定 API
    bind_recorder_types(m);    // RecordingStatus, RecordingStats
    bind_recorder_config(m);   // RecorderConfig 及相关配置
    bind_recorder_api(m);      // RecorderAPI

    // 模块版本信息
    m.attr("__version__") = "1.0.0";
    m.attr("__author__") = "VideoAiAnalysis Team";

    // 导出便捷函数
    m.attr("create_config") = m.attr("default_recorder_config");
}
