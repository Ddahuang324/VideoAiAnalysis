#!/usr/bin/env python
"""
调试包装脚本 - 解决 pybind11 C++ 调试符号加载问题

问题：C++ 调试器 (cppvsdbg) 附加到 Python 进程时，pybind11 模块尚未加载，
     导致调试器无法正确加载 .pyd 文件的符号。

解决方案：
1. 首先预加载所有 pybind11 C++ 模块 (.pyd 文件)
2. 在模块加载后暂停，等待 C++ 调试器附加
3. 调试器附加后再执行实际的主程序

使用方法（不修改源代码）：
   python scripts/debug_wrapper.py python/main.py

原理：
   - import 模块 -> 触发 DLL 加载 -> C++ 调试器可以看到模块
   - 暂停等待 -> 给调试器时间附加并加载符号
   - 继续执行 -> 此时断点生效
"""

import sys
import os
import time
import importlib
import importlib.util
from pathlib import Path

# ============== 配置 ==============
# 可通过环境变量覆盖
WAIT_FOR_DEBUGGER = os.environ.get('CPP_DEBUG_WAIT', 'true').lower() == 'true'
WAIT_TIMEOUT_SECONDS = int(os.environ.get('CPP_DEBUG_WAIT_TIMEOUT', '30'))
INTERACTIVE_WAIT = os.environ.get('CPP_DEBUG_INTERACTIVE', 'true').lower() == 'true'

# 项目根目录
PROJECT_ROOT = Path(__file__).parent.parent.resolve()

# pybind11 模块搜索路径
PYBIND_MODULE_PATHS = [
    PROJECT_ROOT / "build" / "python",
    PROJECT_ROOT / "build" / "bin",
    PROJECT_ROOT / "build" / "lib",
]

# 已知的 pybind11 模块名（不带扩展名）
# 可以自动检测，也可以手动指定
KNOWN_MODULES = [
    "recorder_module",
    "analyzer_module",
    # 添加其他 pybind11 模块...
]


def find_pyd_modules(search_paths: list[Path]) -> dict[str, Path]:
    """
    在指定路径中查找所有 .pyd 文件
    返回：{模块名: 完整路径}
    """
    modules = {}
    for search_path in search_paths:
        if not search_path.exists():
            continue
        for pyd_file in search_path.glob("*.pyd"):
            # 提取模块名：video_analysis_cpp.cp312-win_amd64.pyd -> video_analysis_cpp
            module_name = pyd_file.name.split('.')[0]
            if module_name not in modules:
                modules[module_name] = pyd_file
        # 也搜索 .so 文件（Linux/macOS）
        for so_file in search_path.glob("*.so"):
            module_name = so_file.name.split('.')[0]
            if module_name not in modules:
                modules[module_name] = so_file
    return modules


def preload_cpp_module(module_name: str, module_path: Path) -> bool:
    """
    预加载一个 C++ 扩展模块
    返回：是否加载成功
    """
    try:
        # 将模块目录添加到 Python 路径
        module_dir = str(module_path.parent)
        if module_dir not in sys.path:
            sys.path.insert(0, module_dir)
        
        print(f"  📦 Loading: {module_name}")
        print(f"     Path: {module_path}")
        
        # 动态加载模块
        spec = importlib.util.spec_from_file_location(module_name, str(module_path))
        if spec and spec.loader:
            module = importlib.util.module_from_spec(spec)
            sys.modules[module_name] = module
            spec.loader.exec_module(module)
            print(f"  ✅ Loaded successfully!")
            return True
        else:
            print(f"  ⚠️ Could not create module spec")
            return False
            
    except Exception as e:
        print(f"  ❌ Failed to load: {e}")
        return False


def wait_for_debugger_interactive():
    """
    交互式等待调试器附加
    """
    pid = os.getpid()
    print("\n" + "=" * 70)
    print("READY_TO_ATTACH: C++ DEBUGGER ATTACH POINT")
    print("=" * 70)
    print(f"   Process ID (PID): {pid}")
    print(f"   ")
    print(f"   C++ 模块已加载！现在可以附加 C++ 调试器了。")
    print(f"   ")
    print(f"   在 VS Code 中：")
    print(f"   1. 打开 'Run and Debug' 面板 (Ctrl+Shift+D)")
    print(f"   2. 选择 'C++: Attach Auto PID' 配置")
    print(f"   3. 按 F5 附加调试器")
    print(f"   4. 设置 C++ 断点")
    print(f"   5. 按任意键继续执行...")
    print("=" * 70)
    
    try:
        input("\n>>> 按 Enter 键继续执行主程序...")
    except EOFError:
        # 非交互式环境
        print(">>> 非交互式环境，等待 10 秒后继续...")
        time.sleep(10)


