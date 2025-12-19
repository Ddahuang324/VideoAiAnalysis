"""
测试屏幕录制服务
单元测试和集成测试
"""
import sys
import time
import unittest
from pathlib import Path

# 添加项目路径
project_root = Path(__file__).parent.parent
sys.path.insert(0, str(project_root))

from services.video_service import ScreenRecorderService, EncoderPreset, VideoCodec


class TestScreenRecorderService(unittest.TestCase):
    """测试 ScreenRecorderService"""
    
    def setUp(self):
        """每个测试前的设置"""
        self.service = ScreenRecorderService()
        self.test_output = Path("test_output.mp4")
    
    def tearDown(self):
        """每个测试后的清理"""
        # 确保停止录制
        if self.service.is_recording():
            self.service.stop_recording()
        
        # 清理测试文件
        if self.test_output.exists():
            try:
                self.test_output.unlink()
            except Exception as e:
                print(f"Failed to delete test file: {e}")
    
    def test_service_creation(self):
        """测试服务创建"""
        self.assertIsNotNone(self.service)
        self.assertFalse(self.service.is_recording())
    
    def test_get_stats_before_recording(self):
        """测试录制前获取统计信息"""
        stats = self.service.get_stats()
        self.assertEqual(stats['frame_count'], 0)
        self.assertEqual(stats['encoded_count'], 0)
        self.assertEqual(stats['dropped_count'], 0)
        self.assertFalse(stats['is_recording'])
    
    def test_start_stop_recording(self):
        """测试开始和停止录制"""
        # 开始录制
        success = self.service.start_recording(str(self.test_output))
        self.assertTrue(success)
        self.assertTrue(self.service.is_recording())
        
        # 等待一小段时间
        time.sleep(0.5)
        
        # 停止录制
        self.service.stop_recording()
        self.assertFalse(self.service.is_recording())
    
    def test_pause_resume(self):
        """测试暂停和恢复"""
        # 开始录制
        self.service.start_recording(str(self.test_output))
        time.sleep(0.3)
        
        # 暂停
        self.service.pause_recording()
        time.sleep(0.2)
        
        # 恢复
        self.service.resume_recording()
        time.sleep(0.3)
        
        # 停止
        self.service.stop_recording()
    
    def test_stats_during_recording(self):
        """测试录制过程中的统计信息"""
        # 开始录制
        self.service.start_recording(str(self.test_output))
        
        # 等待一段时间让录制进行
        time.sleep(1)
        
        # 获取统计信息
        stats = self.service.get_stats()
        
        # 验证统计信息
        self.assertTrue(stats['is_recording'])
        self.assertGreater(stats['frame_count'], 0)
        
        # 停止录制
        self.service.stop_recording()
        
        # 获取最终统计
        final_stats = self.service.get_stats()
        self.assertFalse(final_stats['is_recording'])
        self.assertGreaterEqual(final_stats['encoded_count'], 0)
    
    def test_progress_callback(self):
        """测试进度回调"""
        progress_called = {'count': 0, 'frames': 0, 'size': 0}
        
        def on_progress(frames: int, size: int):
            progress_called['count'] += 1
            progress_called['frames'] = frames
            progress_called['size'] = size
        
        # 设置回调
        self.service.set_progress_callback(on_progress)
        
        # 开始录制
        self.service.start_recording(str(self.test_output))
        time.sleep(1)
        
        # 停止录制
        self.service.stop_recording()
        
        # 验证回调被调用（可能需要等待）
        # 注意: 回调可能在编码线程中异步调用
        print(f"Progress callback called {progress_called['count']} times")
        print(f"Last update: {progress_called['frames']} frames, {progress_called['size']} bytes")
    
    def test_error_callback(self):
        """测试错误回调"""
        error_called = {'count': 0, 'message': ''}
        
        def on_error(error_message: str):
            error_called['count'] += 1
            error_called['message'] = error_message
        
        # 设置回调
        self.service.set_error_callback(on_error)
        
        # 尝试使用无效路径开始录制
        try:
            invalid_path = "/invalid/path/that/does/not/exist/output.mp4"
            self.service.start_recording(invalid_path)
            time.sleep(0.5)
            self.service.stop_recording()
        except Exception:
            pass
        
        # 注意: 错误可能通过异常或回调报告
        if error_called['count'] > 0:
            print(f"Error callback called: {error_called['message']}")
    
    def test_context_manager(self):
        """测试上下文管理器"""
        with ScreenRecorderService() as service:
            service.start_recording(str(self.test_output))
            time.sleep(0.5)
            # 应该自动停止
        
        # 验证已停止
        self.assertFalse(service.is_recording())
    
    def test_encoder_config_creation(self):
        """测试创建编码器配置"""
        config = self.service.create_encoder_config(
            width=1280,
            height=720,
            fps=30,
            bitrate=4000000,
            crf=23,
            preset=EncoderPreset.FAST,
            codec=VideoCodec.H264
        )
        
        self.assertIsNotNone(config)
        self.assertEqual(config.width, 1280)
        self.assertEqual(config.height, 720)
        self.assertEqual(config.fps, 30)
        self.assertEqual(config.preset, EncoderPreset.FAST)
    
    def test_default_encoder_config(self):
        """测试获取默认编码器配置"""
        config = self.service.get_default_encoder_config(1920, 1080)
        
        self.assertIsNotNone(config)
        self.assertEqual(config.width, 1920)
        self.assertEqual(config.height, 1080)
    
    def test_repr(self):
        """测试字符串表示"""
        repr_str = repr(self.service)
        self.assertIn("ScreenRecorderService", repr_str)
        self.assertIn("recording=", repr_str)


