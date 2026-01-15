"""
数据库模块
提供 SQLite 数据库管理和数据访问功能
"""
from .database_manager import DatabaseManager
from .models import (
    Recording, KeyFrameVideo, PromptTemplate,
    AIAnalysis, TimestampEvent, KeyFinding, AnalysisMetadata
)
from .recording_dao import RecordingDAO
from .keyframe_dao import KeyFrameVideoDAO
from .prompt_template_dao import PromptTemplateDAO
from .ai_analysis_dao import AIAnalysisDAO
from .timestamp_event_dao import TimestampEventDAO
from .key_finding_dao import KeyFindingDAO
from .analysis_metadata_dao import AnalysisMetadataDAO

__all__ = [
    'DatabaseManager',
    'Recording', 'KeyFrameVideo', 'PromptTemplate',
    'AIAnalysis', 'TimestampEvent', 'KeyFinding', 'AnalysisMetadata',
    'RecordingDAO', 'KeyFrameVideoDAO', 'PromptTemplateDAO',
    'AIAnalysisDAO', 'TimestampEventDAO', 'KeyFindingDAO', 'AnalysisMetadataDAO'
]
