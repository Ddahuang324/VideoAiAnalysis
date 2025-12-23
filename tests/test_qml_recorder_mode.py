"""
测试 QML UI 的 RecorderMode 功能
验证 VideoViewModel 与 RecordPage 的集成
"""
import sys
from pathlib import Path

# 添加 Python 服务路径
project_root = Path(__file__).parent.parent
python_path = project_root / "python"
sys.path.insert(0, str(python_path))

from PySide6.QtCore import QObject, QCoreApplication
from viewmodels.video_viewmodel import VideoViewModel
from services.video_service import RecorderMode

def test_viewmodel_recorder_mode():
    """测试 VideoViewModel 的 RecorderMode 功能"""
    print("=" * 60)
    print("测试 VideoViewModel RecorderMode 功能")
    print("=" * 60)
    
    # 创建 Qt 应用
    app = QCoreApplication(sys.argv)
    
    try:
        # 创建 ViewModel
        viewmodel = VideoViewModel()
        print("\n✅ 成功创建 VideoViewModel")
        
        # 测试 1: 获取默认模式
        print("\n--- 测试 1: 获取默认模式 ---")
        default_mode = viewmodel.recorderMode
        mode_name = viewmodel.getRecorderModeName()
        print(f"默认模式: {mode_name} (值: {default_mode})")
        assert default_mode == RecorderMode.VIDEO.value, "默认模式应该是 VIDEO"
        assert mode_name == "VIDEO", "默认模式名称应该是 VIDEO"
        print("✅ 默认模式正确")
        
        # 测试 2: 设置为 SNAPSHOT 模式
        print("\n--- 测试 2: 设置为 SNAPSHOT 模式 ---")
        viewmodel.setRecorderMode(RecorderMode.SNAPSHOT.value)
        current_mode = viewmodel.recorderMode
        mode_name = viewmodel.getRecorderModeName()
        print(f"当前模式: {mode_name} (值: {current_mode})")
        assert current_mode == RecorderMode.SNAPSHOT.value, "应该是 SNAPSHOT 模式"
        assert mode_name == "SNAPSHOT", "模式名称应该是 SNAPSHOT"
        print("✅ SNAPSHOT 模式设置成功")
        
        # 测试 3: 设置回 VIDEO 模式
        print("\n--- 测试 3: 设置回 VIDEO 模式 ---")
        viewmodel.setRecorderMode(RecorderMode.VIDEO.value)
        current_mode = viewmodel.recorderMode
        mode_name = viewmodel.getRecorderModeName()
        print(f"当前模式: {mode_name} (值: {current_mode})")
        assert current_mode == RecorderMode.VIDEO.value, "应该是 VIDEO 模式"
        assert mode_name == "VIDEO", "模式名称应该是 VIDEO"
        print("✅ VIDEO 模式设置成功")
        
        # 测试 4: 验证信号发射
        print("\n--- 测试 4: 验证信号发射 ---")
        signal_received = []
        
        def on_mode_changed(mode):
            signal_received.append(mode)
            print(f"  📡 收到信号: recorderModeChanged({mode})")
        
        viewmodel.recorderModeChanged.connect(on_mode_changed)
        viewmodel.setRecorderMode(RecorderMode.SNAPSHOT.value)
        
        # 处理事件循环以确保信号被发射
        app.processEvents()
        
        assert len(signal_received) > 0, "应该收到 recorderModeChanged 信号"
        assert signal_received[-1] == RecorderMode.SNAPSHOT.value, "信号值应该是 SNAPSHOT"
        print("✅ 信号发射正常")
        
        # 测试 5: 验证录制时不能切换模式
        print("\n--- 测试 5: 验证录制时不能切换模式 ---")
        # 模拟录制状态
        viewmodel._is_recording = True
        viewmodel.setRecorderMode(RecorderMode.VIDEO.value)
        # 模式不应该改变
        current_mode = viewmodel.recorderMode
        assert current_mode == RecorderMode.SNAPSHOT.value, "录制时不应该改变模式"
        print("✅ 录制时正确阻止模式切换")
        
        # 恢复状态
        viewmodel._is_recording = False
        
        print("\n" + "=" * 60)
        print("🎉 所有 ViewModel 测试通过!")
        print("=" * 60)
        
        # 显示 RecorderMode 枚举信息
        print("\n--- RecorderMode 枚举信息 ---")
        print(f"RecorderMode.VIDEO: {RecorderMode.VIDEO.name} = {RecorderMode.VIDEO.value}")
        print(f"RecorderMode.SNAPSHOT: {RecorderMode.SNAPSHOT.name} = {RecorderMode.SNAPSHOT.value}")
        
        print("\n--- QML 集成说明 ---")
        print("在 QML 中使用:")
        print("  1. videoViewModel.recorderMode  // 获取当前模式 (0=VIDEO, 1=SNAPSHOT)")
        print("  2. videoViewModel.setRecorderMode(0)  // 设置为 VIDEO 模式")
        print("  3. videoViewModel.setRecorderMode(1)  // 设置为 SNAPSHOT 模式")
        print("  4. videoViewModel.getRecorderModeName()  // 获取模式名称")
        print("  5. onRecorderModeChanged: { ... }  // 监听模式变化信号")
        
        return True
        
    except Exception as e:
        print(f"\n❌ 测试失败: {e}")
        import traceback
        traceback.print_exc()
        return False


if __name__ == "__main__":
    success = test_viewmodel_recorder_mode()
    sys.exit(0 if success else 1)
