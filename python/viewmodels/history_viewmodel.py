"""
历史记录视图模型
负责历史记录管理和UI交互
"""
from PySide6.QtCore import QObject, Signal, Slot, Property, QThread
from typing import List, Dict, Any, Optional
from datetime import datetime

from services.history_service import HistoryService, RecordingRecord, AnalysisRecord
from services.markdown_service import MarkdownService
from infrastructure.log_manager import get_logger


class MarkdownWorker(QThread):
    """后台线程渲染 Markdown"""
    finished = Signal(str, str)  # record_id, html

    def __init__(self, record_id: str, markdown: str, service: MarkdownService):
        super().__init__()
        self._record_id = record_id
        self._markdown = markdown
        self._service = service

    def run(self):
        html = self._service.render(self._markdown)
        self.finished.emit(self._record_id, html)


class AIAnalysisWorker(QThread):
    """后台线程执行 AI 分析"""
    finished = Signal(str, str, dict)  # record_id, markdown_result, raw_result
    error = Signal(str, str)  # record_id, error_message

    def __init__(self, record_id: str, video_path: str, prompt: str, gemini_service):
        super().__init__()
        self._record_id = record_id
        self._video_path = video_path
        self._prompt = prompt
        self._gemini_service = gemini_service

    def run(self):
        try:
            result = self._gemini_service.analyze_video(self._video_path, self._prompt)
            if not result:
                self.error.emit(self._record_id, "AI analysis returned empty result")
                return

            # 如果结果是字典（新的 JSON 格式），将其转换为完整的 Markdown
            if isinstance(result, dict):
                md = []
                
                # 1. 摘要
                if result.get("summary_md"):
                    md.append(f"# 视频分析摘要\n\n{result['summary_md']}\n")
                
                # 2. 详细分析
                if result.get("video_analysis_md"):
                    md.append(f"## 画面详细分析\n\n{result['video_analysis_md']}\n")
                
                if result.get("audio_analysis_md"):
                    md.append(f"## 音频详细分析\n\n{result['audio_analysis_md']}\n")
                
                # 4. 时间轴事件和关键发现 (不再合并到 Markdown，由 UI 独立显示)
                
                final_md = "\n".join(md)
                self.finished.emit(self._record_id, final_md, result)
            
            # 兼容旧的字符串格式
            elif isinstance(result, str):
                self.finished.emit(self._record_id, result, {"summary_md": result})
            else:
                self.error.emit(self._record_id, "AI analysis returned unknown result format")
                
        except Exception as e:
            # 获取 logger
            import logging
            logger = logging.getLogger("AIAnalysisWorker")
            logger.error(f"Worker thread error: {e}")
            self.error.emit(self._record_id, str(e))


