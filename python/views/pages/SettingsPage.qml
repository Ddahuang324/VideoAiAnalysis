// SettingsPage.qml - 设置页面
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Dialogs
import "../styles" as Styles

Rectangle {
    id: root
    anchors.fill: parent
    color: "transparent"

    // 文件夹选择对话框
    FolderDialog {
        id: folderDialog
        title: "Select Output Directory"
        onAccepted: {
            settingsViewModel.setOutputDir(selectedFolder.toString().replace("file:///", ""));
        }
    }

    ScrollView {
        id: settingsScroll
        anchors.fill: parent
        anchors.margins: 32
        clip: true
        ScrollBar.vertical.policy: ScrollBar.AsNeeded

        Column {
            width: settingsScroll.width - 24
            spacing: 24

            // 页面标题
            Column {
                spacing: 8
                bottomPadding: 16
                Text {
                    text: "Settings"
                    color: Styles.ThemeManager.text
                    font.pixelSize: 32
                    font.weight: Font.DemiBold
                }
                Text {
                    text: "Configure your recording and analysis preferences"
                    color: Styles.ThemeManager.textSecondary
                    font.pixelSize: 14
                }
            }

            // ========== 视频录制配置 ==========
            SettingGroup {
                title: "Video Recording"
                iconType: "record"
                Column {
                    width: parent.width
                    spacing: 16

                    // 输出目录
                    SettingPathItem {
                        label: "Output Directory"
                        value: settingsViewModel.outputDir
                        onClicked: folderDialog.open()
                    }

                    // 分辨率显示（自动从屏幕获取）
                    SettingItem {
                        label: "Resolution"
                        value: "Auto (Screen Native)"
                    }

                    // 编码器
                    SettingComboBox {
                        label: "Video Codec"
                        model: ["libx264", "libx265", "h264_nvenc", "hevc_nvenc"]
                        currentValue: settingsViewModel.videoCodec
                        onUpdated: function (val) {
                            settingsViewModel.setVideoCodec(val);
                        }
                    }

                    // 编码预设
                    SettingComboBox {
                        label: "Encoding Preset"
                        model: ["ultrafast", "superfast", "veryfast", "faster", "fast", "medium", "slow"]
                        currentValue: settingsViewModel.videoPreset
                        onUpdated: function (val) {
                            settingsViewModel.setVideoPreset(val);
                        }
                    }

                    // 码率
                    SettingSlider {
                        label: "Video Bitrate"
                        value: settingsViewModel.videoBitrate / 1000000
                        from: 1
                        to: 50
                        suffix: " Mbps"
                        onUpdated: function (val) {
                            settingsViewModel.setVideoBitrate(val * 1000000);
                        }
                    }

                    // 录制模式
                    SettingComboBox {
                        label: "Recording Mode"
                        model: ["video", "snapshot"]
                        displayModel: ["Video Mode", "Snapshot Mode"]
                        currentValue: settingsViewModel.recordingMode
                        onUpdated: function (val) {
                            settingsViewModel.setRecordingMode(val);
                        }
                    }

                    // CRF 质量
                    SettingSlider {
                        label: "Quality (CRF)"
                        value: settingsViewModel.videoCrf
                        from: 0
                        to: 51
                        suffix: ""
                        description: "Lower = better quality, larger file"
                        onUpdated: function (val) {
                            settingsViewModel.setVideoCrf(val);
                        }
                    }
                }
            }

            // ========== 音频配置 ==========
            SettingGroup {
                title: "Audio Recording"
                iconType: "audio"
                Column {
                    width: parent.width
                    spacing: 16

                    SettingToggle {
                        label: "Enable Audio Recording"
                        checked: settingsViewModel.audioEnabled
                        onToggled: function (val) {
                            settingsViewModel.setAudioEnabled(val);
                        }
                    }

                    SettingComboBox {
                        label: "Sample Rate"
                        model: ["44100", "48000", "96000"]
                        currentValue: settingsViewModel.audioSampleRate.toString()
                        suffix: " Hz"
                        enabled: settingsViewModel.audioEnabled
                        onUpdated: function (val) {
                            settingsViewModel.setAudioSampleRate(parseInt(val));
                        }
                    }

                    SettingComboBox {
                        label: "Channels"
                        model: ["1", "2"]
                        currentValue: settingsViewModel.audioChannels.toString()
                        displayModel: ["Mono", "Stereo"]
                        enabled: settingsViewModel.audioEnabled
                        onUpdated: function (val) {
                            settingsViewModel.setAudioChannels(parseInt(val));
                        }
                    }

                    SettingSlider {
                        label: "Audio Bitrate"
                        value: settingsViewModel.audioBitrate / 1000
                        from: 64
                        to: 320
                        suffix: " kbps"
                        enabled: settingsViewModel.audioEnabled
                        onValueChanged: function (val) {
                            settingsViewModel.setAudioBitrate(val * 1000);
                        }
                    }

                    SettingComboBox {
                        label: "Audio Codec"
                        model: ["aac", "mp3", "opus"]
                        currentValue: settingsViewModel.audioCodec
                        enabled: settingsViewModel.audioEnabled
                        onUpdated: function (val) {
                            settingsViewModel.setAudioCodec(val);
                        }
                    }
                }
            }

            // ========== AI 分析配置 ==========
            SettingGroup {
                title: "AI Analysis"
                iconType: "analysis"
                Column {
                    width: parent.width
                    spacing: 16

                    SettingToggle {
                        label: "Enable Text Recognition (OCR)"
                        checked: settingsViewModel.textRecognitionEnabled
                        onToggled: function (val) {
                            settingsViewModel.setTextRecognitionEnabled(val);
                        }
                    }

                    SettingToggle {
                        label: "Real-time Analysis (Snapshot)"
                        description: "Instant analysis for 1FPS snapshot mode"
                        checked: settingsViewModel.analysisRealTime
                        onToggled: function (val) {
                            settingsViewModel.setAnalysisRealTime(val);
                        }
                    }

                    SettingSlider {
                        label: "Scene Detection Threshold"
                        value: settingsViewModel.sceneThreshold * 100
                        from: 50
                        to: 100
                        suffix: "%"
                        description: "Higher = less sensitive"
                        onUpdated: function (val) {
                            settingsViewModel.setSceneThreshold(val / 100);
                        }
                    }

                    SettingSlider {
                        label: "Motion Detection Threshold"
                        value: settingsViewModel.motionThreshold * 100
                        from: 10
                        to: 100
                        suffix: "%"
                        onUpdated: function (val) {
                            settingsViewModel.setMotionThreshold(val / 100);
                        }
                    }

                    SettingSlider {
                        label: "Analysis Threads"
                        value: settingsViewModel.analysisThreadCount
                        from: 1
                        to: 16
                        suffix: ""
                        onUpdated: function (val) {
                            settingsViewModel.setAnalysisThreadCount(val);
                        }
                    }
                }
            }

            // ========== 外观 ==========
            SettingGroup {
                title: "Appearance"
                iconType: "appearance"
                Column {
                    width: parent.width
                    spacing: 16
                    SettingItem {
                        label: "Visual Theme"
                        value: Styles.ThemeManager.isDark ? "Dark Mode" : "Light Mode"
                    }
                    SettingItem {
                        label: "Accent Color"
                        value: "System Emerald"
                    }
                }
            }

            // ========== 重置按钮 ==========
            Rectangle {
                width: parent.width
                height: 60
                color: "transparent"

                Button {
                    anchors.centerIn: parent
                    text: "Reset to Defaults"
                    onClicked: settingsViewModel.resetToDefaults()

                    background: Rectangle {
                        implicitWidth: 160
                        implicitHeight: 40
                        radius: 8
                        color: parent.hovered ? Styles.ThemeManager.overlayLight : Styles.ThemeManager.overlayDark
                        border.width: 1
                        border.color: Styles.ThemeManager.border
                    }

                    contentItem: Text {
                        text: parent.text
                        color: Styles.ThemeManager.text
                        font.pixelSize: 14
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }

            // ========== 关于 ==========
            SettingGroup {
                title: "About"
                iconType: "info"
                Column {
                    width: parent.width
                    spacing: 12
                    Text {
                        text: "AI Video Analysis System v2.1.0"
                        color: Styles.ThemeManager.text
                        font.pixelSize: 15
                        font.weight: Font.Medium
                    }
                    Text {
                        text: "Stable Release - Phase 3 Core Architecture"
                        color: Styles.ThemeManager.textMuted
                        font.pixelSize: 13
                    }
                    Text {
                        text: "© 2026 Antigravity Creative. All rights reserved."
                        color: Styles.ThemeManager.textMuted
                        font.pixelSize: 11
                        topPadding: 10
                    }
                }
            }

            Item {
                height: 40
                width: 1
            }
        }
    }

    // ========== 组件定义 ==========

    component SettingGroup: Rectangle {
        id: groupRoot
        property string title: ""
        property string iconType: ""
        default property alias content: innerContent.data

        width: parent.width
        height: groupLayout.implicitHeight + 48
        radius: 16
        color: Styles.ThemeManager.bgSurface
        border.width: 1
        border.color: Styles.ThemeManager.border

        ColumnLayout {
            id: groupLayout
            anchors.fill: parent
            anchors.margins: 24
            spacing: 20

            RowLayout {
                spacing: 12
                Canvas {
                    Layout.preferredWidth: 24
                    Layout.preferredHeight: 24
                    onPaint: {
                        var ctx = getContext("2d");
                        ctx.reset();
                        ctx.strokeStyle = Styles.ThemeManager.primary;
                        ctx.lineWidth = 1.8;

                        if (iconType === "record") {
                            ctx.beginPath();
                            ctx.arc(12, 12, 8, 0, 2 * Math.PI);
                            ctx.stroke();
                            ctx.fillStyle = Styles.ThemeManager.primary;
                            ctx.beginPath();
                            ctx.arc(12, 12, 3, 0, 2 * Math.PI);
                            ctx.fill();
                        } else if (iconType === "audio") {
                            ctx.beginPath();
                            ctx.moveTo(6, 8);
                            ctx.lineTo(6, 16);
                            ctx.moveTo(10, 5);
                            ctx.lineTo(10, 19);
                            ctx.moveTo(14, 8);
                            ctx.lineTo(14, 16);
                            ctx.moveTo(18, 10);
                            ctx.lineTo(18, 14);
                            ctx.stroke();
                        } else if (iconType === "analysis") {
                            ctx.strokeRect(4, 4, 6, 6);
                            ctx.strokeRect(14, 4, 6, 6);
                            ctx.strokeRect(4, 14, 6, 6);
                            ctx.strokeRect(14, 14, 6, 6);
                        } else if (iconType === "appearance") {
                            ctx.beginPath();
                            ctx.arc(12, 12, 8, 0, 2 * Math.PI);
                            ctx.stroke();
                            ctx.fillStyle = Styles.ThemeManager.primary;
                            ctx.beginPath();
                            ctx.arc(12, 12, 8, -Math.PI / 2, Math.PI / 2);
                            ctx.fill();
                        } else {
                            ctx.beginPath();
                            ctx.arc(12, 12, 8, 0, 2 * Math.PI);
                            ctx.stroke();
                            ctx.font = "bold 12px serif";
                            ctx.fillStyle = Styles.ThemeManager.primary;
                            ctx.textAlign = "center";
                            ctx.fillText("i", 12, 16);
                        }
                    }
                }
                Text {
                    text: title
                    color: Styles.ThemeManager.text
                    font.pixelSize: 18
                    font.weight: Font.Bold
                }
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: Styles.ThemeManager.border
                opacity: 0.5
            }

            Item {
                id: innerContent
                Layout.fillWidth: true
                implicitHeight: childrenRect.height
            }
        }
    }

    component SettingItem: RowLayout {
        property string label: ""
        property string value: ""

        width: parent.width
        Text {
            text: label
            color: Styles.ThemeManager.textSecondary
            font.pixelSize: 14
            Layout.fillWidth: true
        }
        Text {
            text: value
            color: Styles.ThemeManager.text
            font.pixelSize: 14
            font.weight: Font.Medium
        }
    }

    component SettingPathItem: RowLayout {
        property string label: ""
        property string value: ""
        signal clicked

        width: parent.width
        Text {
            text: label
            color: Styles.ThemeManager.textSecondary
            font.pixelSize: 14
            Layout.fillWidth: true
        }
        Text {
            text: value + " ↗"
            color: Styles.ThemeManager.primary
            font.pixelSize: 14
            font.weight: Font.Medium
            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: parent.parent.clicked()
            }
        }
    }

    component SettingToggle: RowLayout {
        id: toggleRoot
        property string label: ""
        property string description: ""
        property bool checked: false
        signal toggled(bool value)

        width: parent.width
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 2
            Text {
                text: toggleRoot.label
                color: Styles.ThemeManager.textSecondary
                font.pixelSize: 14
                Layout.fillWidth: true
            }
            Text {
                visible: toggleRoot.description !== ""
                text: toggleRoot.description
                color: Styles.ThemeManager.textMuted
                font.pixelSize: 12
                Layout.fillWidth: true
            }
        }

        Switch {
            id: toggleSwitch
            checked: toggleRoot.checked
            onCheckedChanged: toggleRoot.toggled(checked)

            indicator: Rectangle {
                implicitWidth: 40
                implicitHeight: 20
                radius: 10
                color: toggleSwitch.checked ? Styles.ThemeManager.primary : Styles.ThemeManager.overlayDark
                border.color: toggleSwitch.checked ? Styles.ThemeManager.primary : Styles.ThemeManager.border

                Rectangle {
                    x: toggleSwitch.checked ? parent.width - width - 2 : 2
                    y: 2
                    width: 16
                    height: 16
                    radius: 8
                    color: "white"
                    Behavior on x {
                        NumberAnimation {
                            duration: 200
                        }
                    }
                }
            }
        }
    }

    component SettingComboBox: RowLayout {
        id: comboRoot
        property string label: ""
        property var model: []
        property var displayModel: []
        property string currentValue: ""
        property string suffix: ""
        property bool enabled: true
        signal updated(string value)

        width: parent.width
        opacity: enabled ? 1.0 : 0.5

        Text {
            text: comboRoot.label
            color: Styles.ThemeManager.textSecondary
            font.pixelSize: 14
            Layout.fillWidth: true
        }

        ComboBox {
            id: combo
            Layout.preferredWidth: 180
            enabled: comboRoot.enabled
            model: comboRoot.model
            currentIndex: comboRoot.model.indexOf(comboRoot.currentValue)

            onCurrentIndexChanged: {
                if (currentIndex >= 0 && comboRoot.model[currentIndex] !== comboRoot.currentValue) {
                    comboRoot.updated(comboRoot.model[currentIndex]);
                }
            }

            delegate: ItemDelegate {
                width: combo.width
                contentItem: Text {
                    text: comboRoot.displayModel.length > 0 ? comboRoot.displayModel[index] : modelData + comboRoot.suffix
                    color: Styles.ThemeManager.text
                    font.pixelSize: 14
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    color: highlighted ? Styles.ThemeManager.overlayLight : "transparent"
                }
                highlighted: combo.highlightedIndex === index
            }

            contentItem: Text {
                text: {
                    var idx = combo.currentIndex;
                    if (comboRoot.displayModel.length > 0 && idx >= 0) {
                        return comboRoot.displayModel[idx] + comboRoot.suffix;
                    }
                    return combo.displayText + comboRoot.suffix;
                }
                color: Styles.ThemeManager.text
                font.pixelSize: 14
                verticalAlignment: Text.AlignVCenter
                leftPadding: 12
            }

            background: Rectangle {
                implicitWidth: 180
                implicitHeight: 36
                radius: 8
                color: Styles.ThemeManager.overlayDark
                border.width: 1
                border.color: combo.activeFocus ? Styles.ThemeManager.primary : Styles.ThemeManager.border
            }

            popup: Popup {
                y: combo.height
                width: combo.width
                padding: 1

                contentItem: ListView {
                    clip: true
                    implicitHeight: contentHeight
                    model: combo.popup.visible ? combo.delegateModel : null
                    ScrollIndicator.vertical: ScrollIndicator {}
                }

                background: Rectangle {
                    radius: 8
                    color: Styles.ThemeManager.bgSurface
                    border.color: Styles.ThemeManager.border
                }
            }
        }
    }

    component SettingSlider: ColumnLayout {
        id: sliderRoot
        property string label: ""
        property real value: 0
        property real from: 0
        property real to: 100
        property string suffix: ""
        property string description: ""
        property bool enabled: true
        signal updated(real value)

        width: parent.width
        spacing: 8
        opacity: enabled ? 1.0 : 0.5

        RowLayout {
            Layout.fillWidth: true
            Text {
                text: sliderRoot.label
                color: Styles.ThemeManager.textSecondary
                font.pixelSize: 14
                Layout.fillWidth: true
            }
            Text {
                text: Math.round(slider.value) + sliderRoot.suffix
                color: Styles.ThemeManager.text
                font.pixelSize: 14
                font.weight: Font.Medium
            }
        }

        Slider {
            id: slider
            Layout.fillWidth: true
            enabled: sliderRoot.enabled
            from: sliderRoot.from
            to: sliderRoot.to
            value: sliderRoot.value
            stepSize: 1

            onMoved: sliderRoot.updated(value)

            background: Rectangle {
                x: slider.leftPadding
                y: slider.topPadding + slider.availableHeight / 2 - height / 2
                width: slider.availableWidth
                height: 4
                radius: 2
                color: Styles.ThemeManager.overlayDark

                Rectangle {
                    width: slider.visualPosition * parent.width
                    height: parent.height
                    radius: 2
                    color: Styles.ThemeManager.primary
                }
            }

            handle: Rectangle {
                x: slider.leftPadding + slider.visualPosition * (slider.availableWidth - width)
                y: slider.topPadding + slider.availableHeight / 2 - height / 2
                width: 16
                height: 16
                radius: 8
                color: slider.pressed ? Styles.ThemeManager.primary : "white"
                border.color: Styles.ThemeManager.primary
                border.width: 2
            }
        }

        Text {
            visible: sliderRoot.description !== ""
            text: sliderRoot.description
            color: Styles.ThemeManager.textMuted
            font.pixelSize: 12
        }
    }
}
