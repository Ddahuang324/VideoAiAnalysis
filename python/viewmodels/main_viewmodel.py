"""
主窗口 ViewModel
负责主界面的业务逻辑和状态管理
"""
from PySide6.QtCore import QObject, Signal, Slot, Property
from services.video_service import VideoService


class MainViewModel(QObject):
    """主窗口 ViewModel"""
    
    # 信号定义
    statusMessageChanged = Signal(str)
    isProcessingChanged = Signal(bool)
    
    def __init__(self, parent=None):
        super().__init__(parent)
        self._video_service = VideoService()
        self._status_message = "Ready"
        self._is_processing = False
        
        print("[MainViewModel] Initialized")
    
    @Slot()
    def initialize(self):
        """初始化 ViewModel"""
        try:
            # 初始化服务
            self._video_service.initialize()
            self._status_message = "Application initialized successfully"
            self.statusMessageChanged.emit(self._status_message)
            print("[MainViewModel] Initialized successfully")
        except Exception as e:
            self._status_message = f"Initialization failed: {e}"
            self.statusMessageChanged.emit(self._status_message)
            print(f"[MainViewModel] Error: {e}")
    
    @Slot(str)
    def testCppModule(self, name: str):
        """测试 C++ 模块调用"""
        try:
            self._is_processing = True
            self.isProcessingChanged.emit(self._is_processing)
            
            # 调用服务层
            version = self._video_service.get_version()
            hello_msg = self._video_service.test_hello()
            
            self._status_message = (
                f"✅ C++ Module Test:\n"
                f"Version: {version}\n"
                f"Message: {hello_msg}\n"
                f"Your name: {name}"
            )
            self.statusMessageChanged.emit(self._status_message)
            
            print(f"[MainViewModel] C++ test successful: {hello_msg}")
            
        except Exception as e:
            self._status_message = f"❌ Error: {e}"
            self.statusMessageChanged.emit(self._status_message)
            print(f"[MainViewModel] Error: {e}")
        finally:
            self._is_processing = False
            self.isProcessingChanged.emit(self._is_processing)
    
    @Slot()
    def testVideoProcessor(self):
        """测试视频处理器"""
        try:
            self._is_processing = True
            self.isProcessingChanged.emit(self._is_processing)
            
            # 调用服务层
            info = self._video_service.get_processor_info()
            
            self._status_message = f"✅ Video Processor Info:\n{info}"
            self.statusMessageChanged.emit(self._status_message)
            
            print(f"[MainViewModel] Video processor test successful")
            
        except Exception as e:
            self._status_message = f"❌ Error: {e}"
            self.statusMessageChanged.emit(self._status_message)
            print(f"[MainViewModel] Error: {e}")
        finally:
            self._is_processing = False
            self.isProcessingChanged.emit(self._is_processing)
    
    # Properties
    @Property(str, notify=statusMessageChanged)
    def statusMessage(self):
        """状态消息"""
        return self._status_message
    
    @Property(bool, notify=isProcessingChanged)
    def isProcessing(self):
        """是否正在处理"""
        return self._is_processing
