"""
视频预处理组件
验证视频文件格式和提取元数据
"""
import os
from typing import Dict, Any, Optional
from infrastructure.log_manager import get_logger


class VideoPreprocessor:
    """视频预处理器"""

    SUPPORTED_FORMATS = ['.mp4', '.mpeg', '.mov', '.avi', '.flv', '.mpg', '.webm', '.wmv', '.3gpp', '.mkv']
    MAX_FILE_SIZE = 2 * 1024 * 1024 * 1024  # 2GB

    def __init__(self, recorder_service=None, analyzer_service=None):
        self.logger = get_logger("VideoPreprocessor")
        self._recorder_service = recorder_service
        self._analyzer_service = analyzer_service

    def validate_video(self, video_path: str) -> bool:
        """验证视频文件"""
        if not os.path.exists(video_path):
            self.logger.warning(f"Video file not found: {video_path}")
            return False

        file_size = os.path.getsize(video_path)
        if file_size > self.MAX_FILE_SIZE:
            self.logger.warning(f"Video file too large: {file_size} bytes")
            return False

        ext = os.path.splitext(video_path)[1].lower()
        if ext not in self.SUPPORTED_FORMATS:
            self.logger.warning(f"Unsupported format: {ext}")
            return False

        return True

    def get_video_metadata(self, video_path: str) -> Dict[str, Any]:
        """获取视频元数据，整合文件信息和 C++ 模块数据"""
        metadata = {"file_size": 0}
        if os.path.exists(video_path):
            metadata["file_size"] = os.path.getsize(video_path)

        # 从 C++ 模块获取录制统计
        if self._recorder_service:
            metadata["recording"] = self._recorder_service.get_recording_info()

        # 从 C++ 模块获取分析统计
        if self._analyzer_service:
            metadata["analysis"] = self._analyzer_service.get_analysis_info()

        return metadata
