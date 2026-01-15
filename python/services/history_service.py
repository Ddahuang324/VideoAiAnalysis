"""
历史记录服务
管理录制和分析历史记录
"""
import json
import uuid
from pathlib import Path
from typing import List, Dict, Any, Optional
from datetime import datetime
from dataclasses import dataclass, field, asdict

from infrastructure.log_manager import get_logger
from database.database_manager import DatabaseManager
from database.recording_dao import RecordingDAO
from database.keyframe_dao import KeyFrameVideoDAO
from database.models import Recording, KeyFrameVideo


@dataclass
class RecordingRecord:
    """录制记录数据类"""
    record_id: str
    file_path: str
    start_time: str
    end_time: str
    duration: int  # 秒
    file_size: int  # 字节
    keyframe_count: int = 0
    thumbnail_path: str = ""
    notes: str = ""

    def to_dict(self) -> Dict[str, Any]:
        """转换为字典"""
        return asdict(self)

    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> 'RecordingRecord':
        """从字典创建"""
        return cls(**data)


@dataclass
class AnalysisRecord:
    """分析记录数据类"""
    record_id: str
    recording_id: str  # 关联的录制记录ID
    start_time: str
    end_time: str
    keyframe_count: int
    analyzed_frames: int
    results: List[Dict[str, Any]] = field(default_factory=list)

    def to_dict(self) -> Dict[str, Any]:
        """转换为字典"""
        return asdict(self)

    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> 'AnalysisRecord':
        """从字典创建"""
        return cls(**data)


