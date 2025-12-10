---
description: 一个项目的实现路径
---

段 0：需求分析与三级目录大纲 (Phase 0: Architecture & Outline)
为了达成“独立、完整手敲”的目标，我将整个项目拆解为 12 个核心章节（Modules）。每个章节都包含具体的一级模块、二级组件和三级实现细节。

这份大纲将是你接下来的“施工图纸”。

Chapter 1: 基础设施与架构搭建 (Infrastructure & Architecture)
目标: 搭建 CMake 构建系统，实现 C++ 与 Python 的互操作环境，跑通 "Hello World" (C++ 加载 Python 脚本并在 QML 显示)。

1.1 构建系统 (Build System)

CMakeLists.txt 配置：集成 Qt6, Python3 库, FFmpeg 库。

目录结构规范：src/core (C++), src/ai (Python), src/ui (QML), assets。

1.2 混合语言运行时 (Hybrid Runtime)

PythonInterpreter 单例类：负责初始化/销毁 Python VM。

BridgeObject：实现 C++ 到 Python 的调用接口 (Call Interface)。

1.3 启动引导 (Bootstrap)

main.cpp：Qt 应用启动，注册 QML 上下文属性，加载 Python 核心模块。

Chapter 2: UI 框架与导航系统 (UI Framework & Navigation)
目标: 使用 QML 搭建应用的骨架，实现类似原项目的侧边栏导航和页面切换。

2.1 主窗口 (Main Window)

AppWindow.qml：无边框窗口设计、自定义标题栏、拖拽逻辑。

全局样式系统 (Theme.qml)：定义颜色、字体常量 (参考原项目 Ant Design 风格)。

2.2 导航系统 (Navigation System)

Sidebar.qml：侧边栏菜单，状态切换动画。

PageLoader 组件：基于 StackView 或 Loader 实现页面的懒加载。

2.3 基础组件库 (UI Kit)

GlassButton.qml：毛玻璃效果按钮（复刻原项目视觉）。

StatusIndicator.qml：录制状态指示器（红点呼吸效果）。

Chapter 3: C++ 核心 - 屏幕录制引擎 (Core: Screen Recorder Engine)
目标: 使用 FFmpeg C API 在 C++ 层实现高效的屏幕捕获，不阻塞 UI 线程。

3.1 采集器架构 (Capture Architecture)

IScreenGrabber 接口：定义 start(), stop(), pause() 契约。

FFmpegWrapper 类：RAII 封装 AVFormatContext, AVCodecContext 等资源的申请与释放。

3.2 视频流处理 (Video Pipeline)

FrameGrabberThread：独立线程采集桌面帧（使用 GDI/DXGI on Windows 或 X11/PipeWire on Linux）。

FrameEncoder：将原始 RGB/YUV 数据编码为 H.264/HEVC。

3.3 写入控制 (Writer Control)

MediaWriter：负责将编码包写入本地临时文件 (temp/chunk_timestamp.mp4)。

Chapter 4: 录制控制器与状态机 (Recording Controller)
目标: 连接 UI 与 C++ 引擎，管理“空闲-倒计时-录制-处理”的状态流转。

4.1 状态管理 (State Machine)

RecorderController (C++): 暴露给 QML 的 Q_PROPERTY (status, duration)。

状态流转逻辑：Idle -> Countdown -> Recording -> Finalizing -> Analysis。

4.2 UI 交互绑定 (UI Binding)

TimerService：C++ 提供高精度计时器，驱动 QML 的录制时长显示。

SystemTray：最小化到托盘，后台录制控制。

4.3 屏幕选择 (Screen Selection)

ScreenHelper：获取多显示器信息，生成预览缩略图供用户选择。

Chapter 5: 视频分片与文件管理 (Video Management Strategy)
目标: 复刻原项目的“实时分片”或“流式处理”逻辑，为 AI 分析做准备。

5.1 文件系统管理 (FileSystem Manager)

PathManager：管理 app_data、temp、records 等目录路径。

自动清理策略 (Auto-Cleanup)：分析完成后删除临时分片。

5.2 视频后处理 (Post-Processing)

VideoStitcher (C++)：如果采用分片录制，需实现简单的 MP4 合并逻辑（复刻原项目 stitchVideos）。

