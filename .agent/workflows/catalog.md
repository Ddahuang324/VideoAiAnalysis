---
description: 项目的目录
---

#### 段 0：需求分析与三级目录大纲 (Phase 0: Architecture & Outline)
为了达成“独立、完整手敲”的目标，我将整个项目拆解为 12 个核心章节（Modules）。每个章节都包含具体的一级模块、二级组件和三级实现细节。
这份大纲将是你接下来的“施工图纸”。
#### Chapter 1: 基础设施与架构搭建 (Infrastructure & Architecture)
- 目标: 搭建 CMake 构建系统，实现 C++ 与 Python 的互操作环境，跑通 "Hello World" (C++ 加载 Python 脚本并在 QML 显示)。
- 1.1 构建系统 (Build System)
	- CMakeLists.txt 配置：集成 Qt6, Python3 库, FFmpeg 库。
	- 目录结构规范： src/core (C++), src/ai (Python), src/ui (QML), assets。
- 1.2 混合语言运行时 (Hybrid Runtime)
	- PythonInterpreter 单例类：负责初始化/销毁 Python VM。
	- BridgeObject：实现 C++ 到 Python 的调用接口 (Call Interface)。
- 1.3 启动引导 (Bootstrap)
	- main.cpp：Qt 应用启动，注册 QML 上下文属性，加载 Python 核心模块。
#### Chapter 2: UI 框架与导航系统 (UI Framework & Navigation)
- 目标: 使用 QML 搭建应用的骨架，实现类似原项目的侧边栏导航和页面切换。
- 2.1 主窗口 (Main Window)
	- AppWindow.qml：无边框窗口设计、自定义标题栏、拖拽逻辑。
	- 全局样式系统 ( Theme.qml)：定义颜色、字体常量 (参考原项目 Ant Design 风格)。
- 2.2 导航系统 (Navigation System)
	- Sidebar.qml：侧边栏菜单，状态切换动画。
	- PageLoader 组件：基于 StackView 或 Loader 实现页面的懒加载。
- 2.3 基础组件库 (UI Kit)
	- GlassButton.qml：毛玻璃效果按钮（复刻原项目视觉）。
	- StatusIndicator.qml：录制状态指示器（红点呼吸效果）。
#### Chapter 3: C++ 核心 - 屏幕录制引擎 (Core: Screen Recorder Engine)
- 目标: 使用 FFmpeg C API 在 C++ 层实现高效的屏幕捕获，不阻塞 UI 线程。
- 3.1 采集器架构 (Capture Architecture)
	- IScreenGrabber 接口：定义 start(), stop(), pause() 契约。
	- FFmpegWrapper 类：RAII 封装 AVFormatContext, AVCodecContext 等资源的申请与释放。
- 3.2 视频流处理 (Video Pipeline)
	- FrameGrabberThread：独立线程采集桌面帧（使用 GDI/DXGI on Windows 或 X11/PipeWire on Linux）。
	- FrameEncoder：将原始 RGB/YUV 数据编码为 H.264/HEVC。
- 3.3 写入控制 (Writer Control)
	- MediaWriter：负责将编码包写入本地临时文件 ( temp/chunk_timestamp.mp4)。
#### Chapter 4: 录制控制器与状态机 (Recording Controller)
- 目标: 连接 UI 与 C++ 引擎，管理“空闲-倒计时-录制-处理”的状态流转。
- 4.1 状态管理 (State Machine)
	- RecorderController (C++): 暴露给 QML 的 Q_PROPERTY (status, duration)。
	- 状态流转逻辑：Idle → Countdown → Recording → Finalizing → Analysis。
- 4.2 UI 交互绑定 (UI Binding)
	- TimerService：C++ 提供高精度计时器，驱动 QML 的录制时长显示。
	- SystemTray：最小化到托盘，后台录制控制。
- 4.3 屏幕选择 (Screen Selection)
	- ScreenHelper：获取多显示器信息，生成预览缩略图供用户选择。
