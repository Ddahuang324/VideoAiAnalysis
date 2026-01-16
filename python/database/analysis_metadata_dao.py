"""
分析元数据数据访问对象
"""
import uuid
from typing import List, Optional

from .database_manager import DatabaseManager
from .models import AnalysisMetadata
from infrastructure.log_manager import get_logger


class AnalysisMetadataDAO:
    """分析元数据 DAO"""

    def __init__(self, db_manager: DatabaseManager):
        self.db = db_manager
        self.logger = get_logger("AnalysisMetadataDAO")

    def create(self, metadata: AnalysisMetadata) -> str:
        if not metadata.metadata_id:
            metadata.metadata_id = str(uuid.uuid4())

        query = """
            INSERT INTO analysis_metadata (metadata_id, analysis_id, key, value, data_type)
            VALUES (?, ?, ?, ?, ?)
        """
        params = (metadata.metadata_id, metadata.analysis_id, metadata.key, metadata.value, metadata.data_type)
        self.db.execute_update(query, params)
        return metadata.metadata_id

    def get_by_id(self, metadata_id: str) -> Optional[AnalysisMetadata]:
        query = "SELECT * FROM analysis_metadata WHERE metadata_id = ?"
        results = self.db.execute_query(query, (metadata_id,))
        return self._row_to_metadata(results[0]) if results else None

    def get_by_analysis_id(self, analysis_id: str) -> List[AnalysisMetadata]:
        query = "SELECT * FROM analysis_metadata WHERE analysis_id = ?"
        results = self.db.execute_query(query, (analysis_id,))
        return [self._row_to_metadata(row) for row in results]

    def get_by_key(self, analysis_id: str, key: str) -> Optional[AnalysisMetadata]:
        query = "SELECT * FROM analysis_metadata WHERE analysis_id = ? AND key = ?"
        results = self.db.execute_query(query, (analysis_id, key))
        return self._row_to_metadata(results[0]) if results else None

    def upsert(self, metadata: AnalysisMetadata) -> str:
        existing = self.get_by_key(metadata.analysis_id, metadata.key)
        if existing:
            query = "UPDATE analysis_metadata SET value = ?, data_type = ? WHERE analysis_id = ? AND key = ?"
            self.db.execute_update(query, (metadata.value, metadata.data_type, metadata.analysis_id, metadata.key))
            return existing.metadata_id
        return self.create(metadata)

    def delete(self, metadata_id: str) -> bool:
        return self.db.execute_update("DELETE FROM analysis_metadata WHERE metadata_id = ?", (metadata_id,)) > 0

    def _row_to_metadata(self, row) -> AnalysisMetadata:
        return AnalysisMetadata(
            metadata_id=row['metadata_id'],
            analysis_id=row['analysis_id'],
            key=row['key'],
            value=row['value'],
            data_type=row['data_type']
        )