MetadataExtractor：提取视频时长、大小，生成 JSON 元数据。

Chapter 6: Python AI 服务层 (Python AI Service Layer)
目标: 在 Python 层重写原 Node.js 的 geminiService.ts 逻辑。

6.1 Python 环境集成

VirtualEnv 配置：确保应用携带或通过 pip 安装 google-generative-ai 库。

GeminiClient.py：封装 Google Gemini API 的认证与会话管理。

6.2 视频上传逻辑 (Upload Logic)

FileUploader.py：实现 Gemini File API 的上传、状态轮询 (Active 状态检查)。

错误重试机制：处理网络波动。

6.3 提示词工程模块 (Prompt Engineering)

PromptTemplate.py：移植原项目的 STAGE2_SYSTEM_PROMPT 和转录提示词。

动态注入：支持从 UI 传递自定义 Prompt 参数。

Chapter 7: 业务逻辑 - 分析控制器 (Analysis Orchestrator)
目标: C++ 触发分析信号，Python 接收并协调 AI 任务，最后返回结果。

7.1 跨语言信号槽 (Inter-Language Signals)

AnalysisBridge：C++ 发送 startAnalysis(filePath) 信号 -> Python 槽函数响应。

Python 回调：Python 进度更新 -> C++ analysisProgress(percent, status) -> QML。

7.2 分析流水线 (Analysis Pipeline)

Step 1: 转录 (Transcribe) - 调用 Gemini Flash 模型获取视频内容描述。

Step 2: 摘要 (Summarize) - 将转录结果生成 Activity Cards (JSON)。

Step 3: 结构化 (Synthesize) - 生成最终的分析报告。

7.3 异常处理

处理 API Key 无效、配额超限、网络超时等异常，并反馈给 UI。

Chapter 8: 数据展示 - 活动卡片与时间轴 (Visualization: Activity Cards)
目标: 使用 QML 高级组件渲染复杂的 AI 分析结果。

8.1 数据模型 (Data Models)

AnalysisResultModel (C++ AbstractListModel)：将 Python 返回的 JSON 数据转换为 QML 可用的 ListModel。

ActivityCard 结构体：定义卡片的数据结构。

8.2 卡片视图 (Card View)

TimelineView.qml：垂直时间轴布局。

ActivityCard.qml：包含时间段、标题、摘要、代码块高亮的复合组件。

8.3 详情展示

MarkdownText：支持简单的 Markdown 渲染（用于显示 AI 生成的详细描述）。

Chapter 9: 数据持久化与历史记录 (Persistence & History)
目标: 保存用户的分析历史，支持回看。

9.1 数据库层 (Database Layer)

SQLiteManager (C++)：创建 history.db。

HistoryDao：实现 CRUD 操作（保存分析结果 JSON、视频路径、时间）。

9.2 历史页面 (History Page)

ArchivePage.qml：网格显示历史记录缩略图。

FilterProxyModel：实现按日期、标签搜索历史记录。

Chapter 10: 设置与配置管理 (Settings & Configuration)
目标: 管理 API Key、Prompt 模板、录制参数。

10.1 配置存储 (Config Store)

SettingsManager (C++/QSettings)：持久化存储用户配置。

安全存储：对 API Key 进行简单的加密存储。

10.2 设置界面 (Settings UI)

SettingsPage.qml：API Key 输入框、模型选择 (Flash/Pro)、Prompt 编辑器。

实时生效：修改配置后无需重启即可应用。

Chapter 11: 性能优化与多线程 (Optimization & Threading)
目标: 确保长时间录制不崩溃，界面不卡顿。

11.1 线程模型优化

将 Python 解释器运行在独立线程，避免阻塞 QML 渲染线程 (GUI Thread)。

C++ 录制线程优先级调整。

11.2 内存管理

检测 C++ 与 Python 对象传递时的引用计数 (Reference Counting)，防止内存泄漏。

Chapter 12: 打包与部署 (Packaging & Deployment)
目标: 生成可执行的安装包。

12.1 依赖收集

使用 windeployqt (Windows) 或 macdeployqt (macOS)。

收集 Python 嵌入式环境和依赖库。

12.2 安装包制作

制作最终的 .exe 或 .dmg / .AppImage。