# 视频服务层使用指南

基于 C++ ScreenRecorder 绑定实现的 Python 服务层，提供了高级的屏幕录制功能。

## 目录

- [安装与构建](#安装与构建)
- [快速开始](#快速开始)
- [核心功能](#核心功能)
- [API 参考](#api-参考)
- [示例代码](#示例代码)
- [常见问题](#常见问题)

---

## 安装与构建

### 1. 构建 C++ 模块

```bash
# Windows
.\scripts\cmake_configure.bat
.\scripts\cmake_build.bat
.\scripts\cmake_build_python.bat
```

### 2. 安装 Python 依赖

```bash
pip install -r requirements.txt
```

### 3. 验证安装

```python
from services.video_service import ScreenRecorderService

service = ScreenRecorderService()
print(service)  # 应该显示服务信息
```

---

## 快速开始

### 基本录制

```python
from services.video_service import ScreenRecorderService
import time

# 创建服务
service = ScreenRecorderService()

# 开始录制
service.start_recording("output.mp4")

# 录制 5 秒
time.sleep(5)

# 停止录制
service.stop_recording()

# 查看统计信息
stats = service.get_stats()
print(f"录制了 {stats['frame_count']} 帧")
print(f"文件大小: {stats['output_file_size'] / 1024:.2f} KB")
```

### 使用上下文管理器（推荐）

```python
from services.video_service import ScreenRecorderService
import time

with ScreenRecorderService() as service:
    service.start_recording("output.mp4")
    time.sleep(5)
    # 自动停止录制
```

---

## 核心功能

### 1. 录制控制

#### 开始录制
```python
success = service.start_recording("output.mp4")
if success:
    print("录制已开始")
```

#### 停止录制
```python
service.stop_recording()
```

#### 暂停/恢复录制
```python
service.pause_recording()   # 暂停
service.resume_recording()  # 恢复
```

#### 检查录制状态
```python
is_recording = service.is_recording()
```

---

### 2. 统计信息

获取实时录制统计：

```python
stats = service.get_stats()
print(f"帧数: {stats['frame_count']}")
print(f"已编码: {stats['encoded_count']}")
print(f"丢帧数: {stats['dropped_count']}")
print(f"文件大小: {stats['output_file_size']} 字节")
print(f"当前 FPS: {stats['current_fps']}")
print(f"录制中: {stats['is_recording']}")
```

---

### 3. 回调函数

#### 进度回调

```python
def on_progress(frames: int, size: int):
    print(f"已编码 {frames} 帧, 文件大小 {size / 1024:.2f} KB")

service.set_progress_callback(on_progress)
```

#### 错误回调

```python
def on_error(error_message: str):
    print(f"录制错误: {error_message}")

service.set_error_callback(on_error)
```

**注意**: 
- 回调函数在 C++ 编码线程中调用
- 已自动处理 GIL（全局解释器锁），确保线程安全
- 回调函数应该快速返回，避免阻塞编码线程

---

### 4. 编码器配置

#### 创建自定义配置

```python
from services.video_service import EncoderPreset, VideoCodec

config = service.create_encoder_config(
    width=1920,          # 视频宽度
    height=1080,         # 视频高度
    fps=60,              # 帧率
    bitrate=8000000,     # 码率 (8 Mbps)
    crf=20,              # 质量参数 (0-51, 越小越好)
    preset=EncoderPreset.FASTER,  # 编码速度预设
    codec=VideoCodec.H264         # 编码器
)

print(config)
```

#### 获取默认配置

```python
config = service.get_default_encoder_config(1920, 1080)
```

#### 编码预设选项

| 预设 | 速度 | 文件大小 | 适用场景 |
|------|------|----------|----------|
| `ultrafast` | 最快 | 最大 | 实时编码 |
| `superfast` | 很快 | 较大 | 快速编码 |
| `veryfast` | 快 | 较大 | 一般使用 |
| `faster` | 较快 | 中等 | 推荐 |
| `fast` | 正常 | 中等 | 推荐 |
| `medium` | 正常 | 中等 | 默认 |
| `slow` | 较慢 | 较小 | 高质量 |
| `slower` | 慢 | 小 | 归档 |
| `veryslow` | 最慢 | 最小 | 最高质量 |

#### 支持的编码器

| 编码器 | 说明 | 兼容性 |
|--------|------|--------|
| `libx264` | H.264 编码器 | 最佳 |
| `libx265` | H.265/HEVC 编码器 | 良好 |
| `libvpx-vp9` | VP9 编码器 | 良好 |

---

## API 参考

### ScreenRecorderService

#### 构造函数

```python
service = ScreenRecorderService()
```

#### 方法

| 方法 | 参数 | 返回值 | 说明 |
|------|------|--------|------|
| `start_recording(path)` | `path: str` | `bool` | 开始录制到指定路径 |
| `stop_recording()` | - | - | 停止录制 |
| `pause_recording()` | - | - | 暂停录制 |
| `resume_recording()` | - | - | 恢复录制 |
| `is_recording()` | - | `bool` | 检查是否正在录制 |
| `get_stats()` | - | `dict` | 获取统计信息 |
| `set_progress_callback(callback)` | `callback: Callable[[int, int], None]` | - | 设置进度回调 |
| `set_error_callback(callback)` | `callback: Callable[[str], None]` | - | 设置错误回调 |
| `create_encoder_config(...)` | 多个参数 | `EncoderConfig` | 创建编码器配置 |
| `get_default_encoder_config(width, height)` | `width: int, height: int` | `EncoderConfig` | 获取默认配置 |

#### 统计信息字典

```python
{
    'frame_count': int,        # 已捕获的帧数
    'encoded_count': int,      # 已编码的帧数
    'dropped_count': int,      # 丢弃的帧数
    'output_file_size': int,   # 输出文件大小（字节）
    'current_fps': float,      # 当前帧率
    'is_recording': bool       # 是否正在录制
}
```

---

## 示例代码

### 示例 1: 完整的录制流程

```python
from services.video_service import ScreenRecorderService
import time

def main():
    # 创建服务
    service = ScreenRecorderService()
    
    # 设置回调
    def on_progress(frames, size):
        print(f"进度: {frames} 帧, {size/1024:.1f} KB")
    
    def on_error(error):
        print(f"错误: {error}")
    
    service.set_progress_callback(on_progress)
    service.set_error_callback(on_error)
    
    # 开始录制
    output_path = "my_recording.mp4"
    if service.start_recording(output_path):
        print(f"录制开始: {output_path}")
        
        try:
            # 录制 10 秒
            for i in range(10):
                time.sleep(1)
                stats = service.get_stats()
                print(f"[{i+1}s] FPS: {stats['current_fps']:.1f}, "
                      f"帧数: {stats['frame_count']}")
        
        finally:
            # 确保停止录制
            service.stop_recording()
            print("录制已停止")
        
        # 显示最终统计
        final_stats = service.get_stats()
        print(f"\n录制完成:")
        print(f"  总帧数: {final_stats['frame_count']}")
        print(f"  已编码: {final_stats['encoded_count']}")
        print(f"  丢帧数: {final_stats['dropped_count']}")
        print(f"  文件大小: {final_stats['output_file_size'] / 1024 / 1024:.2f} MB")

if __name__ == "__main__":
    main()
```

### 示例 2: 暂停和恢复

```python
from services.video_service import ScreenRecorderService
import time

def record_with_pause():
    with ScreenRecorderService() as service:
        service.start_recording("pause_test.mp4")
        
        # 录制 3 秒
        print("录制中...")
        time.sleep(3)
        
        # 暂停 2 秒
        service.pause_recording()
        print("已暂停")
        time.sleep(2)
        
        # 恢复录制 3 秒
        service.resume_recording()
        print("已恢复")
        time.sleep(3)
        
        # 自动停止

record_with_pause()
```

### 示例 3: 高级配置

```python
from services.video_service import (
    ScreenRecorderService,
    EncoderPreset,
    VideoCodec
)
import time

def high_quality_recording():
    service = ScreenRecorderService()
    
    # 创建高质量配置
    config = service.create_encoder_config(
        width=1920,
        height=1080,
        fps=60,              # 60 帧
        bitrate=10000000,    # 10 Mbps
        crf=18,              # 高质量
        preset=EncoderPreset.SLOW,
        codec=VideoCodec.H264
    )
    
    print(f"使用配置: {config}")
    
    # 注意: 当前版本可能不支持动态设置配置
    # 这里仅展示配置对象的创建
    
    service.start_recording("high_quality.mp4")
    time.sleep(5)
    service.stop_recording()

high_quality_recording()
```

---

## 常见问题

### Q1: 如何设置录制分辨率？

A: 当前版本的录制分辨率由系统屏幕分辨率决定。可以通过 `EncoderConfig` 配置输出分辨率（如果 C++ 端支持）。

### Q2: 支持哪些视频格式？

A: 主要支持 MP4 容器格式，编码器支持 H.264、H.265 和 VP9。

### Q3: 如何优化录制性能？

A: 
- 使用更快的编码预设（如 `faster` 或 `veryfast`）
- 降低帧率（如 30fps）
- 适当降低码率
- 确保有足够的 CPU 资源

### Q4: 回调函数线程安全吗？

A: 是的。所有回调函数都已经过 GIL 包装，在 C++ 编码线程中调用时会自动获取 GIL，确保线程安全。

### Q5: 如何处理录制错误？

A: 
1. 使用 `set_error_callback()` 设置错误回调
2. 使用 try-except 捕获异常
3. 检查 `start_recording()` 的返回值

示例:
```python
try:
    service.set_error_callback(lambda e: print(f"错误: {e}"))
    if not service.start_recording("output.mp4"):
        print("启动录制失败")
except Exception as e:
    print(f"异常: {e}")
```

### Q6: 可以同时录制多个实例吗？

A: 理论上可以，但需要注意系统资源限制。多个录制实例会消耗大量 CPU 和内存。

### Q7: 如何获取更详细的调试信息？

A: 服务层会输出调试信息到控制台。您也可以：
1. 设置错误回调查看错误详情
2. 定期获取统计信息监控录制状态
3. 检查 `get_stats()` 中的丢帧数

---

## 运行测试

### 运行示例

```bash
cd python
python examples/screen_recording_example.py
```

### 运行单元测试

```bash
cd python
python tests/test_video_service.py
```

### 运行快速测试

```bash
cd python
python -c "from tests.test_video_service import run_quick_test; run_quick_test()"
```

---

## 更多资源

- [C++ 绑定代码](../cpp/bindings/)
- [示例代码](examples/screen_recording_example.py)
- [单元测试](tests/test_video_service.py)

---

## 版本历史

- **v1.0.0** - 初始版本
  - 基本录制功能
  - 暂停/恢复支持
  - 进度和错误回调
  - 统计信息获取
  - 上下文管理器支持

---

## 许可证

请参考项目根目录的 LICENSE 文件。