#### Chapter 5: 视频分片与文件管理 (Video Management Strategy)
- 目标: 复刻原项目的“实时分片”或“流式处理”逻辑，为 AI 分析做准备。
- 5.1 文件系统管理 (FileSystem Manager)
	- PathManager：管理 app_data、 temp、 records 等目录路径。
	- 自动清理策略 (Auto-Cleanup)：分析完成后删除临时分片。
- 5.2 视频后处理 (Post-Processing)
	- VideoStitcher (C++)：如果采用分片录制，需实现简单的 MP4 合并逻辑（复刻原项目 stitchVideos）。
	- MetadataExtractor：提取视频时长、大小，生成 JSON 元数据。
#### Chapter 6: 基于 gRPC 的 Python AI 微服务 (详细三级大纲)
#### 6.1 定义通信契约 (The Contract: Protocol Buffers)
- 6.1.1 接口定义语言 (IDL) 设计
	- 创建 protos/desktop_analyst.proto。
	- Input: AnalysisRequest (视频路径, API Key, Prompt 模板, 模型参数)。
	- Output: AnalysisResponse (流式响应：包含 进度百分比, 当前状态文本, 结构化 JSON 结果)。
	- Service: 定义 rpc StartAnalysis (AnalysisRequest) returns (stream AnalysisResponse)。
- 6.1.2 编译与代码生成
	- 使用 protoc 编译器生成 Python 代码 ( _pb2.py, _pb2_grpc.py)。
	- 使用 protoc 编译器生成 C++ 代码 ( .pb.h, .pb.cc, .grpc.pb.h, .grpc.pb.cc)。
	- 知识点: Protobuf 序列化原理、gRPC Stub 概念。
#### 6.2 Python 服务端核心 (The Server: Python Implementation)
- 6.2.1 服务框架搭建
	- 实现 AnalysisServicer 类（继承自生成的 gRPC 基类）。
	- 建立线程池服务器 ( concurrent.futures.ThreadPoolExecutor) 处理并发请求。
	- 工业级优化: 动态端口绑定 (绑定 0 端口获取随机空闲端口，并打印到 stdout 供 C++ 读取)。
- 6.2.2 Gemini 业务逻辑移植 (Porting Logic from Node.js)
	- 文件上传: 使用 google-generative-ai 库的 upload_file，实现 waiting_for_processing 轮询逻辑。
	- 转录 (Transcription): 移植原项目 transcribeVideo 的 Prompt，调用 Gemini Flash 生成 Observation[]。
	- 摘要 (Summarization): 移植 generateActivityCards 的 Prompt，生成最终卡片数据。
	- 流式反馈: 在每个耗时步骤（上传后、转录中、摘要中）通过 yield response 向 C++ 推送进度。
- 6.2.3 异常处理与保活
	- 捕获 GoogleAPIError、网络超时、文件不存在错误，转化为 gRPC Status 错误码返回。
	- 实现 心跳检测 (Heartbeat): 启动一个后台线程，监听 stdin 或父进程 PID，一旦 C++ 进程消失，Python 自动退出（防止僵尸进程）。
#### 6.3 C++ 客户端集成 (The Client: C++ Integration)
- 6.3.1 gRPC 客户端封装
	- GrpcClient 类：管理 std::shared_ptr<Channel> 和 Stub。
	- 难点: gRPC 的同步调用会阻塞线程，必须将其放入独立的工作线程。
- 6.3.2 异步工作流 (Worker Thread)
	- 创建 AnalysisWorker 类 (继承 QObject)，运行在 QThread 中。
	- 实现 runAnalysis()：调用 stub->StartAnalysis()，使用 while(reader->Read(&msg)) 循环读取流式响应。
	- 信号转换: 将 gRPC 的回调转换为 Qt 信号 emit progressUpdated(float, QString)，确保 UI 更新在主线程执行。
