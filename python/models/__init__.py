"""
Models 包 - 数据模型层
定义应用中使用的数据结构
"""

# 从服务层导出数据模型
from ..services.recorder_service import RecordingSession
from ..services.analyzer_service import KeyFrameResult, AnalysisSession
from ..services.history_service import RecordingRecord, AnalysisRecord

# 保留的旧数据模型
from .video_model import VideoModel, AnalysisResult

__all__ = [
    'RecordingSession',
    'KeyFrameResult',
    'AnalysisSession',
    'RecordingRecord',
    'AnalysisRecord',
    'VideoModel',
    'AnalysisResult',
]