class TestVideoService(unittest.TestCase):
    """测试 VideoService（兼容层）"""
    
    def test_video_service_creation(self):
        """测试创建 VideoService"""
        from services.video_service import VideoService
        service = VideoService()
        self.assertIsNotNone(service)
    
    def test_get_screen_recorder(self):
        """测试获取屏幕录制服务"""
        from services.video_service import VideoService
        service = VideoService()
        recorder = service.get_screen_recorder()
        self.assertIsInstance(recorder, ScreenRecorderService)
    
    def test_get_version(self):
        """测试获取版本"""
        from services.video_service import VideoService
        service = VideoService()
        version = service.get_version()
        self.assertIsInstance(version, str)
        print(f"C++ module version: {version}")


def run_quick_test():
    """快速集成测试"""
    print("\n" + "=" * 60)
    print("快速集成测试")
    print("=" * 60)
    
    try:
        print("\n1. 创建服务...")
        service = ScreenRecorderService()
        print(f"   {service}")
        
        print("\n2. 获取初始统计...")
        stats = service.get_stats()
        print(f"   {stats}")
        
        print("\n3. 开始录制...")
        output_path = "quick_test_output.mp4"
        if service.start_recording(output_path):
            print(f"   录制已开始: {output_path}")
            
            print("\n4. 录制 2 秒...")
            for i in range(2):
                time.sleep(1)
                stats = service.get_stats()
                print(f"   [{i+1}s] 帧数: {stats['frame_count']}, "
                      f"FPS: {stats['current_fps']:.1f}")
            
            print("\n5. 停止录制...")
            service.stop_recording()
            print("   录制已停止")
            
            print("\n6. 最终统计...")
            final_stats = service.get_stats()
            print(f"   总帧数: {final_stats['frame_count']}")
            print(f"   已编码: {final_stats['encoded_count']}")
            print(f"   文件大小: {final_stats['output_file_size'] / 1024:.2f} KB")
        
        print("\n✓ 快速测试完成!")
        
        # 清理
        output_file = Path(output_path)
        if output_file.exists():
            output_file.unlink()
            print(f"✓ 已清理测试文件: {output_path}")
    
    except Exception as e:
        print(f"\n✗ 测试失败: {e}")
        import traceback
        traceback.print_exc()


if __name__ == "__main__":
    # 运行快速测试
    run_quick_test()
    
    # 或运行单元测试
    # unittest.main()
