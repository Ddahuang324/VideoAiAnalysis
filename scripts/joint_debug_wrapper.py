#!/usr/bin/env python
"""
联合调试包装脚本 - 同时支持 Python + C++ 调试

解决方案：
1. 启动 debugpy 监听（不使用 --wait-for-client）
2. 预加载所有 pybind11 C++ 模块
3. 暂停等待用户附加调试器
4. 继续执行主程序

使用方法：
   python scripts/joint_debug_wrapper.py python/main.py
"""

import sys
import os
import time
import importlib
import importlib.util
from pathlib import Path

# ============== 配置 ==============
DEBUGPY_PORT = int(os.environ.get('DEBUGPY_PORT', '5678'))
WAIT_FOR_DEBUGGER = os.environ.get('CPP_DEBUG_WAIT', 'true').lower() == 'true'

# 项目根目录
PROJECT_ROOT = Path(__file__).parent.parent.resolve()

# pybind11 模块搜索路径
PYBIND_MODULE_PATHS = [
    PROJECT_ROOT / "build" / "python",
    PROJECT_ROOT / "build" / "bin",
    PROJECT_ROOT / "build" / "lib",
]


def find_pyd_modules(search_paths: list[Path]) -> dict[str, Path]:
    """在指定路径中查找所有 .pyd/.so 文件"""
    modules = {}
    for search_path in search_paths:
        if not search_path.exists():
            continue
        for pyd_file in search_path.glob("*.pyd"):
            module_name = pyd_file.name.split('.')[0]
            if module_name not in modules:
                modules[module_name] = pyd_file
        for so_file in search_path.glob("*.so"):
            module_name = so_file.name.split('.')[0]
            if module_name not in modules:
                modules[module_name] = so_file
    return modules


def preload_cpp_module(module_name: str, module_path: Path) -> bool:
    """预加载一个 C++ 扩展模块"""
    try:
        module_dir = str(module_path.parent)
        if module_dir not in sys.path:
            sys.path.insert(0, module_dir)
        
        print(f"  📦 Loading: {module_name}")
        print(f"     Path: {module_path}")
        
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


def setup_debugpy():
    """启动 debugpy 监听"""
    try:
        import debugpy
        import socket
        import subprocess
        
        # 检查端口是否可用
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        result = sock.connect_ex(('127.0.0.1', DEBUGPY_PORT))
        sock.close()
        
        if result == 0:
            print(f"⚠️ Port {DEBUGPY_PORT} already in use, attempting to free it...")
            try:
                # 尝试终止占用端口的进程 (Windows)
                if sys.platform == 'win32':
                    subprocess.run(
                        ['powershell', '-Command', 
                         f'Get-NetTCPConnection -LocalPort {DEBUGPY_PORT} -ErrorAction SilentlyContinue | '
                         f'ForEach-Object {{ Stop-Process -Id $_.OwningProcess -Force -ErrorAction SilentlyContinue }}'],
                        capture_output=True, timeout=5
                    )
                    time.sleep(0.5)
                    print(f"✅ Port {DEBUGPY_PORT} freed")
            except Exception as e:
                print(f"⚠️ Could not free port: {e}")
                return False
        
        # 启动 debugpy 监听
        debugpy.listen(('127.0.0.1', DEBUGPY_PORT))
        print(f"🐍 Debugpy listening on port {DEBUGPY_PORT}")
        return True
        
    except ImportError:
        print("⚠️ debugpy not installed, Python debugging disabled")
        return False
    except Exception as e:
        print(f"⚠️ Failed to setup debugpy: {e}")
        return False


def save_pid_to_file():
    """保存 PID 到文件"""
    pid = os.getpid()
    pid_file = PROJECT_ROOT / ".vscode" / "pid.txt"
    pid_file.parent.mkdir(parents=True, exist_ok=True)
    pid_file.write_text(str(pid))
    print(f"📝 PID {pid} saved to {pid_file}")


def wait_for_debuggers():
    """等待用户附加调试器"""
    pid = os.getpid()
    print("\n" + "=" * 70)
    print("🔧 ATTACH DEBUGGERS NOW!")
    print("=" * 70)
    print(f"   Process ID (PID): {pid}")
    print(f"   Debugpy Port: {DEBUGPY_PORT}")
    print(f"   ")
    print(f"   请在 VS Code 中附加调试器：")
    print(f"   ")
    print(f"   ▶ Python 调试器:")
    print(f"     选择 'Python: Attach to Port' → F5")
    print(f"   ")
    print(f"   ▶ C++ 调试器:")
    print(f"     选择 '🔧 C++: Attach with Preload' → F5")
    print(f"   ")
    print(f"   附加完成后，设置断点，然后按 Enter 继续...")
    print("=" * 70)
    
    try:
        input("\n>>> 按 Enter 键继续执行主程序...")
    except EOFError:
        print(">>> 非交互式环境，等待 15 秒后继续...")
        time.sleep(15)


def run_main_script(script_path: str, script_args: list[str]):
    """运行主脚本"""
    sys.argv = [script_path] + script_args
    
    script_path = Path(script_path).resolve()
    if not script_path.exists():
        print(f"❌ Script not found: {script_path}")
        sys.exit(1)
    
    script_globals = {
        '__name__': '__main__',
        '__file__': str(script_path),
        '__builtins__': __builtins__,
    }
    
    print(f"\n🚀 Executing: {script_path}\n")
    print("=" * 70)
    
    os.chdir(PROJECT_ROOT)
    
    with open(script_path, 'r', encoding='utf-8') as f:
        script_code = f.read()
    
    exec(compile(script_code, str(script_path), 'exec'), script_globals)


def main():
    print("=" * 70)
    print("🔧 Joint Debug Wrapper (Python + C++)")
    print("=" * 70)
    print(f"📁 Project root: {PROJECT_ROOT}")
    print(f"🐍 Python: {sys.executable}")
    print(f"   PID: {os.getpid()}")
    print("")
    
    # 保存 PID
    save_pid_to_file()
    
    # 设置 Python 路径
    python_path = str(PROJECT_ROOT / "python")
    build_python_path = str(PROJECT_ROOT / "build" / "python")
    if python_path not in sys.path:
        sys.path.insert(0, python_path)
    if build_python_path not in sys.path:
        sys.path.insert(0, build_python_path)
    
    # 1. 启动 debugpy 监听
    print("🐍 Setting up Python debugger (debugpy)...")
    debugpy_ready = setup_debugpy()
    print("")
    
    # 2. 查找并预加载所有 pybind11 模块
    print("🔍 Searching for pybind11 modules...")
    pyd_modules = find_pyd_modules(PYBIND_MODULE_PATHS)
    
    if not pyd_modules:
        print("⚠️ No .pyd modules found.")
    else:
        print(f"   Found {len(pyd_modules)} module(s):\n")
    
    loaded_count = 0
    for module_name, module_path in pyd_modules.items():
        if preload_cpp_module(module_name, module_path):
            loaded_count += 1
        print("")
    
    print(f"📊 Loaded {loaded_count}/{len(pyd_modules)} C++ modules")
    
    # 3. 等待调试器附加
    if WAIT_FOR_DEBUGGER:
        wait_for_debuggers()
    
    # 4. 执行主脚本
    if len(sys.argv) > 1:
        script_path = sys.argv[1]
        script_args = sys.argv[2:]
        run_main_script(script_path, script_args)
    else:
        print("\n✅ Modules preloaded. No main script specified.")
        print("   Usage: python joint_debug_wrapper.py <your_script.py> [args...]")


if __name__ == "__main__":
    main()