- 6.3.3 进程生命周期管理
	- PythonProcessManager：使用 QProcess 启动 Python 环境。
	- 启动握手: 启动 Python -> 读取 Python stdout 获取实际监听端口 -> 初始化 gRPC Channel。
#### Chapter 7: 业务逻辑 - 分析控制器 (Analysis Orchestrator)
- 目标: C++ 触发分析信号，Python 接收并协调 AI 任务，最后返回结果。
- 7.1 跨语言信号槽 (Inter-Language Signals)
	- AnalysisBridge：C++ 发送 startAnalysis(filePath) 信号 → Python 槽函数响应。
	- Python 回调：Python 进度更新 -> C++ analysisProgress(percent, status) -> QML。
- 7.2 分析流水线 (Analysis Pipeline)
	- Step 1: 转录 (Transcribe) - 调用 Gemini Flash 模型获取视频内容描述。
	- Step 2: 摘要 (Summarize) - 将转录结果生成 Activity Cards (JSON)。
	- Step 3: 结构化 (Synthesize) - 生成最终的分析报告。
- 7.3 异常处理
	- 处理 API Key 无效、配额超限、网络超时等异常，并反馈给 UI。
#### Chapter 8: 数据展示 - 活动卡片与时间轴 (Visualization: Activity Cards)
- 目标: 使用 QML 高级组件渲染复杂的 AI 分析结果。
- 8.1 数据模型 (Data Models)
	- AnalysisResultModel (C++ AbstractListModel)：将 Python 返回的 JSON 数据转换为 QML 可用的 ListModel。
	- ActivityCard 结构体：定义卡片的数据结构。
- 8.2 卡片视图 (Card View)
	- TimelineView.qml：垂直时间轴布局。
	- ActivityCard.qml：包含时间段、标题、摘要、代码块高亮的复合组件。
- 8.3 详情展示
	- MarkdownText：支持简单的 Markdown 渲染（用于显示 AI 生成的详细描述）。
#### Chapter 9: 数据持久化与历史记录 (Persistence & History)
- 目标: 保存用户的分析历史，支持回看。
- 9.1 数据库层 (Database Layer)
	- SQLiteManager (C++)：创建 history.db。
	- HistoryDao：实现 CRUD 操作（保存分析结果 JSON、视频路径、时间）。
- 9.2 历史页面 (History Page)
	- ArchivePage.qml：网格显示历史记录缩略图。
	- FilterProxyModel：实现按日期、标签搜索历史记录。
#### Chapter 10: 设置与配置管理 (Settings & Configuration)
- 目标: 管理 API Key、Prompt 模板、录制参数。
- 10.1 配置存储 (Config Store)
	- SettingsManager (C++/QSettings)：持久化存储用户配置。
	- 安全存储：对 API Key 进行简单的加密存储。
- 10.2 设置界面 (Settings UI)
	- SettingsPage.qml：API Key 输入框、模型选择 (Flash/Pro)、Prompt 编辑器。
	- 实时生效：修改配置后无需重启即可应用。
#### Chapter 11: 性能优化与多线程 (Optimization & Threading)
- 目标: 确保长时间录制不崩溃，界面不卡顿。
- 11.1 线程模型优化
	- 将 Python 解释器运行在独立线程，避免阻塞 QML 渲染线程 (GUI Thread)。
	- C++ 录制线程优先级调整。
- 11.2 内存管理
	- 检测 C++ 与 Python 对象传递时的引用计数 (Reference Counting)，防止内存泄漏。
#### Chapter 12: 打包与部署 (Packaging & Deployment)
- 目标: 生成可执行的安装包。
- 12.1 依赖收集
	- 使用 windeployqt (Windows) 或 macdeployqt (macOS)。
	- 收集 Python 嵌入式环境和依赖库。
- 12.2 安装包制作
	- 制作最终的 .exe 或 .dmg / .AppImage。