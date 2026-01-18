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
                        "category": {"type": "string"},
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
                        "event_type": {"type": "string"},
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
        # 预处理：初步清理非转义换行符（尝试修复常见错误）
        processed_text = self._preprocess_response(response_text)

        try:
            data = json.loads(processed_text)
        except json.JSONDecodeError:
            data = self._extract_json_from_markdown(processed_text)

        if data is None:
            self.logger.error("Failed to extract JSON from response")
            return None

        # 数据清洗：修正常见格式问题
        data = self._sanitize_data(data)

        try:
            jsonschema.validate(instance=data, schema=self.schema)
            return data
        except jsonschema.ValidationError as e:
            self.logger.error(f"JSON validation failed at path '{'.'.join(str(p) for p in e.path)}': {e.message}")
            self.logger.error(f"Failed value: {e.instance}")
            self.logger.error(f"Schema constraint: {e.schema}")
            return None

    def _preprocess_response(self, text: str) -> str:
        """预处理响应文本，修复常见格式错误"""
        if not text:
            return text

        # 1. 移除 JSON 块之前或之后的杂质
        text = text.strip()
        # 如果有 markdown 包裹，提取其内容
        if text.startswith("```"):
            pattern = r'```(?:json)?\s*(\{.*?\})\s*```'
            match = re.search(pattern, text, re.DOTALL)
            if match:
                text = match.group(1).strip()
        
        # 2. 修复物理换行符问题：
        # 寻找被引号包裹的内容，如果其中包含物理换行符且没有转义，尝试将其转换为 \n
        def fix_newlines(match):
            prefix = match.group(1)
            content = match.group(2)
            suffix = match.group(3)
            # 仅在引号内部替换物理换行为转义换行
            fixed_content = content.replace('\n', '\\n').replace('\r', '')
            return f'{prefix}"{fixed_content}"{suffix}'

        # 匹配 pattern: "key": "value"
        # 匹配开始引号后的内容，直到遇到下一个 后面跟着逗号或闭括号的引号
        pattern = r'(\s*:\s*)"(.*?)"(\s*[,}\]])'
        try:
            fixed_text = re.sub(pattern, fix_newlines, text, flags=re.DOTALL)
            return fixed_text
        except Exception as e:
            self.logger.warning(f"Regex preprocessing failed: {e}")
            return text

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

    def _sanitize_data(self, data: Dict[str, Any]) -> Dict[str, Any]:
        """清洗数据，修正常见格式问题"""
        if not isinstance(data, dict):
            return data

        # 清洗 key_findings
        if "key_findings" in data and isinstance(data["key_findings"], list):
            sanitized_findings = []
            for finding in data["key_findings"]:
                if not isinstance(finding, dict):
                    continue
                # 确保 confidence_score 是整数且在范围内
                if "confidence_score" in finding:
                    try:
                        score = int(float(finding["confidence_score"]))
                        finding["confidence_score"] = max(0, min(100, score))
                    except (ValueError, TypeError):
                        finding["confidence_score"] = 80
                # 确保 related_timestamps 是数组
                if "related_timestamps" in finding and not isinstance(finding["related_timestamps"], list):
                    finding["related_timestamps"] = []
                sanitized_findings.append(finding)
            data["key_findings"] = sanitized_findings

        # 清洗 timestamp_events
        if "timestamp_events" in data and isinstance(data["timestamp_events"], list):
            sanitized_events = []
            for event in data["timestamp_events"]:
                if not isinstance(event, dict):
                    continue
                # 确保 timestamp_seconds 是非负数
                if "timestamp_seconds" in event:
                    try:
                        ts = float(event["timestamp_seconds"])
                        event["timestamp_seconds"] = max(0.0, ts)
                    except (ValueError, TypeError):
                        event["timestamp_seconds"] = 0.0
                # 确保 importance_score 是整数且在范围内
                if "importance_score" in event:
                    try:
                        score = int(float(event["importance_score"]))
                        event["importance_score"] = max(1, min(10, score))
                    except (ValueError, TypeError):
                        event["importance_score"] = 5
                sanitized_events.append(event)
            data["timestamp_events"] = sanitized_events

        return data
