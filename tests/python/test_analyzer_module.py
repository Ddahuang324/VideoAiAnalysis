"""
测试 analyzer_module Pybind11 绑定

运行方式:
    python -m pytest tests/python/test_analyzer_module.py -v
    或
    python tests/python/test_analyzer_module.py
"""

import sys
import os

# 添加构建输出目录到路径
build_python_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), "../../build/python"))
build_bin_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), "../../build/bin"))

if os.path.exists(build_python_dir):
    sys.path.insert(0, build_python_dir)
    if hasattr(os, 'add_dll_directory'):
        os.add_dll_directory(build_python_dir)
        if os.path.exists(build_bin_dir):
            os.add_dll_directory(build_bin_dir)
else:
    print(f"警告: 无法找到构建输出目录 {build_python_dir}")

try:
    import analyzer_module as ana
except ImportError:
    print("警告: 无法导入 analyzer_module，请先编译项目")
    print("预期模块位置: build/python/analyzer_module.pyd")
    ana = None


def is_approx(a, b, rel_tol=1e-6):
    """辅助函数，用于浮点数近似相等比较"""
    import math
    return math.isclose(a, b, rel_tol=rel_tol)


class TestAnalyzerConfig:
    """测试 AnalyzerConfig 类型绑定"""

    def test_default_config_creation(self):
        """测试默认配置创建"""
        if ana is None:
            return

        config = ana.default_analyzer_config()
        assert config.zmq_subscriber.endpoint == "tcp://localhost:5555"
        assert config.pipeline.analysis_thread_count == 4
        assert config.enable_text_recognition == False
        print("✓ 默认配置创建测试通过")

    def test_config_modification(self):
        """测试配置修改"""
        if ana is None:
            return

        config = ana.default_analyzer_config()
        config.enable_text_recognition = True
        config.pipeline.analysis_thread_count = 8
        assert config.enable_text_recognition == True
        assert config.pipeline.analysis_thread_count == 8
        print("✓ 配置修改测试通过")

    def test_config_repr(self):
        """测试配置字符串表示"""
        if ana is None:
            return

        config = ana.default_analyzer_config()
        repr_str = repr(config)
        assert "AnalyzerConfig" in repr_str
        print("✓ 配置字符串表示测试通过")

    def test_zmq_config(self):
        """测试 ZMQ 配置"""
        if ana is None:
            return

        zmq_config = ana.ZMQConfig()
        zmq_config.endpoint = "tcp://*:5566"
        zmq_config.timeout_ms = 500
        assert zmq_config.endpoint == "tcp://*:5566"
        assert zmq_config.timeout_ms == 500
        print("✓ ZMQ 配置测试通过")

    def test_model_paths_config(self):
        """测试模型路径配置"""
        if ana is None:
            return

        model_config = ana.ModelPathsConfig()
        model_config.base_path = "/path/to/models"
        model_config.motion_model_path = "yolov8n.onnx"
        assert model_config.base_path == "/path/to/models"
        assert model_config.motion_model_path == "yolov8n.onnx"
        print("✓ 模型路径配置测试通过")

    def test_motion_detector_config(self):
        """测试运动检测器配置"""
        if ana is None:
            return

        motion_config = ana.MotionDetectorConfig()
        motion_config.confidence_threshold = 0.5
        motion_config.nms_threshold = 0.5
        assert is_approx(motion_config.confidence_threshold, 0.5)
        print("✓ 运动检测器配置测试通过")

    def test_scene_detector_config(self):
        """测试场景检测器配置"""
        if ana is None:
            return

        scene_config = ana.SceneChangeDetectorConfig()
        scene_config.similarity_threshold = 0.85
        assert is_approx(scene_config.similarity_threshold, 0.85)
        print("✓ 场景检测器配置测试通过")

    def test_text_detector_config(self):
        """测试文本检测器配置"""
        if ana is None:
            return

        text_config = ana.TextDetectorConfig()
        text_config.enable_recognition = True
        assert text_config.enable_recognition == True
        print("✓ 文本检测器配置测试通过")


class TestAnalysisStatus:
    """测试 AnalysisStatus 枚举"""

    def test_enum_values(self):
        """测试枚举值"""
        if ana is None:
            return

        assert int(ana.AnalysisStatus.IDLE) == 0
        assert int(ana.AnalysisStatus.RUNNING) == 2
        print("✓ 枚举值测试通过")

    def test_enum_string_conversion(self):
        """测试枚举字符串转换"""
        if ana is None:
            return

        status = ana.AnalysisStatus.RUNNING
        assert "RUNNING" in str(status)
        print("✓ 枚举字符串转换测试通过")


class TestKeyFrameRecord:
    """测试 KeyFrameRecord 类型"""

    def test_record_creation(self):
        """测试记录创建"""
        if ana is None:
            return

        record = ana.KeyFrameRecord()
        assert hasattr(record, "frame_index")
        assert hasattr(record, "score")
        assert hasattr(record, "timestamp")
        print("✓ 记录创建测试通过")

    def test_record_values(self):
        """测试记录值设置"""
        if ana is None:
            return

        record = ana.KeyFrameRecord()
        record.frame_index = 100
        record.score = 0.85
        record.timestamp = 10.5
        assert record.frame_index == 100
        assert is_approx(record.score, 0.85)
        assert is_approx(record.timestamp, 10.5)
        print("✓ 记录值设置测试通过")

    def test_record_to_dict(self):
        """测试记录转换为字典"""
        if ana is None:
            return

        record = ana.KeyFrameRecord()
        record.frame_index = 200
        record.score = 0.92
        record.timestamp = 20.0

        dict_repr = record.to_dict()
        assert dict_repr["frame_index"] == 200
        assert is_approx(dict_repr["score"], 0.92)
        print("✓ 记录转换为字典测试通过")