def wait_for_debugger_timed(timeout: int):
    """
    定时等待调试器 - 用于自动化场景
    """
    pid = os.getpid()
    print("\n" + "=" * 70)
    print("🔧 C++ DEBUGGER ATTACH POINT")
    print("=" * 70)
    print(f"   Process ID (PID): {pid}")
    print(f"   等待 {timeout} 秒让调试器附加...")
    print("=" * 70)
    
    for i in range(timeout, 0, -1):
        print(f"\r   倒计时: {i} 秒   ", end='', flush=True)
        time.sleep(1)
    print("\n   继续执行...")


def save_pid_to_file():
    """
    保存 PID 到文件，便于 VS Code 读取
    """
    pid = os.getpid()
    pid_file = PROJECT_ROOT / ".vscode" / "pid.txt"
    pid_file.parent.mkdir(parents=True, exist_ok=True)
    pid_file.write_text(str(pid))
    print(f"📝 PID {pid} saved to {pid_file}")


def run_main_script(script_path_str: str, script_args: list[str]):
    """
    运行主脚本
    """
    # 修改 sys.argv 让主脚本认为它是直接运行的
    sys.argv = [script_path_str] + script_args
    
    # 读取并执行主脚本
    script_path = Path(script_path_str).resolve()
    if not script_path.exists():
        print(f"❌ Script not found: {script_path}")
        sys.exit(1)
    
    # 设置 __file__ 和 __name__
    script_globals = {
        '__name__': '__main__',
        '__file__': str(script_path),
        '__builtins__': __builtins__,
    }
    
    print(f"\n🚀 Executing: {script_path}\n")
    print("=" * 70)
    
    # 切换工作目录到项目根目录
    os.chdir(PROJECT_ROOT)
    
    # 执行脚本
    with open(script_path, 'r', encoding='utf-8') as f:
        script_code = f.read()
    
    exec(compile(script_code, str(script_path), 'exec'), script_globals)


def main():
    print("=" * 70)
    print("🔧 C++ Debug Wrapper for pybind11 Modules")
    print("=" * 70)
    print(f"📁 Project root: {PROJECT_ROOT}")
    print(f"🐍 Python: {sys.executable}")
    print(f"   PID: {os.getpid()}")
    print("")
    
    # 保存 PID
    save_pid_to_file()

    # 设置 DLL 搜索路径 (Windows)
    if sys.platform == 'win32':
        dll_paths = [
            PROJECT_ROOT / "build" / "bin",
            PROJECT_ROOT / "build" / "_deps" / "ffmpeg_prebuilt-src" / "bin",
            PROJECT_ROOT / "build" / "_deps" / "opencv_prebuilt-src" / "Debug" / "bin",
            PROJECT_ROOT / "build" / "_deps" / "onnxruntime_prebuilt-src" / "lib",
        ]
        for p in dll_paths:
            if p.exists():
                print(f"  📁 Adding DLL directory: {p}")
                os.add_dll_directory(str(p))
                os.environ['PATH'] = str(p) + os.pathsep + os.environ.get('PATH', '')
    
    # 设置 Python 路径
    python_path = str(PROJECT_ROOT / "python")
    build_python_path = str(PROJECT_ROOT / "build" / "python")
    if python_path not in sys.path:
        sys.path.insert(0, python_path)
    if build_python_path not in sys.path:
        sys.path.insert(0, build_python_path)
    
    # 1. 预加载指定的 pybind11 模块
    print("🔍 Preloading designated pybind11 modules...")
    all_pyd_modules = find_pyd_modules(PYBIND_MODULE_PATHS)
    
    target_modules = ["recorder_module", "analyzer_module"]
    loaded_count = 0
    
    for module_name in target_modules:
        if module_name in all_pyd_modules:
            if preload_cpp_module(module_name, all_pyd_modules[module_name]):
                loaded_count += 1
        else:
            print(f"  ⚠️ Target module not found: {module_name}")
        print("")
    
    print(f"📊 Loaded {loaded_count}/{len(target_modules)} C++ modules")
    
    # 3. 等待调试器附加
    if WAIT_FOR_DEBUGGER and loaded_count > 0:
        if INTERACTIVE_WAIT:
            wait_for_debugger_interactive()
        else:
            wait_for_debugger_timed(WAIT_TIMEOUT_SECONDS)
    
    # 4. 执行主脚本（如果指定了）
    if len(sys.argv) > 1:
        script_path = sys.argv[1]
        script_args = sys.argv[2:]
        run_main_script(script_path, script_args)
    else:
        print("\n✅ C++ modules preloaded. No main script specified.")
        print("   Usage: python debug_wrapper.py <your_script.py> [args...]")


if __name__ == "__main__":
    main()
