# KeyFrame Analyzer 配置文件说明

本目录包含 KeyFrameAnalyzerService 的配置文件示例。

## 配置文件结构

### analyzer_config.json - 标准配置

这是 KeyFrame 分析器的标准配置文件，包含所有可配置参数。

## 配置项说明

### ZMQ 通信配置

| 参数 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `zmqSubscriber.endpoint` | string | "tcp://localhost:5555" | 订阅帧数据的 ZMQ 端点 |
| `zmqPublisher.endpoint` | string | "tcp://*:5556" | 发布关键帧元数据的 ZMQ 端点 |
| `timeoutMs` | int | 100 | ZMQ 接收超时时间(毫秒) |
| `ioThreads` | int | 1 | ZMQ IO 线程数 |

### 模型路径配置

| 参数 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `models.basePath` | string | "Models" | 模型文件基础路径 |
| `models.sceneModelPath` | string | - | 场景检测模型路径 |
| `models.motionModelPath` | string | - | 运动检测模型路径 |
| `models.textDetModelPath` | string | - | 文本检测模型路径 |
| `models.textRecModelPath` | string | - | 文本识别模型路径 |

### 功能开关

| 参数 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `enableTextRecognition` | bool | false | 是否启用文本识别 (性能敏感) |

### 运动检测器配置

| 参数 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `confidenceThreshold` | float | 0.25 | 检测置信度阈值 [0.0-1.0] |
| `nmsThreshold` | float | 0.45 | NMS 阈值 [0.0-1.0] |
| `inputWidth` | int | 640 | 模型输入宽度 |
| `maxTrackedObjects` | int | 50 | 最大跟踪对象数 |
| `trackHighThreshold` | float | 0.6 | 跟踪高置信度阈值 |
| `trackLowThreshold` | float | 0.1 | 跟踪低置信度阈值 |
| `trackBufferSize` | int | 30 | 跟踪缓冲区大小 |
| `pixelMotionWeight` | float | 0.8 | 像素运动权重 |
| `objectMotionWeight` | float | 0.2 | 对象运动权重 |

### 场景变化检测器配置

| 参数 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `similarityThreshold` | float | 0.8 | 相似度阈值 [0.0-1.0] |
| `featureDim` | int | 1000 | 特征维度 |
| `inputSize` | int | 224 | 输入尺寸 |
| `enableCache` | bool | true | 是否启用缓存 |

### 文本检测器配置

| 参数 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `detInputHeight` | int | 960 | 检测模型输入高度 |
| `detInputWidth` | int | 960 | 检测模型输入宽度 |
| `recInputHeight` | int | 48 | 识别模型输入高度 |
| `recInputWidth` | int | 320 | 识别模型输入宽度 |
| `detThreshold` | float | 0.3 | 检测置信度阈值 |
| `recThreshold` | float | 0.5 | 识别置信度阈值 |
| `enableRecognition` | bool | false | 是否启用文本识别 |
| `alpha` | float | 0.6 | 文本覆盖率权重 |
| `beta` | float | 0.4 | 文本变化率权重 |

### 动态权重计算器配置

| 参数 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `baseWeights` | array | [0.45, 0.2, 0.35] | 基础权重 [场景, 运动, 文本] |
| `currentFrameWeight` | float | 0.3 | 当前帧权重 |
| `activationInfluence` | float | 0.5 | 激活度影响因子 |
| `historyWindowSize` | int | 30 | 历史窗口大小 |
| `minWeight` | float | 0.05 | 最小权重 |
| `maxWeight` | float | 0.7 | 最大权重 |

### 帧评分器配置

| 参数 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `enableDynamicWeighting` | bool | true | 是否启用动态权重 |
| `enableSmoothing` | bool | true | 是否启用时间平滑 |
| `smoothingWindowSize` | int | 3 | 平滑窗口大小 |
| `smoothingEMAAlpha` | float | 0.6 | EMA 平滑系数 |
| `sceneChangeBoost` | float | 1.2 | 场景变化提升因子 |
| `motionIncreaseBoost` | float | 1.1 | 运动增加提升因子 |
| `textIncreaseBoost` | float | 1.1 | 文本增加提升因子 |

### 关键帧检测器配置

| 参数 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `targetKeyFrameCount` | int | 50 | 目标关键帧数量 |
| `targetCompressionRatio` | float | 0.1 | 目标压缩比 |
| `minKeyFrameCount` | int | 5 | 最小关键帧数量 |
| `maxKeyFrameCount` | int | 500 | 最大关键帧数量 |
| `minTemporalDistance` | float | 1.0 | 最小时间间隔(秒) |
| `useThresholdMode` | bool | true | 是否使用阈值模式 |
| `highQualityThreshold` | float | 0.75 | 高质量帧阈值 |
| `minScoreThreshold` | float | 0.3 | 最小分数阈值 |
| `alwaysIncludeSceneChanges` | bool | true | 始终包含场景变化帧 |

### 管道配置

| 参数 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `analysisThreadCount` | int | 1 | 分析线程数 |
| `frameBufferSize` | int | 100 | 帧缓冲区大小 |
| `scoreBufferSize` | int | 200 | 分数缓冲区大小 |

## 使用示例

### C++ 代码中使用新配置系统

```cpp
#include "core/Config/UnifiedConfig.h"

// 加载配置
Config::KeyFrameAnalyzerConfig config;
if (!config.loadFromFile("configs/analyzer_config.json")) {
    LOG_ERROR("Failed to load config");
    return -1;
}

// 创建服务
auto service = std::make_unique<KeyFrame::KeyFrameAnalyzerService>(config);
if (!service->start()) {
    LOG_ERROR("Failed to start service");
    return -1;
}
```

### 配置验证

配置加载时会自动进行验证，如果发现错误会在日志中显示：

```
[ERROR] Config validation failed for KeyFrameAnalyzerConfig:
Errors:
  - [Motion Detector] Confidence threshold must be in range [0, 1], got 1.5
```

### 保存当前配置

```cpp
// 修改配置后保存
config.motionDetector.confidenceThreshold = 0.5f;
config.saveToFile("configs/analyzer_config_modified.json");
```
