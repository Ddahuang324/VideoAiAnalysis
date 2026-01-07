"""
视频录制服务
封装 C++ ScreenRecorder 模块的调用
"""
import sys
from pathlib import Path
from typing import Callable, Optional, Dict, Any
from enum import IntEnum

# 添加 C++ 编译输出目录到 Python 路径
build_path = Path(__file__).parent.parent.parent / "build" / "python"
sys.path.insert(0, str(build_path))

try:
    import Video_Recording_Moudle as vac
    print(f"[VideoService] Successfully imported C++ module from: {build_path}")
except ImportError as e:
    print(f"[VideoService] Failed to import C++ module: {e}")
    print(f"[VideoService] Please run: cmake --build build")
    vac = None


class PixelFormat(IntEnum):
    """像素格式枚举（与 C++ 端保持一致）"""
    UNKNOWN = 0
    BGRA = 1
    RGBA = 2
    RGB24 = 3


class RecorderMode(IntEnum):
    VIDEO = 0     # 视频模式 (高帧率, 如 30fps)
    SNAPSHOT = 1  # 快照模式 (低帧率, 如 1fps)


class EncoderPreset:
    """编码器预设"""
    ULTRAFAST = "ultrafast"
    SUPERFAST = "superfast"
    VERYFAST = "veryfast"
    FASTER = "faster"
    FAST = "fast"
    MEDIUM = "medium"
    SLOW = "slow"
    SLOWER = "slower"
    VERYSLOW = "veryslow"


class VideoCodec:
    """视频编码器"""
    H264 = "libx264"
    H265 = "libx265"
    VP9 = "libvpx-vp9"


