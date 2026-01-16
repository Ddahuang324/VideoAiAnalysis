"""
Gemini API 调用封装组件
管理与 Google Gemini API 的交互
"""
import os
import time
import json
from typing import Dict, Any, Optional
from infrastructure.log_manager import get_logger
from AiService.video_preprocessor import VideoPreprocessor
from AiService.response_parser import ResponseParser


class GeminiService:
    """Gemini API 服务封装"""
    model: Any
    _genai: Any
    api_key: Optional[str]
    model_name: str
    parser: ResponseParser

    def __init__(self, api_key: Optional[str] = None, config_path: str = "configs/gemini_config.json",
                 recorder_service=None, analyzer_service=None):
        self.logger = get_logger("GeminiService")
        self._config = self._load_config(config_path)
        self.api_key = api_key or os.getenv("GOOGLE_API_KEY")
        self.model_name = self._config.get("model", {}).get("name", "gemini-2.5-flash-lite")
        self.model = None
        self._genai = None  # 增加显式初始化
        self.parser = ResponseParser()

        # C++ 模块服务引用
        self._recorder_service = recorder_service
        self._analyzer_service = analyzer_service
        self.preprocessor = VideoPreprocessor(recorder_service, analyzer_service)

        if not self.api_key:
            raise ValueError("API key is required")

        self._init_client()

    def _load_config(self, config_path: str) -> Dict:
        """加载配置文件"""
        try:
            with open(config_path, 'r', encoding='utf-8') as f:
                return json.load(f)
        except Exception as e:
            self.logger.warning(f"Failed to load config: {e}, using defaults")
            return {}

    def _init_client(self):
        """初始化 Gemini 客户端"""
        try:
            from google import genai
            self._genai = genai.Client(api_key=self.api_key)
            self.model = self.model_name
            self.logger.info(f"Gemini client initialized with model: {self.model_name}")
        except Exception as e:
            self.logger.error(f"Failed to initialize Gemini client: {e}")
            raise

    def upload_video(self, video_path: str) -> str:
        """上传视频文件"""
        if not os.path.exists(video_path):
            raise FileNotFoundError(f"Video file not found: {video_path}")

        file_size = os.path.getsize(video_path)
        size_mb = file_size / (1024 * 1024)
        self.logger.info(f"Preparing to upload video: {video_path} ({size_mb:.2f} MB)")

        if not self._genai:
            raise RuntimeError("Gemini client not initialized")

        start_time = time.time()
        try:
            result = self._retry_with_backoff(
                lambda: self._genai.files.upload(file=video_path)
            )
            duration = time.time() - start_time
            self.logger.info(f"Upload completed in {duration:.2f}s")
        except Exception as e:
            duration = time.time() - start_time
            self.logger.error(f"Upload failed after {duration:.2f}s: {e}")
            raise

        if not result:
            raise RuntimeError("Failed to upload video")
        return result.name

    def wait_for_file_active(self, file_name: str, timeout: int = 300) -> bool:
        """等待文件处理完成"""
        if not self._genai:
            raise RuntimeError("Gemini client not initialized")

        start_time = time.time()
        while time.time() - start_time < timeout:
            file_info = self._genai.files.get(name=file_name)
            if file_info.state.name == "ACTIVE":
                return True
            if file_info.state.name == "FAILED":
                self.logger.error("File processing failed")
                return False
            time.sleep(2)
        self.logger.error("File processing timeout")
        return False

    def analyze_video(self, video_path: str, prompt: str) -> Optional[Dict[str, Any]]:
        """分析视频"""
        self.logger.info(f"Starting video analysis for: {video_path}")
        
        if not self.preprocessor.validate_video(video_path):
            self.logger.error("Video validation failed")
            return None

        try:
            if not self._genai:
                raise RuntimeError("Gemini client not initialized")

            self.logger.info("Uploading video to Gemini...")
            file_name = self.upload_video(video_path)
            self.logger.info(f"Video uploaded successfully: {file_name}")
            
            self.logger.info("Waiting for file to become active...")
            if not self.wait_for_file_active(file_name):
                self.logger.error("File activation timeout or failed")
                return None
            self.logger.info("File is now active")

            video_file = self._genai.files.get(name=file_name)
            
            # 添加日志：输出提示词
            self.logger.info(f"Sending prompt (first 200 chars): {prompt[:200]}...")
            
            self.logger.info("Calling Gemini API for content generation...")
            response = self._genai.models.generate_content(
                model=self.model,
                contents=[video_file, prompt],
                config=self._config.get("generation_config", {})
            )
            self.logger.info("Received response from Gemini API")
            
            # 添加日志：输出原始响应
            self.logger.info(f"Gemini raw response (first 500 chars): {response.text[:500]}...")
            
            parsed_result = self.parser.parse(response.text)
            if parsed_result is None:
                self.logger.error(f"Failed to parse response. Full response: {response.text}")
            else:
                self.logger.info("Successfully parsed Gemini response")
            return parsed_result
        except Exception as e:
            self.logger.error(f"Video analysis failed: {e}")
            return None

    def _retry_with_backoff(self, func, max_retries: int = 3):
        """指数退避重试"""
        for attempt in range(max_retries):
            try:
                return func()
            except Exception as e:
                if attempt == max_retries - 1:
                    raise
                wait_time = 2 ** attempt
                self.logger.warning(f"Retry {attempt + 1}/{max_retries} after {wait_time}s: {e}")
                time.sleep(wait_time)