class TestAnalysisStats:
    """测试 AnalysisStats 类型"""

    def test_stats_creation(self):
        """测试统计信息创建"""
        if ana is None:
            return

        stats = ana.AnalysisStats()
        assert hasattr(stats, "received_frame_count")
        assert hasattr(stats, "analyzed_frame_count")
        assert hasattr(stats, "keyframe_count")
        assert hasattr(stats, "latest_keyframes")
        assert hasattr(stats, "active_config")
        assert hasattr(stats, "avg_processing_time")
        print("✓ 统计信息创建测试通过")

    def test_stats_values(self):
        """测试统计信息值设置"""
        if ana is None:
            return

        stats = ana.AnalysisStats()
        stats.received_frame_count = 1000
        stats.analyzed_frame_count = 950
        stats.keyframe_count = 50
        assert stats.received_frame_count == 1000
        assert stats.analyzed_frame_count == 950
        assert stats.keyframe_count == 50
        print("✓ 统计信息值设置测试通过")

    def test_stats_repr(self):
        """测试统计信息字符串表示"""
        if ana is None:
            return

        stats = ana.AnalysisStats()
        stats.received_frame_count = 500
        stats.keyframe_count = 25
        repr_str = repr(stats)
        assert "AnalysisStats" in repr_str
        print("✓ 统计信息字符串表示测试通过")

    def test_stats_to_dict(self):
        """测试统计信息转换为字典"""
        if ana is None:
            return

        stats = ana.AnalysisStats()
        stats.received_frame_count = 300
        stats.analyzed_frame_count = 280
        stats.keyframe_count = 15

        dict_repr = stats.to_dict()
        assert dict_repr["received_frame_count"] == 300
        assert dict_repr["analyzed_frame_count"] == 280
        assert dict_repr["keyframe_count"] == 15
        print("✓ 统计信息转换为字典测试通过")


class TestAnalyzerAPI:
    """测试 AnalyzerAPI 类绑定"""

    def test_api_creation(self):
        """测试 API 对象创建"""
        if ana is None:
            return

        api = ana.AnalyzerAPI()
        assert api.status == ana.AnalysisStatus.IDLE
        print("✓ API 对象创建测试通过")

    def test_api_properties(self):
        """测试 API 属性"""
        if ana is None:
            return

        api = ana.AnalyzerAPI()
        assert hasattr(api, "status")
        assert hasattr(api, "stats")
        assert hasattr(api, "last_error")
        assert hasattr(api, "is_running")
        assert hasattr(api, "received_frame_count")
        assert hasattr(api, "analyzed_frame_count")
        assert hasattr(api, "keyframe_count")
        print("✓ API 属性测试通过")

    def test_api_repr(self):
        """测试 API 字符串表示"""
        if ana is None:
            return

        api = ana.AnalyzerAPI()
        repr_str = repr(api)
        assert "AnalyzerAPI" in repr_str
        str_repr = str(api)
        assert "AnalyzerAPI" in str_repr
        print("✓ API 字符串表示测试通过")

    def test_api_context_manager(self):
        """测试上下文管理器"""
        if ana is None:
            return

        with ana.AnalyzerAPI() as api:
            assert api is not None
            assert api.status == ana.AnalysisStatus.IDLE
        print("✓ 上下文管理器测试通过")

    def test_api_callbacks(self):
        """测试回调设置"""
        if ana is None:
            return

        api = ana.AnalyzerAPI()

        status_changes = []
        keyframes = []

        def on_status_change(status):
            status_changes.append(status)

        def on_keyframe(frame_idx):
            keyframes.append(frame_idx)

        api.set_status_callback(on_status_change)
        api.set_keyframe_callback(on_keyframe)
        print("✓ 回调设置测试通过")


def run_all_tests():
    """运行所有测试"""
    print("=" * 60)
    print("开始测试 analyzer_module")
    print("=" * 60)

    if ana is None:
        print("模块未加载，跳过测试")
        return

    # 配置测试
    print("\n--- AnalyzerConfig 测试 ---")
    test_config = TestAnalyzerConfig()
    test_config.test_default_config_creation()
    test_config.test_config_modification()
    test_config.test_config_repr()
    test_config.test_zmq_config()
    test_config.test_model_paths_config()
    test_config.test_motion_detector_config()
    test_config.test_scene_detector_config()
    test_config.test_text_detector_config()

    # 枚举测试
    print("\n--- AnalysisStatus 测试 ---")
    test_status = TestAnalysisStatus()
    test_status.test_enum_values()
    test_status.test_enum_string_conversion()

    # 关键帧记录测试
    print("\n--- KeyFrameRecord 测试 ---")
    test_record = TestKeyFrameRecord()
    test_record.test_record_creation()
    test_record.test_record_values()
    test_record.test_record_to_dict()

    # 统计信息测试
    print("\n--- AnalysisStats 测试 ---")
    test_stats = TestAnalysisStats()
    test_stats.test_stats_creation()
    test_stats.test_stats_values()
    test_stats.test_stats_repr()
    test_stats.test_stats_to_dict()

    # API 测试
    print("\n--- AnalyzerAPI 测试 ---")
    test_api = TestAnalyzerAPI()
    test_api.test_api_creation()
    test_api.test_api_properties()
    test_api.test_api_repr()
    test_api.test_api_context_manager()
    test_api.test_api_callbacks()

    print("\n" + "=" * 60)
    print("所有测试完成!")
    print("=" * 60)


if __name__ == "__main__":
    run_all_tests()
