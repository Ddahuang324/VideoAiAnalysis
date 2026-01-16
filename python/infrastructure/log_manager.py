"""
日志管理器
提供统一的日志管理功能
"""
import logging
from logging.handlers import RotatingFileHandler
from pathlib import Path
from typing import Optional


class LogManager:
    """统一日志管理器"""

    _instance: Optional['LogManager'] = None

    def __new__(cls, *args, **kwargs):
        """单例模式"""
        if cls._instance is None:
            cls._instance = super().__new__(cls)
            cls._instance._initialized = False
        return cls._instance

    def __init__(self, log_file: str = "logs/python_app.log", level: str = "INFO"):
        """
        初始化日志管理器

        Args:
            log_file: 日志文件路径
            level: 日志级别 (DEBUG, INFO, WARNING, ERROR, CRITICAL)
        """
        if self._initialized:
            return

        self.log_file = log_file
        self.level = level
        self.logger = self._setup_logger()
        self._initialized = True

    def _setup_logger(self) -> logging.Logger:
        """配置日志系统"""
        logger = logging.getLogger("AIVideoAnalysis")
        logger.setLevel(getattr(logging, self.level.upper()))

        # 避免重复添加handler
        if logger.handlers:
            return logger

        # 文件处理器
        log_file = Path(self.log_file)
        log_file.parent.mkdir(parents=True, exist_ok=True)

        file_handler = RotatingFileHandler(
            log_file,
            maxBytes=10 * 1024 * 1024,  # 10 MB
            backupCount=5,
            encoding='utf-8'
        )
        file_handler.setFormatter(logging.Formatter(
            '%(asctime)s - %(name)s - %(levelname)s - %(message)s',
            datefmt='%Y-%m-%d %H:%M:%S'
        ))

        # 控制台处理器
        console_handler = logging.StreamHandler()
        console_handler.setFormatter(logging.Formatter(
            '%(levelname)s - %(message)s'
        ))

        logger.addHandler(file_handler)
        logger.addHandler(console_handler)

        return logger

    def info(self, message: str):
        """记录信息日志"""
        self.logger.info(message)

    def warning(self, message: str):
        """记录警告日志"""
        self.logger.warning(message)

    def error(self, message: str):
        """记录错误日志"""
        self.logger.error(message)

    def debug(self, message: str):
        """记录调试日志"""
        self.logger.debug(message)

    def critical(self, message: str):
        """记录严重错误日志"""
        self.logger.critical(message)


def get_logger(name: Optional[str] = None) -> logging.Logger:
    """
    获取命名日志记录器

    Args:
        name: 日志记录器名称

    Returns:
        logging.Logger: 日志记录器实例
    """
    full_name = f"AIVideoAnalysis.{name}" if name else "AIVideoAnalysis"
    return logging.getLogger(full_name)
