// RecordPage.qml - 录制页面
// 显示录制控制和屏幕预览

import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../styles" as Styles

Rectangle {
    id: root
    color: Styles.ThemeManager.bgPrimary

    // ==================== 状态 ====================

    property bool isRecording: false
    property int recordingTime: 0  // 秒

    // ==================== 录制计时器 ====================

    Timer {
        id: recordingTimer
        interval: 1000
        repeat: true
        running: isRecording
        onTriggered: recordingTime++
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
                text: "🎬 屏幕录制"
                color: Styles.ThemeManager.textPrimary
                font.pixelSize: Styles.ThemeManager.fontSizeH2
                font.weight: Font.Bold
            }

            Item {
                Layout.fillWidth: true
            }

            // 录制状态指示
            Row {
                spacing: Styles.ThemeManager.spacingSm
                visible: isRecording

                Rectangle {
                    width: 12
                    height: 12
                    radius: 6
                    color: Styles.ThemeManager.error
                    anchors.verticalCenter: parent.verticalCenter

                    SequentialAnimation on opacity {
                        running: isRecording
                        loops: Animation.Infinite
                        NumberAnimation {
                            to: 0.4
                            duration: 800
                        }
                        NumberAnimation {
                            to: 1.0
                            duration: 800
                        }
                    }
                }

                Text {
                    text: "录制中 " + formatTime(recordingTime)
                    color: Styles.ThemeManager.error
                    font.pixelSize: Styles.ThemeManager.fontSizeBody
                    font.weight: Font.Medium
                }
            }
        }

        // 屏幕预览区域
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: Styles.ThemeManager.radiusLg
            color: Styles.ThemeManager.bgCard
            border.width: 2
            border.color: isRecording ? Styles.ThemeManager.error : Styles.ThemeManager.border

            Behavior on border.color {
                ColorAnimation {
                    duration: Styles.ThemeManager.animNormal
                }
            }

            ColumnLayout {
                anchors.centerIn: parent
                spacing: Styles.ThemeManager.spacingMd

                Text {
                    text: isRecording ? "🔴" : "🖥️"
                    font.pixelSize: 60
                    Layout.alignment: Qt.AlignHCenter
                }

                Text {
                    text: isRecording ? "正在录制屏幕..." : "屏幕预览区域"
                    color: Styles.ThemeManager.textSecondary
                    font.pixelSize: Styles.ThemeManager.fontSizeBody
                    Layout.alignment: Qt.AlignHCenter
                }

                Text {
                    text: "(Chapter 3 将实现真正的屏幕捕获)"
                    color: Styles.ThemeManager.textMuted
                    font.pixelSize: Styles.ThemeManager.fontSizeSmall
                    Layout.alignment: Qt.AlignHCenter
                }
            }
        }

        // 控制按钮行
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 60
            spacing: Styles.ThemeManager.spacingMd

            Item {
                Layout.fillWidth: true
            }

            // 开始/停止录制按钮
            Rectangle {
                width: 200
                height: 50
                radius: Styles.ThemeManager.radiusMd
                color: isRecording ? Styles.ThemeManager.error : Styles.ThemeManager.primary

                Behavior on color {
                    ColorAnimation {
                        duration: Styles.ThemeManager.animNormal
                    }
                }

                Text {
                    anchors.centerIn: parent
                    text: isRecording ? "⏹ 停止录制" : "🎬 开始录制"
                    color: "#ffffff"
                    font.pixelSize: Styles.ThemeManager.fontSizeBody
                    font.weight: Font.Medium
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        if (isRecording) {
                            isRecording = false;
                            recordingTime = 0;
                            console.log("[Record] Stopped recording");
                        } else {
                            isRecording = true;
                            console.log("[Record] Started recording");
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
