# Pybind11 绑定系统

## 概述

本项目实现了两个独立的 Python 绑定模块，用于控制视频录制和 AI 分析进程：

- **recorder_module** - 录制进程 Python 绑定
- **analyzer_module** - 分析进程 Python 绑定

## 目录结构

```
cpp/bindings/
├── CMakeLists.txt                    # 主构建文件
├── common/                           # 公共类型绑定 (预留)
├── recorder/                         # 录制进程绑定
│   ├── CMakeLists.txt
│   ├── recorder_module.cpp           # 模块入口
│   ├── bind_recorder_api.cpp         # RecorderAPI 绑定
│   ├── bind_recorder_config.cpp      # RecorderConfig 绑定
│   └── bind_recorder_types.cpp       # RecordingStatus/Stats 绑定
└── analyzer/                         # 分析进程绑定
    ├── CMakeLists.txt
    ├── analyzer_module.cpp           # 模块入口
    ├── bind_analyzer_api.cpp         # AnalyzerAPI 绑定
    ├── bind_analyzer_config.cpp      # AnalyzerConfig 绑定
    └── bind_analyzer_types.cpp       # AnalysisStatus/Stats 绑定
```

## 构建方法

### 前置要求

- CMake >= 3.20
- Visual Studio 2019+ (Windows) 或 GCC 10+ (Linux)
- Python 3.8+
- pybind11

### 构建步骤

```bash
# 配置 CMake
cmake -B build -S .

# 构建
cmake --build build --config Release

# 输出文件位置
# Windows: build/python/recorder_module.pyd, analyzer_module.pyd
# Linux:   build/python/recorder_module.cpython-*.so, analyzer_module.cpython-*.so
```

## 使用方法

### Recorder Module

```python
import recorder_module as rec

# 创建默认配置
config = rec.default_recorder_config()
config.video.output_file_path = "output.mp4"
config.video.width = 1920
config.video.height = 1080
config.video.fps = 30

# 使用录制器
with rec.RecorderAPI() as api:
    # 设置回调
    def on_status_change(status):
        print(f"状态变更: {status}")

    api.set_status_callback(on_status_change)

    # 初始化并启动
    api.initialize(config)
    api.start()

    # ... 录制中 ...

    api.stop()
```

### Analyzer Module

```python
import analyzer_module as ana

# 创建默认配置
config = ana.default_analyzer_config()
config.enable_text_recognition = True
config.pipeline.analysis_thread_count = 4

# 使用分析器
with ana.AnalyzerAPI() as api:
    # 设置回调
    def on_keyframe(frame_idx):
        print(f"检测到关键帧: {frame_idx}")

    api.set_keyframe_callback(on_keyframe)

    # 初始化并启动
    api.initialize(config)
    api.start()

    # ... 分析中 ...

    api.stop()
```

## API 参考

### RecorderAPI

| 方法 | 说明 |
|-----|------|
| `initialize(config)` | 初始化录制器 |
| `start()` | 启动录制 |
| `pause()` | 暂停录制 |
| `resume()` | 恢复录制 |
| `stop()` | 停止录制 |
| `shutdown()` | 关闭录制器 |
| `get_status()` | 获取当前状态 |
| `get_stats()` | 获取统计信息 |
| `get_last_error()` | 获取最后错误 |
| `set_status_callback(callback)` | 设置状态回调 |
| `set_error_callback(callback)` | 设置错误回调 |

### RecordingStatus

| 值 | 说明 |
|-----|------|
| `IDLE` | 空闲状态 |
| `INITIALIZING` | 初始化中 |
| `RECORDING` | 录制中 |
| `PAUSED` | 已暂停 |
| `STOPPING` | 停止中 |
| `ERROR` | 错误状态 |

### AnalyzerAPI

| 方法 | 说明 |
|-----|------|
| `initialize(config)` | 初始化分析器 |
| `start()` | 启动分析 |
| `stop()` | 停止分析 |
| `shutdown()` | 关闭分析器 |
| `get_status()` | 获取当前状态 |
| `get_stats()` | 获取统计信息 |
| `get_last_error()` | 获取最后错误 |
| `set_status_callback(callback)` | 设置状态回调 |
| `set_keyframe_callback(callback)` | 设置关键帧回调 |

## 测试

运行测试脚本：

```bash
# 运行所有测试
python tests/python/run_tests.py

# 或单独运行
python tests/python/test_recorder_module.py
python tests/python/test_analyzer_module.py
```

## 设计原则

1. **职责分离** - 录制和分析模块完全独立
2. **类型安全** - 强类型绑定，避免运行时错误
3. **异常透明** - C++ 异常正确传播到 Python
4. **GIL 管理** - 合理使用 `gil_scoped_release` 避免阻塞
5. **上下文管理器** - 支持 `with` 语句自动资源清理

## GIL 管理

所有长时间运行的方法（如 `start()`, `stop()`, `initialize()`）都会自动释放 GIL，避免阻塞其他 Python 线程。回调函数会自动获取 GIL，确保线程安全。
