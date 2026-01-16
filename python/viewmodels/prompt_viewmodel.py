"""
提示词模板视图模型
负责提示词模板的管理和UI交互
"""
from PySide6.QtCore import QObject, Signal, Slot, Property
from typing import Optional, List

from database.prompt_template_dao import PromptTemplateDAO
from database.models import PromptTemplate
from infrastructure.log_manager import get_logger


class PromptViewModel(QObject):
    """提示词模板视图模型"""

    # 信号
    templatesChanged = Signal()
    currentTemplateChanged = Signal()
    errorOccurred = Signal(str)

    def __init__(self, prompt_dao: PromptTemplateDAO):
        super().__init__()
        self._dao = prompt_dao
        self._templates: List[PromptTemplate] = []
        self._current_template: Optional[PromptTemplate] = None
        self.logger = get_logger("PromptViewModel")

    @Slot()
    def loadTemplates(self):
        """加载所有模板"""
        try:
            self._templates = self._dao.get_all()
            self.templatesChanged.emit()
            # 设置默认模板为当前模板
            default = self._dao.get_default()
            if default:
                self._current_template = default
                self.currentTemplateChanged.emit()
            self.logger.info(f"Loaded {len(self._templates)} templates")
        except Exception as e:
            self.errorOccurred.emit(str(e))
            self.logger.error(f"Failed to load templates: {e}")

    @Slot(str)
    def selectTemplate(self, prompt_id: str):
        """选择模板"""
        for t in self._templates:
            if t.prompt_id == prompt_id:
                self._current_template = t
                self.currentTemplateChanged.emit()
                self.logger.info(f"Selected template: {t.name}")
                return

    @Slot(str, str, str, str)
    def createTemplate(self, name: str, content: str, description: str, category: str):
        """创建新模板"""
        try:
            template = PromptTemplate(
                name=name,
                prompt_content=content,
                description=description,
                category=category or "general"
            )
            self._dao.create(template)
            self.loadTemplates()
            self.logger.info(f"Created template: {name}")
        except Exception as e:
            self.errorOccurred.emit(str(e))
            self.logger.error(f"Failed to create template: {e}")

    @Slot(str, str, str, str, str)
    def updateTemplate(self, prompt_id: str, name: str, content: str, description: str, category: str):
        """更新模板"""
        try:
            template = self._dao.get_by_id(prompt_id)
            if template:
                template.name = name
                template.prompt_content = content
                template.description = description
                template.category = category
                self._dao.update(template)
                self.loadTemplates()
                self.logger.info(f"Updated template: {name}")
        except Exception as e:
            self.errorOccurred.emit(str(e))
            self.logger.error(f"Failed to update template: {e}")

    @Slot(str)
    def deleteTemplate(self, prompt_id: str):
        """删除模板"""
        try:
            self._dao.delete(prompt_id)
            self.loadTemplates()
            self.logger.info(f"Deleted template: {prompt_id}")
        except Exception as e:
            self.errorOccurred.emit(str(e))
            self.logger.error(f"Failed to delete template: {e}")

    @Slot(str)
    def setAsDefault(self, prompt_id: str):
        """设为默认模板"""
        try:
            template = self._dao.get_by_id(prompt_id)
            if template:
                self._dao.set_as_default(prompt_id, template.category)
                self.loadTemplates()
                self.logger.info(f"Set default template: {prompt_id}")
        except Exception as e:
            self.errorOccurred.emit(str(e))
            self.logger.error(f"Failed to set default: {e}")

    @Property(list, notify=templatesChanged)
    def templates(self) -> list:
        """模板列表 (转换为 QML 可用的 dict 列表)"""
        return [
            {
                "promptId": t.prompt_id,
                "name": t.name,
                "description": t.description,
                "category": t.category,
                "isDefault": t.is_default == 1,
                "content": t.prompt_content
            }
            for t in self._templates
        ]

    @Property(str, notify=currentTemplateChanged)
    def currentTemplateId(self) -> str:
        return self._current_template.prompt_id if self._current_template else ""

    @Property(str, notify=currentTemplateChanged)
    def currentTemplateName(self) -> str:
        return self._current_template.name if self._current_template else "Default Analysis"

    @Property(str, notify=currentTemplateChanged)
    def currentTemplateContent(self) -> str:
        return self._current_template.prompt_content if self._current_template else ""
