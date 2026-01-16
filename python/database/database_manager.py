"""
SQLite 数据库管理器
"""
import sqlite3
import threading
from pathlib import Path
from typing import List
from contextlib import contextmanager

from infrastructure.log_manager import get_logger


class DatabaseManager:
    """SQLite 数据库管理器 (单例)"""

    _instance = None
    _lock = threading.Lock()

    def __new__(cls, *args, **kwargs):
        if not cls._instance:
            with cls._lock:
                if not cls._instance:
                    cls._instance = super().__new__(cls)
        return cls._instance

    def __init__(self, db_path: str = "data/keyframe_analysis.db"):
        if hasattr(self, '_initialized'):
            return

        self.db_path = Path(db_path)
        self.db_path.parent.mkdir(parents=True, exist_ok=True)
        self.logger = get_logger("DatabaseManager")
        self._local = threading.local()
        self._initialize_database()
        self._initialized = True
        self.logger.info(f"Database initialized at: {self.db_path}")

    def _get_connection(self) -> sqlite3.Connection:
        if not hasattr(self._local, 'connection'):
            self._local.connection = sqlite3.connect(
                str(self.db_path), check_same_thread=False, timeout=30.0
            )
            self._local.connection.execute("PRAGMA foreign_keys = ON")
            self._local.connection.row_factory = sqlite3.Row
        return self._local.connection

    @contextmanager
    def get_cursor(self):
        conn = self._get_connection()
        cursor = conn.cursor()
        try:
            yield cursor
            conn.commit()
        except Exception as e:
            conn.rollback()
            self.logger.error(f"Database error: {e}")
            raise
        finally:
            cursor.close()

    @contextmanager
    def transaction(self):
        conn = self._get_connection()
        try:
            yield conn
            conn.commit()
        except Exception as e:
            conn.rollback()
            self.logger.error(f"Transaction error: {e}")
            raise

    def _initialize_database(self):
        with self.get_cursor() as cursor:
            # recording 表
            cursor.execute("""
                CREATE TABLE IF NOT EXISTS recording (
                    record_id TEXT PRIMARY KEY,
                    original_video_path TEXT NOT NULL UNIQUE,
                    title TEXT DEFAULT '',
                    description TEXT DEFAULT '',
                    duration_seconds INTEGER DEFAULT 0,
                    file_size_bytes INTEGER DEFAULT 0,
                    thumbnail_path TEXT DEFAULT '',
                    created_at TEXT NOT NULL,
                    updated_at TEXT NOT NULL,
                    tags TEXT DEFAULT '[]',
                    metadata TEXT DEFAULT '{}'
                )
            """)
            cursor.execute("CREATE INDEX IF NOT EXISTS idx_recording_created_at ON recording(created_at DESC)")
            cursor.execute("CREATE INDEX IF NOT EXISTS idx_recording_title ON recording(title)")

            # keyframe_video 表
            cursor.execute("""
                CREATE TABLE IF NOT EXISTS keyframe_video (
                    keyframe_id TEXT PRIMARY KEY,
                    recording_id TEXT NOT NULL,
                    keyframe_video_path TEXT NOT NULL UNIQUE,
                    keyframe_audio_path TEXT DEFAULT '',
                    keyframe_count INTEGER DEFAULT 0,
                    duration_seconds INTEGER DEFAULT 0,
                    file_size_bytes INTEGER DEFAULT 0,
                    compression_ratio REAL DEFAULT 0.0,
                    created_at TEXT NOT NULL,
                    extraction_config TEXT DEFAULT '{}',
                    FOREIGN KEY (recording_id) REFERENCES recording(record_id) ON DELETE CASCADE
                )
            """)
            cursor.execute("CREATE INDEX IF NOT EXISTS idx_keyframe_recording_id ON keyframe_video(recording_id)")
            cursor.execute("CREATE INDEX IF NOT EXISTS idx_keyframe_created_at ON keyframe_video(created_at DESC)")

            # prompt_template 表
            cursor.execute("""
                CREATE TABLE IF NOT EXISTS prompt_template (
                    prompt_id TEXT PRIMARY KEY,
                    name TEXT NOT NULL,
                    description TEXT DEFAULT '',
                    prompt_content TEXT NOT NULL,
                    category TEXT DEFAULT 'general',
                    is_default INTEGER DEFAULT 0,
                    created_at TEXT NOT NULL,
                    updated_at TEXT NOT NULL,
                    tags TEXT DEFAULT '[]',
                    variables TEXT DEFAULT '[]',
                    CHECK (is_default IN (0, 1))
                )
            """)
            cursor.execute("CREATE INDEX IF NOT EXISTS idx_prompt_category ON prompt_template(category)")
            cursor.execute("CREATE INDEX IF NOT EXISTS idx_prompt_is_default ON prompt_template(is_default)")

            # ai_analysis 表
            cursor.execute("""
                CREATE TABLE IF NOT EXISTS ai_analysis (
                    analysis_id TEXT PRIMARY KEY,
                    keyframe_id TEXT,
                    prompt_id TEXT,
                    analysis_type TEXT DEFAULT 'general',
                    model_name TEXT NOT NULL,
                    model_version TEXT DEFAULT '',
                    status TEXT DEFAULT 'pending',
                    video_analysis_md TEXT DEFAULT '',
                    audio_analysis_md TEXT DEFAULT '',
                    summary_md TEXT DEFAULT '',
                    started_at TEXT,
                    completed_at TEXT,
                    processing_time_ms INTEGER DEFAULT 0,
                    error_message TEXT DEFAULT '',
                    FOREIGN KEY (keyframe_id) REFERENCES keyframe_video(keyframe_id) ON DELETE CASCADE,
                    FOREIGN KEY (prompt_id) REFERENCES prompt_template(prompt_id) ON DELETE SET NULL
                )
            """)
            cursor.execute("CREATE INDEX IF NOT EXISTS idx_analysis_keyframe_id ON ai_analysis(keyframe_id)")
            cursor.execute("CREATE INDEX IF NOT EXISTS idx_analysis_prompt_id ON ai_analysis(prompt_id)")
            cursor.execute("CREATE INDEX IF NOT EXISTS idx_analysis_status ON ai_analysis(status)")
            cursor.execute("CREATE INDEX IF NOT EXISTS idx_analysis_completed_at ON ai_analysis(completed_at DESC)")

            # timestamp_event 表
            cursor.execute("""
                CREATE TABLE IF NOT EXISTS timestamp_event (
                    event_id TEXT PRIMARY KEY,
                    analysis_id TEXT NOT NULL,
                    timestamp_seconds REAL NOT NULL,
                    event_type TEXT DEFAULT 'highlight',
                    title TEXT NOT NULL,
                    description TEXT DEFAULT '',
                    thumbnail_path TEXT DEFAULT '',
                    importance_score INTEGER DEFAULT 5,
                    metadata TEXT DEFAULT '{}',
                    FOREIGN KEY (analysis_id) REFERENCES ai_analysis(analysis_id) ON DELETE CASCADE,
                    CHECK (timestamp_seconds >= 0),
                    CHECK (importance_score BETWEEN 1 AND 10)
                )
            """)
            cursor.execute("CREATE INDEX IF NOT EXISTS idx_event_analysis_id ON timestamp_event(analysis_id)")
            cursor.execute("CREATE INDEX IF NOT EXISTS idx_event_timestamp ON timestamp_event(timestamp_seconds)")

            # key_finding 表
            cursor.execute("""
                CREATE TABLE IF NOT EXISTS key_finding (
                    finding_id TEXT PRIMARY KEY,
                    analysis_id TEXT NOT NULL,
                    sequence_order INTEGER NOT NULL,
                    category TEXT DEFAULT 'general',
                    title TEXT NOT NULL,
                    content TEXT NOT NULL,
                    related_timestamps TEXT DEFAULT '[]',
                    confidence_score INTEGER DEFAULT 80,
                    FOREIGN KEY (analysis_id) REFERENCES ai_analysis(analysis_id) ON DELETE CASCADE,
                    CHECK (confidence_score BETWEEN 0 AND 100)
                )
            """)
            cursor.execute("CREATE INDEX IF NOT EXISTS idx_finding_analysis_id ON key_finding(analysis_id)")
            cursor.execute("CREATE INDEX IF NOT EXISTS idx_finding_sequence ON key_finding(analysis_id, sequence_order)")

            # analysis_metadata 表
            cursor.execute("""
                CREATE TABLE IF NOT EXISTS analysis_metadata (
                    metadata_id TEXT PRIMARY KEY,
                    analysis_id TEXT NOT NULL,
                    key TEXT NOT NULL,
                    value TEXT NOT NULL,
                    data_type TEXT DEFAULT 'string',
                    FOREIGN KEY (analysis_id) REFERENCES ai_analysis(analysis_id) ON DELETE CASCADE,
                    UNIQUE(analysis_id, key)
                )
            """)
            cursor.execute("CREATE INDEX IF NOT EXISTS idx_metadata_analysis_id ON analysis_metadata(analysis_id)")

    def execute_query(self, query: str, params: tuple = ()) -> List[sqlite3.Row]:
        with self.get_cursor() as cursor:
            cursor.execute(query, params)
            return cursor.fetchall()

    def execute_update(self, query: str, params: tuple = ()) -> int:
        with self.get_cursor() as cursor:
            cursor.execute(query, params)
            return cursor.rowcount

    def close(self):
        if hasattr(self._local, 'connection'):
            self._local.connection.close()
            delattr(self._local, 'connection')
