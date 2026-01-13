"""
测试 recorder_module Pybind11 绑定

运行方式:
    python -m pytest tests/python/test_recorder_module.py -v
    或
    python tests/python/test_recorder_module.py
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
    import recorder_module as rec
except ImportError:
    print("警告: 无法导入 recorder_module，请先编译项目")
    print("预期模块位置: build/python/recorder_module.pyd")
    rec = None


class TestRecorderConfig:
    """测试 RecorderConfig 类型绑定"""

    def test_default_config_creation(self):
        """测试默认配置创建"""
        if rec is None:
            return

        config = rec.default_recorder_config()
        assert config.video.width == 1920
        assert config.video.height == 1080
        assert config.video.fps == 30
        assert config.audio.enabled == True
        print("✓ 默认配置创建测试通过")

    def test_config_modification(self):
        """测试配置修改"""
        if rec is None:
            return

        config = rec.default_recorder_config()
        config.video.width = 1280
        config.video.height = 720
        assert config.video.width == 1280
        assert config.video.height == 720
        print("✓ 配置修改测试通过")

    def test_config_repr(self):
        """测试配置字符串表示"""
        if rec is None:
            return

        config = rec.default_recorder_config()
        repr_str = repr(config)
        assert "RecorderConfig" in repr_str
        assert "1920" in repr_str
        print("✓ 配置字符串表示测试通过")

    def test_video_config(self):
        """测试视频配置"""
        if rec is None:
            return

        video_config = rec.VideoEncoderConfig()
        video_config.width = 2560
        video_config.height = 1440
        video_config.fps = 60
        video_config.codec = "libx265"
        assert video_config.width == 2560
        assert video_config.codec == "libx265"
        print("✓ 视频配置测试通过")

    def test_audio_config(self):
        """测试音频配置"""
        if rec is None:
            return

        audio_config = rec.AudioEncoderConfig()
        audio_config.enabled = False
        audio_config.sample_rate = 44100
        audio_config.channels = 1
        assert audio_config.enabled == False
        assert audio_config.sample_rate == 44100
        print("✓ 音频配置测试通过")

    def test_zmq_config(self):
        """测试 ZMQ 配置"""
        if rec is None:
            return

        zmq_config = rec.ZMQConfig()
        zmq_config.endpoint = "tcp://*:5556"
        zmq_config.timeout_ms = 200
        assert zmq_config.endpoint == "tcp://*:5556"
        assert zmq_config.timeout_ms == 200
        print("✓ ZMQ 配置测试通过")


class TestRecordingStatus:
    """测试 RecordingStatus 枚举"""

    def test_enum_values(self):
        """测试枚举值"""
        if rec is None:
            return

        assert int(rec.RecordingStatus.IDLE) == 0
        assert int(rec.RecordingStatus.RECORDING) == 2
        print("✓ 枚举值测试通过")

    def test_enum_string_conversion(self):
        """测试枚举字符串转换"""
        if rec is None:
            return

        status = rec.RecordingStatus.RECORDING
        assert "RECORDING" in str(status)
        print("✓ 枚举字符串转换测试通过")


class TestRecordingStats:
    """测试 RecordingStats 结构体"""

    def test_stats_creation(self):
        """测试统计信息创建"""
        if rec is None:
            return

        stats = rec.RecordingStats()
        assert hasattr(stats, "frame_count")
        assert hasattr(stats, "encoded_count")
        assert hasattr(stats, "dropped_count")
        assert hasattr(stats, "current_fps")
        print("✓ 统计信息创建测试通过")

    def test_stats_repr(self):
        """测试统计信息字符串表示"""
        if rec is None:
            return

        stats = rec.RecordingStats()
        stats.frame_count = 100
        stats.current_fps = 30.0
        repr_str = repr(stats)
        assert "RecordingStats" in repr_str
        assert "100" in repr_str
        print("✓ 统计信息字符串表示测试通过")

    def test_stats_to_dict(self):
        """测试统计信息转换为字典"""
        if rec is None:
            return

        stats = rec.RecordingStats()
        stats.frame_count = 150
        stats.encoded_count = 145
        stats.current_fps = 29.97

        dict_repr = stats.to_dict()
        assert dict_repr["frame_count"] == 150
        assert dict_repr["encoded_count"] == 145
        print("✓ 统计信息转换为字典测试通过")


class TestRecorderAPI:
    """测试 RecorderAPI 类绑定"""

    def test_api_creation(self):
        """测试 API 对象创建"""
        if rec is None:
            return

        api = rec.RecorderAPI()
        assert api.status == rec.RecordingStatus.IDLE
        print("✓ API 对象创建测试通过")

    def test_api_properties(self):
        """测试 API 属性"""
        if rec is None:
            return

        api = rec.RecorderAPI()
        assert hasattr(api, "status")
        assert hasattr(api, "stats")
        assert hasattr(api, "last_error")
        assert hasattr(api, "is_recording")
        assert hasattr(api, "frame_count")
        print("✓ API 属性测试通过")

    def test_api_repr(self):
        """测试 API 字符串表示"""
        if rec is None:
            return

        api = rec.RecorderAPI()
        repr_str = repr(api)
        assert "RecorderAPI" in repr_str
        str_repr = str(api)
        assert "RecorderAPI" in str_repr
        print("✓ API 字符串表示测试通过")

    def test_api_context_manager(self):
        """测试上下文管理器"""
        if rec is None:
            return

        with rec.RecorderAPI() as api:
            assert api is not None
            assert api.status == rec.RecordingStatus.IDLE
        print("✓ 上下文管理器测试通过")

    def test_api_callbacks(self):
        """测试回调设置"""
        if rec is None:
            return

        api = rec.RecorderAPI()

        status_changes = []
        errors = []

        def on_status_change(status):
            status_changes.append(status)

        def on_error(error_msg):
            errors.append(error_msg)

        api.set_status_callback(on_status_change)
        api.set_error_callback(on_error)
        print("✓ 回调设置测试通过")


def run_all_tests():
    """运行所有测试"""
    print("=" * 60)
    print("开始测试 recorder_module")
    print("=" * 60)

    if rec is None:
        print("模块未加载，跳过测试")
        return

    # 配置测试
    print("\n--- RecorderConfig 测试 ---")
    test_config = TestRecorderConfig()
    test_config.test_default_config_creation()
    test_config.test_config_modification()
    test_config.test_config_repr()
    test_config.test_video_config()
    test_config.test_audio_config()
    test_config.test_zmq_config()

    # 枚举测试
    print("\n--- RecordingStatus 测试 ---")
    test_status = TestRecordingStatus()
    test_status.test_enum_values()
    test_status.test_enum_string_conversion()

    # 统计信息测试
    print("\n--- RecordingStats 测试 ---")
    test_stats = TestRecordingStats()
    test_stats.test_stats_creation()
    test_stats.test_stats_repr()
    test_stats.test_stats_to_dict()

    # API 测试
    print("\n--- RecorderAPI 测试 ---")
    test_api = TestRecorderAPI()
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
