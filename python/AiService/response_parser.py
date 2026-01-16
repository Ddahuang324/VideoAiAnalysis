"""
响应解析组件
解析和验证 Gemini API 返回的 JSON 响应
"""
import json
import re
from typing import Dict, Any, Optional
import jsonschema
from infrastructure.log_manager import get_logger


class ResponseParser:
    """Gemini 响应解析器"""

    SCHEMA = {
        "type": "object",
        "required": ["video_analysis_md", "summary_md", "key_findings", "timestamp_events"],
        "properties": {
            "video_analysis_md": {"type": "string"},
            "audio_analysis_md": {"type": ["string", "null"]},
            "summary_md": {"type": "string", "minLength": 10},
            "key_findings": {
                "type": "array",
                "items": {
                    "type": "object",
                    "required": ["category", "title", "content", "confidence_score"],
                    "properties": {
                        "sequence_order": {"type": "integer", "minimum": 0},
                        "category": {"type": "string", "enum": ["technical", "action", "visual"]},
                        "title": {"type": "string"},
                        "content": {"type": "string"},
                        "confidence_score": {"type": "integer", "minimum": 0, "maximum": 100},
                        "related_timestamps": {"type": "array", "items": {"type": "number", "minimum": 0}}
                    }
                }
            },
            "timestamp_events": {
                "type": "array",
                "items": {
                    "type": "object",
                    "required": ["timestamp_seconds", "event_type", "title"],
                    "properties": {
                        "timestamp_seconds": {"type": "number", "minimum": 0},
                        "event_type": {"type": "string", "enum": ["highlight", "action", "technical", "visual"]},
                        "title": {"type": "string"},
                        "description": {"type": ["string", "null"]},
                        "importance_score": {"type": "integer", "minimum": 1, "maximum": 10}
                    }
                }
            },
            "analysis_metadata": {
                "type": "array",
                "items": {
                    "type": "object",
                    "required": ["key", "value"],
                    "properties": {
                        "key": {"type": "string"},
                        "value": {"type": ["string", "number", "null"]},
                        "data_type": {"type": ["string", "null"]}
                    }
                }
            }
        }
    }

    def __init__(self, schema: Optional[Dict] = None):
        self.logger = get_logger("ResponseParser")
        self.schema = schema or self.SCHEMA

    def parse(self, response_text: str) -> Optional[Dict[str, Any]]:
        """解析响应文本"""
        try:
            data = json.loads(response_text)
        except json.JSONDecodeError:
            data = self._extract_json_from_markdown(response_text)

        if data is None:
            self.logger.error("Failed to extract JSON from response")
            return None

        try:
            jsonschema.validate(instance=data, schema=self.schema)
            return data
        except jsonschema.ValidationError as e:
            self.logger.error(f"JSON validation failed: {e.message}")
            return None

    def _extract_json_from_markdown(self, text: str) -> Optional[Dict]:
        """从 Markdown 代码块中提取 JSON"""
        pattern = r'```(?:json)?\s*(\{.*?\})\s*```'
        match = re.search(pattern, text, re.DOTALL)
        if match:
            try:
                return json.loads(match.group(1))
            except json.JSONDecodeError:
                pass
        return None
