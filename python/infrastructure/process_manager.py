"""
进程管理器
负责C++模块的生命周期管理
注意: 当前使用Pybind11模块，此管理器适配模块架构而非独立进程
"""
import sys
import os
import time
import threading
from pathlib import Path
from typing import Optional, Dict, Any
from enum import Enum

from .log_manager import LogManager, get_logger


class ProcessStatus(Enum):
    """进程状态枚举"""
    STOPPED = 0
    STARTING = 1
    RUNNING = 2
    PAUSED = 3
    ERROR = 4


class ModuleManager:
    """
    C++ 模块管理器
    负责加载和管理 Pybind11 模块的生命周期
    """

    def __init__(self, log_manager: LogManager):
        """
        初始化模块管理器

        Args:
            log_manager: 日志管理器实例
        """
        self.log_manager = log_manager
        self.logger = get_logger("ModuleManager")

        # 模块状态
        self._recorder_status = ProcessStatus.STOPPED
        self._analyzer_status = ProcessStatus.STOPPED

        # 模块实例
        self._recorder_api = None
        self._analyzer_api = None

        # 构建路径 (适配 PyInstaller)
        if getattr(sys, 'frozen', False):
            # 打包状态下，DLL和模块都在根目录
            self.build_python_path = Path(sys._MEIPASS)
            self.build_bin_path = Path(sys._MEIPASS)
        else:
            # 源码运行状态
            self.build_python_path = Path(__file__).parent.parent.parent / "build" / "python"
            self.build_bin_path = Path(__file__).parent.parent.parent / "build" / "bin"

        # 监控标志
        self._is_monitoring = False
        self._monitor_thread: Optional[threading.Thread] = None

        # 回调函数
        self._recorder_status_callbacks = []
        self._analyzer_status_callbacks = []

    def _ensure_build_path(self):
        """确保构建路径在Python搜索路径中"""
        build_path_str = str(self.build_python_path)

        if build_path_str not in sys.path:
            sys.path.insert(0, build_path_str)

        # Windows平台添加DLL目录
        if sys.platform == 'win32':
            if hasattr(os, 'add_dll_directory'):
                if self.build_python_path.exists():
                    try:
                        os.add_dll_directory(str(self.build_python_path))
                    except Exception as e:
                        self.logger.warning(f"Failed to add DLL directory: {e}")

                if self.build_bin_path.exists():
                    try:
                        os.add_dll_directory(str(self.build_bin_path))
                    except Exception as e:
                        self.logger.warning(f"Failed to add bin DLL directory: {e}")

    def get_recorder_module(self):
        """
        获取录制器模块

        Returns:
            recorder_module or None
        """
        self._ensure_build_path()

        try:
            import recorder_module as rec
            self.logger.debug("Recorder module imported successfully")
            return rec
        except ImportError as e:
            self.logger.error(f"Failed to import recorder_module: {e}")
            self.log_manager.error(f"Recorder module not found. Please build the project first.")
            return None
        except Exception as e:
            self.logger.error(f"Unexpected error importing recorder_module: {e}")
            return None

    def get_analyzer_module(self):
        """
        获取分析器模块

        Returns:
            analyzer_module or None
        """
        self._ensure_build_path()

        try:
            import analyzer_module as ana
            self.logger.debug("Analyzer module imported successfully")
            return ana
        except ImportError as e:
            self.logger.error(f"Failed to import analyzer_module: {e}")
            self.log_manager.error(f"Analyzer module not found. Please build the project first.")
            return None
        except Exception as e:
            self.logger.error(f"Unexpected error importing analyzer_module: {e}")
            return None

    def create_recorder_api(self, config=None):
        """
        创建录制器API实例

        Args:
            config: RecorderConfig配置对象 (可选)

        Returns:
            RecorderAPI实例或None
        """
        rec_module = self.get_recorder_module()
        if rec_module is None:
            self._recorder_status = ProcessStatus.ERROR
            return None

        try:
            self._recorder_status = ProcessStatus.STARTING
            self._recorder_api = rec_module.RecorderAPI()

            # 如果提供了配置，进行初始化
            if config is not None:
                self._recorder_api.initialize(config)

            self._recorder_status = ProcessStatus.RUNNING
            self.logger.info("RecorderAPI created successfully")
            self._notify_recorder_status_change(ProcessStatus.RUNNING)
            return self._recorder_api

        except Exception as e:
            self._recorder_status = ProcessStatus.ERROR
            self.logger.error(f"Failed to create RecorderAPI: {e}")
            self.log_manager.error(f"Failed to create recorder: {e}")
            self._notify_recorder_status_change(ProcessStatus.ERROR)
            return None

    def create_analyzer_api(self, config=None):
        """
        创建分析器API实例

        Args:
            config: AnalyzerConfig配置对象 (可选)

        Returns:
            AnalyzerAPI实例或None
        """
        ana_module = self.get_analyzer_module()
        if ana_module is None:
            self._analyzer_status = ProcessStatus.ERROR
            return None

        try:
            self._analyzer_status = ProcessStatus.STARTING
            self._analyzer_api = ana_module.AnalyzerAPI()

            # 如果提供了配置，进行初始化
            if config is not None:
                self._analyzer_api.initialize(config)

            self._analyzer_status = ProcessStatus.RUNNING
            self.logger.info("AnalyzerAPI created successfully")
            self._notify_analyzer_status_change(ProcessStatus.RUNNING)
            return self._analyzer_api

        except Exception as e:
            self._analyzer_status = ProcessStatus.ERROR
            self.logger.error(f"Failed to create AnalyzerAPI: {e}")
            self.log_manager.error(f"Failed to create analyzer: {e}")
            self._notify_analyzer_status_change(ProcessStatus.ERROR)
            return None

    def get_recorder_api(self):
        """获取已创建的录制器API实例"""
        return self._recorder_api

    def get_analyzer_api(self):
        """获取已创建的分析器API实例"""
        return self._analyzer_api

    def shutdown_recorder(self):
        """关闭录制器API"""
        if self._recorder_api is not None:
            try:
                self._recorder_api.shutdown()
                self.logger.info("RecorderAPI shutdown successfully")
            except Exception as e:
                self.logger.error(f"Error shutting down RecorderAPI: {e}")
            finally:
                self._recorder_api = None
                self._recorder_status = ProcessStatus.STOPPED
                self._notify_recorder_status_change(ProcessStatus.STOPPED)

    def shutdown_analyzer(self):
        """关闭分析器API"""
        if self._analyzer_api is not None:
            try:
                self._analyzer_api.shutdown()
                self.logger.info("AnalyzerAPI shutdown successfully")
            except Exception as e:
                self.logger.error(f"Error shutting down AnalyzerAPI: {e}")
            finally:
                self._analyzer_api = None
                self._analyzer_status = ProcessStatus.STOPPED
                self._notify_analyzer_status_change(ProcessStatus.STOPPED)

    def shutdown_all(self):
        """关闭所有API实例"""
        self.shutdown_recorder()
        self.shutdown_analyzer()
        self._stop_monitoring()

    def get_recorder_status(self) -> ProcessStatus:
        """获取录制器状态"""
        return self._recorder_status

    def get_analyzer_status(self) -> ProcessStatus:
        """获取分析器状态"""
        return self._analyzer_status

    def set_recorder_status(self, status: ProcessStatus):
        """设置录制器状态"""
        self._recorder_status = status
        self._notify_recorder_status_change(status)

    def set_analyzer_status(self, status: ProcessStatus):
        """设置分析器状态"""
        self._analyzer_status = status
        self._notify_analyzer_status_change(status)

    def register_recorder_status_callback(self, callback):
        """注册录制器状态变化回调"""
        if callback not in self._recorder_status_callbacks:
            self._recorder_status_callbacks.append(callback)

    def register_analyzer_status_callback(self, callback):
        """注册分析器状态变化回调"""
        if callback not in self._analyzer_status_callbacks:
            self._analyzer_status_callbacks.append(callback)

    def _notify_recorder_status_change(self, status: ProcessStatus):
        """通知录制器状态变化"""
        for callback in self._recorder_status_callbacks:
            try:
                callback(status)
            except Exception as e:
                self.logger.error(f"Error in status callback: {e}")

    def _notify_analyzer_status_change(self, status: ProcessStatus):
        """通知分析器状态变化"""
        for callback in self._analyzer_status_callbacks:
            try:
                callback(status)
            except Exception as e:
                self.logger.error(f"Error in status callback: {e}")

    def _start_monitoring(self):
        """启动监控线程"""
        if self._is_monitoring:
            return

        self._is_monitoring = True
        self._monitor_thread = threading.Thread(target=self._monitor_health, daemon=True)
        self._monitor_thread.start()
        self.logger.info("Module monitoring started")

    def _stop_monitoring(self):
        """停止监控"""
        self._is_monitoring = False
        if self._monitor_thread:
            self._monitor_thread.join(timeout=2)
            self._monitor_thread = None

    def _monitor_health(self):
        """监控模块健康状态"""
        while self._is_monitoring:
            time.sleep(5)  # 每5秒检查一次

            # 检查录制器API状态
            if self._recorder_api is not None:
                try:
                    # 通过调用API来检查健康状态
                    status = self._recorder_api.status
                    if hasattr(status, 'name') and 'ERROR' in status.name:
                        self.logger.warning("RecorderAPI in error state")
                        self._recorder_status = ProcessStatus.ERROR
                except Exception as e:
                    self.logger.error(f"RecorderAPI health check failed: {e}")
                    self._recorder_status = ProcessStatus.ERROR

            # 检查分析器API状态
            if self._analyzer_api is not None:
                try:
                    status = self._analyzer_api.status
                    if hasattr(status, 'name') and 'ERROR' in status.name:
                        self.logger.warning("AnalyzerAPI in error state")
                        self._analyzer_status = ProcessStatus.ERROR
                except Exception as e:
                    self.logger.error(f"AnalyzerAPI health check failed: {e}")
                    self._analyzer_status = ProcessStatus.ERROR


# 为兼容性保留ProcessManager别名
ProcessManager = ModuleManager
