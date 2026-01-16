"""
时间戳事件数据访问对象
"""
import json
import uuid
from typing import List, Optional

from .database_manager import DatabaseManager
from .models import TimestampEvent
from infrastructure.log_manager import get_logger


class TimestampEventDAO:
    """时间戳事件 DAO"""

    def __init__(self, db_manager: DatabaseManager):
        self.db = db_manager
        self.logger = get_logger("TimestampEventDAO")

    def create(self, event: TimestampEvent) -> str:
        if not event.event_id:
            event.event_id = str(uuid.uuid4())

        query = """
            INSERT INTO timestamp_event (
                event_id, analysis_id, timestamp_seconds, event_type,
                title, description, thumbnail_path, importance_score, metadata
            ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
        """
        params = (
            event.event_id, event.analysis_id, event.timestamp_seconds,
            event.event_type, event.title, event.description,
            event.thumbnail_path, event.importance_score,
            json.dumps(event.metadata, ensure_ascii=False)
        )
        self.db.execute_update(query, params)
        return event.event_id

    def get_by_id(self, event_id: str) -> Optional[TimestampEvent]:
        query = "SELECT * FROM timestamp_event WHERE event_id = ?"
        results = self.db.execute_query(query, (event_id,))
        return self._row_to_event(results[0]) if results else None

    def get_by_analysis_id(self, analysis_id: str) -> List[TimestampEvent]:
        query = "SELECT * FROM timestamp_event WHERE analysis_id = ? ORDER BY timestamp_seconds"
        results = self.db.execute_query(query, (analysis_id,))
        return [self._row_to_event(row) for row in results]

    def get_by_importance(self, analysis_id: str, min_score: int = 7) -> List[TimestampEvent]:
        query = """
            SELECT * FROM timestamp_event
            WHERE analysis_id = ? AND importance_score >= ?
            ORDER BY importance_score DESC
        """
        results = self.db.execute_query(query, (analysis_id, min_score))
        return [self._row_to_event(row) for row in results]

    def delete(self, event_id: str) -> bool:
        return self.db.execute_update("DELETE FROM timestamp_event WHERE event_id = ?", (event_id,)) > 0

    def _row_to_event(self, row) -> TimestampEvent:
        return TimestampEvent(
            event_id=row['event_id'],
            analysis_id=row['analysis_id'],
            timestamp_seconds=row['timestamp_seconds'],
            event_type=row['event_type'],
            title=row['title'],
            description=row['description'],
            thumbnail_path=row['thumbnail_path'],
            importance_score=row['importance_score'],
            metadata=json.loads(row['metadata'])
        )
