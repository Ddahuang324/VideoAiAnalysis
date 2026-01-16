"""
关键要点数据访问对象
"""
import json
import uuid
from typing import List, Optional

from .database_manager import DatabaseManager
from .models import KeyFinding
from infrastructure.log_manager import get_logger


class KeyFindingDAO:
    """关键要点 DAO"""

    def __init__(self, db_manager: DatabaseManager):
        self.db = db_manager
        self.logger = get_logger("KeyFindingDAO")

    def create(self, finding: KeyFinding) -> str:
        if not finding.finding_id:
            finding.finding_id = str(uuid.uuid4())

        query = """
            INSERT INTO key_finding (
                finding_id, analysis_id, sequence_order, category,
                title, content, related_timestamps, confidence_score
            ) VALUES (?, ?, ?, ?, ?, ?, ?, ?)
        """
        params = (
            finding.finding_id, finding.analysis_id, finding.sequence_order,
            finding.category, finding.title, finding.content,
            json.dumps(finding.related_timestamps, ensure_ascii=False),
            finding.confidence_score
        )
        self.db.execute_update(query, params)
        return finding.finding_id

    def get_by_id(self, finding_id: str) -> Optional[KeyFinding]:
        query = "SELECT * FROM key_finding WHERE finding_id = ?"
        results = self.db.execute_query(query, (finding_id,))
        return self._row_to_finding(results[0]) if results else None

    def get_by_analysis_id(self, analysis_id: str) -> List[KeyFinding]:
        query = "SELECT * FROM key_finding WHERE analysis_id = ? ORDER BY sequence_order"
        results = self.db.execute_query(query, (analysis_id,))
        return [self._row_to_finding(row) for row in results]

    def get_by_category(self, analysis_id: str, category: str) -> List[KeyFinding]:
        query = "SELECT * FROM key_finding WHERE analysis_id = ? AND category = ? ORDER BY sequence_order"
        results = self.db.execute_query(query, (analysis_id, category))
        return [self._row_to_finding(row) for row in results]

    def delete(self, finding_id: str) -> bool:
        return self.db.execute_update("DELETE FROM key_finding WHERE finding_id = ?", (finding_id,)) > 0

    def _row_to_finding(self, row) -> KeyFinding:
        return KeyFinding(
            finding_id=row['finding_id'],
            analysis_id=row['analysis_id'],
            sequence_order=row['sequence_order'],
            category=row['category'],
            title=row['title'],
            content=row['content'],
            related_timestamps=json.loads(row['related_timestamps']),
            confidence_score=row['confidence_score']
        )
