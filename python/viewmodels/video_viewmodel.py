"""
视频处理 ViewModel
负责视频处理相关的业务逻辑
"""
from PySide6.QtCore import QObject, Signal, Slot, Property
from services.video_service import VideoService, ScreenRecorderService, RecorderMode
from models.video_model import VideoModel, AnalysisResult
from datetime import datetime
from pathlib import Path


class VideoViewModel(QObject):
    """视频处理 ViewModel"""
    
    # 信号定义
    statusChanged = Signal(str)
    progressChanged = Signal(float)
    resultChanged = Signal(str)
    
    # 录制相关信号
    recordingStateChanged = Signal(bool)  # 录制状态改变
    recordingStatsChanged = Signal()     # 录制统计信息改变
    recordingError = Signal(str)         # 录制错误
    recorderModeChanged = Signal(int)    # 录制模式改变
    
    def __init__(self, parent=None):
        super().__init__(parent)
        self._service = VideoService()
        self._status = "Ready"
        self._progress = 0.0
        self._result = ""
        
        # 录制相关状态
        self._is_recording = False
        self._recorder_mode = RecorderMode.VIDEO  # 默认为 VIDEO 模式
        self._recording_stats = {
            'frame_count': 0,
            'encoded_count': 0,
            'dropped_count': 0,
            'output_file_size': 0,
            'current_fps': 0.0
        }
        
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
    
    # ==================== 录制功能 ====================
    
    @Slot(result=bool)
    def startRecording(self):
        """开始录制"""
        try:
            # 生成输出文件名（带时间戳）
            timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
            output_dir = Path.home() / "Videos" / "ScreenRecordings"
            output_dir.mkdir(parents=True, exist_ok=True)
            output_path = output_dir / f"recording_{timestamp}.mp4"
            
            print(f"[VideoViewModel] 开始录制: {output_path}")
            
            # 获取录制服务
            recorder = self._service.get_screen_recorder()
            
            # 开始录制
            success = recorder.start_recording(str(output_path))
            
            if success:
                self._is_recording = True
                self.recordingStateChanged.emit(True)
                self._status = f"Recording to: {output_path.name}"
                self.statusChanged.emit(self._status)
                print(f"[VideoViewModel] ✅ 录制已开始")
                return True
            else:
                error_msg = "Failed to start recording"
                print(f"[VideoViewModel] ❌ {error_msg}")
                self.recordingError.emit(error_msg)
                return False
                
        except Exception as e:
            error_msg = f"录制启动失败: {e}"
            print(f"[VideoViewModel] ❌ {error_msg}")
            self.recordingError.emit(error_msg)
            return False
    
    @Slot(result=bool)
    def stopRecording(self):
        """停止录制"""
        try:
            print("[VideoViewModel] 停止录制")
            
            recorder = self._service.get_screen_recorder()
            recorder.stop_recording()
            
            self._is_recording = False
            self.recordingStateChanged.emit(False)
            self._status = "Recording stopped"
            self.statusChanged.emit(self._status)
            
            print("[VideoViewModel] ✅ 录制已停止")
            return True
            
        except Exception as e:
            error_msg = f"停止录制失败: {e}"
            print(f"[VideoViewModel] ❌ {error_msg}")
            self.recordingError.emit(error_msg)
            return False
    
    @Slot()
    def pauseRecording(self):
        """暂停录制"""
        try:
            recorder = self._service.get_screen_recorder()
            recorder.pause_recording()
            self._status = "Recording paused"
            self.statusChanged.emit(self._status)
            print("[VideoViewModel] ⏸️ 录制已暂停")
        except Exception as e:
            error_msg = f"暂停录制失败: {e}"
            print(f"[VideoViewModel] ❌ {error_msg}")
            self.recordingError.emit(error_msg)
    
    @Slot()
    def resumeRecording(self):
        """恢复录制"""
        try:
            recorder = self._service.get_screen_recorder()
            recorder.resume_recording()
            self._status = "Recording resumed"
            self.statusChanged.emit(self._status)
            print("[VideoViewModel] ▶️ 录制已恢复")
        except Exception as e:
            error_msg = f"恢复录制失败: {e}"
            print(f"[VideoViewModel] ❌ {error_msg}")
            self.recordingError.emit(error_msg)
    
    @Slot()
    def updateRecordingStats(self):
        """更新录制统计信息"""
        try:
            recorder = self._service.get_screen_recorder()
            self._recording_stats = recorder.get_stats()
            self.recordingStatsChanged.emit()
        except Exception as e:
            print(f"[VideoViewModel] ⚠️ 获取统计信息失败: {e}")
    
    # 录制状态属性
    @Property(bool, notify=recordingStateChanged)
    def isRecording(self):
        """是否正在录制"""
        return self._is_recording
    
    @Property(int, notify=recordingStatsChanged)
    def frameCount(self):
        """已捕获的帧数"""
        return self._recording_stats.get('frame_count', 0)
    
    @Property(int, notify=recordingStatsChanged)
    def encodedCount(self):
        """已编码的帧数"""
        return self._recording_stats.get('encoded_count', 0)
    
    @Property(int, notify=recordingStatsChanged)
    def droppedCount(self):
        """丢帧数"""
        return self._recording_stats.get('dropped_count', 0)
    
    @Property(float, notify=recordingStatsChanged)
    def currentFps(self):
        """当前帧率"""
        return self._recording_stats.get('current_fps', 0.0)
    
    @Property(int, notify=recordingStatsChanged)
    def outputFileSize(self):
        """输出文件大小（字节）"""
        return self._recording_stats.get('output_file_size', 0)
    
    # ==================== 录制模式功能 ====================
    
    @Property(int, notify=recorderModeChanged)
    def recorderMode(self):
        """
        当前录制模式
        返回: 0 = VIDEO, 1 = SNAPSHOT
        """
        return self._recorder_mode.value
    
    @Slot(int)
    def setRecorderMode(self, mode: int):
        """
        设置录制模式
        
        Args:
            mode: 0 = VIDEO (高帧率), 1 = SNAPSHOT (低帧率)
        """
        try:
            if self._is_recording:
                error_msg = "Cannot change mode while recording"
                print(f"[VideoViewModel] ⚠️ {error_msg}")
                self.recordingError.emit(error_msg)
                return
            
            # 转换为 RecorderMode 枚举
            new_mode = RecorderMode.VIDEO if mode == 0 else RecorderMode.SNAPSHOT
            
            if new_mode != self._recorder_mode:
                self._recorder_mode = new_mode
                
                # 更新 C++ 层的模式
                recorder = self._service.get_screen_recorder()
                recorder.set_recorder_mode(new_mode)
                
                mode_name = "VIDEO" if new_mode == RecorderMode.VIDEO else "SNAPSHOT"
                print(f"[VideoViewModel] 📹 Recorder mode set to: {mode_name}")
                
                self.recorderModeChanged.emit(mode)
                self._status = f"Mode: {mode_name}"
                self.statusChanged.emit(self._status)
                
        except Exception as e:
            error_msg = f"设置录制模式失败: {e}"
            print(f"[VideoViewModel] ❌ {error_msg}")
            self.recordingError.emit(error_msg)
    
    @Slot(result=str)
    def getRecorderModeName(self):
        """
        获取当前录制模式的名称
        返回: "VIDEO" 或 "SNAPSHOT"
        """
        return "VIDEO" if self._recorder_mode == RecorderMode.VIDEO else "SNAPSHOT"

