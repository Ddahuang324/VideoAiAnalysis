// RecordPage.qml - 录制页面
// 显示录制控制和屏幕预览

import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../styles" as Styles
import "../components" as Components

Rectangle {
    id: root
    anchors.fill: parent
    color: Styles.ThemeManager.bgPrimary

    // ==================== 状态 ====================

    // 使用 videoViewModel 的录制状态
    property bool isRecording: videoViewModel.isRecording
    property int recordingTime: 0  // 秒

    // 监听录制状态变化
    Connections {
        target: videoViewModel
        function onRecordingStateChanged(recording) {
            if (!recording) {
                recordingTime = 0;
            }
        }

        function onRecordingError(errorMsg) {
            console.error("[RecordPage] Recording error:", errorMsg);
        }
    }

    // ==================== 录制计时器 ====================

    Timer {
        id: recordingTimer
        interval: 1000
        repeat: true
        running: isRecording
        onTriggered: {
            recordingTime++;
            // 定期更新录制统计信息
            videoViewModel.updateRecordingStats();
        }
    }

    // 格式化时间
    function formatTime(seconds) {
        var mins = Math.floor(seconds / 60);
        var secs = seconds % 60;
        return mins.toString().padStart(2, '0') + ":" + secs.toString().padStart(2, '0');
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Styles.ThemeManager.spacingLg
        spacing: Styles.ThemeManager.spacingLg

        // 标题行
        RowLayout {
            Layout.fillWidth: true

            Text {
                text: "Screen Recorder"
                color: Styles.ThemeManager.textPrimary
                font.pixelSize: Styles.ThemeManager.fontSizeH2
                font.weight: Font.ExtraBold
                font.family: Styles.ThemeManager.fontFamily
                // font.letterSpacing: -0.5
            }

            Item {
                Layout.fillWidth: true
            }

            // 录制模式切换器 (Minimalist Toggle)
            Rectangle {
                width: 200
                height: 36
                radius: 18
                color: Styles.ThemeManager.bgCard
                border.width: 1
                border.color: Styles.ThemeManager.border
                visible: !isRecording  // 录制时隐藏

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 3
                    spacing: 0

                    // VIDEO 模式按钮
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        radius: 15
                        color: videoViewModel.recorderMode === 0 ? Styles.ThemeManager.primary : "transparent"

                        Behavior on color {
                            ColorAnimation {
                                duration: 200
                            }
                        }

                        Text {
                            anchors.centerIn: parent
                            text: "VIDEO"
                            color: videoViewModel.recorderMode === 0 ? "#FFFFFF" : Styles.ThemeManager.textSecondary
                            font.pixelSize: 11
                            font.weight: Font.Bold
                            font.family: Styles.ThemeManager.fontFamily
                            font.letterSpacing: 0.5
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                console.log("[RecordPage] Switching to VIDEO mode");
                                videoViewModel.setRecorderMode(0);
                            }
                        }
                    }

                    // SNAPSHOT 模式按钮
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        radius: 15
                        color: videoViewModel.recorderMode === 1 ? Styles.ThemeManager.primary : "transparent"

                        Behavior on color {
                            ColorAnimation {
                                duration: 200
                            }
                        }

                        Text {
                            anchors.centerIn: parent
                            text: "SNAPSHOT"
                            color: videoViewModel.recorderMode === 1 ? "#FFFFFF" : Styles.ThemeManager.textSecondary
                            font.pixelSize: 11
                            font.weight: Font.Bold
                            font.family: Styles.ThemeManager.fontFamily
                            font.letterSpacing: 0.5
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                console.log("[RecordPage] Switching to SNAPSHOT mode");
                                videoViewModel.setRecorderMode(1);
                            }
                        }
                    }
                }
            }

            // 录制状态指示 (Minimalist)
            Row {
                spacing: Styles.ThemeManager.spacingSm
                visible: isRecording

                Rectangle {
                    width: 10
                    height: 10
                    radius: 5
                    color: Styles.ThemeManager.error
                    anchors.verticalCenter: parent.verticalCenter

                    SequentialAnimation on opacity {
                        running: isRecording
                        loops: Animation.Infinite
                        NumberAnimation {
                            to: 0.2
                            duration: 600
                            easing.type: Easing.InOutQuad
                        }
                        NumberAnimation {
                            to: 1.0
                            duration: 600
                            easing.type: Easing.InOutQuad
                        }
                    }
                }

                Text {
                    text: "REC " + formatTime(recordingTime)
                    color: Styles.ThemeManager.error
                    font.pixelSize: Styles.ThemeManager.fontSizeSmall
                    font.family: Styles.ThemeManager.fontFamily
                    font.weight: Font.Bold
                }
            }
        }

        // 屏幕预览区域
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: Styles.ThemeManager.radiusLg
            color: Styles.ThemeManager.bgCard
            border.width: isRecording ? 2 : 1
            border.color: isRecording ? Styles.ThemeManager.error : Styles.ThemeManager.border

            Behavior on border.color {
                ColorAnimation {
                    duration: 300
                }
            }

            // Grid Pattern for technical look
            Image {
                anchors.fill: parent
                source: "" // Placeholder for grid pattern if available, otherwise just bgCard
                opacity: 0.05
            }

            ColumnLayout {
                anchors.centerIn: parent
                spacing: Styles.ThemeManager.spacingMd

                // Placeholder preview visual (Minimalist Frame)
                Rectangle {
                    width: 120
                    height: 80
                    color: "transparent"
                    border.width: 2
                    border.color: isRecording ? Styles.ThemeManager.error : Styles.ThemeManager.textMuted
                    radius: 4
                    opacity: 0.5

                    Rectangle {
                        anchors.centerIn: parent
                        width: 40
                        height: 40
                        radius: 20
                        color: isRecording ? Styles.ThemeManager.error : "transparent"
                        border.width: isRecording ? 0 : 2
                        border.color: Styles.ThemeManager.textMuted
                        opacity: isRecording ? 1.0 : 0.5
                    }
                }

                Text {
                    text: isRecording ? "Recording in progress..." : "Preview Area"
                    color: Styles.ThemeManager.textSecondary
                    font.pixelSize: Styles.ThemeManager.fontSizeBody
                    font.family: Styles.ThemeManager.fontFamily
                    Layout.alignment: Qt.AlignHCenter
                }

                Text {
                    text: {
                        var modeText = videoViewModel.getRecorderModeName();
                        var modeDesc = modeText === "VIDEO" ? "30 FPS" : "1 FPS";
                        return "Mode: " + modeText + " (" + modeDesc + ") | Audio: On";
                    }
                    color: Styles.ThemeManager.textMuted
                    font.pixelSize: Styles.ThemeManager.fontSizeSmall
                    font.family: Styles.ThemeManager.fontFamily
                    Layout.alignment: Qt.AlignHCenter
                }
            }
        }

        // 控制按钮行
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 80
            spacing: Styles.ThemeManager.spacingMd

            Item {
                Layout.fillWidth: true
            }

            // Clean Control Bar
            Components.GlassButton {
                Layout.preferredWidth: 200
                Layout.preferredHeight: 48
                text: isRecording ? "STOP RECORDING" : "START RECORDING"

                // Override colors logic inside GlassButton usage or just let it be standard?
                // GlassButton doesn't support easy color override without property alias.
                // Let's stick to using a custom Rectangle here for the Primary Action since it needs specific colors (Red/Primary).
                visible: false // Hiding GlassButton to use custom button below
            }

            // Custom Primary Action Button
            Rectangle {
                width: 240
                height: 56
                radius: 28 // Pill shape
                color: isRecording ? Styles.ThemeManager.surfaceHover : Styles.ThemeManager.primary
                border.width: isRecording ? 2 : 0
                border.color: Styles.ThemeManager.error

                Behavior on color {
                    ColorAnimation {
                        duration: 200
                    }
                }

                RowLayout {
                    anchors.centerIn: parent
                    spacing: 12

                    // Icon
                    Rectangle {
                        width: 16
                        height: 16
                        radius: isRecording ? 2 : 8 // Square when recording (Stop), Circle when idle (Record)
                        color: isRecording ? Styles.ThemeManager.error : "#FFFFFF"
                        Behavior on radius {
                            NumberAnimation {
                                duration: 200
                            }
                        }
                        Behavior on color {
                            ColorAnimation {
                                duration: 200
                            }
                        }
                    }

                    Text {
                        text: isRecording ? "STOP RECORDING" : "START RECORDING"
                        color: isRecording ? Styles.ThemeManager.error : "#FFFFFF"
                        font.pixelSize: 14
                        font.weight: Font.ExtraBold
                        font.family: Styles.ThemeManager.fontFamily
                        font.letterSpacing: 1
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    hoverEnabled: true
                    onEntered: parent.opacity = 0.9
                    onExited: parent.opacity = 1.0
                    onClicked: {
                        if (root.isRecording) {
                            // 停止录制
                            console.log("[Record] Stopping recording...");
                            videoViewModel.stopRecording();
                        } else {
                            // 开始录制
                            console.log("[Record] Starting recording...");
                            videoViewModel.startRecording();
                        }
                    }
                }
            }

            Item {
                Layout.fillWidth: true
            }
        }
    }
}
