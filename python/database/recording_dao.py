"""
录制记录数据访问对象
"""
import json
import uuid
from datetime import datetime
from typing import List, Optional

from .database_manager import DatabaseManager
from .models import Recording
from infrastructure.log_manager import get_logger


class RecordingDAO:
    """录制记录 DAO"""

    def __init__(self, db_manager: DatabaseManager):
        self.db = db_manager
        self.logger = get_logger("RecordingDAO")

    def create(self, recording: Recording) -> str:
        if not recording.record_id:
            recording.record_id = str(uuid.uuid4())
        now = datetime.now().isoformat()
        if not recording.created_at:
            recording.created_at = now
        recording.updated_at = now

        query = """
            INSERT INTO recording (
                record_id, original_video_path, title, description,
                duration_seconds, file_size_bytes, thumbnail_path,
                created_at, updated_at, tags, metadata
            ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        """
        params = (
            recording.record_id, recording.original_video_path,
            recording.title, recording.description,
            recording.duration_seconds, recording.file_size_bytes,
            recording.thumbnail_path, recording.created_at, recording.updated_at,
            json.dumps(recording.tags, ensure_ascii=False),
            json.dumps(recording.metadata, ensure_ascii=False)
        )
        self.db.execute_update(query, params)
        self.logger.info(f"Created recording: {recording.record_id}")
        return recording.record_id

    def get_by_id(self, record_id: str) -> Optional[Recording]:
        query = "SELECT * FROM recording WHERE record_id = ?"
        results = self.db.execute_query(query, (record_id,))
        return self._row_to_recording(results[0]) if results else None

    def get_by_path(self, video_path: str) -> Optional[Recording]:
        query = "SELECT * FROM recording WHERE original_video_path = ?"
        results = self.db.execute_query(query, (video_path,))
        return self._row_to_recording(results[0]) if results else None

    def get_all(self, limit: int = 100, offset: int = 0) -> List[Recording]:
        query = "SELECT * FROM recording ORDER BY created_at DESC LIMIT ? OFFSET ?"
        results = self.db.execute_query(query, (limit, offset))
        return [self._row_to_recording(row) for row in results]

    def update(self, recording: Recording) -> bool:
        recording.updated_at = datetime.now().isoformat()
        query = """
            UPDATE recording SET
                title = ?, description = ?, duration_seconds = ?,
                file_size_bytes = ?, thumbnail_path = ?, updated_at = ?,
                tags = ?, metadata = ?
            WHERE record_id = ?
        """
        params = (
            recording.title, recording.description, recording.duration_seconds,
            recording.file_size_bytes, recording.thumbnail_path, recording.updated_at,
            json.dumps(recording.tags, ensure_ascii=False),
            json.dumps(recording.metadata, ensure_ascii=False),
            recording.record_id
        )
        return self.db.execute_update(query, params) > 0

    def delete(self, record_id: str) -> bool:
        affected = self.db.execute_update("DELETE FROM recording WHERE record_id = ?", (record_id,))
        if affected > 0:
            self.logger.info(f"Deleted recording: {record_id}")
        return affected > 0

    def search_by_title(self, keyword: str) -> List[Recording]:
        query = "SELECT * FROM recording WHERE title LIKE ? ORDER BY created_at DESC"
        results = self.db.execute_query(query, (f"%{keyword}%",))
        return [self._row_to_recording(row) for row in results]

    def _row_to_recording(self, row) -> Recording:
        return Recording(
            record_id=row['record_id'],
            original_video_path=row['original_video_path'],
            title=row['title'],
            description=row['description'],
            duration_seconds=row['duration_seconds'],
            file_size_bytes=row['file_size_bytes'],
            thumbnail_path=row['thumbnail_path'],
            created_at=row['created_at'],
            updated_at=row['updated_at'],
            tags=json.loads(row['tags']),
            metadata=json.loads(row['metadata'])
        )
