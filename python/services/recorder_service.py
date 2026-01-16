"""
录制服务
封装录制业务逻辑，管理录制会话
"""
from typing import Optional, Callable, Dict, Any
from datetime import datetime
from pathlib import Path
import uuid
import threading

from infrastructure.process_manager import ModuleManager, ProcessStatus
from infrastructure.log_manager import get_logger

try:
    from recorder_module import RecorderMode
except ImportError:
    # 如果模块还未构建，使用占位符
    RecorderMode = None


class RecordingSession:
    """录制会话数据类"""

    def __init__(self, output_path: str):
        self.session_id = str(uuid.uuid4())
        self.output_path = output_path
        self.start_time: Optional[datetime] = None
        self.end_time: Optional[datetime] = None
        self.stats: Dict[str, Any] = {
            "frame_count": 0,
            "encoded_count": 0,
            "dropped_count": 0,
            "file_size": 0,
            "duration": 0.0
        }

    def to_dict(self) -> Dict[str, Any]:
        """转换为字典"""
        return {
            "session_id": self.session_id,
            "output_path": self.output_path,
            "start_time": self.start_time.isoformat() if self.start_time else None,
            "end_time": self.end_time.isoformat() if self.end_time else None,
            "stats": self.stats.copy()
        }


class RecorderService:
    """
    录制业务服务
    封装录制业务逻辑，管理录制会话
    """

    def __init__(self, module_manager: ModuleManager):
        """
        初始化录制服务

        Args:
            module_manager: 模块管理器实例
        """
        self.module_manager = module_manager
        self.logger = get_logger("RecorderService")

        # 录制器API
        self._api = None
        self._recorder_module = None

        # 当前会话
        self._current_session: Optional[RecordingSession] = None

        # 回调函数
        self._status_callbacks = []
        self._progress_callbacks = []
        self._error_callbacks = []

        # 状态锁
        self._lock = threading.Lock()

        # 输出目录配置
        self._default_output_dir = Path.home() / "Videos" / "ScreenRecordings"

        # 分析服务引用（用于SNAPSHOT模式实时分析）
        from services.analyzer_service import AnalyzerService
        from services.history_service import HistoryService
        self._analyzer_service: Optional[AnalyzerService] = None
        self._history_service: Optional[HistoryService] = None
        self._auto_enable_realtime: bool = True  # SNAPSHOT模式自动启用实时分析

        # 缓存的录制模式（解决API未初始化时设置模式的时序问题）
        self._pending_mode = None

        # 设置视图模型引用（用于获取用户配置）
        self._settings_viewmodel = None

    def initialize(self, config=None) -> bool:
        """
        初始化录制服务

        Args:
            config: RecorderConfig配置对象 (可选)

        Returns:
            bool: 初始化成功返回True
        """
        try:
            # 获取录制器模块
            self._recorder_module = self.module_manager.get_recorder_module()
            if self._recorder_module is None:
                self.logger.error("Failed to get recorder module")
                return False

            # 创建API实例
            if config is None:
                # 使用默认配置
                config = self._recorder_module.default_recorder_config()

            self._api = self.module_manager.create_recorder_api(config)
            if self._api is None:
                self.logger.error("Failed to create recorder API")
                return False

            # 设置内部状态回调
            self._setup_internal_callbacks()

            self.logger.info("RecorderService initialized successfully")
            return True

        except Exception as e:
            self.logger.error(f"Failed to initialize RecorderService: {e}")
            return False

    def _setup_internal_callbacks(self):
        """设置内部回调函数"""
        if self._api is None:
            return

        def on_status_change(status):
            """处理状态变化"""
            self.logger.debug(f"Recorder status changed: {status}")

            # 更新模块管理器状态
            if hasattr(status, 'name'):
                if 'IDLE' in status.name:
                    self.module_manager.set_recorder_status(ProcessStatus.STOPPED)
                elif 'RECORDING' in status.name:
                    self.module_manager.set_recorder_status(ProcessStatus.RUNNING)
                elif 'PAUSED' in status.name:
                    self.module_manager.set_recorder_status(ProcessStatus.PAUSED)
                elif 'ERROR' in status.name:
                    self.module_manager.set_recorder_status(ProcessStatus.ERROR)

            # 通知外部回调
            for callback in self._status_callbacks:
                try:
                    callback(status)
                except Exception as e:
                    self.logger.error(f"Error in status callback: {e}")

        def on_error(error_msg):
            """处理错误"""
            self.logger.error(f"Recorder error: {error_msg}")
            for callback in self._error_callbacks:
                try:
                    callback(error_msg)
                except Exception as e:
                    self.logger.error(f"Error in error callback: {e}")

        self._api.set_status_callback(on_status_change)
        self._api.set_error_callback(on_error)

    def start_recording(self, output_path: Optional[str] = None) -> bool:
        """
        开始录制

        Args:
            output_path: 输出文件路径，如果为None则自动生成

        Returns:
            bool: 成功返回True
        """
        recording_started = False
        with self._lock:
            if self._current_session is not None:
                self.logger.warning("Recording already in progress")
                return False

            if self._api is None:
                self.logger.error("Recorder API not initialized")
                return False

            try:
                # 生成输出路径
                if output_path is None:
                    # 优先使用用户设置的输出目录
                    if self._settings_viewmodel:
                        output_dir = Path(self._settings_viewmodel.outputDir)
                    else:
                        output_dir = self._default_output_dir
                    output_dir.mkdir(parents=True, exist_ok=True)

                    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
                    output_path = str(output_dir / f"recording_{timestamp}.mp4")

                # 创建会话
                self._current_session = RecordingSession(output_path)
                self._current_session.start_time = datetime.now()

                # 预创建数据库记录
                if self._history_service:
                    self._history_service.start_recording(
                        file_path=output_path,
                        start_time=self._current_session.start_time,
                        record_id=self._current_session.session_id
                    )

                # 更新配置的输出路径
                if self._recorder_module is None:
                    self.logger.error("Recorder module not initialized")
                    return False

                config = self._recorder_module.default_recorder_config()
                config.video.output_file_path = output_path

                # 应用用户设置到配置
                if self._settings_viewmodel:
                    self._settings_viewmodel.apply_to_recorder_config(config)

                # 初始化并启动
                if self._api is None:
                    self.logger.error("Recorder API not initialized")
                    return False

                self._api.initialize(config)

                # 应用缓存的录制模式（解决时序问题）
                if self._pending_mode is not None:
                    self._api.set_recording_mode(self._pending_mode)
                    mode_name = "SNAPSHOT" if self._pending_mode == RecorderMode.SNAPSHOT else "VIDEO"
                    self.logger.info(f"Applied pending recording mode: {mode_name}")

                result = self._api.start()

                if result:
                    self.logger.info(f"Recording started: {output_path}")
                    recording_started = True
                else:
                    self.logger.error(f"Failed to start recording: {self._api.last_error}")
                    self._current_session = None
                    return False

            except Exception as e:
                self.logger.error(f"Error starting recording: {e}")
                self._current_session = None
                return False

        # SNAPSHOT模式：启动实时分析（在锁外调用，避免死锁）
        if recording_started and self._pending_mode == RecorderMode.SNAPSHOT and self._auto_enable_realtime and self._analyzer_service:
            # 获取录制 ID
            record_id = self._current_session.session_id if self._current_session else ""
            self._analyzer_service.start_realtime_analysis(record_id)
            self.logger.info(f"SNAPSHOT mode: Started realtime analysis for recording {record_id}")

        return recording_started

    def stop_recording(self) -> bool:
        """
        停止录制

        Returns:
            bool: 成功返回True
        """
        with self._lock:
            if self._current_session is None:
                self.logger.warning("No active recording session")
                return False

            if self._api is None:
                self.logger.error("Recorder API not initialized")
                return False

            try:
                # 优先尝试优雅停止，等待 AI 分析和关键帧同步
                # 注意：必须先 graceful_stop 发送 STOP_SIGNAL，让 AI 处理完剩余帧
                # 然后再停止 AI 分析，否则 AI 收不到信号，关键帧无法编码
                if hasattr(self._api, 'graceful_stop'):
                    self.logger.info("🎬 Stopping recording gracefully...")
                    self._api.graceful_stop(5000)  # 5秒超时
                else:
                    self._api.stop()

                # 停止实时分析（在 graceful_stop 之后）
                if self._analyzer_service and self._analyzer_service.is_realtime_mode():
                    self._analyzer_service.stop_realtime_analysis()
                    self.logger.info("Stopped realtime analysis")

                # 更新会话
                self._current_session.end_time = datetime.now()

                # 更新统计信息
                stats = self._api.stats
                if hasattr(stats, 'to_dict'):
                    self._current_session.stats = stats.to_dict()
                else:
                    self._current_session.stats = {
                        "frame_count": getattr(stats, 'frame_count', 0),
                        "encoded_count": getattr(stats, 'encoded_count', 0),
                        "dropped_count": getattr(stats, 'dropped_count', 0),
                        "file_size": getattr(stats, 'output_file_size', 0),
                    }

                session_info = self._current_session.to_dict()
                self.logger.info(f"Recording stopped: {self._current_session.output_path}")

                # 更新录制历史到数据库
                if self._history_service:
                    try:
                        self._history_service.update_recording(
                            record_id=self._current_session.session_id,
                            end_time=self._current_session.end_time,
                            file_size=session_info["stats"].get("file_size", 0),
                            duration=int(session_info["stats"].get("duration", 0)),
                            notes=f"Recorded in {'SNAPSHOT' if self._pending_mode == RecorderMode.SNAPSHOT else 'VIDEO'} mode"
                        )
                        self.logger.info(f"Updated recording history: {self._current_session.session_id}")
                    except Exception as e:
                        self.logger.error(f"Failed to update recording history: {e}")

                # 清空会话
                self._current_session = None

                return True

            except Exception as e:
                self.logger.error(f"Error stopping recording: {e}")
                return False

    def set_recording_mode(self, mode) -> bool:
        """
        设置录制模式（VIDEO 或 SNAPSHOT）

        Args:
            mode: RecorderMode.VIDEO 或 RecorderMode.SNAPSHOT

        Returns:
            bool: 设置成功返回True
        """
        if RecorderMode is None:
            self.logger.warning("RecorderMode not available, module may not be built yet")
            return False

        # 缓存模式，以便在API初始化后应用
        self._pending_mode = mode
        mode_name = "SNAPSHOT" if mode == RecorderMode.SNAPSHOT else "VIDEO"

        if self._api is None:
            self.logger.info(f"Capture mode set to {mode_name} (pending, API not initialized)")
            return True

        try:
            self._api.set_recording_mode(mode)
            self.logger.info(f"Recording mode set to: {mode_name}")

            # SNAPSHOT模式：自动启用实时分析
            if self._auto_enable_realtime and self._analyzer_service:
                if mode == RecorderMode.SNAPSHOT:
                    self._analyzer_service.start_realtime_analysis()
                    self.logger.info("📊 SNAPSHOT模式：启用实时分析")
                else:
                    # VIDEO模式：停止实时分析（后续使用离线分析）
                    self._analyzer_service.stop_realtime_analysis()
                    self.logger.info("📹 VIDEO模式：禁用实时分析")

            return True
        except Exception as e:
            self.logger.error(f"Failed to set recording mode: {e}")
            return False

    def pause_recording(self) -> bool:
        """
        暂停录制

        Returns:
            bool: 成功返回True
        """
        if self._api is None:
            return False

        try:
            self._api.pause()
            self.logger.info("Recording paused")
            return True
        except Exception as e:
            self.logger.error(f"Error pausing recording: {e}")
            return False

    def resume_recording(self) -> bool:
        """
        恢复录制

        Returns:
            bool: 成功返回True
        """
        if self._api is None:
            return False

        try:
            self._api.resume()
            self.logger.info("Recording resumed")
            return True
        except Exception as e:
            self.logger.error(f"Error resuming recording: {e}")
            return False

    def graceful_stop_recording(self, timeout_ms: int = 5000) -> bool:
        """
        优雅停止录制,等待AI分析完成

        Args:
            timeout_ms: 等待超时时间(毫秒),默认5000ms

        Returns:
            bool: 成功返回True
        """
        with self._lock:
            if self._current_session is None:
                self.logger.warning("No active recording session")
                return False

            if self._api is None:
                self.logger.error("Recorder API not initialized")
                return False

            try:
                self.logger.info(f"Gracefully stopping recording (timeout={timeout_ms}ms)...")
                
                # 调用优雅停止
                self._api.graceful_stop(timeout_ms)

                # 更新会话
                self._current_session.end_time = datetime.now()

                # 更新统计信息
                stats = self._api.stats
                if hasattr(stats, 'to_dict'):
                    self._current_session.stats = stats.to_dict()
                else:
                    self._current_session.stats = {
                        "frame_count": getattr(stats, 'frame_count', 0),
                        "encoded_count": getattr(stats, 'encoded_count', 0),
                        "dropped_count": getattr(stats, 'dropped_count', 0),
                        "file_size": getattr(stats, 'output_file_size', 0),
                    }

                session_info = self._current_session.to_dict()
                self.logger.info(f"Recording gracefully stopped: {self._current_session.output_path}")

                # 清空会话
                self._current_session = None

                return True

            except Exception as e:
                self.logger.error(f"Error gracefully stopping recording: {e}")
                return False

    def get_recording_info(self) -> Dict[str, Any]:
        """
        获取录制信息

        Returns:
            dict: 录制信息字典
        """
        if self._current_session is None:
            return {
                "is_recording": False,
                "frame_count": 0,
                "encoded_count": 0,
                "dropped_count": 0,
                "duration": 0.0,
                "file_size": 0,
                "current_fps": 0.0
            }

        try:
            if self._api is None or self._current_session is None or self._current_session.start_time is None:
                return {
                    "is_recording": self._current_session is not None,
                    "frame_count": 0,
                    "encoded_count": 0,
                    "dropped_count": 0,
                    "duration": 0.0,
                    "file_size": 0,
                    "current_fps": 0.0
                }

            stats = self._api.stats
            duration = (datetime.now() - self._current_session.start_time).total_seconds()

            return {
                "is_recording": True,
                "session_id": self._current_session.session_id,
                "output_path": self._current_session.output_path,
                "frame_count": getattr(stats, 'frame_count', 0),
                "encoded_count": getattr(stats, 'encoded_count', 0),
                "dropped_count": getattr(stats, 'dropped_count', 0),
                "duration": duration,
                "file_size": getattr(stats, 'output_file_size', 0),
                "current_fps": getattr(stats, 'current_fps', 0.0)
            }

        except Exception as e:
            self.logger.error(f"Error getting recording info: {e}")
            return {
                "is_recording": False,
                "error": str(e)
            }

    def get_status(self) -> ProcessStatus:
        """
        获取录制器状态

        Returns:
            ProcessStatus: 当前状态
        """
        return self.module_manager.get_recorder_status()

    def is_recording(self) -> bool:
        """
        检查是否正在录制

        Returns:
            bool: 正在录制返回True
        """
        return self._current_session is not None

    def get_api(self):
        """
        获取底层API实例

        Returns:
            RecorderAPI实例或None
        """
        return self._api

    def set_status_callback(self, callback: Callable):
        """设置状态变化回调"""
        if callback not in self._status_callbacks:
            self._status_callbacks.append(callback)

    def set_progress_callback(self, callback: Callable):
        """设置进度回调"""
        if callback not in self._progress_callbacks:
            self._progress_callbacks.append(callback)

    def set_error_callback(self, callback: Callable):
        """设置错误回调"""
        if callback not in self._error_callbacks:
            self._error_callbacks.append(callback)

    def shutdown(self):
        """关闭录制服务"""
        self.stop_recording()
        self.module_manager.shutdown_recorder()
        self._api = None
        self._current_session = None
        self.logger.info("RecorderService shutdown")
