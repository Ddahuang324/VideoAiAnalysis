"""
Pybind11 绑定模块测试运行脚本

运行方式:
    python tests/python/run_tests.py
"""

import sys
import os

# 添加测试目录到路径
script_dir = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, script_dir)

import subprocess


def main():
    print("=" * 70)
    print(" " * 20 + "Pybind11 绑定测试套件")
    print("=" * 70)
    print()

    # 添加构建输出目录到路径
    build_python_dir = os.path.abspath(os.path.join(script_dir, "../../build/python"))
    build_bin_dir = os.path.abspath(os.path.join(script_dir, "../../build/bin"))

    if os.path.exists(build_python_dir):
        sys.path.insert(0, build_python_dir)
        if hasattr(os, 'add_dll_directory'):
            os.add_dll_directory(build_python_dir)
            if os.path.exists(build_bin_dir):
                os.add_dll_directory(build_bin_dir)
        print(f"✓ 已添加模块路径: {build_python_dir}")
        if os.path.exists(build_bin_dir):
             print(f"✓ 已添加 DLL 路径: {build_bin_dir}")
    else:
        print(f"⚠ 警告: 构建目录不存在: {build_python_dir}")
        print("  请先编译项目 (cmake --build build)")

    print()
    print("-" * 70)

    # 运行 recorder 测试
    print()
    print(">>> 正在运行 recorder_module 测试...")
    res1 = subprocess.run([sys.executable, os.path.join(script_dir, "test_recorder_module.py")], capture_output=False)

    print()
    print("-" * 70)
    print()

    # 运行 analyzer 测试
    print()
    print(">>> 正在运行 analyzer_module 测试...")
    res2 = subprocess.run([sys.executable, os.path.join(script_dir, "test_analyzer_module.py")], capture_output=False)

    print()
    print("=" * 70)
    print(" " * 25 + "测试套件运行完成")
    print("=" * 70)


if __name__ == "__main__":
    main()