class ScreenRecorderService:
    """屏幕录制服务 - 封装 C++ ScreenRecorder 调用"""
    
    def __init__(self):
        """初始化服务"""
        if vac is None:
            raise RuntimeError("C++ module not available. Please build the project first.")
        
        self._recorder = None
        self._is_recording = False
        self._output_path = None
        
        # 回调函数存储（防止被 Python GC 回收）
        self._progress_callback = None
        self._error_callback = None
        
        print("[ScreenRecorderService] Created")
    
    def _ensure_recorder(self):
        """确保录制器已创建"""
        if self._recorder is None:
            if vac is None:
                raise RuntimeError("C++ module not available. Please build the project first.")
            self._recorder = vac.ScreenRecorder()
            print("[ScreenRecorderService] ScreenRecorder instance created")
    
    def start_recording(
        self, 
        output_path: str,
        mode: Optional[RecorderMode] = None
    ) -> bool:
        """
        开始录制
        
        Args:
            output_path: 输出文件路径（如 'output.mp4'）
            mode: 录制模式 (RecorderMode.VIDEO 或 RecorderMode.SNAPSHOT)
                  默认为 None,使用当前设置的模式
            
        Returns:
            bool: 成功返回 True
        """
        self._ensure_recorder()
        
        if self._recorder is None or vac is None:
            return False
        
        try:
            # 如果指定了模式,传递给 C++ 层
            if mode is not None:
                # 将 Python 枚举转换为 C++ 枚举
                cpp_mode = vac.RecorderMode.VIDEO if mode == RecorderMode.VIDEO else vac.RecorderMode.SNAPSHOT
                success = self._recorder.start_recording(output_path, cpp_mode)
            else:
                success = self._recorder.start_recording(output_path)
            
            if success:
                self._is_recording = True
                self._output_path = output_path
                mode_str = "VIDEO" if mode == RecorderMode.VIDEO else "SNAPSHOT" if mode == RecorderMode.SNAPSHOT else "DEFAULT"
                print(f"[ScreenRecorderService] Recording started: {output_path} (Mode: {mode_str})")
            else:
                print(f"[ScreenRecorderService] Failed to start recording")
            return success
        except Exception as e:
            print(f"[ScreenRecorderService] Error starting recording: {e}")
            raise
    
    def stop_recording(self):
        """停止录制"""
        if not self._recorder:
            print("[ScreenRecorderService] No active recorder")
            return
        
        try:
            self._recorder.stop_recording()
            self._is_recording = False
            print(f"[ScreenRecorderService] Recording stopped")
        except Exception as e:
            print(f"[ScreenRecorderService] Error stopping recording: {e}")
            raise
    
    def pause_recording(self):
        """暂停录制"""
        if not self._recorder:
            print("[ScreenRecorderService] No active recorder")
            return
        
        try:
            self._recorder.pause_recording()
            print(f"[ScreenRecorderService] Recording paused")
        except Exception as e:
            print(f"[ScreenRecorderService] Error pausing recording: {e}")
            raise
    
    def resume_recording(self):
        """恢复录制"""
        if not self._recorder:
            print("[ScreenRecorderService] No active recorder")
            return
        
        try:
            self._recorder.resume_recording()
            print(f"[ScreenRecorderService] Recording resumed")
        except Exception as e:
            print(f"[ScreenRecorderService] Error resuming recording: {e}")
            raise

    def start_publishing(self) -> bool:
        """开始发布帧数据到 ZMQ"""
        if not self._recorder:
            return False
        try:
            return self._recorder.start_publishing()
        except Exception as e:
            print(f"[ScreenRecorderService] Error starting publishing: {e}")
            return False

    def stop_publishing(self):
        """停止发布帧数据"""
        if not self._recorder:
            return
        try:
            self._recorder.stop_publishing()
        except Exception as e:
            print(f"[ScreenRecorderService] Error stopping publishing: {e}")

    def start_key_frame_receiving(self, key_frame_path: str) -> bool:
        """开始接收关键帧并启动编码"""
        if not self._recorder:
            return False
        try:
            # key_frame_path 需要是完整的文件路径
            return self._recorder.start_key_frame_receiving(key_frame_path)
        except Exception as e:
            print(f"[ScreenRecorderService] Error starting key frame receiving: {e}")
            return False

    def stop_key_frame_receiving(self):
        """停止关键帧接收"""
        if not self._recorder:
            return
        try:
            self._recorder.stop_key_frame_receiving()
        except Exception as e:
            print(f"[ScreenRecorderService] Error stopping key frame receiving: {e}")

    def is_recording(self) -> bool:
        """检查是否正在录制"""
        if not self._recorder:
            return False
        return self._recorder.is_recording
    
    def set_recorder_mode(self, mode: RecorderMode):
        """
        设置录制模式
        
        Args:
            mode: 录制模式 (RecorderMode.VIDEO 或 RecorderMode.SNAPSHOT)
        
        Note:
            必须在开始录制前设置,录制过程中无法切换模式
        """
        self._ensure_recorder()
        
        if self._recorder is None or vac is None:
            raise RuntimeError("Recorder not initialized or C++ module not available")
        
        if self.is_recording():
            raise RuntimeError("Cannot change mode while recording")
        
        try:
            # 将 Python 枚举转换为 C++ 枚举
            cpp_mode = vac.RecorderMode.VIDEO if mode == RecorderMode.VIDEO else vac.RecorderMode.SNAPSHOT
            self._recorder.recorder_mode = cpp_mode
            mode_str = "VIDEO" if mode == RecorderMode.VIDEO else "SNAPSHOT"
            print(f"[ScreenRecorderService] Recorder mode set to: {mode_str}")
        except Exception as e:
            print(f"[ScreenRecorderService] Error setting recorder mode: {e}")
            raise
    
    def get_recorder_mode(self) -> RecorderMode:
        """
        获取当前录制模式
        
        Returns:
            RecorderMode: 当前录制模式
        """
        if not self._recorder or vac is None:
            return RecorderMode.VIDEO  # 默认返回 VIDEO 模式
        
        try:
            cpp_mode = self._recorder.recorder_mode
            # 将 C++ 枚举转换为 Python 枚举
            return RecorderMode.VIDEO if cpp_mode == vac.RecorderMode.VIDEO else RecorderMode.SNAPSHOT
        except Exception as e:
            print(f"[ScreenRecorderService] Error getting recorder mode: {e}")
            return RecorderMode.VIDEO
    
    def get_stats(self) -> Dict[str, Any]:
        """
        获取录制统计信息
        
        Returns:
            dict: 包含帧数、文件大小、帧率、录制模式等信息
        """
        if not self._recorder:
            return {
                'frame_count': 0,
                'encoded_count': 0,
                'dropped_count': 0,
                'output_file_size': 0,
                'current_fps': 0.0,
                'is_recording': False,
                'recorder_mode': 'VIDEO'
            }
        
        # 获取当前模式
        current_mode = self.get_recorder_mode()
        mode_str = 'VIDEO' if current_mode == RecorderMode.VIDEO else 'SNAPSHOT'
        
        return {
            'frame_count': self._recorder.frame_count,
            'encoded_count': self._recorder.encoded_count,
            'dropped_count': self._recorder.dropped_count,
            'output_file_size': self._recorder.output_file_size,
            'current_fps': self._recorder.current_fps,
            'is_recording': self._recorder.is_recording,
            'recorder_mode': mode_str
        }
    
    def set_progress_callback(self, callback: Callable[[int, int], None]):
        """
        设置进度回调函数
        
        Args:
            callback: 回调函数，签名为 callback(frames: int, size: int)
                     frames - 已编码的帧数
                     size - 输出文件大小（字节）
        """
        self._ensure_recorder()
        self._progress_callback = callback  # 防止被 GC 回收
        
        if self._recorder is None:
            raise RuntimeError("Recorder not initialized")
        
        try:
            self._recorder.set_progress_callback(callback)
            print("[ScreenRecorderService] Progress callback set")
        except Exception as e:
            print(f"[ScreenRecorderService] Error setting progress callback: {e}")
            raise
    
    def set_error_callback(self, callback: Callable[[str], None]):
        """
        设置错误回调函数
        
        Args:
            callback: 回调函数，签名为 callback(error_message: str)
        """
        self._ensure_recorder()
        self._error_callback = callback  # 防止被 GC 回收
        
        if self._recorder is None:
            raise RuntimeError("Recorder not initialized")
        
        try:
            self._recorder.set_error_callback(callback)
            print("[ScreenRecorderService] Error callback set")
        except Exception as e:
            print(f"[ScreenRecorderService] Error setting error callback: {e}")
            raise
    
    def create_encoder_config(
        self,
        width: int = 1920,
        height: int = 1080,
        fps: int = 30,
        bitrate: int = 5000000,
        crf: int = 23,
        preset: str = EncoderPreset.MEDIUM,
        codec: str = VideoCodec.H264
    ):
        """
        创建编码器配置
        
        Args:
            width: 视频宽度（默认 1920）
            height: 视频高度（默认 1080）
            fps: 帧率（默认 30）
            bitrate: 码率（默认 5Mbps）
            crf: 质量参数，范围 0-51，越小质量越好（默认 23）
            preset: 编码预设，影响编码速度和压缩率（默认 medium）
            codec: 编码器（默认 libx264）
            
        Returns:
            EncoderConfig: 编码器配置对象
        """
        if vac is None:
            raise RuntimeError("C++ module not available")
        
        try:
            config = vac.EncoderConfig()
            config.width = width
            config.height = height
            config.fps = fps
            config.bitrate = bitrate
            config.crf = crf
            config.preset = preset
            config.codec = codec
            return config
        except Exception as e:
            print(f"[ScreenRecorderService] Error creating encoder config: {e}")
            raise
    
    def get_default_encoder_config(
        self,
        width: int = 1920,
        height: int = 1080
    ):
        """
        获取默认编码器配置
        
        Args:
            width: 视频宽度（默认 1920）
            height: 视频高度（默认 1080）
            
        Returns:
            EncoderConfig: 默认编码器配置
        """
        if vac is None:
            raise RuntimeError("C++ module not available")
        
        try:
            return vac.default_encoder_config(width, height)
        except Exception as e:
            print(f"[ScreenRecorderService] Error getting default config: {e}")
            raise
    
    def __enter__(self):
        """上下文管理器入口"""
        self._ensure_recorder()
        return self
    
    def __exit__(self, exc_type, exc_val, exc_tb):
        """上下文管理器退出"""
        if self._recorder and self._is_recording:
            self.stop_recording()
        return False
    
    def __repr__(self):
        """字符串表示"""
        stats = self.get_stats()
        return (f"<ScreenRecorderService "
                f"recording={stats['is_recording']} "
                f"frames={stats['frame_count']} "
                f"encoded={stats['encoded_count']} "
                f"fps={stats['current_fps']:.1f}>")