class HistoryViewModel(QObject):
    """
    历史记录视图模型
    负责历史记录管理和UI交互
    """

    # ========== 信号定义 ==========
    historyListChanged = Signal()
    totalCountChanged = Signal(int)
    errorOccurred = Signal(str)
    recordDeleted = Signal(str)
    statisticsChanged = Signal()
    analysisHtmlChanged = Signal()
    # 处理状态信号
    processingStarted = Signal(str)   # record_id
    processingCompleted = Signal(str) # record_id

    def __init__(self, history_service: HistoryService, gemini_service=None):
        super().__init__()
        self._service = history_service
        self._markdown_service = MarkdownService()
        self._gemini_service = gemini_service
        self.logger = get_logger("HistoryViewModel")

        # 数据
        self._history_list: List[RecordingRecord] = []
        self._filtered_list: List[RecordingRecord] = []
        self._search_filter = ""
        self._total_count = 0
        self._current_analysis_html = ""

        # 统计信息缓存
        self._statistics: Dict[str, Any] = {}

        # 缓存已渲染的 HTML
        self._html_cache: Dict[str, str] = {}
        # 正在处理的记录
        self._processing_ids: set = set()
        # 当前正在查看的记录 ID
        self._current_record_id: str = ""
        # 当前工作线程
        self._worker: Optional[MarkdownWorker] = None
        self._ai_worker: Optional[AIAnalysisWorker] = None
        # 防抖标志，防止信号循环
        self._loading_in_progress = False

        self.logger.info("HistoryViewModel initialized")

    @Slot()
    def loadHistory(self):
        """加载历史记录"""
        # 防抖：防止信号循环导致重复调用
        if self._loading_in_progress:
            self.logger.debug("loadHistory() skipped - already in progress")
            return

        self._loading_in_progress = True
        try:
            records = self._service.get_all_recordings()
            # 按开始时间降序排序（最新的在前）
            records.sort(key=lambda r: r.start_time, reverse=True)
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
        finally:
            self._loading_in_progress = False

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

    @Slot(str, result=bool)
    def toggleFavorite(self, record_id: str) -> bool:
        """切换收藏状态"""
        try:
            return self._service.toggle_favorite(record_id)
        except Exception as e:
            self.errorOccurred.emit(f"Failed to toggle favorite: {e}")
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

    @Property(str, notify=analysisHtmlChanged)
    def currentAnalysisHtml(self) -> str:
        """当前分析结果的 HTML"""
        return self._current_analysis_html

    @Slot(str, result=bool)
    def isProcessing(self, record_id: str) -> bool:
        """检查记录是否正在处理中"""
        return record_id in self._processing_ids

    @Slot(str, result=bool)
    def isReady(self, record_id: str) -> bool:
        """检查记录是否已准备好（已缓存）"""
        return record_id in self._html_cache

    def _on_render_finished(self, record_id: str, html: str):
        """渲染完成回调"""
        self._html_cache[record_id] = html
        self._processing_ids.discard(record_id)
        # 持久化渲染结果到数据库
        self._service.save_rendered_html(record_id, html)
        self.processingCompleted.emit(record_id)
        # 只有当渲染的是当前查看的记录时才更新全局 HTML
        if record_id == self._current_record_id:
            self._current_analysis_html = html
            self.analysisHtmlChanged.emit()

    @Slot(str)
    def preloadAnalysisContent(self, record_id: str):
        """预加载分析内容到缓存（后台执行）"""
        if record_id in self._html_cache or record_id in self._processing_ids:
            return
        try:
            # 使用新的统一接口从数据库获取
            details = self._service.get_analysis_details(record_id)
            raw_md = details.get("videoAnalysisMd", "")
            
            if not raw_md:
                # 回退：尝试旧的 JSON 缓存方式
                analyses = self._service.get_analyses_for_recording(record_id)
                if analyses:
                    latest = analyses[-1]
                    raw_md = latest.results.get("markdown", "") if isinstance(latest.results, dict) else str(latest.results)
            
            if raw_md:
                self._processing_ids.add(record_id)
                self.processingStarted.emit(record_id)
                self._worker = MarkdownWorker(record_id, raw_md, self._markdown_service)
                self._worker.finished.connect(self._on_render_finished)
                self._worker.start()
            else:
                self.logger.warning(f"No markdown content found for {record_id}")
        except Exception as e:
            self.logger.error(f"Failed to preload: {e}")


    @Slot(str)
    def loadAnalysisContent(self, record_id: str):
        """加载分析内容（优先使用缓存，其次数据库，最后重新渲染）"""
        # 更新当前查看的记录 ID
        self._current_record_id = record_id
        # 已缓存，直接使用
        if record_id in self._html_cache:
            self._current_analysis_html = self._html_cache[record_id]
            self.analysisHtmlChanged.emit()
            return
        # 正在处理中，等待完成
        if record_id in self._processing_ids:
            return
        # 尝试从数据库加载已渲染的 HTML
        saved_html = self._service.get_rendered_html(record_id)
        if saved_html:
            self._html_cache[record_id] = saved_html
            self._current_analysis_html = saved_html
            self.analysisHtmlChanged.emit()
            return
        # 启动预加载（重新渲染）
        self.preloadAnalysisContent(record_id)

    @Slot(str)
    def setAnalysisMarkdown(self, markdown: str):
        """
        直接设置 Markdown 内容并渲染

        Args:
            markdown: Markdown 文本
        """
        self._current_analysis_html = self._markdown_service.render(markdown)
        self.analysisHtmlChanged.emit()

    @Slot(str, result="QVariant")
    def getAnalysisDetails(self, record_id: str) -> Dict[str, Any]:
        """获取分析详情（标题、时间戳、关键发现、参数）"""
        try:
            return self._service.get_analysis_details(record_id)
        except Exception as e:
            self.logger.error(f"Failed to get analysis details: {e}")
            return {"title": "", "subtitle": "", "timestamps": [], "keyFindings": [], "parameters": []}

    @Slot(str, str, str)
    def startAIAnalysis(self, record_id: str, video_path: str, prompt: str):
        """启动后台AI分析"""
        if not self._gemini_service:
            self.logger.error("GeminiService not available")
            return
        if record_id in self._processing_ids:
            return

        self._processing_ids.add(record_id)
        self.processingStarted.emit(record_id)
        self.historyListChanged.emit()

        self._ai_worker = AIAnalysisWorker(record_id, video_path, prompt, self._gemini_service)
        self._ai_worker.finished.connect(self._on_ai_analysis_finished)
        self._ai_worker.error.connect(self._on_ai_analysis_error)
        self._ai_worker.start()
        self.logger.info(f"Started AI analysis for {record_id}")

    def _on_ai_analysis_finished(self, record_id: str, markdown_result: str, raw_result: dict = None):
        """AI分析完成回调"""
        self._processing_ids.discard(record_id)
        
        # 保存结构化分析结果
        if raw_result:
            self.logger.info(f"Saving structured AI analysis result for {record_id}")
            self._service.save_ai_analysis_result(record_id, raw_result)
        else:
            # 回退到旧的简单保存方式
            self.saveAnalysisResult(record_id, markdown_result)
            
        self.loadHistory()
        self.processingCompleted.emit(record_id)
        self.preloadAnalysisContent(record_id)
        self.logger.info(f"AI analysis finished for {record_id}")

    def _on_ai_analysis_error(self, record_id: str, error_msg: str):
        """AI分析错误回调"""
        self._processing_ids.discard(record_id)
        self.errorOccurred.emit(f"AI analysis failed: {error_msg}")
        self.historyListChanged.emit()
        self.logger.error(f"AI analysis error for {record_id}: {error_msg}")

    @Slot(str, str, result=bool)
    def saveAnalysisResult(self, recording_id: str, markdown_result: str) -> bool:
        """
        保存 AI 分析结果并刷新列表
        保存 AI 分析结果并刷新列表 (此方法已废弃，请使用 _on_ai_analysis_finished 中的逻辑)

        Args:
            recording_id: 关联的录制记录ID
            markdown_result: Markdown 格式的分析结果

        Returns:
            bool: 成功返回 True
        """
        self.logger.warning(f"saveAnalysisResult is deprecated. Saving markdown for {recording_id}.")
        try:
            from datetime import datetime
            self._service.add_analysis(
                recording_id=recording_id,
                start_time=datetime.now(),
                end_time=datetime.now(),
                keyframe_count=0,
                analyzed_frames=0,
                results={"markdown": markdown_result}
            )
            # 刷新列表
            self.loadHistory()
            self.logger.info(f"Analysis result saved for recording: {recording_id}")
            return True
        except Exception as e:
            self.logger.error(f"Failed to save analysis result: {e}")
            self.errorOccurred.emit(f"保存分析结果失败: {e}")
            return False
