"""
AI Video Analysis System - 主程序入口
采用 MVVM 架构,Python 负责业务逻辑,C++ 负责底层功能
"""
import sys
from pathlib import Path

# 添加项目根目录到 Python 路径
project_root = Path(__file__).parent.parent
sys.path.insert(0, str(project_root))

from PySide6.QtWidgets import QApplication
from PySide6.QtQml import QQmlApplicationEngine
from PySide6.QtCore import QUrl

from viewmodels.main_viewmodel import MainViewModel
from viewmodels.video_viewmodel import VideoViewModel


def main():
    """主函数"""
    print("=" * 70)
    print("AI Video Analysis System - MVVM Architecture")
    print("Python: Business Logic | C++: Core Functions")
    print("=" * 70)
    
    # 1. 创建 Qt 应用
    app = QApplication(sys.argv)
    app.setApplicationName("AI Video Analysis System")
    app.setOrganizationName("AI Video Team")
    
    # 2. 创建 QML 引擎
    engine = QQmlApplicationEngine()
    
    # 3. 创建 ViewModels
    main_viewmodel = MainViewModel()
    video_viewmodel = VideoViewModel()
    
    # 4. 将 ViewModels 注入到 QML 上下文
    context = engine.rootContext()
    context.setContextProperty("mainViewModel", main_viewmodel)
    context.setContextProperty("videoViewModel", video_viewmodel)
    
    # 5. 加载 QML 文件
    qml_file = project_root / "python" / "views" / "main.qml"
    if not qml_file.exists():
        print(f"❌ Error: QML file not found: {qml_file}")
        return -1
    
    engine.load(QUrl.fromLocalFile(str(qml_file)))
    
    # 6. 检查加载结果
    if not engine.rootObjects():
        print("❌ Error: Failed to load QML file")
        return -1
    
    print("\n✅ Application started successfully!")
    print(f"📝 QML file: {qml_file}")
    print(f"🏗️  Architecture: MVVM")
    print(f"🐍 Python: {sys.version.split()[0]}")
    print(f"🔗 C++ module: video_analysis_cpp")
    print("\n" + "=" * 70)
    
    # 7. 初始化 ViewModels
    main_viewmodel.initialize()
    
    # 8. 进入事件循环
    return app.exec()


if __name__ == "__main__":
    sys.exit(main())
