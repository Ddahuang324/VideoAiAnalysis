"""
分析服务
封装关键帧分析业务逻辑
"""
from typing import Optional, Callable, List, Dict, Any
from datetime import datetime
import threading
from dataclasses import dataclass, field

from infrastructure.process_manager import ModuleManager, ProcessStatus
from infrastructure.log_manager import get_logger


@dataclass
class KeyFrameResult:
    """关键帧结果数据类"""
    frame_index: int
    timestamp: float
    score: float
    detector_type: str  # "scene_change", "motion", "text"
    thumbnail_path: str = ""
    metadata: Dict[str, Any] = field(default_factory=dict)

    def to_dict(self) -> Dict[str, Any]:
        """转换为字典"""
        return {
            "frame_index": self.frame_index,
            "timestamp": self.timestamp,
            "score": self.score,
            "detector_type": self.detector_type,
            "thumbnail_path": self.thumbnail_path,
            "metadata": self.metadata
        }


@dataclass
class AnalysisSession:
    """分析会话数据类"""
    session_id: str
    start_time: datetime
    recording_id: str = ""
    end_time: Optional[datetime] = None
    keyframe_results: List[KeyFrameResult] = field(default_factory=list)
    stats: Dict[str, Any] = field(default_factory=dict)

    def to_dict(self) -> Dict[str, Any]:
        """转换为字典"""
        return {
            "session_id": self.session_id,
            "recording_id": self.recording_id,
            "start_time": self.start_time.isoformat(),
            "end_time": self.end_time.isoformat() if self.end_time else None,
            "keyframe_count": len(self.keyframe_results),
            "stats": self.stats.copy()
        }


