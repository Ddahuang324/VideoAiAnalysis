# RecorderMode 双模式使用指南

## 概述

`RecorderMode` 提供两种录制模式:
- **VIDEO**: 视频模式 (高帧率, 默认 30fps)
- **SNAPSHOT**: 快照模式 (低帧率, 1fps)

## 模式枚举

```python
from services.video_service import RecorderMode

# 两种模式
RecorderMode.VIDEO     # 值: 0, 视频模式 (30fps)
RecorderMode.SNAPSHOT  # 值: 1, 快照模式 (1fps)
```

## 使用方法

### 方法 1: 在启动录制时指定模式

```python
from services.video_service import ScreenRecorderService, RecorderMode

recorder = ScreenRecorderService()

# 使用 VIDEO 模式录制 (30fps)
recorder.start_recording("output_video.mp4", mode=RecorderMode.VIDEO)

# 或使用 SNAPSHOT 模式录制 (1fps)
recorder.start_recording("output_snapshot.mp4", mode=RecorderMode.SNAPSHOT)

# 不指定模式,使用当前设置的模式
recorder.start_recording("output_default.mp4")
```

### 方法 2: 预先设置模式

```python
from services.video_service import ScreenRecorderService, RecorderMode

recorder = ScreenRecorderService()

# 设置为 SNAPSHOT 模式
recorder.set_recorder_mode(RecorderMode.SNAPSHOT)

# 启动录制 (使用 SNAPSHOT 模式)
recorder.start_recording("output.mp4")

# 获取当前模式
current_mode = recorder.get_recorder_mode()
print(f"当前模式: {current_mode.name}")  # 输出: SNAPSHOT
```

### 方法 3: 在统计信息中查看模式

```python
recorder = ScreenRecorderService()

# 获取统计信息
stats = recorder.get_stats()
print(stats)

# 输出示例:
# {
#     'frame_count': 0,
#     'encoded_count': 0,
#     'dropped_count': 0,
#     'output_file_size': 0,
#     'current_fps': 0.0,
#     'is_recording': False,
#     'recorder_mode': 'VIDEO'  # ← 当前模式
# }
```

## 完整示例

### 示例 1: 录制高帧率视频

```python
from services.video_service import ScreenRecorderService, RecorderMode
import time

recorder = ScreenRecorderService()

# 设置进度回调
def on_progress(frames, size):
    print(f"已编码 {frames} 帧, 文件大小 {size} 字节")

recorder.set_progress_callback(on_progress)

# 使用 VIDEO 模式录制
print("开始录制高帧率视频...")
recorder.start_recording("high_fps_video.mp4", mode=RecorderMode.VIDEO)

# 录制 10 秒
time.sleep(10)

# 停止录制
recorder.stop_recording()
print("录制完成!")
```

### 示例 2: 录制低帧率快照

```python
from services.video_service import ScreenRecorderService, RecorderMode
import time

recorder = ScreenRecorderService()

# 使用 SNAPSHOT 模式录制
print("开始录制快照序列...")
recorder.start_recording("snapshot_sequence.mp4", mode=RecorderMode.SNAPSHOT)

# 录制 60 秒 (将产生约 60 帧)
time.sleep(60)

# 停止录制
recorder.stop_recording()
print("快照序列录制完成!")
```

### 示例 3: 动态切换模式

```python
from services.video_service import ScreenRecorderService, RecorderMode

recorder = ScreenRecorderService()

# 第一次录制: VIDEO 模式
print("第一次录制: VIDEO 模式")
recorder.set_recorder_mode(RecorderMode.VIDEO)
recorder.start_recording("video1.mp4")
# ... 录制 ...
recorder.stop_recording()

# 第二次录制: SNAPSHOT 模式
print("第二次录制: SNAPSHOT 模式")
recorder.set_recorder_mode(RecorderMode.SNAPSHOT)
recorder.start_recording("snapshot1.mp4")
# ... 录制 ...
recorder.stop_recording()
```

## 注意事项

1. **录制前设置**: 必须在开始录制前设置模式,录制过程中无法切换
   ```python
   recorder.start_recording("output.mp4")
   # ❌ 错误: 录制中无法切换模式
   recorder.set_recorder_mode(RecorderMode.SNAPSHOT)  # 会抛出异常
   ```

2. **模式差异**:
   - **VIDEO 模式**: 适合录制流畅的视频,帧率高 (30fps),文件较大
   - **SNAPSHOT 模式**: 适合录制时间序列快照,帧率低 (1fps),文件较小

3. **采集器选择**:
   - **VIDEO 模式**: 自动选择最佳采集器 (Windows 上通常是 GDI)
   - **SNAPSHOT 模式**: 使用 GDI 采集器 (更适合低帧率)

## C++ 层实现细节

### 帧率控制

```cpp
// ScreenRecorder.cpp:53
int target_fps = (m_mode == RecorderMode::SNAPSHOT) ? 1 : m_grabber_->getFps();
```

### 采集器选择

```cpp
// ScreenRecorder.cpp:31-32
GrabberType preferredType = 
    (m_mode == RecorderMode::SNAPSHOT) ? GrabberType::GDI : GrabberType::AUTO;
```

### 编码器配置

```cpp
// ScreenRecorder.cpp:73-75
if (m_mode == RecorderMode::SNAPSHOT) {
    config.fps = 1;
}
```

## 测试

运行测试脚本验证功能:

```bash
python tests/test_recorder_modes.py
```

## API 参考

### `RecorderMode` 枚举

| 值 | 名称 | 描述 | 帧率 |
|----|------|------|------|
| 0 | VIDEO | 视频模式 | ~30fps |
| 1 | SNAPSHOT | 快照模式 | 1fps |

### `ScreenRecorderService` 方法

#### `set_recorder_mode(mode: RecorderMode)`
设置录制模式

**参数**:
- `mode`: RecorderMode.VIDEO 或 RecorderMode.SNAPSHOT

**异常**:
- `RuntimeError`: 录制过程中尝试切换模式

#### `get_recorder_mode() -> RecorderMode`
获取当前录制模式

**返回**: RecorderMode 枚举值

#### `start_recording(output_path: str, mode: Optional[RecorderMode] = None) -> bool`
开始录制

**参数**:
- `output_path`: 输出文件路径
- `mode`: 可选,录制模式

**返回**: 成功返回 True

#### `get_stats() -> Dict[str, Any]`
获取录制统计信息

**返回**: 包含以下字段的字典:
- `frame_count`: 已捕获帧数
- `encoded_count`: 已编码帧数
- `dropped_count`: 丢弃帧数
- `output_file_size`: 输出文件大小 (字节)
- `current_fps`: 当前帧率
- `is_recording`: 是否正在录制
- `recorder_mode`: 当前录制模式 ('VIDEO' 或 'SNAPSHOT')
