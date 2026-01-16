"""
AI 分析结果数据访问对象
"""
import uuid
from datetime import datetime
from typing import List, Optional

from .database_manager import DatabaseManager
from .models import AIAnalysis
from infrastructure.log_manager import get_logger


class AIAnalysisDAO:
    """AI 分析结果 DAO"""

    def __init__(self, db_manager: DatabaseManager):
        self.db = db_manager
        self.logger = get_logger("AIAnalysisDAO")

    def create(self, analysis: AIAnalysis) -> str:
        if not analysis.analysis_id:
            analysis.analysis_id = str(uuid.uuid4())
        if not analysis.started_at:
            analysis.started_at = datetime.now().isoformat()

        query = """
            INSERT INTO ai_analysis (
                analysis_id, keyframe_id, recording_id, prompt_id, analysis_type, model_name,
                model_version, status, video_analysis_md, audio_analysis_md,
                summary_md, rendered_html, started_at, completed_at, processing_time_ms, error_message
            ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        """
        params = (
            analysis.analysis_id, analysis.keyframe_id or None, analysis.recording_id or None,
            analysis.prompt_id or None, analysis.analysis_type, analysis.model_name, analysis.model_version,
            analysis.status, analysis.video_analysis_md, analysis.audio_analysis_md,
            analysis.summary_md, analysis.rendered_html, analysis.started_at, analysis.completed_at,
            analysis.processing_time_ms, analysis.error_message
        )
        self.db.execute_update(query, params)
        self.logger.info(f"Created AI analysis: {analysis.analysis_id}")
        return analysis.analysis_id

    def get_by_id(self, analysis_id: str) -> Optional[AIAnalysis]:
        query = "SELECT * FROM ai_analysis WHERE analysis_id = ?"
        results = self.db.execute_query(query, (analysis_id,))
        return self._row_to_analysis(results[0]) if results else None

    def get_by_keyframe_id(self, keyframe_id: str) -> List[AIAnalysis]:
        query = "SELECT * FROM ai_analysis WHERE keyframe_id = ? ORDER BY started_at DESC"
        results = self.db.execute_query(query, (keyframe_id,))
        return [self._row_to_analysis(row) for row in results]

    def get_all(self, limit: int = 100, offset: int = 0) -> List[AIAnalysis]:
        query = "SELECT * FROM ai_analysis ORDER BY started_at DESC LIMIT ? OFFSET ?"
        results = self.db.execute_query(query, (limit, offset))
        return [self._row_to_analysis(row) for row in results]

    def update_status(self, analysis_id: str, status: str, error_message: str = "") -> bool:
        completed_at = datetime.now().isoformat() if status in ("completed", "failed") else ""
        query = "UPDATE ai_analysis SET status = ?, error_message = ?, completed_at = ? WHERE analysis_id = ?"
        return self.db.execute_update(query, (status, error_message, completed_at, analysis_id)) > 0

    def update_results(self, analysis_id: str, video_analysis_md: str = "",
                       audio_analysis_md: str = "", summary_md: str = "") -> bool:
        query = """
            UPDATE ai_analysis SET
                video_analysis_md = ?, audio_analysis_md = ?, summary_md = ?
            WHERE analysis_id = ?
        """
        return self.db.execute_update(query, (video_analysis_md, audio_analysis_md, summary_md, analysis_id)) > 0

    def delete(self, analysis_id: str) -> bool:
        affected = self.db.execute_update("DELETE FROM ai_analysis WHERE analysis_id = ?", (analysis_id,))
        if affected > 0:
            self.logger.info(f"Deleted AI analysis: {analysis_id}")
        return affected > 0

    def update_rendered_html(self, analysis_id: str, rendered_html: str) -> bool:
        """更新渲染后的 HTML"""
        query = "UPDATE ai_analysis SET rendered_html = ? WHERE analysis_id = ?"
        return self.db.execute_update(query, (rendered_html, analysis_id)) > 0

    def get_by_recording_id(self, recording_id: str) -> List[AIAnalysis]:
        query = "SELECT * FROM ai_analysis WHERE recording_id = ? ORDER BY started_at DESC"
        results = self.db.execute_query(query, (recording_id,))
        return [self._row_to_analysis(row) for row in results]

    def _row_to_analysis(self, row) -> AIAnalysis:
        return AIAnalysis(
            analysis_id=row['analysis_id'],
            keyframe_id=row['keyframe_id'],
            recording_id=row['recording_id'] if 'recording_id' in row.keys() else "",
            prompt_id=row['prompt_id'] or "",
            analysis_type=row['analysis_type'],
            model_name=row['model_name'],
            model_version=row['model_version'],
            status=row['status'],
            video_analysis_md=row['video_analysis_md'],
            audio_analysis_md=row['audio_analysis_md'],
            summary_md=row['summary_md'],
            rendered_html=row['rendered_html'] if 'rendered_html' in row.keys() else "",
            started_at=row['started_at'] or "",
            completed_at=row['completed_at'] or "",
            processing_time_ms=row['processing_time_ms'],
            error_message=row['error_message']
        )
