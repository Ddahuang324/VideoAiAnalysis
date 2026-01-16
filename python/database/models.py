"""
数据模型定义
"""
from dataclasses import dataclass, field
from typing import List, Dict, Any


@dataclass
class Recording:
    """录制记录数据类"""
    record_id: str = ""
    original_video_path: str = ""
    title: str = ""
    description: str = ""
    duration_seconds: int = 0
    file_size_bytes: int = 0
    thumbnail_path: str = ""
    created_at: str = ""
    updated_at: str = ""
    tags: List[str] = field(default_factory=list)
    metadata: Dict[str, Any] = field(default_factory=dict)


@dataclass
class KeyFrameVideo:
    """关键帧视频数据类"""
    keyframe_id: str = ""
    recording_id: str = ""
    keyframe_video_path: str = ""
    keyframe_audio_path: str = ""
    keyframe_count: int = 0
    duration_seconds: int = 0
    file_size_bytes: int = 0
    compression_ratio: float = 0.0
    created_at: str = ""
    extraction_config: Dict[str, Any] = field(default_factory=dict)


@dataclass
class PromptTemplate:
    """提示词模板数据类"""
    prompt_id: str = ""
    name: str = ""
    prompt_content: str = ""
    description: str = ""
    category: str = "general"
    is_default: int = 0
    created_at: str = ""
    updated_at: str = ""
    tags: List[str] = field(default_factory=list)
    variables: List[Dict[str, str]] = field(default_factory=list)


@dataclass
class AIAnalysis:
    """AI 分析结果数据类"""
    analysis_id: str = ""
    keyframe_id: str = ""
    recording_id: str = ""
    prompt_id: str = ""
    analysis_type: str = "general"
    model_name: str = ""
    model_version: str = ""
    status: str = "pending"
    video_analysis_md: str = ""
    audio_analysis_md: str = ""
    summary_md: str = ""
    rendered_html: str = ""
    started_at: str = ""
    completed_at: str = ""
    processing_time_ms: int = 0
    error_message: str = ""


@dataclass
class TimestampEvent:
    """时间戳事件数据类"""
    event_id: str = ""
    analysis_id: str = ""
    timestamp_seconds: float = 0.0
    event_type: str = "highlight"
    title: str = ""
    description: str = ""
    thumbnail_path: str = ""
    importance_score: int = 5
    metadata: Dict[str, Any] = field(default_factory=dict)


@dataclass
class KeyFinding:
    """关键要点数据类"""
    finding_id: str = ""
    analysis_id: str = ""
    sequence_order: int = 0
    category: str = "general"
    title: str = ""
    content: str = ""
    related_timestamps: List[float] = field(default_factory=list)
    confidence_score: int = 80


@dataclass
class AnalysisMetadata:
    """分析元数据数据类"""
    metadata_id: str = ""
    analysis_id: str = ""
    key: str = ""
    value: str = ""
    data_type: str = "string"
