import os
import sys
import re
from pathlib import Path

def update_cmake_batch(project_root, files_to_add):
    """
    批量将头文件、源文件以及包含目录自动添加到 cpp/CMakeLists.txt 中
    files_to_add: list of tuples (rel_header, rel_cpp)
    """
    if not files_to_add:
        return

    cmake_path = project_root / "cpp" / "CMakeLists.txt"
    if not cmake_path.exists():
        print(f"[!] 警告: 找不到 {cmake_path}")
        return

    with open(cmake_path, "r", encoding="utf-8") as f:
        content = f.read()
    
    lines = content.splitlines(keepends=True)
    
    # 1. 处理头文件和源文件
    h_to_add = [str(Path("include") / f[0]).replace("\\", "/") for f in files_to_add]
    cpp_to_add = [str(Path("src") / f[1]).replace("\\", "/") for f in files_to_add]

    h_to_add = [h for h in h_to_add if h not in content]
    cpp_to_add = [c for c in cpp_to_add if c not in content]

    # 2. 处理包含目录 (target_include_directories)
    # 自动识别所有包含头文件的目录
    dir_set = set()
    for rel_h, _ in files_to_add:
        # 获取头文件所在的目录
        parent = rel_h.parent
        dir_path = f"${{CMAKE_CURRENT_SOURCE_DIR}}/include/{str(parent).replace('\\', '/')}"
        if parent != Path(".") and dir_path not in content:
            dir_set.add(dir_path)
    
    dirs_to_add = sorted(list(dir_set))

    if not h_to_add and not cpp_to_add and not dirs_to_add:
        return

    new_lines = []
    in_h_block = False
    in_cpp_block = False
    in_include_block = False
    
    for line in lines:
        stripped = line.strip()
        # 匹配块结束
        if in_h_block and ")" in stripped:
            for h in sorted(set(h_to_add)):
                new_lines.append(f"    {h}\n")
            in_h_block = False
        
        if in_cpp_block and ")" in stripped:
            for c in sorted(set(cpp_to_add)):
                new_lines.append(f"    {c}\n")
            in_cpp_block = False

        # 仅在主 target_include_directories 块结束时插入（通常是一个独立的右括号）
        if in_include_block and stripped == ")":
            for d in dirs_to_add:
                new_lines.append(f"    {d}\n")
            in_include_block = False
            
        new_lines.append(line)
        
        # 匹配块开始
        if "set(CORE_HEADERS" in stripped:
            in_h_block = True
        elif "add_library(ai_video_core" in stripped:
            in_cpp_block = True
        # 仅匹配顶层的主包含目录配置，排除 if() 缩进里的单行配置
        elif stripped.startswith("target_include_directories(ai_video_core PUBLIC") and ")" not in stripped:
            in_include_block = True

    with open(cmake_path, "w", encoding="utf-8") as f:
        f.writelines(new_lines)
    
    print(f"[*] CMakeLists.txt 已同步更新。")
    if dirs_to_add:
        print(f"[+] 新增包含目录: {len(dirs_to_add)} 个")

def generate_cpp_files():
    script_path = Path(__file__).resolve()
    project_root = script_path.parent.parent
    
    include_dir = project_root / "cpp" / "include"
    src_dir = project_root / "cpp" / "src"
    
    headers = list(include_dir.rglob("*.h")) + list(include_dir.rglob("*.hpp"))
    
    created_files = []
    already_exists_count = 0

    for header_path in headers:
        try:
            rel_path = header_path.relative_to(include_dir)
        except ValueError: continue
            
        rel_cpp_path = rel_path.with_suffix(".cpp")
        cpp_path = src_dir / rel_cpp_path
        
        # 收集所有头文件-源文件对用于 CMake 更新
        created_files.append((rel_path, rel_cpp_path))

        if cpp_path.exists():
            already_exists_count += 1
            continue
            
        cpp_path.parent.mkdir(parents=True, exist_ok=True)
        include_rel_str = str(rel_path).replace("\\", "/")
        with open(cpp_path, "w", encoding="utf-8") as f:
            f.write(f'#include "{include_rel_str}"\n')
        print(f"[+] 创建源文件: {rel_cpp_path}")

    # 批量更新 CMake
    update_cmake_batch(project_root, created_files)

    print("-" * 30)
    print(f"处理完成！跳过已有: {already_exists_count}, 总计同步 CMake 项: {len(created_files)}")

if __name__ == "__main__":
    generate_cpp_files()
