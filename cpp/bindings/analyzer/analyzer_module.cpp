/**
 * @file analyzer_module.cpp
 * @brief analyzer_module Python 模块入口点
 */

#include <pybind11/pybind11.h>

namespace py = pybind11;

// 前向声明
namespace Analyzer::Bindings {
void bind_analyzer_types(py::module& m);
void bind_analyzer_config(py::module& m);
void bind_analyzer_api(py::module& m);
}  // namespace Analyzer::Bindings

PYBIND11_MODULE(analyzer_module, m) {
    using namespace Analyzer::Bindings;

    m.doc() = "分析进程 Python 绑定模块\n\n"
              "提供 AnalyzerAPI 的完整 Python 接口，用于控制 AI 视频分析进程。\n\n"
              "主要类:\n"
              "    - AnalyzerAPI: 分析器主类\n"
              "    - AnalyzerConfig: 分析器配置\n"
              "    - AnalysisStatus: 状态枚举\n"
              "    - AnalysisStats: 统计信息\n\n"
              "示例:\n"
              "    import analyzer_module as ana\n\n"
              "    # 创建配置\n"
              "    config = ana.default_analyzer_config()\n"
              "    config.enable_text_recognition = True\n\n"
              "    # 使用分析器\n"
              "    with ana.AnalyzerAPI() as api:\n"
              "        api.initialize(config)\n"
              "        api.set_keyframe_callback(lambda idx: print(f'Keyframe: {idx}'))\n"
              "        api.start()\n"
              "        # ... 分析中 ...\n"
              "        api.stop()\n\n"
              "    # 使用回调\n"
              "    def on_status_change(status):\n"
              "        print(f'状态变更: {status}')\n\n"
              "    api = ana.AnalyzerAPI()\n"
              "    api.set_status_callback(on_status_change)";

    // 绑定顺序：先绑定类型，再绑定配置，最后绑定 API
    bind_analyzer_types(m);    // AnalysisStatus, KeyFrameRecord, AnalysisStats
    bind_analyzer_config(m);   // AnalyzerConfig 及相关配置
    bind_analyzer_api(m);      // AnalyzerAPI

    // 模块版本信息
    m.attr("__version__") = "1.0.0";
    m.attr("__author__") = "VideoAiAnalysis Team";

    // 导出便捷函数
    m.attr("create_config") = m.attr("default_analyzer_config");
}
