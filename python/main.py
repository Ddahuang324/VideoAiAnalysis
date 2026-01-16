"""
AI Video Analysis System - 主程序入口 (新架构)
采�� MVVM 架构，分层设计：基础设施层 -> 服务层 -> 视图模型层 -> UI层
"""
import sys
import os
from pathlib import Path
from typing import Optional
from dotenv import load_dotenv

# 添加项目根目录到 Python 路径
project_root = Path(__file__).parent.parent

# 加载 .env 文件
load_dotenv(project_root / ".env")
sys.path.insert(0, str(project_root))

# 添加 C++ Python 绑定目录
build_python_path = project_root / "build" / "python"
if build_python_path.exists():
    sys.path.insert(0, str(build_python_path))

# 添加 FFmpeg DLL 目录到 PATH (Windows)
if sys.platform == 'win32':
    bin_path = project_root / "build" / "bin"
    ffmpeg_bin_path = project_root / "build" / "_deps" / "ffmpeg_prebuilt-src" / "bin"

    current_path = os.environ.get('PATH', '')
    new_paths = []

    if bin_path.exists():
        new_paths.append(str(bin_path))
        if hasattr(os, 'add_dll_directory'):
            try:
                os.add_dll_directory(str(bin_path))
            except Exception as e:
                print(f"[WARN] Failed to add DLL directory {bin_path}: {e}")

    if ffmpeg_bin_path.exists():
        new_paths.append(str(ffmpeg_bin_path))
        if hasattr(os, 'add_dll_directory'):
            try:
                os.add_dll_directory(str(ffmpeg_bin_path))
            except Exception as e:
                print(f"[WARN] Failed to add DLL directory {ffmpeg_bin_path}: {e}")

    if new_paths:
        os.environ['PATH'] = os.pathsep.join(new_paths + [current_path])

from PySide6.QtWidgets import QApplication
from PySide6.QtQml import QQmlApplicationEngine
from PySide6.QtCore import QUrl


from services.recorder_service import RecorderService
from services.analyzer_service import AnalyzerService
from services.history_service import HistoryService
from services.gemini_service import GeminiService
from viewmodels.recorder_viewmodel import RecorderViewModel
from viewmodels.analyzer_viewmodel import AnalyzerViewModel
from viewmodels.history_viewmodel import HistoryViewModel
from viewmodels.settings_viewmodel import SettingsViewModel
from viewmodels.prompt_viewmodel import PromptViewModel
from database.prompt_template_dao import PromptTemplateDAO
from infrastructure.log_manager import LogManager
from infrastructure.process_manager import ModuleManager

