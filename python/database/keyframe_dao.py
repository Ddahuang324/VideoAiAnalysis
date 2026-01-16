"""
关键帧视频数据访问对象
"""
import json
import uuid
from datetime import datetime
from typing import List, Optional

from .database_manager import DatabaseManager
from .models import KeyFrameVideo
from infrastructure.log_manager import get_logger


class KeyFrameVideoDAO:
    """关键帧视频 DAO"""

    def __init__(self, db_manager: DatabaseManager):
        self.db = db_manager
        self.logger = get_logger("KeyFrameVideoDAO")

    def create(self, keyframe: KeyFrameVideo) -> str:
        if not keyframe.keyframe_id:
            keyframe.keyframe_id = str(uuid.uuid4())
        if not keyframe.created_at:
            keyframe.created_at = datetime.now().isoformat()

        query = """
            INSERT INTO keyframe_video (
                keyframe_id, recording_id, keyframe_video_path, keyframe_audio_path,
                keyframe_count, duration_seconds, file_size_bytes,
                compression_ratio, created_at, extraction_config
            ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        """
        params = (
            keyframe.keyframe_id, keyframe.recording_id,
            keyframe.keyframe_video_path, keyframe.keyframe_audio_path,
            keyframe.keyframe_count, keyframe.duration_seconds,
            keyframe.file_size_bytes, keyframe.compression_ratio,
            keyframe.created_at, json.dumps(keyframe.extraction_config, ensure_ascii=False)
        )
        self.db.execute_update(query, params)
        self.logger.info(f"Created keyframe video: {keyframe.keyframe_id}")
        return keyframe.keyframe_id

    def get_by_id(self, keyframe_id: str) -> Optional[KeyFrameVideo]:
        query = "SELECT * FROM keyframe_video WHERE keyframe_id = ?"
        results = self.db.execute_query(query, (keyframe_id,))
        return self._row_to_keyframe(results[0]) if results else None

    def get_by_recording_id(self, recording_id: str) -> List[KeyFrameVideo]:
        query = "SELECT * FROM keyframe_video WHERE recording_id = ? ORDER BY created_at DESC"
        results = self.db.execute_query(query, (recording_id,))
        return [self._row_to_keyframe(row) for row in results]

    def get_all(self, limit: int = 100, offset: int = 0) -> List[KeyFrameVideo]:
        query = "SELECT * FROM keyframe_video ORDER BY created_at DESC LIMIT ? OFFSET ?"
        results = self.db.execute_query(query, (limit, offset))
        return [self._row_to_keyframe(row) for row in results]

    def delete(self, keyframe_id: str) -> bool:
        affected = self.db.execute_update("DELETE FROM keyframe_video WHERE keyframe_id = ?", (keyframe_id,))
        if affected > 0:
            self.logger.info(f"Deleted keyframe video: {keyframe_id}")
        return affected > 0

    def _row_to_keyframe(self, row) -> KeyFrameVideo:
        return KeyFrameVideo(
            keyframe_id=row['keyframe_id'],
            recording_id=row['recording_id'],
            keyframe_video_path=row['keyframe_video_path'],
            keyframe_audio_path=row['keyframe_audio_path'],
            keyframe_count=row['keyframe_count'],
            duration_seconds=row['duration_seconds'],
            file_size_bytes=row['file_size_bytes'],
            compression_ratio=row['compression_ratio'],
            created_at=row['created_at'],
            extraction_config=json.loads(row['extraction_config'])
        )
