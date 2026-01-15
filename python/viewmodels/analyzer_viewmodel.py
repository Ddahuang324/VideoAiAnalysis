"""
分析视图模型
负责关键帧分析状态管理和UI交互
"""
from PySide6.QtCore import QObject, Signal, Slot, Property, QTimer
from typing import List, Dict, Any, Optional
from datetime import datetime

from services.analyzer_service import AnalyzerService, KeyFrameResult
from infrastructure.process_manager import ModuleManager, ProcessStatus
from infrastructure.log_manager import get_logger


class AnalyzerViewModel(QObject):
    """
    分析视图模型
    负责关键帧分析状态管理和UI交互
    """

    # ========== 信号定义 ==========
    # 状态信号
    statusChanged = Signal(str)
    isRunningChanged = Signal(bool)

    # 关键帧信号
    keyframeCountChanged = Signal(int)
    newKeyFrameDetected = Signal(dict)  # 新关键帧检测到
    keyframesUpdated = Signal()

    # 统计信号
    receivedFrameCountChanged = Signal(int)
    analyzedFrameCountChanged = Signal(int)
    avgProcessingTimeChanged = Signal(float)

    # 错误信号
    errorOccurred = Signal(str)

    # 配置信号
    detectorConfigChanged = Signal()

    def __init__(
        self,
        analyzer_service: AnalyzerService,
        module_manager: ModuleManager
    ):
        """
        初始化分析视图模型

        Args:
            analyzer_service: 分析服务实例
            module_manager: 模块管理器实例
        """
        super().__init__()
        self._service = analyzer_service
        self._module_manager = module_manager
        self.logger = get_logger("AnalyzerViewModel")

        # 状态属性
        self._status = "Ready"
        self._is_running = False

        # 统计属性
        self._keyframe_count = 0
        self._received_frame_count = 0
        self._analyzed_frame_count = 0
        self._avg_processing_time = 0.0

        # 关键帧结果缓存
        self._keyframe_results: List[KeyFrameResult] = []

        # 检测器配置
        self._detector_config = {
            "scene_change": {"enabled": True, "threshold": 0.85},
            "motion": {"enabled": True, "threshold": 0.5},
            "text": {"enabled": False, "threshold": 0.7}
        }

        # 定时器 - 用于更新分析统计
        self._update_timer = QTimer()
        self._update_timer.timeout.connect(self._update_stats)
        self._update_timer.setInterval(2000)  # 每2秒更新

        # 设置服务回调
        self._setup_service_callbacks()

        self.logger.info("AnalyzerViewModel initialized")

    def _setup_service_callbacks(self):
        """设置服务回调"""
        def on_status_change(status):
            """处理状态变化"""
            if hasattr(status, 'name'):
                self._status = status.name
                self.statusChanged.emit(self._status)

                # 更新运行状态
                if 'RUNNING' in status.name:
                    self._is_running = True
                    self.isRunningChanged.emit(True)
                elif 'IDLE' in status.name or 'STOPPED' in status.name:
                    self._is_running = False
                    self.isRunningChanged.emit(False)

        def on_keyframe(keyframe: KeyFrameResult):
            """处理关键帧检测"""
            self._keyframe_count += 1
            self.keyframeCountChanged.emit(self._keyframe_count)

            # 添加到结果列表
            self._keyframe_results.append(keyframe)

            # 发射新关键帧信号
            self.newKeyFrameDetected.emit(keyframe.to_dict())
            self.keyframesUpdated.emit()

            self.logger.debug(f"Keyframe detected: {keyframe.frame_index}")

        def on_error(error_msg):
            """处理错误"""
            self.errorOccurred.emit(error_msg)

        self._service.set_status_callback(on_status_change)
        self._service.set_keyframe_callback(on_keyframe)
        self._service.set_error_callback(on_error)

    @Slot()
    def initialize(self):
        """初始化分析服务"""
        try:
            if self._service.initialize():
                self._status = "Initialized"
                self.statusChanged.emit(self._status)
                self.logger.info("Analyzer service initialized")
            else:
                self._status = "Initialization failed"
                self.statusChanged.emit(self._status)
                self.errorOccurred.emit("Failed to initialize analyzer service")
        except Exception as e:
            self._status = "Error"
            self.statusChanged.emit(self._status)
            self.errorOccurred.emit(f"Initialization error: {e}")
            self.logger.error(f"Initialization error: {e}")

    @Slot()
    def startAnalysis(self):
        """开始分析"""
        if self._is_running:
            self.errorOccurred.emit("Analysis already in progress")
            return

        try:
            # 清空之前的结果
            self._keyframe_results.clear()
            self._keyframe_count = 0
            self.keyframeCountChanged.emit(0)

            if self._service.start_analysis():
                self._is_running = True
                self.isRunningChanged.emit(True)
                self._status = "Analyzing..."
                self.statusChanged.emit(self._status)

                # 启动统计更新定时器
                self._update_timer.start()

                self.logger.info("Analysis started")
            else:
                api = self._service.get_api()
                if api:
                    error_msg = api.last_error
                else:
                    error_msg = "Unknown error"
                self.errorOccurred.emit(f"Failed to start analysis: {error_msg}")
        except Exception as e:
            self.errorOccurred.emit(f"Start analysis error: {e}")
            self.logger.error(f"Start analysis error: {e}")

    @Slot()
    def stopAnalysis(self):
        """停止分析"""
        if not self._is_running:
            return

        try:
            if self._service.stop_analysis():
                self._is_running = False
                self.isRunningChanged.emit(False)

                # 停止统计更新定时器
                self._update_timer.stop()

                self._status = f"Stopped ({self._keyframe_count} keyframes)"
                self.statusChanged.emit(self._status)
                self.logger.info("Analysis stopped")
            else:
                self.errorOccurred.emit("Failed to stop analysis")
        except Exception as e:
            self.errorOccurred.emit(f"Stop analysis error: {e}")
            self.logger.error(f"Stop analysis error: {e}")

    @Slot(result=list)
    def getKeyFrameResults(self) -> List[Dict[str, Any]]:
        """
        获取关键帧结果列表

        Returns:
            List[Dict]: 关键帧结果字典列表
        """
        return [kf.to_dict() for kf in self._keyframe_results]

    @Slot(str, result=bool)
    def exportKeyFrames(self, output_path: str) -> bool:
        """
        导出关键帧到文件

        Args:
            output_path: 输出文件路径（JSON格式）

        Returns:
            bool: 成功返回True
        """
        try:
            import json
            from pathlib import Path

            data = {
                "export_time": datetime.now().isoformat(),
                "total_keyframes": self._keyframe_count,
                "keyframes": [kf.to_dict() for kf in self._keyframe_results]
            }

            output_file = Path(output_path)
            output_file.parent.mkdir(parents=True, exist_ok=True)

            with open(output_file, 'w', encoding='utf-8') as f:
                json.dump(data, f, indent=2, ensure_ascii=False)

            self._status = f"Exported {self._keyframe_count} keyframes to {output_file.name}"
            self.statusChanged.emit(self._status)
            self.logger.info(f"Keyframes exported to {output_file}")
            return True

        except Exception as e:
            self.errorOccurred.emit(f"Export error: {e}")
            self.logger.error(f"Export error: {e}")
            return False

    @Slot(str, bool)
    def setDetectorEnabled(self, detector: str, enabled: bool):
        """
        设置检测器开关

        Args:
            detector: 检测器名称 ("scene_change", "motion", "text")
            enabled: 是否启用
        """
        if detector not in self._detector_config:
            self.errorOccurred.emit(f"Unknown detector: {detector}")
            return

        self._detector_config[detector]["enabled"] = enabled
        self._service.set_detector_config(detector, enabled=enabled)
        self.detectorConfigChanged.emit()

        self._status = f"{detector} detector {'enabled' if enabled else 'disabled'}"
        self.statusChanged.emit(self._status)

    @Slot(str, float)
    def setDetectorThreshold(self, detector: str, value: float):
        """
        设置检测器阈值

        Args:
            detector: 检测器名称
            value: 阈值（0.0 - 1.0）
        """
        if detector not in self._detector_config:
            self.errorOccurred.emit(f"Unknown detector: {detector}")
            return

        if not 0.0 <= value <= 1.0:
            self.errorOccurred.emit("Threshold must be between 0.0 and 1.0")
            return

        self._detector_config[detector]["threshold"] = value
        self._service.set_detector_config(detector, threshold=value)
        self.detectorConfigChanged.emit()

    @Slot(str, result=bool)
    def isDetectorEnabled(self, detector: str) -> bool:
        """检查检测器是否启用"""
        return self._detector_config.get(detector, {}).get("enabled", False)

    @Slot(str, result=float)
    def getDetectorThreshold(self, detector: str) -> float:
        """获取检测器阈值"""
        return self._detector_config.get(detector, {}).get("threshold", 0.5)

    def _update_stats(self):
        """更新分析统计信息"""
        try:
            info = self._service.get_analysis_info()

            # 更新接收帧数
            new_received = info.get("received_frame_count", 0)
            if new_received != self._received_frame_count:
                self._received_frame_count = new_received
                self.receivedFrameCountChanged.emit(self._received_frame_count)

            # 更新分析帧数
            new_analyzed = info.get("analyzed_frame_count", 0)
            if new_analyzed != self._analyzed_frame_count:
                self._analyzed_frame_count = new_analyzed
                self.analyzedFrameCountChanged.emit(self._analyzed_frame_count)

            # 更新平均处理时间
            new_avg_time = info.get("avg_processing_time", 0.0)
            if abs(new_avg_time - self._avg_processing_time) > 0.001:
                self._avg_processing_time = new_avg_time
                self.avgProcessingTimeChanged.emit(self._avg_processing_time)

        except Exception as e:
            self.logger.error(f"Error updating stats: {e}")

    # ========== Properties ==========

    @Property(bool, notify=isRunningChanged)
    def isRunning(self) -> bool:
        """是否正在分析"""
        return self._is_running

    @Property(str, notify=statusChanged)
    def status(self) -> str:
        """当前状态"""
        return self._status

    @Property(int, notify=keyframeCountChanged)
    def keyframeCount(self) -> int:
        """关键帧数量"""
        return self._keyframe_count

    @Property(int, notify=receivedFrameCountChanged)
    def receivedFrameCount(self) -> int:
        """已接收帧数"""
        return self._received_frame_count

    @Property(int, notify=analyzedFrameCountChanged)
    def analyzedFrameCount(self) -> int:
        """已分析帧数"""
        return self._analyzed_frame_count

    @Property(float, notify=avgProcessingTimeChanged)
    def avgProcessingTime(self) -> float:
        """平均处理时间（毫秒）"""
        return self._avg_processing_time

    @Property(str, notify=keyframeCountChanged)
    def statusSummary(self) -> str:
        """状态摘要"""
        if self._is_running:
            return f"Analyzing... {self._keyframe_count} keyframes detected"
        elif self._keyframe_count > 0:
            return f"Finished - {self._keyframe_count} keyframes"
        else:
            return "Ready to analyze"

    def shutdown(self):
        """关闭视图模型"""
        self._update_timer.stop()
        self._service.shutdown()
        self.logger.info("AnalyzerViewModel shutdown")
