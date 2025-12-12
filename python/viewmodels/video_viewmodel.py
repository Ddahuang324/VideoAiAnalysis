"""
视频处理 ViewModel
负责视频处理相关的业务逻辑
"""
from PySide6.QtCore import QObject, Signal, Slot, Property
from services.video_service import VideoService
from models.video_model import VideoModel, AnalysisResult


class VideoViewModel(QObject):
    """视频处理 ViewModel"""
    
    # 信号定义
    statusChanged = Signal(str)
    progressChanged = Signal(float)
    resultChanged = Signal(str)
    
    def __init__(self, parent=None):
        super().__init__(parent)
        self._service = VideoService()
        self._status = "Ready"
        self._progress = 0.0
        self._result = ""
        
        print("[VideoViewModel] Initialized")
    
    @Slot(str)
    def processVideo(self, video_path: str):
        """处理视频"""
        try:
            self._status = "Processing..."
            self._progress = 0.0
            self.statusChanged.emit(self._status)
            self.progressChanged.emit(self._progress)
            
            # 调用服务层处理视频
            result = self._service.process_frame(video_path)
            
            self._progress = 100.0
            self._status = "Completed"
            self._result = result
            
            self.statusChanged.emit(self._status)
            self.progressChanged.emit(self._progress)
            self.resultChanged.emit(self._result)
            
            print(f"[VideoViewModel] Video processed: {result}")
            
        except Exception as e:
            self._status = f"Error: {e}"
            self.statusChanged.emit(self._status)
            print(f"[VideoViewModel] Error: {e}")
    
    @Slot(str, float)
    def setParameter(self, key: str, value: float):
        """设置处理参数"""
        try:
            self._service.set_parameter(key, value)
            self._status = f"Parameter '{key}' set to {value}"
            self.statusChanged.emit(self._status)
            print(f"[VideoViewModel] Parameter set: {key}={value}")
        except Exception as e:
            self._status = f"Error: {e}"
            self.statusChanged.emit(self._status)
            print(f"[VideoViewModel] Error: {e}")
    
    # Properties
    @Property(str, notify=statusChanged)
    def status(self):
        """状态"""
        return self._status
    
    @Property(float, notify=progressChanged)
    def progress(self):
        """进度"""
        return self._progress
    
    @Property(str, notify=resultChanged)
    def result(self):
        """结果"""
        return self._result
    
    @Slot(result=str)
    def testCppCall(self):
        """测试 C++ 调用 - 用于调试跟踪"""
        try:
            print("\n" + "=" * 50)
            print("[VideoViewModel] 🚀 开始测试 C++ 调用...")
            print("=" * 50)
            
            # 1. 初始化
            self._service.initialize()
            print("[VideoViewModel] ✅ C++ 初始化完成")
            
            # 2. 获取版本
            version = self._service.get_version()
            print(f"[VideoViewModel] 📦 C++ 模块版本: {version}")
            
            # 3. 处理一帧测试数据
            result = self._service.process_frame("test_frame_data_12345")
            print(f"[VideoViewModel] 🎬 帧处理结果: {result}")
            
            # 4. 设置参数
            self._service.set_parameter("threshold", 0.75)
            print("[VideoViewModel] ⚙️ 参数设置完成")
            
            # 5. 获取处理器信息
            info = self._service.get_processor_info()
            print(f"[VideoViewModel] 📋 处理器信息:\n{info}")
            
            print("=" * 50)
            print("[VideoViewModel] ✅ C++ 调用测试完成!")
            print("=" * 50 + "\n")
            
            self._result = f"C++ 测试成功! 版本: {version}"
            self.resultChanged.emit(self._result)
            return self._result
            
        except Exception as e:
            error_msg = f"C++ 调用失败: {e}"
            print(f"[VideoViewModel] ❌ {error_msg}")
            self._result = error_msg
            self.resultChanged.emit(self._result)
            return error_msg
