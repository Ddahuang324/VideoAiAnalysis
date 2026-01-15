"""
设置视图模型
管理录制和分析配置，支持 QML 双向绑定
"""
from PySide6.QtCore import QObject, Signal, Slot, Property
from pathlib import Path
import json

from infrastructure.log_manager import get_logger


class SettingsViewModel(QObject):
    """设置视图模型 - 管理所有可配置参数"""

    # 信号定义
    settingsChanged = Signal()
    outputDirChanged = Signal()

    # 视频配置信号
    videoWidthChanged = Signal()
    videoHeightChanged = Signal()
    videoFpsChanged = Signal()
    videoBitrateChanged = Signal()
    videoCrfChanged = Signal()
    videoPresetChanged = Signal()
    videoCodecChanged = Signal()

    # 音频配置信号
    audioEnabledChanged = Signal()
    audioSampleRateChanged = Signal()
    audioChannelsChanged = Signal()
    audioBitrateChanged = Signal()
    audioCodecChanged = Signal()

    # 分析配置信号
    textRecognitionEnabledChanged = Signal()
    sceneThresholdChanged = Signal()
    motionThresholdChanged = Signal()
    analysisThreadCountChanged = Signal()
    recordingModeChanged = Signal()
    analysisRealTimeChanged = Signal()

    def __init__(self):
        super().__init__()
        self.logger = get_logger("SettingsViewModel")

        # 配置文件路径
        self._config_path = Path.home() / ".aivideo" / "settings.json"

        # 录制配置 - 视频
        self._output_dir = str(Path.home() / "Videos" / "ScreenRecordings")
        self._video_width = 1920
        self._video_height = 1080
        self._video_fps = 30
        self._video_bitrate = 4000000
        self._video_crf = 23
        self._video_preset = "fast"
        self._video_codec = "libx264"

        # 录制配置 - 音频
        self._audio_enabled = True
        self._audio_sample_rate = 48000
        self._audio_channels = 2
        self._audio_bitrate = 128000
        self._audio_codec = "aac"

        # 分析配置
        self._text_recognition_enabled = False
        self._scene_threshold = 0.8
        self._motion_threshold = 0.25
        self._analysis_thread_count = 4
        self._recording_mode = "video"
        self._analysis_realtime = True  # 默认开启实时分析（针对SNAPSHOT模式）

        # 加载保存的配置
        self._load_settings()
        self.logger.info("SettingsViewModel initialized")

    # ========== 输出目录 ==========
    @Property(str, notify=outputDirChanged)
    def outputDir(self):
        return self._output_dir

    @outputDir.setter
    def outputDir(self, value):
        if self._output_dir != value:
            self._output_dir = value
            self.outputDirChanged.emit()
            self._save_settings()

    @Slot(str)
    def setOutputDir(self, value):
        self.outputDir = value

    # ========== 视频配置 ==========
    @Property(int, notify=videoWidthChanged)
    def videoWidth(self):
        return self._video_width

    @videoWidth.setter
    def videoWidth(self, value):
        if self._video_width != value:
            self._video_width = value
            self.videoWidthChanged.emit()
            self._save_settings()

    @Slot(int)
    def setVideoWidth(self, value):
        self.videoWidth = value

    @Property(int, notify=videoHeightChanged)
    def videoHeight(self):
        return self._video_height

    @videoHeight.setter
    def videoHeight(self, value):
        if self._video_height != value:
            self._video_height = value
            self.videoHeightChanged.emit()
            self._save_settings()

    @Slot(int)
    def setVideoHeight(self, value):
        self.videoHeight = value

    @Property(int, notify=videoFpsChanged)
    def videoFps(self):
        return self._video_fps

    @videoFps.setter
    def videoFps(self, value):
        if self._video_fps != value:
            self._video_fps = value
            self.videoFpsChanged.emit()
            self._save_settings()

    @Slot(int)
    def setVideoFps(self, value):
        self.videoFps = value

    @Property(int, notify=videoBitrateChanged)
    def videoBitrate(self):
        return self._video_bitrate

    @videoBitrate.setter
    def videoBitrate(self, value):
        if self._video_bitrate != value:
            self._video_bitrate = value
            self.videoBitrateChanged.emit()
            self._save_settings()

    @Slot(int)
    def setVideoBitrate(self, value):
        self.videoBitrate = value

    @Property(int, notify=videoCrfChanged)
    def videoCrf(self):
        return self._video_crf

    @videoCrf.setter
    def videoCrf(self, value):
        if self._video_crf != value:
            self._video_crf = max(0, min(51, value))
            self.videoCrfChanged.emit()
            self._save_settings()

    @Slot(int)
    def setVideoCrf(self, value):
        self.videoCrf = value

    @Property(str, notify=videoPresetChanged)
    def videoPreset(self):
        return self._video_preset

    @videoPreset.setter
    def videoPreset(self, value):
        if self._video_preset != value:
            self._video_preset = value
            self.videoPresetChanged.emit()
            self._save_settings()

    @Slot(str)
    def setVideoPreset(self, value):
        self.videoPreset = value

    @Property(str, notify=videoCodecChanged)
    def videoCodec(self):
        return self._video_codec

    @videoCodec.setter
    def videoCodec(self, value):
        if self._video_codec != value:
            self._video_codec = value
            self.videoCodecChanged.emit()
            self._save_settings()

    @Slot(str)
    def setVideoCodec(self, value):
        self.videoCodec = value

    # ========== 音频配置 ==========
    @Property(bool, notify=audioEnabledChanged)
    def audioEnabled(self):
        return self._audio_enabled

    @audioEnabled.setter
    def audioEnabled(self, value):
        if self._audio_enabled != value:
            self._audio_enabled = value
            self.audioEnabledChanged.emit()
            self._save_settings()

    @Slot(bool)
    def setAudioEnabled(self, value):
        self.audioEnabled = value

    @Property(int, notify=audioSampleRateChanged)
    def audioSampleRate(self):
        return self._audio_sample_rate

    @audioSampleRate.setter
    def audioSampleRate(self, value):
        if self._audio_sample_rate != value:
            self._audio_sample_rate = value
            self.audioSampleRateChanged.emit()
            self._save_settings()

    @Slot(int)
    def setAudioSampleRate(self, value):
        self.audioSampleRate = value

    @Property(int, notify=audioChannelsChanged)
    def audioChannels(self):
        return self._audio_channels

    @audioChannels.setter
    def audioChannels(self, value):
        if self._audio_channels != value:
            self._audio_channels = value
            self.audioChannelsChanged.emit()
            self._save_settings()

    @Slot(int)
    def setAudioChannels(self, value):
        self.audioChannels = value

    @Property(int, notify=audioBitrateChanged)
    def audioBitrate(self):
        return self._audio_bitrate

    @audioBitrate.setter
    def audioBitrate(self, value):
        if self._audio_bitrate != value:
            self._audio_bitrate = value
            self.audioBitrateChanged.emit()
            self._save_settings()

    @Slot(int)
    def setAudioBitrate(self, value):
        self.audioBitrate = value

    @Property(str, notify=audioCodecChanged)
    def audioCodec(self):
        return self._audio_codec

    @audioCodec.setter
    def audioCodec(self, value):
        if self._audio_codec != value:
            self._audio_codec = value
            self.audioCodecChanged.emit()
            self._save_settings()

    @Slot(str)
    def setAudioCodec(self, value):
        self.audioCodec = value

    # ========== 分析配置 ==========
    @Property(bool, notify=textRecognitionEnabledChanged)
    def textRecognitionEnabled(self):
        return self._text_recognition_enabled

    @textRecognitionEnabled.setter
    def textRecognitionEnabled(self, value):
        if self._text_recognition_enabled != value:
            self._text_recognition_enabled = value
            self.textRecognitionEnabledChanged.emit()
            self._save_settings()

    @Slot(bool)
    def setTextRecognitionEnabled(self, value):
        self.textRecognitionEnabled = value

    @Property(float, notify=sceneThresholdChanged)
    def sceneThreshold(self):
        return self._scene_threshold

    @sceneThreshold.setter
    def sceneThreshold(self, value):
        if self._scene_threshold != value:
            self._scene_threshold = max(0.0, min(1.0, value))
            self.sceneThresholdChanged.emit()
            self._save_settings()

    @Slot(float)
    def setSceneThreshold(self, value):
        self.sceneThreshold = value

    @Property(float, notify=motionThresholdChanged)
    def motionThreshold(self):
        return self._motion_threshold

    @motionThreshold.setter
    def motionThreshold(self, value):
        if self._motion_threshold != value:
            self._motion_threshold = max(0.0, min(1.0, value))
            self.motionThresholdChanged.emit()
            self._save_settings()

    @Slot(float)
    def setMotionThreshold(self, value):
        self.motionThreshold = value

    @Property(int, notify=analysisThreadCountChanged)
    def analysisThreadCount(self):
        return self._analysis_thread_count

    @analysisThreadCount.setter
    def analysisThreadCount(self, value):
        if self._analysis_thread_count != value:
            self._analysis_thread_count = max(1, min(16, value))
            self.analysisThreadCountChanged.emit()
            self._save_settings()

    @Slot(int)
    def setAnalysisThreadCount(self, value):
        self.analysisThreadCount = value

    # ========== 录制模式 ==========
    @Property(str, notify=recordingModeChanged)
    def recordingMode(self):
        return self._recording_mode

    @recordingMode.setter
    def recordingMode(self, value):
        if self._recording_mode != value:
            self._recording_mode = value
            self.logger.info(f"Persistent recording mode changed to: {value}")
            self.recordingModeChanged.emit()
            self._save_settings()

    @Slot(str)
    def setRecordingMode(self, value):
        self.recordingMode = value

    # ========== 实时分析控制 ==========
    @Property(bool, notify=analysisRealTimeChanged)
    def analysisRealTime(self):
        return self._analysis_realtime

    @analysisRealTime.setter
    def analysisRealTime(self, value):
        if self._analysis_realtime != value:
            self._analysis_realtime = value
            self.logger.info(f"Realtime analysis setting changed to: {value}")
            self.analysisRealTimeChanged.emit()
            self._save_settings()

    @Slot(bool)
    def setAnalysisRealTime(self, value):
        self.analysisRealTime = value

    # ========== 便捷属性 ==========
    @Property(str, notify=videoWidthChanged)
    def videoResolution(self):
        return f"{self._video_width}x{self._video_height}"

    @Property(str, notify=videoBitrateChanged)
    def formattedVideoBitrate(self):
        mbps = self._video_bitrate / 1000000
        return f"{mbps:.1f} Mbps"

    @Property(str, notify=audioBitrateChanged)
    def formattedAudioBitrate(self):
        kbps = self._audio_bitrate / 1000
        return f"{int(kbps)} kbps"

    # ========== 配置应用方法 ==========
    def apply_to_recorder_config(self, config):
        """将设置应用到 RecorderConfig"""
        config.video.width = self._video_width
        config.video.height = self._video_height
        config.video.fps = self._video_fps
        config.video.bitrate = self._video_bitrate
        config.video.crf = self._video_crf
        config.video.preset = self._video_preset
        config.video.codec = self._video_codec

        config.audio.enabled = self._audio_enabled
        config.audio.sample_rate = self._audio_sample_rate
        config.audio.channels = self._audio_channels
        config.audio.bitrate = self._audio_bitrate
        config.audio.codec = self._audio_codec
        config.recordingMode = self._recording_mode

        return config

    def apply_to_analyzer_config(self, config):
        """将设置应用到 AnalyzerConfig"""
        config.enable_text_recognition = self._text_recognition_enabled
        config.scene_detector.similarity_threshold = self._scene_threshold
        config.motion_detector.confidence_threshold = self._motion_threshold
        config.pipeline.analysis_thread_count = self._analysis_thread_count
        
        # 如果底层 API 支持 realtime_enabled（需要确认 AnalyzerConfig 是否有该字段，或者通过 Service 手动设置）
        # 这里主要作为 UI 状态持久化
        return config

    # ========== 持久化 ==========
    def _load_settings(self):
        """从文件加载设置"""
        try:
            if self._config_path.exists():
                with open(self._config_path, 'r', encoding='utf-8') as f:
                    data = json.load(f)

                # 视频配置
                self._output_dir = data.get('output_dir', self._output_dir)
                self._video_width = data.get('video_width', self._video_width)
                self._video_height = data.get('video_height', self._video_height)
                self._video_fps = data.get('video_fps', self._video_fps)
                self._video_bitrate = data.get('video_bitrate', self._video_bitrate)
                self._video_crf = data.get('video_crf', self._video_crf)
                self._video_preset = data.get('video_preset', self._video_preset)
                self._video_codec = data.get('video_codec', self._video_codec)

                # 音频配置
                self._audio_enabled = data.get('audio_enabled', self._audio_enabled)
                self._audio_sample_rate = data.get('audio_sample_rate', self._audio_sample_rate)
                self._audio_channels = data.get('audio_channels', self._audio_channels)
                self._audio_bitrate = data.get('audio_bitrate', self._audio_bitrate)
                self._audio_codec = data.get('audio_codec', self._audio_codec)

                # 分析配置
                self._text_recognition_enabled = data.get('text_recognition_enabled', self._text_recognition_enabled)
                self._scene_threshold = data.get('scene_threshold', self._scene_threshold)
                self._motion_threshold = data.get('motion_threshold', self._motion_threshold)
                self._analysis_thread_count = data.get('analysis_thread_count', self._analysis_thread_count)
                self._recording_mode = data.get('recording_mode', self._recording_mode)
                self._analysis_realtime = data.get('analysis_realtime', self._analysis_realtime)

                self.logger.info("Settings loaded from file")
        except Exception as e:
            self.logger.warning(f"Failed to load settings: {e}")

    def _save_settings(self):
        """保存设置到文件"""
        try:
            self._config_path.parent.mkdir(parents=True, exist_ok=True)

            data = {
                'output_dir': self._output_dir,
                'video_width': self._video_width,
                'video_height': self._video_height,
                'video_fps': self._video_fps,
                'video_bitrate': self._video_bitrate,
                'video_crf': self._video_crf,
                'video_preset': self._video_preset,
                'video_codec': self._video_codec,
                'audio_enabled': self._audio_enabled,
                'audio_sample_rate': self._audio_sample_rate,
                'audio_channels': self._audio_channels,
                'audio_bitrate': self._audio_bitrate,
                'audio_codec': self._audio_codec,
                'text_recognition_enabled': self._text_recognition_enabled,
                'scene_threshold': self._scene_threshold,
                'motion_threshold': self._motion_threshold,
                'analysis_thread_count': self._analysis_thread_count,
                'recording_mode': self._recording_mode,
                'analysis_realtime': self._analysis_realtime,
            }

            with open(self._config_path, 'w', encoding='utf-8') as f:
                json.dump(data, f, indent=2)

            self.settingsChanged.emit()
        except Exception as e:
            self.logger.error(f"Failed to save settings: {e}")

    @Slot()
    def resetToDefaults(self):
        """重置为默认值"""
        self._output_dir = str(Path.home() / "Videos" / "ScreenRecordings")
        self._video_width = 1920
        self._video_height = 1080
        self._video_fps = 30
        self._video_bitrate = 4000000
        self._video_crf = 23
        self._video_preset = "fast"
        self._video_codec = "libx264"
        self._audio_enabled = True
        self._audio_sample_rate = 48000
        self._audio_channels = 2
        self._audio_bitrate = 128000
        self._audio_codec = "aac"
        self._text_recognition_enabled = False
        self._scene_threshold = 0.8
        self._motion_threshold = 0.25
        self._analysis_thread_count = 4
        self._recording_mode = "video"
        self._analysis_realtime = True

        self._save_settings()

        # 发出所有信号
        self.outputDirChanged.emit()
        self.videoWidthChanged.emit()
        self.videoHeightChanged.emit()
        self.videoFpsChanged.emit()
        self.videoBitrateChanged.emit()
        self.videoCrfChanged.emit()
        self.videoPresetChanged.emit()
        self.videoCodecChanged.emit()
        self.audioEnabledChanged.emit()
        self.audioSampleRateChanged.emit()
        self.audioChannelsChanged.emit()
        self.audioBitrateChanged.emit()
        self.audioCodecChanged.emit()
        self.textRecognitionEnabledChanged.emit()
        self.sceneThresholdChanged.emit()
        self.motionThresholdChanged.emit()
        self.analysisThreadCountChanged.emit()
        self.recordingModeChanged.emit()
        self.analysisRealTimeChanged.emit()

        self.logger.info("Settings reset to defaults")