class HistoryService:
    """
    历史记录服务
    负责录制和分析历史记录的持久化管理
    """

    def __init__(self, data_dir: str = "data/history", db_path: str = "data/keyframe_analysis.db"):
        """
        初始化历史记录服务

        Args:
            data_dir: 历史记录数据存储目录
            db_path: 数据库文件路径
        """
        self.data_dir = Path(data_dir)
        self.data_dir.mkdir(parents=True, exist_ok=True)

        self.logger = get_logger("HistoryService")

        # 初始化数据库
        self.db_manager = DatabaseManager(db_path)
        self.recording_dao = RecordingDAO(self.db_manager)
        self.keyframe_dao = KeyFrameVideoDAO(self.db_manager)

        # 数据文件
        self._recordings_file = self.data_dir / "recordings.json"
        self._analyses_file = self.data_dir / "analyses.json"

        # 内存缓存
        self._recordings: Dict[str, RecordingRecord] = {}
        self._analyses: Dict[str, AnalysisRecord] = {}

        # 加载数据
        self._load_data()

    def _load_data(self):
        """加载历史数据"""
        self._load_recordings()
        self._load_analyses()

    def _load_recordings(self):
        """加载录制记录"""
        if not self._recordings_file.exists():
            self.logger.info("No recordings history file found, starting fresh")
            return

        try:
            with open(self._recordings_file, 'r', encoding='utf-8') as f:
                data = json.load(f)

            for record_data in data:
                record = RecordingRecord.from_dict(record_data)
                self._recordings[record.record_id] = record

            self.logger.info(f"Loaded {len(self._recordings)} recording records")

        except Exception as e:
            self.logger.error(f"Error loading recordings: {e}")

    def _load_analyses(self):
        """加载分析记录"""
        if not self._analyses_file.exists():
            self.logger.info("No analyses history file found, starting fresh")
            return

        try:
            with open(self._analyses_file, 'r', encoding='utf-8') as f:
                data = json.load(f)

            for record_data in data:
                record = AnalysisRecord.from_dict(record_data)
                self._analyses[record.record_id] = record

            self.logger.info(f"Loaded {len(self._analyses)} analysis records")

        except Exception as e:
            self.logger.error(f"Error loading analyses: {e}")

    def _save_recordings(self):
        """保存录制记录"""
        try:
            data = [record.to_dict() for record in self._recordings.values()]

            with open(self._recordings_file, 'w', encoding='utf-8') as f:
                json.dump(data, f, indent=2, ensure_ascii=False)

        except Exception as e:
            self.logger.error(f"Error saving recordings: {e}")

    def _save_analyses(self):
        """保存分析记录"""
        try:
            data = [record.to_dict() for record in self._analyses.values()]

            with open(self._analyses_file, 'w', encoding='utf-8') as f:
                json.dump(data, f, indent=2, ensure_ascii=False)

        except Exception as e:
            self.logger.error(f"Error saving analyses: {e}")

    # ========== 录制记录管理 ==========

    def start_recording(
        self,
        file_path: str,
        start_time: datetime,
        record_id: str = ""
    ) -> str:
        """
        开始录制记录（预创建）

        Args:
            file_path: 录制文件路径
            start_time: 开始时间
            record_id: 可选的记录ID，如果为空则生成

        Returns:
            str: 记录ID
        """
        if not record_id:
            record_id = str(uuid.uuid4())

        try:
            db_recording = Recording(
                record_id=record_id,
                original_video_path=file_path,
                title=Path(file_path).stem,
                created_at=start_time.isoformat(),
                updated_at=start_time.isoformat(),
                metadata={"status": "recording"}
            )
            self.recording_dao.create(db_recording)
            self.logger.info(f"Pre-created recording in database: {record_id}")
            return record_id
        except Exception as e:
            self.logger.error(f"Failed to pre-create recording in database: {e}")
            return record_id

    def update_recording(
        self,
        record_id: str,
        end_time: datetime,
        file_size: int,
        duration: int = 0,
        keyframe_count: int = 0,
        thumbnail_path: str = "",
        notes: str = ""
    ) -> bool:
        """
        更新录制记录（完成录制时）

        Args:
            record_id: 记录ID
            end_time: 结束时间
            file_size: 文件大小（字节）
            duration: 时长（秒）
            keyframe_count: 关键帧数量
            thumbnail_path: 缩略图路径
            notes: 备注

        Returns:
            bool: 成功返回True
        """
        try:
            db_recording = self.recording_dao.get_by_id(record_id)
            if not db_recording:
                self.logger.error(f"Recording record not found in database: {record_id}")
                return False

            db_recording.duration_seconds = duration
            db_recording.file_size_bytes = file_size
            db_recording.thumbnail_path = thumbnail_path
            db_recording.description = notes
            db_recording.updated_at = end_time.isoformat()
            db_recording.metadata["status"] = "completed"

            # 更新数据库
            self.recording_dao.update(db_recording)

            # 更新内存和JSON缓存
            start_time = datetime.fromisoformat(db_recording.created_at)
            record = RecordingRecord(
                record_id=record_id,
                file_path=db_recording.original_video_path,
                start_time=db_recording.created_at,
                end_time=db_recording.updated_at,
                duration=duration or int((end_time - start_time).total_seconds()),
                file_size=file_size,
                keyframe_count=keyframe_count,
                thumbnail_path=thumbnail_path,
                notes=notes
            )
            self._recordings[record_id] = record
            self._save_recordings()

            self.logger.info(f"Updated recording in database and cache: {record_id}")
            return True
        except Exception as e:
            self.logger.error(f"Failed to update recording: {e}")
            return False

    def add_recording(
        self,
        file_path: str,
        start_time: datetime,
        end_time: datetime,
        file_size: int,
        keyframe_count: int = 0,
        thumbnail_path: str = "",
        notes: str = ""
    ) -> str:
        """
        添加录制记录（完整流程）
        """
        record_id = str(uuid.uuid4())
        self.start_recording(file_path, start_time, record_id)
        duration = int((end_time - start_time).total_seconds())
        self.update_recording(record_id, end_time, file_size, duration, keyframe_count, thumbnail_path, notes)
        return record_id

    def get_all_recordings(self) -> List[RecordingRecord]:
        """获取所有录制记录"""
        return list(self._recordings.values())

    def get_recording(self, record_id: str) -> Optional[RecordingRecord]:
        """获取单个录制记录"""
        return self._recordings.get(record_id)

    def delete_recording(self, record_id: str) -> bool:
        """
        删除录制记录

        Args:
            record_id: 记录ID

        Returns:
            bool: 成功返回True
        """
        if record_id not in self._recordings:
            return False

        # 删除关联的分析记录
        self._analyses = {
            aid: arec for aid, arec in self._analyses.items()
            if arec.recording_id != record_id
        }
        self._save_analyses()

        # 删除录制记录
        del self._recordings[record_id]
        self._save_recordings()

        self.logger.info(f"Deleted recording record: {record_id}")
        return True

    def search_recordings(self, keyword: str) -> List[RecordingRecord]:
        """
        搜索录制记录

        Args:
            keyword: 搜索关键词

        Returns:
            List[RecordingRecord]: 匹配的记录列表
        """
        keyword_lower = keyword.lower()

        return [
            record for record in self._recordings.values()
            if keyword_lower in record.file_path.lower() or
               keyword_lower in record.notes.lower()
        ]

    def update_recording_notes(self, record_id: str, notes: str) -> bool:
        """
        更新录制记录备注

        Args:
            record_id: 记录ID
            notes: 新备注

        Returns:
            bool: 成功返回True
        """
        if record_id not in self._recordings:
            return False

        self._recordings[record_id].notes = notes
        self._save_recordings()
        return True

    def add_keyframe_video(
        self,
        recording_id: str,
        video_path: str,
        keyframe_count: int,
        duration: float,
        file_size: int,
        audio_path: str = "",
        compression_ratio: float = 0.0,
        extraction_config: Dict[str, Any] = None
    ) -> str:
        """
        添加关键帧视频记录

        Args:
            recording_id: 关联的录制记录ID
            video_path: 关键帧视频路径
            keyframe_count: 关键帧数量
            duration: 视频时长（秒）
            file_size: 文件大小（字节）
            audio_path: 关联音频路径
            compression_ratio: 压缩比
            extraction_config: 提取配置

        Returns:
            str: 关键帧视频ID
        """
        keyframe_id = str(uuid.uuid4())
        
        try:
            db_keyframe = KeyFrameVideo(
                keyframe_id=keyframe_id,
                recording_id=recording_id,
                keyframe_video_path=video_path,
                keyframe_audio_path=audio_path,
                keyframe_count=keyframe_count,
                duration_seconds=int(duration),
                file_size_bytes=file_size,
                compression_ratio=compression_ratio,
                created_at=datetime.now().isoformat(),
                extraction_config=extraction_config or {}
            )
            self.keyframe_dao.create(db_keyframe)
            self.logger.info(f"Saved keyframe video to database: {keyframe_id}")
            return keyframe_id
        except Exception as e:
            self.logger.error(f"Failed to save keyframe video to database: {e}")
            return ""

    # ========== 分析记录管理 ==========

    def add_analysis(
        self,
        recording_id: str,
        start_time: datetime,
        end_time: datetime,
        keyframe_count: int,
        analyzed_frames: int,
        results: List[Dict[str, Any]] = None
    ) -> str:
        """
        添加分析记录

        Args:
            recording_id: 关联的录制记录ID
            start_time: 开始时间
            end_time: 结束时间
            keyframe_count: 关键帧数量
            analyzed_frames: 已分析帧数
            results: 分析结果列表

        Returns:
            str: 记录ID
        """
        record_id = str(uuid.uuid4())

        record = AnalysisRecord(
            record_id=record_id,
            recording_id=recording_id,
            start_time=start_time.isoformat(),
            end_time=end_time.isoformat(),
            keyframe_count=keyframe_count,
            analyzed_frames=analyzed_frames,
            results=results or []
        )

        self._analyses[record_id] = record
        self._save_analyses()

        self.logger.info(f"Added analysis record: {record_id}")
        return record_id

    def get_all_analyses(self) -> List[AnalysisRecord]:
        """获取所有分析记录"""
        return list(self._analyses.values())

    def get_analyses_for_recording(self, recording_id: str) -> List[AnalysisRecord]:
        """获取指定录制记录的所有分析"""
        return [
            record for record in self._analyses.values()
            if record.recording_id == recording_id
        ]

    def get_analysis(self, record_id: str) -> Optional[AnalysisRecord]:
        """获取单个分析记录"""
        return self._analyses.get(record_id)

    def delete_analysis(self, record_id: str) -> bool:
        """
        删除分析记录

        Args:
            record_id: 记录ID

        Returns:
            bool: 成功返回True
        """
        if record_id not in self._analyses:
            return False

        del self._analyses[record_id]
        self._save_analyses()

        self.logger.info(f"Deleted analysis record: {record_id}")
        return True

    # ========== 统计信息 ==========

    def get_statistics(self) -> Dict[str, Any]:
        """获取统计信息"""
        total_recordings = len(self._recordings)
        total_analyses = len(self._analyses)

        if total_recordings == 0:
            return {
                "total_recordings": 0,
                "total_analyses": 0,
                "total_duration": 0,
                "total_size": 0,
                "total_keyframes": 0
            }

        total_duration = sum(r.duration for r in self._recordings.values())
        total_size = sum(r.file_size for r in self._recordings.values())
        total_keyframes = sum(r.keyframe_count for r in self._recordings.values())

        return {
            "total_recordings": total_recordings,
            "total_analyses": total_analyses,
            "total_duration": total_duration,
            "total_size": total_size,
            "total_keyframes": total_keyframes,
            "average_duration": total_duration / total_recordings if total_recordings > 0 else 0,
            "average_size": total_size / total_recordings if total_recordings > 0 else 0
        }

    def clear_all(self) -> bool:
        """
        清空所有历史记录

        Returns:
            bool: 成功返回True
        """
        try:
            self._recordings.clear()
            self._analyses.clear()
            self._save_recordings()
            self._save_analyses()

            self.logger.info("Cleared all history records")
            return True

        except Exception as e:
            self.logger.error(f"Error clearing history: {e}")
            return False