class ApplicationContainer:
    """
    应用容器
    负责依赖注入和组件生命周期管理
    """

    def __init__(self):
        """初始化应用容器"""
        self.app: Optional[QApplication] = None
        self.engine: Optional[QQmlApplicationEngine] = None

        # 基础设施层
        self.log_manager: Optional[LogManager] = None
        self.module_manager: Optional[ModuleManager] = None

        # 服务层
        self.recorder_service: Optional[RecorderService] = None
        self.analyzer_service: Optional[AnalyzerService] = None
        self.history_service: Optional[HistoryService] = None
        self.gemini_service: Optional[GeminiService] = None

        # 视图模型层
        self.recorder_viewmodel: Optional[RecorderViewModel] = None
        self.analyzer_viewmodel: Optional[AnalyzerViewModel] = None
        self.history_viewmodel: Optional[HistoryViewModel] = None
        self.settings_viewmodel: Optional[SettingsViewModel] = None
        self.prompt_viewmodel: Optional[PromptViewModel] = None

    def initialize_infrastructure(self):
        """初始化基础设施层"""
        from infrastructure.log_manager import LogManager
        from infrastructure.process_manager import ModuleManager

        # 创建日志管理器
        log_manager = LogManager()
        self.log_manager = log_manager
        log_manager.info("=" * 70)
        log_manager.info("AI Video Analysis System - MVVM Architecture")
        log_manager.info("Phase 3: Python Layer Refactoring")
        log_manager.info("=" * 70)

        # 创建模块管理器
        self.module_manager = ModuleManager(log_manager)

        print("[OK] Infrastructure layer initialized")

    def initialize_services(self):
        """初始化服务层"""
        from services.recorder_service import RecorderService
        from services.analyzer_service import AnalyzerService
        from services.history_service import HistoryService

        if self.module_manager is None:
            raise RuntimeError("ModuleManager not initialized")

        # 创建历史记录服务
        data_dir = project_root / "data" / "history"
        self.history_service = HistoryService(str(data_dir))

        # 创建分析服务
        self.analyzer_service = AnalyzerService(self.module_manager)
        if self.analyzer_service:
            self.analyzer_service._history_service = self.history_service

        # 创建录制服务
        self.recorder_service = RecorderService(self.module_manager)
        if self.recorder_service:
            self.recorder_service._analyzer_service = self.analyzer_service  # type: ignore
            self.recorder_service._history_service = self.history_service

        # 创建 Gemini AI 服务
        self.gemini_service = GeminiService()

        if self.log_manager:
            self.log_manager.info("Service layer initialized")
        print("[OK] Service layer initialized")

    def initialize_viewmodels(self):
        """初始化视图模型层"""
        from viewmodels.recorder_viewmodel import RecorderViewModel
        from viewmodels.analyzer_viewmodel import AnalyzerViewModel
        from viewmodels.history_viewmodel import HistoryViewModel
        from viewmodels.settings_viewmodel import SettingsViewModel

        if self.recorder_service is None or self.module_manager is None:
            raise RuntimeError("RecorderService or ModuleManager not initialized")

        # 创建录制视图模型
        self.recorder_viewmodel = RecorderViewModel(
            self.recorder_service,
            self.module_manager,
            self.analyzer_service  # 传递analyzer_service
        )

        if self.analyzer_service is None:
             raise RuntimeError("AnalyzerService not initialized")

        # 创建分析视图模型
        self.analyzer_viewmodel = AnalyzerViewModel(
            self.analyzer_service,
            self.module_manager
        )

        if self.history_service is None:
            raise RuntimeError("HistoryService not initialized")

        # 创建历史记录视图模型
        self.history_viewmodel = HistoryViewModel(self.history_service, self.gemini_service)

        # 创建设置视图模型
        self.settings_viewmodel = SettingsViewModel()

        # 创建提示词视图模型
        prompt_dao = PromptTemplateDAO(self.history_service.db_manager)
        self.prompt_viewmodel = PromptViewModel(prompt_dao)
        self.prompt_viewmodel.loadTemplates()

        # 注入 history_viewmodel 和 prompt_viewmodel 到 recorder_viewmodel
        if self.recorder_viewmodel:
            self.recorder_viewmodel._history_viewmodel = self.history_viewmodel
            self.recorder_viewmodel._prompt_viewmodel = self.prompt_viewmodel

        # 将设置视图模型注入到录制服务
        if self.recorder_service and self.settings_viewmodel:
            self.recorder_service._settings_viewmodel = self.settings_viewmodel
        
        # 将设置应用到服务
        if self.recorder_service and self.settings_viewmodel:
            # Pylance 可能无法正确识别 PySide6 Property，使用 bool() 明确转换
            self.recorder_service._auto_enable_realtime = bool(self.settings_viewmodel.analysisRealTime)

            # 监听设置变化
            def on_realtime_changed():
                if self.recorder_service and self.settings_viewmodel:
                    self.recorder_service._auto_enable_realtime = bool(self.settings_viewmodel.analysisRealTime)
            self.settings_viewmodel.analysisRealTimeChanged.connect(on_realtime_changed)

        # 同步录制模式：RecordPage ↔ Settings
        def sync_mode_to_settings():
            """RecordPage 模式变化 → 同步到 Settings"""
            if not self.recorder_viewmodel or not self.settings_viewmodel:
                return
            mode = self.recorder_viewmodel.captureMode
            settings_mode = "snapshot" if mode == "SNAPSHOT MODE" else "video"
            if self.settings_viewmodel.recordingMode != settings_mode:
                self.settings_viewmodel.recordingMode = settings_mode  # type: ignore

        def sync_mode_to_recorder():
            """Settings 模式变化 → 同步到 RecordPage"""
            if not self.recorder_viewmodel or not self.settings_viewmodel:
                return
            mode = self.settings_viewmodel.recordingMode
            recorder_mode = "SNAPSHOT MODE" if mode == "snapshot" else "VIDEO MODE"
            if self.recorder_viewmodel.captureMode != recorder_mode:
                self.recorder_viewmodel.captureMode = recorder_mode  # type: ignore

        if self.recorder_viewmodel and self.settings_viewmodel:
            self.recorder_viewmodel.captureModeChanged.connect(sync_mode_to_settings)
            self.settings_viewmodel.recordingModeChanged.connect(sync_mode_to_recorder)

        # 启动时从 Settings 初始化 RecordPage 的模式
        sync_mode_to_recorder()

        if self.log_manager:
            self.log_manager.info("ViewModel layer initialized")
        print("[OK] ViewModel layer initialized")

    def create_qt_application(self):
        """创建Qt应用"""
        self.app = QApplication(sys.argv)
        self.app.setApplicationName("AI Video Analysis System")
        self.app.setOrganizationName("AI Video Team")

        if self.log_manager:
            self.log_manager.info("Qt application created")
        print("[OK] Qt application created")

    def create_qml_engine(self):
        """创建QML引擎"""
        self.engine = QQmlApplicationEngine()

        # 添加 QML 导入路径
        views_path = project_root / "python" / "views"
        self.engine.addImportPath(str(views_path))

        # PySide6 自带的 Qt QML 模块路径
        import PySide6
        qt_qml_path = Path(PySide6.__file__).resolve().parent / "qml"
        if qt_qml_path.exists():
            self.engine.addImportPath(str(qt_qml_path))
            os.environ.setdefault("QML_IMPORT_PATH", str(qt_qml_path))
            os.environ.setdefault("QT_QUICK_CONTROLS_STYLE", "Material")

        # 注入 ViewModels 到 QML 上下文
        context = self.engine.rootContext()
        context.setContextProperty("recorderViewModel", self.recorder_viewmodel)
        context.setContextProperty("analyzerViewModel", self.analyzer_viewmodel)
        context.setContextProperty("historyViewModel", self.history_viewmodel)
        context.setContextProperty("settingsViewModel", self.settings_viewmodel)
        context.setContextProperty("promptViewModel", self.prompt_viewmodel)

        # 加载 QML 文件
        qml_file = views_path / "main.qml"
        if not qml_file.exists():
            if self.log_manager:
                self.log_manager.error(f"QML file not found: {qml_file}")
            print(f"[ERROR] Error: QML file not found: {qml_file}")
            return False

        self.engine.load(QUrl.fromLocalFile(str(qml_file)))

        if not self.engine.rootObjects():
            if self.log_manager:
                self.log_manager.error("Failed to load QML file")
            print("[ERROR] Error: Failed to load QML file")
            return False

        if self.log_manager:
            self.log_manager.info(f"QML file loaded: {qml_file}")
        print(f"[OK] QML file loaded: {qml_file}")
        return True

    def initialize_services_async(self):
        """异步初始化服务（在QML加载后）"""
        # 初始化录制服务
        if self.recorder_viewmodel:
            self.recorder_viewmodel.initialize()

        # 初始化分析服务
        if self.analyzer_viewmodel:
            self.analyzer_viewmodel.initialize()

        # 加载历史记录
        if self.history_viewmodel:
            self.history_viewmodel.loadHistory()

        if self.log_manager:
            self.log_manager.info("All services initialized")
        print("[OK] All services initialized")

    def run(self):
        """运行应用"""
        try:
            # 1. 创建Qt应用
            self.create_qt_application()

            # 2. 初始化基础设施层
            self.initialize_infrastructure()

            # 3. 初始化服务层
            self.initialize_services()

            # 4. 初始化视图模型层
            self.initialize_viewmodels()

            # 5. 创建QML引擎
            if not self.create_qml_engine():
                return -1

            # 6. 异步初始化服务
            self.initialize_services_async()

            # 打印启动信息
            print("\n" + "=" * 70)
            print("Application started successfully!")
            print(f"QML file: {project_root / 'python' / 'views' / 'main.qml'}")
            print(f"Architecture: MVVM (Phase 3)")
            print(f"Python: {sys.version.split()[0]}")
            print(f"C++ modules: recorder_module, analyzer_module")
            print("=" * 70 + "\n")

            # 7. 进入事件循环
            if self.app:
                return self.app.exec()
            else:
                return -1

        except Exception as e:
            if self.log_manager:
                self.log_manager.error(f"Application error: {e}")
            print(f"[ERROR] Application error: {e}")
            import traceback
            traceback.print_exc()
            return -1

        finally:
            # 清理资源
            self.shutdown()

    def shutdown(self):
        """关闭应用"""
        if self.log_manager:
            self.log_manager.info("Shutting down application...")

        if self.recorder_viewmodel:
            self.recorder_viewmodel.shutdown()
        if self.analyzer_viewmodel:
            self.analyzer_viewmodel.shutdown()

        if self.module_manager:
            self.module_manager.shutdown_all()


def main():
    """主函数"""
    container = ApplicationContainer()
    return container.run()


if __name__ == "__main__":
    sys.exit(main())
