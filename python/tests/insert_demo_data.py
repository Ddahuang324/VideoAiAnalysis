"""
插入演示数据到数据库
用于测试 Library 页面的真实数据展示
"""
import sys
from pathlib import Path

# 添加项目路径
project_root = Path(__file__).parent.parent.parent
sys.path.insert(0, str(project_root / "python"))

from datetime import datetime, timedelta
from services.history_service import HistoryService

# 模拟的 AI 分析 Markdown 结果
DEMO_ANALYSIS_MARKDOWN = """
# 视频分析报告：进程分离架构设计评审

## 📊 Key Findings

| 发现项 | 严重性 | 时间戳 |
|--------|--------|--------|
| 架构耦合问题 | 🔴 高 | 00:02:15 |
| ZMQ通信冗余 | 🟡 中 | 00:05:32 |
| Python层业务逻辑缺失 | 🔴 高 | 00:08:47 |
| 进程同步复杂度 | 🟡 中 | 00:12:03 |

---

## 🎯 核心问题分析

### 1. 当前架构问题 (00:02:15 - 00:04:30)

```mermaid
graph TB
    subgraph "问题诊断"
        A[ScreenRecorder 单体类] --> B[职责不清晰]
        A --> C[难以维护]
        A --> D[资源隔离差]
    end

    style A fill:#ff9999
```

**关键帧截图分析**：
- 在 `00:02:15` 检测到代码结构问题
- `ScreenRecorder` 类同时包含采集、编码、发布、接收逻辑
- 建议拆分为独立模块

### 2. 目标架构设计 (00:05:32 - 00:10:15)

```mermaid
graph LR
    subgraph "推荐架构"
        PM[ProcessManager] --> RP[RecorderProcess]
        PM --> AP[AnalyzerProcess]
        RP -->|ZMQ| AP
    end

    style PM fill:#90EE90
    style RP fill:#87CEEB
    style AP fill:#FFD700
```

**优化建议**：
1. ✅ 将录制与分析分离为独立进程
2. ✅ 使用 ZMQ 进行进程间通信
3. ✅ Python 层实现业务逻辑

---

## 📈 时间线分析

| 时间段 | 内容 | 重要性 |
|--------|------|--------|
| 00:00:00 - 00:02:00 | 项目背景介绍 | ⭐ |
| 00:02:15 - 00:05:30 | 当前架构问题分析 | ⭐⭐⭐ |
| 00:05:32 - 00:10:15 | 目标架构设计 | ⭐⭐⭐ |
| 00:10:20 - 00:15:00 | C++ 层设计方案 | ⭐⭐ |
| 00:15:05 - 00:20:30 | Python 层重构 | ⭐⭐⭐ |
| 00:20:35 - 00:25:00 | 实施计划 | ⭐⭐ |

---

## 🔧 技术细节

### RecorderAPI 接口设计

```cpp
class RecorderAPI {
public:
    bool initialize(const Config& config);
    bool start();
    void pause();
    void resume();
    void stop();
    void shutdown();

    RecordingStatus getStatus() const;
    RecordingStats getStats() const;
};
```

### AnalyzerAPI 接口设计

```cpp
class AnalyzerAPI {
public:
    bool initialize(const Config& config);
    bool start();
    void stop();
    void shutdown();

    AnalysisStatus getStatus() const;
    AnalysisStats getStats() const;
};
```

---

## ⚠️ 风险评估

```mermaid
pie title 风险分布
    "ZMQ通信延迟" : 25
    "进程同步复杂" : 30
    "调试困难" : 20
    "Python重构" : 25
```

---

## 🎯 预期收益

- ✅ **职责清晰**: 录制与分析完全解耦
- ✅ **独立部署**: 可单独升级某个进程
- ✅ **资源隔离**: 分析进程崩溃不影响录制
- ✅ **并行处理**: 录制和分析真正并行

---

## 📋 总结

本次视频分析共检测到 **4 个关键问题**，提出了 **进程分离架构** 的解决方案。
建议优先处理架构耦合和 Python 层业务逻辑缺失问题。

**分析完成时间**: {analysis_time}
**总关键帧数**: 12
**分析置信度**: 94.7%
"""


def main():
    # 初始化服务
    data_dir = project_root / "data" / "history"
    service = HistoryService(str(data_dir))

    # 创建演示录制记录 - 使用时间戳生成唯一ID
    now = datetime.now()
    unique_id = f"demo-arch-{now.strftime('%Y%m%d%H%M%S')}"
    unique_path = f"D:/recordings/architecture_review_{now.strftime('%H%M%S')}.mp4"

    record_id = service.start_recording(
        file_path=unique_path,
        start_time=now - timedelta(minutes=25),
        record_id=unique_id
    )

    # 更新录制信息
    service.update_recording(
        record_id=record_id,
        end_time=now,
        file_size=156_000_000,  # 156 MB
        duration=25 * 60,  # 25 分钟
        keyframe_count=12,
        notes="进程分离架构设计评审会议录制"
    )

    # 添加分析结果
    analysis_md = DEMO_ANALYSIS_MARKDOWN.replace(
        "{analysis_time}", now.strftime("%Y-%m-%d %H:%M:%S")
    )

    service.add_analysis(
        recording_id=record_id,
        start_time=now - timedelta(minutes=5),
        end_time=now,
        keyframe_count=12,
        analyzed_frames=450,
        results={"markdown": analysis_md}
    )

    print(f"[OK] Demo data inserted")
    print(f"    Record ID: {record_id}")
    print(f"    File: {unique_path}")
    print(f"    Duration: 25:00")
    print(f"    Keyframes: 12")
    print(f"\nPlease start the app to view Library page")


if __name__ == "__main__":
    main()
