"""
录制视图模型
负责录制状态管理和UI交互
"""
from PySide6.QtCore import QObject, Signal, Slot, Property, QTimer
from typing import Optional

from services.recorder_service import RecorderService
from infrastructure.process_manager import ModuleManager, ProcessStatus
from infrastructure.log_manager import get_logger


class RecorderViewModel(QObject):
    """
    录制视图模型
    负责录制状态管理和UI交互
    """

    # ========== 信号定义 ==========
    # 状态信号
    statusChanged = Signal(str)
    isRecordingChanged = Signal(bool)
    isPausedChanged = Signal(bool)

    # 进度信号
    progressChanged = Signal(float)
    frameCountChanged = Signal(int)
    encodedCountChanged = Signal(int)
    droppedCountChanged = Signal(int)
    fileSizeChanged = Signal(int)
    currentFpsChanged = Signal(float)

    # 错误信号
    errorOccurred = Signal(str)

    def __init__(
        self,
        recorder_service: RecorderService,
        module_manager: ModuleManager
    ):
        """
        初始化录制视图模型

        Args:
            recorder_service: 录制服务实例
            module_manager: 模块管理器实例
        """
        super().__init__()
        self._service = recorder_service
        self._module_manager = module_manager
        self.logger = get_logger("RecorderViewModel")

        # 状态属性
        self._status = "Ready"
        self._is_recording = False
        self._is_paused = False

        # 统计属性
        self._progress = 0.0  # 录制时长（秒）
        self._frame_count = 0
        self._encoded_count = 0
        self._dropped_count = 0
        self._file_size = 0
        self._current_fps = 0.0

        # 定时器 - 用于更新录制统计
        self._update_timer = QTimer()
        self._update_timer.timeout.connect(self._update_stats)
        self._update_timer.setInterval(1000)  # 每秒更新

        # 设置服务回调
        self._setup_service_callbacks()

        self.logger.info("RecorderViewModel initialized")

    def _setup_service_callbacks(self):
        """设置服务回调"""
        def on_status_change(status):
            """处理状态变化"""
            if hasattr(status, 'name'):
                self._status = status.name
                self.statusChanged.emit(self._status)

                # 更新录制状态
                if 'RECORDING' in status.name:
                    self._is_recording = True
                    self._is_paused = False
                    self.isRecordingChanged.emit(True)
                    self.isPausedChanged.emit(False)
                elif 'PAUSED' in status.name:
                    self._is_paused = True
                    self.isPausedChanged.emit(True)
                elif 'IDLE' in status.name or 'STOPPED' in status.name:
                    self._is_recording = False
                    self._is_paused = False
                    self.isRecordingChanged.emit(False)
                    self.isPausedChanged.emit(False)

        def on_error(error_msg):
            """处理错误"""
            self.errorOccurred.emit(error_msg)

        self._service.set_status_callback(on_status_change)
        self._service.set_error_callback(on_error)

    @Slot()
    def initialize(self):
        """初始化录制服务"""
        try:
            if self._service.initialize():
                self._status = "Initialized"
                self.statusChanged.emit(self._status)
                self.logger.info("Recorder service initialized")
            else:
                self._status = "Initialization failed"
                self.statusChanged.emit(self._status)
                self.errorOccurred.emit("Failed to initialize recorder service")
        except Exception as e:
            self._status = "Error"
            self.statusChanged.emit(self._status)
            self.errorOccurred.emit(f"Initialization error: {e}")
            self.logger.error(f"Initialization error: {e}")

    @Slot()
    def startRecording(self):
        """开始录制"""
        if self._is_recording:
            self.errorOccurred.emit("Recording already in progress")
            return

        try:
            if self._service.start_recording():
                self._is_recording = True
                self.isRecordingChanged.emit(True)
                self._status = "Recording..."
                self.statusChanged.emit(self._status)

                # 启动统计更新定时器
                self._update_timer.start()

                self.logger.info("Recording started")
            else:
                error_msg = self._service.get_api().last_error if self._service.get_api() else "Unknown error"
                self.errorOccurred.emit(f"Failed to start recording: {error_msg}")
        except Exception as e:
            self.errorOccurred.emit(f"Start recording error: {e}")
            self.logger.error(f"Start recording error: {e}")

    @Slot()
    def stopRecording(self):
        """停止录制"""
        if not self._is_recording:
            return

        try:
            if self._service.stop_recording():
                self._is_recording = False
                self._is_paused = False
                self.isRecordingChanged.emit(False)
                self.isPausedChanged.emit(False)

                # 停止统计更新定时器
                self._update_timer.stop()

                self._status = "Stopped"
                self.statusChanged.emit(self._status)
                self.logger.info("Recording stopped")
            else:
                self.errorOccurred.emit("Failed to stop recording")
        except Exception as e:
            self.errorOccurred.emit(f"Stop recording error: {e}")
            self.logger.error(f"Stop recording error: {e}")

    @Slot()
    def pauseRecording(self):
        """暂停录制"""
        if not self._is_recording or self._is_paused:
            return

        try:
            if self._service.pause_recording():
                self._is_paused = True
                self.isPausedChanged.emit(True)
                self._status = "Paused"
                self.statusChanged.emit(self._status)
                self.logger.info("Recording paused")
            else:
                self.errorOccurred.emit("Failed to pause recording")
        except Exception as e:
            self.errorOccurred.emit(f"Pause error: {e}")
            self.logger.error(f"Pause error: {e}")

    @Slot()
    def resumeRecording(self):
        """恢复录制"""
        if not self._is_recording or not self._is_paused:
            return

        try:
            if self._service.resume_recording():
                self._is_paused = False
                self.isPausedChanged.emit(False)
                self._status = "Recording..."
                self.statusChanged.emit(self._status)
                self.logger.info("Recording resumed")
            else:
                self.errorOccurred.emit("Failed to resume recording")
        except Exception as e:
            self.errorOccurred.emit(f"Resume error: {e}")
            self.logger.error(f"Resume error: {e}")

    def _update_stats(self):
        """更新录制统计信息"""
        try:
            info = self._service.get_recording_info()

            # 更新进度（录制时长）
            new_progress = info.get("duration", 0.0)
            if abs(new_progress - self._progress) > 0.1:
                self._progress = new_progress
                self.progressChanged.emit(self._progress)

            # 更新帧数
            new_frame_count = info.get("frame_count", 0)
            if new_frame_count != self._frame_count:
                self._frame_count = new_frame_count
                self.frameCountChanged.emit(self._frame_count)

            # 更新编码帧数
            new_encoded_count = info.get("encoded_count", 0)
            if new_encoded_count != self._encoded_count:
                self._encoded_count = new_encoded_count
                self.encodedCountChanged.emit(self._encoded_count)

            # 更新丢帧数
            new_dropped_count = info.get("dropped_count", 0)
            if new_dropped_count != self._dropped_count:
                self._dropped_count = new_dropped_count
                self.droppedCountChanged.emit(self._dropped_count)

            # 更新文件大小
            new_file_size = info.get("file_size", 0)
            if new_file_size != self._file_size:
                self._file_size = new_file_size
                self.fileSizeChanged.emit(self._file_size)

            # 更新帧率
            new_fps = info.get("current_fps", 0.0)
            if abs(new_fps - self._current_fps) > 0.5:
                self._current_fps = new_fps
                self.currentFpsChanged.emit(self._current_fps)

        except Exception as e:
            self.logger.error(f"Error updating stats: {e}")

    # ========== Properties ==========

    @Property(bool, notify=isRecordingChanged)
    def isRecording(self) -> bool:
        """是否正在录制"""
        return self._is_recording

    @Property(bool, notify=isPausedChanged)
    def isPaused(self) -> bool:
        """是否已暂停"""
        return self._is_paused

    @Property(str, notify=statusChanged)
    def status(self) -> str:
        """当前状态"""
        return self._status

    @Property(float, notify=progressChanged)
    def progress(self) -> float:
        """录制时长（秒）"""
        return self._progress

    @Property(int, notify=frameCountChanged)
    def frameCount(self) -> int:
        """已捕获帧数"""
        return self._frame_count

    @Property(int, notify=encodedCountChanged)
    def encodedCount(self) -> int:
        """已编码帧数"""
        return self._encoded_count

    @Property(int, notify=droppedCountChanged)
    def droppedCount(self) -> int:
        """丢帧数"""
        return self._dropped_count

    @Property(int, notify=fileSizeChanged)
    def fileSize(self) -> int:
        """输出文件大小（字节）"""
        return self._file_size

    @Property(float, notify=currentFpsChanged)
    def currentFps(self) -> float:
        """当前帧率"""
        return self._current_fps

    @Property(str, notify=progressChanged)
    def formattedDuration(self) -> str:
        """格式化的录制时长"""
        minutes = int(self._progress // 60)
        seconds = int(self._progress % 60)
        return f"{minutes:02d}:{seconds:02d}"

    @Property(str, notify=fileSizeChanged)
    def formattedFileSize(self) -> str:
        """格式化的文件大小"""
        for unit in ['B', 'KB', 'MB', 'GB']:
            if self._file_size < 1024.0:
                return f"{self._file_size:.2f} {unit}"
            self._file_size /= 1024.0
        return f"{self._file_size:.2f} TB"

    def shutdown(self):
        """关闭视图模型"""
        self._update_timer.stop()
        self._service.shutdown()
        self.logger.info("RecorderViewModel shutdown")