class VideoService:
    """视频服务 - 兼容旧接口的包装器"""
    
    def __init__(self):
        """初始化服务"""
        self._recorder_service = ScreenRecorderService()
        print("[VideoService] Created (compatibility wrapper)")
    
    def initialize(self):
        """初始化服务"""
        print("[VideoService] Initializing...")
        # 这里可以执行一些全局初始化逻辑
        return True

    def process_frame(self, data: str) -> str:
        """处理帧数据 (Stub)"""
        print(f"[VideoService] Processing frame: {data[:20]}...")
        return f"Processed: {data[:10]}"

    def set_parameter(self, key: str, value: float):
        """设置参数 (Stub)"""
        print(f"[VideoService] Setting parameter: {key} = {value}")

    def get_processor_info(self) -> str:
        """获取处理器信息 (Stub)"""
        return "AI Video Analysis Processor v1.0.0 (C++ Backend)"

    def test_hello(self) -> str:
        """测试方法 (Stub)"""
        return "Hello from VideoService!"

    def get_screen_recorder(self) -> ScreenRecorderService:
        """获取屏幕录制服务"""
        return self._recorder_service
    
    def get_version(self) -> str:
        """获取 C++ 模块版本"""
        if vac is None:
            return "N/A (module not loaded)"
        
        try:
            # 如果有版本信息，返回它
            if hasattr(vac, 'version'):
                return vac.version()
            return "1.0.0"
        except Exception as e:
            print(f"[VideoService] Error getting version: {e}")
            return "Unknown"
