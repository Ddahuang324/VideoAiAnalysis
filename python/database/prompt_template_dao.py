"""
提示词模板数据访问对象
"""
import json
import uuid
from datetime import datetime
from typing import List, Optional, Dict

from .database_manager import DatabaseManager
from .models import PromptTemplate
from infrastructure.log_manager import get_logger


class PromptTemplateDAO:
    """提示词模板 DAO"""

    def __init__(self, db_manager: DatabaseManager):
        self.db = db_manager
        self.logger = get_logger("PromptTemplateDAO")

    def create(self, template: PromptTemplate) -> str:
        if not template.prompt_id:
            template.prompt_id = str(uuid.uuid4())
        now = datetime.now().isoformat()
        if not template.created_at:
            template.created_at = now
        template.updated_at = now

        query = """
            INSERT INTO prompt_template (
                prompt_id, name, description, prompt_content,
                category, is_default, created_at, updated_at, tags, variables
            ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        """
        params = (
            template.prompt_id, template.name, template.description,
            template.prompt_content, template.category, template.is_default,
            template.created_at, template.updated_at,
            json.dumps(template.tags, ensure_ascii=False),
            json.dumps(template.variables, ensure_ascii=False)
        )
        self.db.execute_update(query, params)
        self.logger.info(f"Created prompt template: {template.prompt_id}")
        return template.prompt_id

    def get_by_id(self, prompt_id: str) -> Optional[PromptTemplate]:
        query = "SELECT * FROM prompt_template WHERE prompt_id = ?"
        results = self.db.execute_query(query, (prompt_id,))
        return self._row_to_template(results[0]) if results else None

    def get_all(self, category: str = None) -> List[PromptTemplate]:
        if category:
            query = "SELECT * FROM prompt_template WHERE category = ? ORDER BY is_default DESC, created_at DESC"
            results = self.db.execute_query(query, (category,))
        else:
            query = "SELECT * FROM prompt_template ORDER BY category, is_default DESC, created_at DESC"
            results = self.db.execute_query(query)
        return [self._row_to_template(row) for row in results]

    def get_default(self, category: str = "general") -> Optional[PromptTemplate]:
        query = "SELECT * FROM prompt_template WHERE category = ? AND is_default = 1 LIMIT 1"
        results = self.db.execute_query(query, (category,))
        return self._row_to_template(results[0]) if results else None

    def update(self, template: PromptTemplate) -> bool:
        template.updated_at = datetime.now().isoformat()
        query = """
            UPDATE prompt_template SET
                name = ?, description = ?, prompt_content = ?,
                category = ?, is_default = ?, updated_at = ?, tags = ?, variables = ?
            WHERE prompt_id = ?
        """
        params = (
            template.name, template.description, template.prompt_content,
            template.category, template.is_default, template.updated_at,
            json.dumps(template.tags, ensure_ascii=False),
            json.dumps(template.variables, ensure_ascii=False),
            template.prompt_id
        )
        return self.db.execute_update(query, params) > 0

    def delete(self, prompt_id: str) -> bool:
        affected = self.db.execute_update("DELETE FROM prompt_template WHERE prompt_id = ?", (prompt_id,))
        if affected > 0:
            self.logger.info(f"Deleted prompt template: {prompt_id}")
        return affected > 0

    def set_as_default(self, prompt_id: str, category: str) -> bool:
        self.db.execute_update("UPDATE prompt_template SET is_default = 0 WHERE category = ?", (category,))
        return self.db.execute_update("UPDATE prompt_template SET is_default = 1 WHERE prompt_id = ?", (prompt_id,)) > 0

    def render_prompt(self, template: PromptTemplate, variables: Dict[str, str] = None) -> str:
        content = template.prompt_content
        if not variables:
            variables = {}
        for var_def in template.variables:
            var_name = var_def.get("name")
            if var_name and var_name not in variables:
                variables[var_name] = var_def.get("default", "")
        for key, value in variables.items():
            content = content.replace(f"{{{key}}}", value)
        return content

    def _row_to_template(self, row) -> PromptTemplate:
        return PromptTemplate(
            prompt_id=row['prompt_id'],
            name=row['name'],
            description=row['description'],
            prompt_content=row['prompt_content'],
            category=row['category'],
            is_default=row['is_default'],
            created_at=row['created_at'],
            updated_at=row['updated_at'],
            tags=json.loads(row['tags']),
            variables=json.loads(row['variables'])
        )
