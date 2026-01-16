"""
多级 Prompt 构建组件
构建用于 Gemini API 的分析提示词
"""
from typing import Dict, Any, Optional
from infrastructure.log_manager import get_logger
from database.prompt_template_dao import PromptTemplateDAO


class PromptBuilder:
    """多级 Prompt 构建器"""

    SYSTEM_PROMPT = """你是一位专业的视频内容分析专家，擅长深度理解视频内容并生成结构清晰、视觉精美的分析文档。

## 🎯 核心任务 (Core Task)
**你必须认真观看并分析视频的实际内容**，包括：
- 视频中出现的人物、物体、场景、文字
- 发生的事件、动作、对话
- 屏幕录制中的软件界面、代码、操作流程
- 任何可见的信息和上下文

## 📋 文档风格要求 (Style Requirements)
1. **结构化标题**：
   - 每个二级标题必须以 Emoji 开头（例如：📋 概述, 🎯 目标, 📊 分析, � 发现, � 建议, ⏱️ 时间线, ✅ 总结）。
   - 使用分明的层次结构（H1, H2, H3, H4）。

2. **必须：可视化图表 (Mermaid)**：
   - 在 `video_analysis_md` 中**必须包含至少两个 Mermaid 图表**。
   - 根据视频内容选择合适的图表类型：
     * `graph TB/LR`：流程图、结构图、关系图
     * `sequenceDiagram`：交互流程、操作时序
     * `timeline`：事件时间线
     * `mindmap`：内容概念图
     * `flowchart`：决策流程
   - 使用样式美化（如 `style Node fill:#color`）。
   - **重要**: 所有代码块（包括 mermaid）必须正确闭合，以三个反引号 ``` 开头和结尾。

3. **按需：表格化呈现**：
   - 当内容涉及对比、分类、列表、统计等场景时，使用 Markdown 表格呈现。
   - 例如：功能对比、时间节点、人物介绍、问题清单等。

4. **按需：代码展示 (Code Snippets)**：
   - 如果视频内容涉及编程、代码、技术实现，则提供相关代码片段。
   - 代码应包含必要的注释，展示关键逻辑。
   - 如果视频不涉及代码，则不需要此部分。

5. **行动建议/总结**：
   - 文档末尾提供基于视频内容的总结或建议。
   - 可使用任务列表（- [ ]）展示待办事项（如适用）。

## 🔍 分析准则 (Analysis Principles)
1. **忠于内容**：分析必须基于视频中实际呈现的内容，不要凭空捏造或假设。
2. **深度洞察**：不仅描述表面内容，还要分析其含义、目的和价值。
3. **时间标注**：对重要事件标注其在视频中的大致时间点。
4. **专业表达**：使用与视频内容领域相关的专业术语（技术、商业、教育等）。"""

    OUTPUT_FORMAT_PROMPT = """请严格按照以下JSON格式输出分析结果：
{
    "video_analysis_md": "基于视频实际内容的完整分析文档。必须包含 Emoji 标题和至少两个 Mermaid 图表，按需使用表格和代码。",
    "audio_analysis_md": "音频内容中的对话或语音描述（如有）",
    "summary_md": "一句话核心摘要，概括视频的主要内容（用于列表展示，至少10个字符）",
    "key_findings": [
        {
            "sequence_order": 0,
            "category": "technical|action|visual",
            "title": "关键发现标题",
            "content": "基于视频内容的简练描述",
            "confidence_score": 90,
            "related_timestamps": [0.0]
        }
    ],
    "timestamp_events": [
        {
            "timestamp_seconds": 0.0,
            "event_type": "technical|action|visual|highlight",
            "title": "事件简短标题",
            "description": "事件描述",
            "importance_score": 8
        }
    ],
    "analysis_metadata": [
        {"key": "content_type", "value": "视频内容类型", "data_type": "string"}
    ]
}"""

    def __init__(self, prompt_dao: Optional[PromptTemplateDAO] = None):
        self.logger = get_logger("PromptBuilder")
        self.prompt_dao = prompt_dao

    def build_prompt(
        self,
        scenario_category: str = "general",
        video_context: Optional[Dict[str, Any]] = None,
        custom_variables: Optional[Dict[str, str]] = None
    ) -> str:
        """构建完整的分析提示词"""
        prompt_parts = [self.SYSTEM_PROMPT]

        # Level 2: 任务提示词
        if self.prompt_dao:
            task_template = self.prompt_dao.get_default(category=scenario_category)
            if task_template:
                task_prompt = self.prompt_dao.render_prompt(task_template, custom_variables or {})
                prompt_parts.append(f"**分析任务：**\n{task_prompt}")

        # Level 3: 上下文提示词
        if video_context:
            context_prompt = self._build_context_prompt(video_context)
            prompt_parts.append(context_prompt)

        # Level 4: 输出格式
        prompt_parts.append(self.OUTPUT_FORMAT_PROMPT)

        return "\n\n---\n\n".join(prompt_parts)

    def _build_context_prompt(self, video_context: Dict[str, Any]) -> str:
        """构建视频上下文提示词"""
        lines = ["**视频信息：**"]

        if "duration" in video_context:
            lines.append(f"- 时长: {video_context['duration']:.1f} 秒")

        if "keyframe_count" in video_context:
            lines.append(f"- 关键帧数量: {video_context['keyframe_count']}")

        if "file_size" in video_context:
            size_mb = video_context['file_size'] / (1024 * 1024)
            lines.append(f"- 文件大小: {size_mb:.2f} MB")

        if "width" in video_context and "height" in video_context:
            lines.append(f"- 分辨率: {video_context['width']}x{video_context['height']}")

        return "\n".join(lines)
