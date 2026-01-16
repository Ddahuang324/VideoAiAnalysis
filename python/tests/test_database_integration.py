"""数据库集成测试"""
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent.parent))

from database.database_manager import DatabaseManager
from database.recording_dao import RecordingDAO
from database.keyframe_dao import KeyFrameVideoDAO
from database.ai_analysis_dao import AIAnalysisDAO
from database.timestamp_event_dao import TimestampEventDAO
from database.key_finding_dao import KeyFindingDAO
from database.models import Recording, KeyFrameVideo, AIAnalysis, TimestampEvent, KeyFinding


class TestDatabaseIntegration(unittest.TestCase):
    """数据库集成测试"""

    def setUp(self):
        """每个测试前重置单例并创建临时数据库"""
        DatabaseManager._instance = None
        self.temp_db_path = tempfile.mktemp(suffix='.db')
        self.db = DatabaseManager(self.temp_db_path)

        self.recording_dao = RecordingDAO(self.db)
        self.keyframe_dao = KeyFrameVideoDAO(self.db)
        self.analysis_dao = AIAnalysisDAO(self.db)
        self.event_dao = TimestampEventDAO(self.db)
        self.finding_dao = KeyFindingDAO(self.db)

    def tearDown(self):
        """清理"""
        self.db.close()
        DatabaseManager._instance = None
        try:
            Path(self.temp_db_path).unlink(missing_ok=True)
        except PermissionError:
            pass  # Windows 下文件可能仍被占用

    def test_recording_crud(self):
        """测试 Recording CRUD"""
        # Create
        rec = Recording(original_video_path="/test/video.mp4", title="Test Video")
        rec_id = self.recording_dao.create(rec)
        self.assertIsNotNone(rec_id)

        # Read
        fetched = self.recording_dao.get_by_id(rec_id)
        self.assertEqual(fetched.title, "Test Video")

        # Update
        fetched.title = "Updated Title"
        self.assertTrue(self.recording_dao.update(fetched))
        updated = self.recording_dao.get_by_id(rec_id)
        self.assertEqual(updated.title, "Updated Title")

        # Delete
        self.assertTrue(self.recording_dao.delete(rec_id))
        self.assertIsNone(self.recording_dao.get_by_id(rec_id))

    def test_full_data_chain(self):
        """测试完整数据链路: Recording -> KeyFrame -> Analysis -> Event/Finding"""
        # 创建 Recording
        rec = Recording(original_video_path="/test/chain.mp4", title="Chain Test")
        rec_id = self.recording_dao.create(rec)

        # 创建 KeyFrameVideo
        kf = KeyFrameVideo(recording_id=rec_id, keyframe_video_path="/test/kf.mp4")
        kf_id = self.keyframe_dao.create(kf)
        self.assertIsNotNone(kf_id)

        # 创建 AIAnalysis
        analysis = AIAnalysis(keyframe_id=kf_id, model_name="test-model", status="completed")
        analysis_id = self.analysis_dao.create(analysis)
        self.assertIsNotNone(analysis_id)

        # 创建 TimestampEvent
        event = TimestampEvent(analysis_id=analysis_id, timestamp_seconds=10.5, title="Test Event")
        event_id = self.event_dao.create(event)
        self.assertIsNotNone(event_id)

        # 创建 KeyFinding
        finding = KeyFinding(analysis_id=analysis_id, sequence_order=1, title="Finding", content="Test")
        finding_id = self.finding_dao.create(finding)
        self.assertIsNotNone(finding_id)

        # 验证关联查询
        keyframes = self.keyframe_dao.get_by_recording_id(rec_id)
        self.assertEqual(len(keyframes), 1)

        events = self.event_dao.get_by_analysis_id(analysis_id)
        self.assertEqual(len(events), 1)

    def test_cascade_delete(self):
        """测试级联删除"""
        # 创建完整链路
        rec = Recording(original_video_path="/test/cascade.mp4", title="Cascade Test")
        rec_id = self.recording_dao.create(rec)

        kf = KeyFrameVideo(recording_id=rec_id, keyframe_video_path="/test/cascade_kf.mp4")
        kf_id = self.keyframe_dao.create(kf)

        analysis = AIAnalysis(keyframe_id=kf_id, model_name="test-model")
        analysis_id = self.analysis_dao.create(analysis)

        event = TimestampEvent(analysis_id=analysis_id, timestamp_seconds=5.0, title="Event")
        self.event_dao.create(event)

        # 删除 Recording，验证级联删除
        self.recording_dao.delete(rec_id)

        self.assertIsNone(self.keyframe_dao.get_by_id(kf_id))
        self.assertIsNone(self.analysis_dao.get_by_id(analysis_id))
        self.assertEqual(len(self.event_dao.get_by_analysis_id(analysis_id)), 0)

    def test_foreign_key_constraint(self):
        """测试外键约束"""
        # 尝试创建指向不存在 recording_id 的 KeyFrameVideo
        kf = KeyFrameVideo(recording_id="non-existent-id", keyframe_video_path="/test/orphan.mp4")
        with self.assertRaises(Exception):
            self.keyframe_dao.create(kf)


if __name__ == '__main__':
    unittest.main()
