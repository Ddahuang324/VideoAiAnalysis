# OpenCV 安装指南

由于 OpenCV 的预编译包比较复杂,推荐使用 **vcpkg** 包管理器来安装。

## 方式一: 使用 vcpkg (推荐)

### 1. 安装 vcpkg

```powershell
# 克隆 vcpkg 仓库
cd C:\
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg

# 运行引导脚本
.\bootstrap-vcpkg.bat
```

### 2. 安装 OpenCV

```powershell
# 安装 OpenCV (x64 Windows)
.\vcpkg install opencv:x64-windows

# 这会自动下载并编译 OpenCV,大约需要 10-20 分钟
```

### 3. 配置 CMake

在项目根目录运行:

```powershell
cmake -B build -S . -G "Ninja" `
  -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
```

---

## 方式二: 手动下载预编译包

### 1. 下载 OpenCV

访问: https://opencv.org/releases/
下载 **OpenCV 4.8.0 - Windows**

### 2. 解压到固定位置

```powershell
# 解压到 C:\opencv
# 最终路径应该是: C:\opencv\build
```

### 3. 设置环境变量

```powershell
# 添加到系统 PATH
$env:Path += ";C:\opencv\build\x64\vc16\bin"

# 或者永久添加
[System.Environment]::SetEnvironmentVariable(
    "Path",
    $env:Path + ";C:\opencv\build\x64\vc16\bin",
    [System.EnvironmentVariableTarget]::User
)
```

### 4. 配置 CMake

```powershell
cmake -B build -S . -G "Ninja" `
  -DOpenCV_DIR=C:/opencv/build
```

---

## 验证安装

运行以下命令验证 OpenCV 是否正确安装:

```powershell
cmake -B build -S . -G "Ninja"
```

预期输出应包含:
```
✓ Found OpenCV via find_package
  Version: 4.8.0
  Include: C:/opencv/build/include
```

---

## 推荐方案

**强烈推荐使用方式一 (vcpkg)**,因为:
- ✅ 自动管理依赖
- ✅ 与 CMake 无缝集成
- ✅ 支持多个版本共存
- ✅ 便于团队协作
