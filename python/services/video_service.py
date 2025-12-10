"""
视频处理服务
封装 C++ 视频处理模块的调用
"""
import sys
from pathlib import Path

# 添加 C++ 编译输出目录到 Python 路径
build_path = Path(__file__).parent.parent.parent / "build" / "python"
sys.path.insert(0, str(build_path))

try:
    import video_analysis_cpp
    print(f"[VideoService] Successfully imported C++ module from: {build_path}")
except ImportError as e:
    print(f"[VideoService] Failed to import C++ module: {e}")
    print(f"[VideoService] Please run: cmake --build build")
    video_analysis_cpp = None


class VideoService:
    """视频处理服务 - 封装 C++ 调用"""
    
    def __init__(self):
        """初始化服务"""
        self._processor = None
        self._initialized = False
        print("[VideoService] Created")
    
    def initialize(self):
        """初始化处理器"""
        if video_analysis_cpp is None:
            raise RuntimeError("C++ module not available")
        
        try:
            self._processor = video_analysis_cpp.VideoProcessor()
            success = self._processor.initialize()
            self._initialized = success
            print(f"[VideoService] Initialized: {success}")
            return success
        except Exception as e:
            print(f"[VideoService] Initialization failed: {e}")
            raise
    
    def process_frame(self, frame_data: str) -> str:
        """处理单帧视频"""
        if not self._initialized:
            self.initialize()
        
        try:
            result = self._processor.process_frame(frame_data)
            print(f"[VideoService] Processed frame: {result}")
            return result
        except Exception as e:
            print(f"[VideoService] Error processing frame: {e}")
            raise
    
    def process_frames(self, frames: list) -> list:
        """批量处理多帧"""
        if not self._initialized:
            self.initialize()
        
        try:
            results = self._processor.process_frames(frames)
            print(f"[VideoService] Processed {len(frames)} frames")
            return results
        except Exception as e:
            print(f"[VideoService] Error processing frames: {e}")
            raise
    
    def get_processor_info(self) -> str:
        """获取处理器信息"""
        if not self._initialized:
            self.initialize()
        
        try:
            info = self._processor.get_info()
            return info
        except Exception as e:
            print(f"[VideoService] Error getting info: {e}")
            raise
    
    def set_parameter(self, key: str, value: float):
        """设置处理参数"""
        if not self._initialized:
            self.initialize()
        
        try:
            self._processor.set_parameter(key, value)
            print(f"[VideoService] Set parameter: {key}={value}")
        except Exception as e:
            print(f"[VideoService] Error setting parameter: {e}")
            raise
    
    def get_parameter(self, key: str) -> float:
        """获取处理参数"""
        if not self._initialized:
            self.initialize()
        
        try:
            value = self._processor.get_parameter(key)
            return value
        except Exception as e:
            print(f"[VideoService] Error getting parameter: {e}")
            raise
    
    def get_version(self) -> str:
        """获取 C++ 模块版本"""
        if video_analysis_cpp is None:
            return "N/A (module not loaded)"
        
        try:
            return video_analysis_cpp.version()
        except Exception as e:
            print(f"[VideoService] Error getting version: {e}")
            return "Unknown"
    
    def test_hello(self) -> str:
        """测试 C++ 模块"""
        if video_analysis_cpp is None:
            raise RuntimeError("C++ module not available")
        
        try:
            return video_analysis_cpp.hello()
        except Exception as e:
            print(f"[VideoService] Error in hello test: {e}")
            raise