class AnalyzerService:
    """
    分析业务服务
    封装关键帧分析业务逻辑
    """

    def __init__(self, module_manager: ModuleManager):
        """
        初始化分析服务

        Args:
            module_manager: 模块管理器实例
        """
        self.module_manager = module_manager
        self.logger = get_logger("AnalyzerService")

        # 分析器API
        self._api = None
        self._analyzer_module = None

        # 当前会话
        self._current_session: Optional[AnalysisSession] = None
        
        # 历史记录服务
        from services.history_service import HistoryService
        self._history_service: Optional[HistoryService] = None

        # 回调函数
        self._status_callbacks = []
        self._keyframe_callbacks = []
        self._error_callbacks = []
        self._keyframe_video_callbacks = []

        # 状态锁
        self._lock = threading.Lock()

        # 检测器配置
        self._detector_config = {
            "scene_change": {"enabled": True, "threshold": 0.85},
            "motion": {"enabled": True, "threshold": 0.5},
            "text": {"enabled": False, "threshold": 0.7}
        }

        # 实时分析模式状态
        self._realtime_running = False
        self._analysis_mode = None  # 将在导入模块后设置

    def initialize(self, config=None) -> bool:
        """
        初始化分析服务

        Args:
            config: AnalyzerConfig配置对象 (可选)

        Returns:
            bool: 初始化成功返回True
        """
        try:
            # 获取分析器模块
            self._analyzer_module = self.module_manager.get_analyzer_module()
            if self._analyzer_module is None:
                self.logger.error("Failed to get analyzer module")
                return False

            # 创建API实例
            if config is None:
                # 从配置文件加载配置
                config = self._analyzer_module.AnalyzerConfig()
                config_path = "configs/analyzer_config.json"
                try:
                    config.load_from_file(config_path)
                    self.logger.info(f"Loaded analyzer config from {config_path}")
                except Exception as e:
                    self.logger.warning(f"Failed to load config from {config_path}: {e}, using default config")
                    config = self._analyzer_module.default_analyzer_config()

            self._api = self.module_manager.create_analyzer_api(config)
            if self._api is None:
                self.logger.error("Failed to create analyzer API")
                return False

            # 设置内部回调
            self._setup_internal_callbacks()

            self.logger.info("AnalyzerService initialized successfully")
            return True

        except Exception as e:
            self.logger.error(f"Failed to initialize AnalyzerService: {e}")
            return False

    def _setup_internal_callbacks(self):
        """设置内部回调函数"""
        if self._api is None:
            return

        def on_status_change(status):
            """处理状态变化"""
            self.logger.debug(f"Analyzer status changed: {status}")

            # 更新模块管理器状态
            if hasattr(status, 'name'):
                if 'IDLE' in status.name:
                    self.module_manager.set_analyzer_status(ProcessStatus.STOPPED)
                elif 'RUNNING' in status.name:
                    self.module_manager.set_analyzer_status(ProcessStatus.RUNNING)
                elif 'ERROR' in status.name:
                    self.module_manager.set_analyzer_status(ProcessStatus.ERROR)

            # 通知外部回调
            for callback in self._status_callbacks:
                try:
                    callback(status)
                except Exception as e:
                    self.logger.error(f"Error in status callback: {e}")

        def on_keyframe(keyframe_data):
            """处理关键帧检测"""
            self.logger.debug(f"Keyframe detected: {keyframe_data}")

            # 解析关键帧数据
            try:
                if hasattr(keyframe_data, 'to_dict'):
                    data = keyframe_data.to_dict()
                else:
                    data = {
                        "frame_index": getattr(keyframe_data, 'frame_index', 0),
                        "timestamp": getattr(keyframe_data, 'timestamp', 0.0),
                        "score": getattr(keyframe_data, 'score', 0.0)
                    }

                # 创建结果对象
                result = KeyFrameResult(
                    frame_index=data.get("frame_index", 0),
                    timestamp=data.get("timestamp", 0.0),
                    score=data.get("score", 0.0),
                    detector_type=self._detect_detector_type(data)
                )

                # 添加到当前会话
                if self._current_session is not None:
                    self._current_session.keyframe_results.append(result)

                # 通知外部回调
                for callback in self._keyframe_callbacks:
                    try:
                        callback(result)
                    except Exception as e:
                        self.logger.error(f"Error in keyframe callback: {e}")

            except Exception as e:
                self.logger.error(f"Error processing keyframe data: {e}")

        def on_error(error_msg):
            """处理错误"""
            self.logger.error(f"Analyzer error: {error_msg}")
            for callback in self._error_callbacks:
                try:
                    callback(error_msg)
                except Exception as e:
                    self.logger.error(f"Error in error callback: {e}")

        self._api.set_status_callback(on_status_change)
        self._api.set_keyframe_callback(on_keyframe)

        def on_keyframe_video(video_path):
            """处理关键帧视频生成"""
            self.logger.info(f"Keyframe video generated: {video_path}")
            
            # 保存到数据库
            if self._history_service and self._current_session:
                try:
                    import os
                    file_size = os.path.getsize(video_path) if os.path.exists(video_path) else 0
                    keyframe_count = len(self._current_session.keyframe_results)
                    recording_id = self._current_session.recording_id
                    
                    # 尝试获取视频时长（此处简化处理，实际可能需要cv2读取）
                    duration = 0.0
                    if keyframe_count > 0:
                        # 假设每秒1帧或者类似逻辑，或者通过属性获取
                        pass

                    self._history_service.add_keyframe_video(
                        recording_id=recording_id,
                        video_path=video_path,
                        keyframe_count=keyframe_count,
                        duration=duration,
                        file_size=file_size,
                        extraction_config=self._detector_config
                    )
                except Exception as e:
                    self.logger.error(f"Failed to auto-save keyframe video: {e}")

            for callback in self._keyframe_video_callbacks:
                try:
                    callback(video_path)
                except Exception as e:
                    self.logger.error(f"Error in keyframe video callback: {e}")

        self._api.set_keyframe_video_callback(on_keyframe_video)

    def _detect_detector_type(self, data: Dict[str, Any]) -> str:
        """根据数据推断检测器类型"""
        # 这里可以根据实际数据结构判断
        # 简化处理，默认返回scene_change
        return "scene_change"

    def start_analysis(self, recording_id: str = "") -> bool:
        """
        开始实时分析 (ZMQ 订阅模式)

        Args:
            recording_id: 关联的录制记录ID

        Returns:
            bool: 成功返回True
        """
        with self._lock:
            if self._current_session is not None:
                self.logger.warning("Analysis already in progress")
                return False

            if self._api is None:
                self.logger.error("Analyzer API not initialized")
                return False

            try:
                # 创建新会话
                import uuid
                self._current_session = AnalysisSession(
                    session_id=str(uuid.uuid4()),
                    recording_id=recording_id,
                    start_time=datetime.now(),
                    stats={"mode": "realtime"}
                )

                # 启动分析
                result = self._api.start()

                if result:
                    self.logger.info("Real-time analysis started")
                    return True
                else:
                    self.logger.error(f"Failed to start analysis: {self._api.last_error}")
                    self._current_session = None
                    return False

            except Exception as e:
                self.logger.error(f"Error starting analysis: {e}")
                self._current_session = None
                return False

    def start_file_analysis(self, file_path: str) -> bool:
        """
        开始视频文件分析 (离线模式)

        Args:
            file_path: 视频文件的绝对路径

        Returns:
            bool: 成功启动返回True
        """
        with self._lock:
            if self._current_session is not None:
                self.logger.warning("Analysis already in progress")
                return False

            if self._api is None:
                self.logger.error("Analyzer API not initialized")
                return False

            try:
                # 创建新会话
                import uuid
                self._current_session = AnalysisSession(
                    session_id=str(uuid.uuid4()),
                    start_time=datetime.now(),
                    stats={"mode": "offline", "file_path": file_path}
                )

                # 调用底层 C++ 离线分析接口
                result = self._api.analyze_video_file(file_path)

                if result:
                    self.logger.info(f"Offline file analysis started: {file_path}")
                    return True
                else:
                    self.logger.error(f"Failed to start file analysis: {self._api.last_error}")
                    self._current_session = None
                    return False

            except Exception as e:
                self.logger.error(f"Error starting file analysis: {e}")
                self._current_session = None
                return False

    def stop_analysis(self) -> bool:
        """
        停止分析

        Returns:
            bool: 成功返回True
        """
        with self._lock:
            if self._current_session is None:
                self.logger.warning("No active analysis session")
                return False

            if self._api is None:
                self.logger.error("Analyzer API not initialized")
                return False

            try:
                # 停止分析
                self._api.stop()

                # 更新会话
                self._current_session.end_time = datetime.now()

                # 更新统计信息
                stats = self._api.stats
                if hasattr(stats, 'to_dict'):
                    self._current_session.stats = stats.to_dict()

                session_info = self._current_session.to_dict()
                self.logger.info(f"Analysis stopped: {session_info['keyframe_count']} keyframes detected")

                # 清空会话
                self._current_session = None

                return True

            except Exception as e:
                self.logger.error(f"Error stopping analysis: {e}")
                return False

    def get_keyframe_results(self) -> List[KeyFrameResult]:
        """
        获取关键帧结果列表

        Returns:
            List[KeyFrameResult]: 关键帧结果列表
        """
        if self._current_session is None:
            return []

        return self._current_session.keyframe_results.copy()

    def get_analysis_info(self) -> Dict[str, Any]:
        """
        获取分析信息

        Returns:
            dict: 分析信息字典
        """
        if self._current_session is None:
            return {
                "is_running": False,
                "keyframe_count": 0,
                "received_frame_count": 0,
                "analyzed_frame_count": 0,
                "duration": 0.0
            }

        try:
            stats = self._api.stats
            duration = (datetime.now() - self._current_session.start_time).total_seconds()

            return {
                "is_running": True,
                "session_id": self._current_session.session_id,
                "keyframe_count": len(self._current_session.keyframe_results),
                "received_frame_count": getattr(stats, 'received_frame_count', 0),
                "analyzed_frame_count": getattr(stats, 'analyzed_frame_count', 0),
                "avg_processing_time": getattr(stats, 'avg_processing_time', 0.0),
                "duration": duration
            }

        except Exception as e:
            self.logger.error(f"Error getting analysis info: {e}")
            return {
                "is_running": False,
                "error": str(e)
            }

    def get_status(self) -> ProcessStatus:
        """
        获取分析器状态

        Returns:
            ProcessStatus: 当前状态
        """
        return self.module_manager.get_analyzer_status()

    def is_running(self) -> bool:
        """
        检查是否正在分析

        Returns:
            bool: 正在分析返回True
        """
        return self._current_session is not None

    def get_api(self):
        """
        获取底层API实例

        Returns:
            AnalyzerAPI实例或None
        """
        return self._api

    def set_status_callback(self, callback: Callable):
        """设置状态变化回调"""
        if callback not in self._status_callbacks:
            self._status_callbacks.append(callback)

    def set_keyframe_callback(self, callback: Callable):
        """设置关键帧回调"""
        if callback not in self._keyframe_callbacks:
            self._keyframe_callbacks.append(callback)

    def set_error_callback(self, callback: Callable):
        """设置错误回调"""
        if callback not in self._error_callbacks:
            self._error_callbacks.append(callback)

    def set_keyframe_video_callback(self, callback: Callable):
        """设置关键帧视频生成回调"""
        if callback not in self._keyframe_video_callbacks:
            self._keyframe_video_callbacks.append(callback)

    def set_detector_config(self, detector: str, enabled: bool = None, threshold: float = None):
        """
        设置检测器配置

        Args:
            detector: 检测器名称 ("scene_change", "motion", "text")
            enabled: 是否启用
            threshold: 检测阈值
        """
        if detector not in self._detector_config:
            self.logger.warning(f"Unknown detector: {detector}")
            return

        if enabled is not None:
            self._detector_config[detector]["enabled"] = enabled

        if threshold is not None:
            self._detector_config[detector]["threshold"] = threshold

        self.logger.debug(f"Detector config updated: {detector} = {self._detector_config[detector]}")

    def get_detector_config(self, detector: str) -> Dict[str, Any]:
        """
        获取检测器配置

        Args:
            detector: 检测器名称

        Returns:
            dict: 检测器配置
        """
        return self._detector_config.get(detector, {}).copy()

    def start_realtime_analysis(self, recording_id: str = "") -> bool:
        """
        启动实时分析（ZMQ接收+分析）
        
        适合SNAPSHOT模式（1FPS），启动ZMQ帧接收和实时分析线程。
        
        Returns:
            bool: 成功返回True
        """
        with self._lock:
            if self._realtime_running:
                self.logger.warning("Realtime analysis already running")
                return True

            if self._api is None:
                self.logger.error("Analyzer API not initialized")
                return False

            try:
                # 创建新会话
                import uuid
                self._current_session = AnalysisSession(
                    session_id=str(uuid.uuid4()),
                    recording_id=recording_id,
                    start_time=datetime.now(),
                    stats={"mode": "realtime"}
                )

                result = self._api.start_realtime_analysis()
                
                if result:
                    self._realtime_running = True
                    if self._analyzer_module and hasattr(self._analyzer_module, 'AnalysisMode'):
                        self._analysis_mode = self._analyzer_module.AnalysisMode.REALTIME
                    self.logger.info(f"✅ Realtime analysis started for recording: {recording_id}")
                    return True
                else:
                    self.logger.error(f"Failed to start realtime analysis: {self._api.last_error}")
                    self._current_session = None
                    return False

            except Exception as e:
                self.logger.error(f"Error starting realtime analysis: {e}")
                self._current_session = None
                return False

    def stop_realtime_analysis(self):
        """
        停止实时分析
        
        停止ZMQ接收和分析线程。
        """
        with self._lock:
            if not self._realtime_running:
                return

            if self._api is None:
                return

            try:
                self._api.stop_realtime_analysis()
                self._realtime_running = False
                
                # 清理会话
                if self._current_session:
                    self._current_session.end_time = datetime.now()
                    session_info = self._current_session.to_dict()
                    self.logger.info(f"⏸️ Realtime analysis stopped. Session: {session_info['session_id']}")
                    self._current_session = None
                
                self.logger.info("✅ Realtime analysis stopped successfully")
            except Exception as e:
                self.logger.error(f"Error stopping realtime analysis: {e}")

    def is_realtime_mode(self) -> bool:
        """
        检查是否处于实时分析模式
        
        Returns:
            bool: 实时模式返回True
        """
        return self._realtime_running

    @property
    def analysis_mode(self):
        """获取当前分析模式"""
        if self._api and hasattr(self._api, 'get_analysis_mode'):
            try:
                return self._api.get_analysis_mode()
            except:
                pass
        return self._analysis_mode

    def shutdown(self):
        """关闭分析服务"""
        self.stop_analysis()
        self.module_manager.shutdown_analyzer()
        self._api = None
        self._current_session = None
        self.logger.info("AnalyzerService shutdown")
