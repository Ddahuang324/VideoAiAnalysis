"""
历史记录视图模型
负责历史记录管理和UI交互
"""
from PySide6.QtCore import QObject, Signal, Slot, Property
from typing import List, Dict, Any, Optional
from datetime import datetime

from services.history_service import HistoryService, RecordingRecord, AnalysisRecord
from infrastructure.log_manager import get_logger


class HistoryViewModel(QObject):
    """
    历史记录视图模型
    负责历史记录管理和UI交互
    """

    # ========== 信号定义 ==========
    historyListChanged = Signal()
    totalCountChanged = Signal(int)
    errorOccurred = Signal(str)
    recordDeleted = Signal(str)  # 记录被删除
    statisticsChanged = Signal()

    def __init__(self, history_service: HistoryService):
        """
        初始化历史记录视图模型

        Args:
            history_service: 历史记录服务实例
        """
        super().__init__()
        self._service = history_service
        self.logger = get_logger("HistoryViewModel")

        # 数据
        self._history_list: List[RecordingRecord] = []
        self._filtered_list: List[RecordingRecord] = []
        self._search_filter = ""
        self._total_count = 0

        # 统计信息缓存
        self._statistics: Dict[str, Any] = {}

        self.logger.info("HistoryViewModel initialized")

    @Slot()
    def loadHistory(self):
        """加载历史记录"""
        try:
            records = self._service.get_all_recordings()
            self._history_list = records.copy()
            self._filtered_list = records.copy()
            self._total_count = len(records)

            # 更新统计信息
            self._update_statistics()

            self.historyListChanged.emit()
            self.totalCountChanged.emit(self._total_count)
            self.statisticsChanged.emit()

            self.logger.info(f"Loaded {self._total_count} history records")

        except Exception as e:
            self.errorOccurred.emit(f"Failed to load history: {e}")
            self.logger.error(f"Failed to load history: {e}")

    @Slot(str, result=bool)
    def deleteRecord(self, record_id: str) -> bool:
        """
        删除历史记录

        Args:
            record_id: 记录ID

        Returns:
            bool: 成功返回True
        """
        try:
            if self._service.delete_recording(record_id):
                # 从列表中移除
                self._history_list = [r for r in self._history_list if r.record_id != record_id]
                self._apply_filter()

                self._total_count = len(self._history_list)
                self.totalCountChanged.emit(self._total_count)
                self.recordDeleted.emit(record_id)

                # 更新统计信息
                self._update_statistics()
                self.statisticsChanged.emit()

                self.logger.info(f"Deleted record: {record_id}")
                return True

            return False

        except Exception as e:
            self.errorOccurred.emit(f"Failed to delete record: {e}")
            self.logger.error(f"Failed to delete record: {e}")
            return False

    @Slot(str)
    def searchRecords(self, keyword: str):
        """
        搜索历史记录

        Args:
            keyword: 搜索关键词（匹配文件名或备注）
        """
        self._search_filter = keyword.lower()
        self._apply_filter()

    @Slot()
    def clearFilter(self):
        """清除搜索过滤"""
        self._search_filter = ""
        self._apply_filter()

    @Slot(str, result=dict)
    def getRecordDetails(self, record_id: str) -> Dict[str, Any]:
        """
        获取记录详情

        Args:
            record_id: 记录ID

        Returns:
            dict: 记录详情字典
        """
        for record in self._history_list:
            if record.record_id == record_id:
                return record.to_dict()
        return {}

    @Slot(str, result=list)
    def getRecordAnalyses(self, record_id: str) -> List[Dict[str, Any]]:
        """
        获取记录关联的分析

        Args:
            record_id: 录制记录ID

        Returns:
            List[Dict]: 分析记录列表
        """
        try:
            analyses = self._service.get_analyses_for_recording(record_id)
            return [a.to_dict() for a in analyses]
        except Exception as e:
            self.logger.error(f"Failed to get analyses: {e}")
            return []

    @Slot(result=list)
    def getHistoryList(self) -> List[Dict[str, Any]]:
        """
        获取历史记录列表（转换为字典以便QML使用）

        Returns:
            List[Dict]: 历史记录字典列表
        """
        return [self._format_record(r) for r in self._filtered_list]

    @Slot(result=dict)
    def getStatistics(self) -> Dict[str, Any]:
        """获取统计信息"""
        return self._statistics.copy()

    @Slot(str, str, result=bool)
    def updateRecordNotes(self, record_id: str, notes: str) -> bool:
        """
        更新记录备注

        Args:
            record_id: 记录ID
            notes: 新备注

        Returns:
            bool: 成功返回True
        """
        try:
            if self._service.update_recording_notes(record_id, notes):
                # 更新内存中的记录
                for record in self._history_list:
                    if record.record_id == record_id:
                        record.notes = notes
                        break

                self.historyListChanged.emit()
                return True

            return False

        except Exception as e:
            self.errorOccurred.emit(f"Failed to update notes: {e}")
            return False

    def _apply_filter(self):
        """应用搜索过滤"""
        if not self._search_filter:
            self._filtered_list = self._history_list.copy()
        else:
            self._filtered_list = [
                r for r in self._history_list
                if self._search_filter in r.file_path.lower() or
                   self._search_filter in r.notes.lower()
            ]

        self.historyListChanged.emit()

    def _format_record(self, record: RecordingRecord) -> Dict[str, Any]:
        """
        格式化记录为QML友好的字典

        Args:
            record: 录制记录

        Returns:
            dict: 格式化后的字典
        """
        # 提取文件名
        file_name = record.file_path.split("/")[-1] if "/" in record.file_path else \
                   record.file_path.split("\\")[-1]

        # 格式化时间
        try:
            start_dt = datetime.fromisoformat(record.start_time)
            start_time_str = start_dt.strftime("%Y-%m-%d %H:%M:%S")
        except:
            start_time_str = record.start_time

        return {
            "recordId": record.record_id,
            "fileName": file_name,
            "filePath": record.file_path,
            "startTime": start_time_str,
            "duration": self._format_duration(record.duration),
            "durationSeconds": record.duration,
            "fileSize": self._format_file_size(record.file_size),
            "fileSizeBytes": record.file_size,
            "keyframeCount": record.keyframe_count,
            "thumbnailPath": record.thumbnail_path,
            "notes": record.notes
        }

    def _format_duration(self, seconds: int) -> str:
        """格式化时长"""
        hours = seconds // 3600
        minutes = (seconds % 3600) // 60
        secs = seconds % 60

        if hours > 0:
            return f"{hours:02d}:{minutes:02d}:{secs:02d}"
        else:
            return f"{minutes:02d}:{secs:02d}"

    def _format_file_size(self, bytes_size: int) -> str:
        """格式化文件大小"""
        size = float(bytes_size)
        for unit in ['B', 'KB', 'MB', 'GB']:
            if size < 1024.0:
                return f"{size:.2f} {unit}"
            size /= 1024.0
        return f"{size:.2f} TB"

    def _update_statistics(self):
        """更新统计信息"""
        try:
            stats = self._service.get_statistics()

            # 添加格式化信息
            stats["totalDurationFormatted"] = self._format_duration(stats["total_duration"])
            stats["totalSizeFormatted"] = self._format_file_size(stats["total_size"])
            stats["averageDurationFormatted"] = self._format_duration(int(stats["average_duration"]))
            stats["averageSizeFormatted"] = self._format_file_size(int(stats["average_size"]))

            self._statistics = stats

        except Exception as e:
            self.logger.error(f"Failed to update statistics: {e}")

    # ========== Properties ==========

    @Property(int, notify=totalCountChanged)
    def totalCount(self) -> int:
        """总记录数"""
        return self._total_count

    @Property(int, notify=historyListChanged)
    def filteredCount(self) -> int:
        """过滤后的记录数"""
        return len(self._filtered_list)

    @Property(str, notify=statisticsChanged)
    def summaryText(self) -> str:
        """统计摘要文本"""
        if not self._statistics:
            return "No records"

        total = self._statistics.get("total_recordings", 0)
        duration = self._statistics.get("totalDurationFormatted", "0:00")
        size = self._statistics.get("totalSizeFormatted", "0 B")

        return f"{total} recordings • {duration} • {size}"
