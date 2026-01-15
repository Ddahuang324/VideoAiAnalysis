// RecordPage.qml - 录制页面
// 参考 videosys-ai 设计风格
// 大预览区域 + 底部控制栏

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import "../styles" as Styles
import "../components"

Rectangle {
    id: root
    anchors.fill: parent
    color: "transparent"

    // 录制状态 (连接到 ViewModel)
    property bool isRecording: recorderViewModel ? recorderViewModel.isRecording : false
    property bool isPaused: recorderViewModel ? recorderViewModel.isPaused : false
    property int recordingTime: 0  // 秒
    property string outputPath: ""

    // 录制计时器
    Timer {
        id: recordingTimer
        interval: 1000
        repeat: true
        running: isRecording && !isPaused
        onTriggered: {
            // 时间同步可以通过 ViewModel 更好，这里临时保持简单的逻辑或同步
            recordingTime++;
        }
    }

    // ==================== 主布局 ====================

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ========== 预览区域 (占主要空间) ==========
        Rectangle {
            id: previewArea
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: 24
            radius: Styles.ThemeManager.radiusXl
            color: Styles.ThemeManager.bgSurface
            border.width: isRecording ? 2 : 1
            border.color: isRecording ? Styles.ThemeManager.accent : Styles.ThemeManager.border

            Behavior on border.color {
                ColorAnimation {
                    duration: 300
                }
            }

            Behavior on border.width {
                NumberAnimation {
                    duration: 300
                }
            }

            clip: true

            // 网格背景图案
            Canvas {
                anchors.fill: parent
                opacity: 0.03
                onPaint: {
                    var ctx = getContext("2d");
                    ctx.clearRect(0, 0, width, height);
                    ctx.strokeStyle = Styles.ThemeManager.text;
                    ctx.lineWidth = 1;

                    var gridSize = 40;

                    // 垂直线
                    for (var x = 0; x < width; x += gridSize) {
                        ctx.beginPath();
                        ctx.moveTo(x, 0);
                        ctx.lineTo(x, height);
                        ctx.stroke();
                    }

                    // 水平线
                    for (var y = 0; y < height; y += gridSize) {
                        ctx.beginPath();
                        ctx.moveTo(0, y);
                        ctx.lineTo(width, y);
                        ctx.stroke();
                    }
                }
            }

            // ========== 录制中指示器 (左上角) ==========
            Row {
                visible: isRecording
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.margins: 24
                spacing: 12

                // 脉冲红点
                Item {
                    width: 12
                    height: 12
                    anchors.verticalCenter: parent.verticalCenter

                    Rectangle {
                        anchors.centerIn: parent
                        width: 12
                        height: 12
                        radius: 6
                        color: Styles.ThemeManager.accent
                    }

                    Rectangle {
                        anchors.centerIn: parent
                        width: 12
                        height: 12
                        radius: 6
                        color: Styles.ThemeManager.accent

                        SequentialAnimation on scale {
                            loops: Animation.Infinite
                            NumberAnimation {
                                to: 2
                                duration: 1000
                            }
                            NumberAnimation {
                                to: 1
                                duration: 0
                            }
                        }
                        SequentialAnimation on opacity {
                            loops: Animation.Infinite
                            NumberAnimation {
                                to: 0
                                duration: 1000
                            }
                            NumberAnimation {
                                to: 1
                                duration: 0
                            }
                        }
                    }
                }

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: "REC " + formatTime(recordingTime)
                    color: Styles.ThemeManager.accent
                    font.pixelSize: 13
                    font.family: "monospace"
                    font.letterSpacing: 2
                }
            }

            // ========== 空闲状态中心提示 ==========
            Column {
                visible: !isRecording
                anchors.centerIn: parent
                spacing: 16

                opacity: 0.3

                Canvas {
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: 64
                    height: 64
                    onPaint: {
                        var ctx = getContext("2d");
                        ctx.reset();
                        ctx.strokeStyle = Styles.ThemeManager.text;
                        ctx.lineWidth = 2;
                        ctx.beginPath();
                        ctx.arc(32, 32, 28, 0, 2 * Math.PI);
                        ctx.stroke();
                        ctx.beginPath();
                        ctx.arc(32, 32, 12, 0, 2 * Math.PI);
                        ctx.stroke();
                    }
                }

                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: "READY TO CAPTURE"
                    color: Styles.ThemeManager.text
                    font.pixelSize: 12
                    font.letterSpacing: 3
                }
            }

            // ========== 右上角模式和Prompt选择 (New) ==========
            Row {
                visible: !isRecording
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.margins: 24
                spacing: 12

                // Prompt 选择
                Rectangle {
                    width: 180
                    height: 36
                    radius: 8
                    color: Styles.ThemeManager.overlayDark
                    border.width: 1
                    border.color: Styles.ThemeManager.border

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 12
                        anchors.rightMargin: 12
                        spacing: 8

                        Text {
                            text: "PROMPT:"
                            color: Styles.ThemeManager.textMuted
                            font.pixelSize: 10
                            font.weight: Font.Bold
                        }

                        Text {
                            Layout.fillWidth: true
                            text: "Default Analysis"
                            color: Styles.ThemeManager.textSecondary
                            font.pixelSize: 11
                            elide: Text.ElideRight
                        }

                        Text {
                            text: "▾"
                            color: Styles.ThemeManager.textSecondary
                            font.pixelSize: 12
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: console.log("Open Prompt Selector")
                    }
                }

                // 模式选择
                ComboBox {
                    id: modeCombo
                    width: 140
                    height: 36
                    model: ["VIDEO MODE", "SNAPSHOT MODE"]
                    currentIndex: recorderViewModel ? (recorderViewModel.captureMode === "VIDEO MODE" ? 0 : 1) : 0

                    onActivated: index => {
                        console.log("QML: Mode selected: " + modeCombo.model[index]);
                        recorderViewModel.captureMode = modeCombo.model[index];
                    }

                    background: Rectangle {
                        radius: 8
                        color: Styles.ThemeManager.overlayDark
                        border.width: 1
                        border.color: Styles.ThemeManager.border
                    }

                    contentItem: Item {
                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                            spacing: 8

                            Text {
                                Layout.fillWidth: true
                                text: modeCombo.displayText
                                color: Styles.ThemeManager.textSecondary
                                font.pixelSize: 11
                                font.weight: Font.Medium
                                verticalAlignment: Text.AlignVCenter
                            }

                            Text {
                                text: "▾"
                                color: Styles.ThemeManager.textSecondary
                                font.pixelSize: 12
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                    }

                    delegate: ItemDelegate {
                        width: modeCombo.width
                        contentItem: Text {
                            text: modelData
                            color: Styles.ThemeManager.text
                            font.pixelSize: 11
                            verticalAlignment: Text.AlignVCenter
                            leftPadding: 12
                        }
                        background: Rectangle {
                            color: highlighted ? Styles.ThemeManager.overlayLight : "transparent"
                        }
                    }

                    popup: Popup {
                        y: modeCombo.height + 4
                        width: modeCombo.width
                        padding: 1

                        contentItem: ListView {
                            clip: true
                            implicitHeight: contentHeight
                            model: modeCombo.popup.visible ? modeCombo.delegateModel : null
                        }

                        background: Rectangle {
                            radius: 8
                            color: Styles.ThemeManager.bgSurface
                            border.color: Styles.ThemeManager.border
                            border.width: 1
                        }
                    }
                }
            }

            // ========== 左下角基础信息 (New) ==========
            Column {
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.margins: 24
                spacing: 8

                Row {
                    spacing: 12
                    opacity: 0.7

                    // 分辨率
                    Row {
                        spacing: 6
                        Rectangle {
                            width: 12
                            height: 12
                            radius: 2
                            color: "transparent"
                            border.width: 1.5
                            border.color: Styles.ThemeManager.textSecondary
                            anchors.verticalCenter: parent.verticalCenter
                            Rectangle {
                                anchors.centerIn: parent
                                width: 4
                                height: 4
                                radius: 1
                                color: Styles.ThemeManager.textSecondary
                            }
                        }
                        Text {
                            text: "1080P"
                            color: Styles.ThemeManager.textSecondary
                            font.pixelSize: 11
                            font.weight: Font.Bold
                        }
                    }

                    // 帧率
                    Row {
                        spacing: 6
                        Text {
                            text: "FPS"
                            color: Styles.ThemeManager.textMuted
                            font.pixelSize: 10
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        Text {
                            text: "60"
                            color: Styles.ThemeManager.textSecondary
                            font.pixelSize: 11
                            font.weight: Font.Bold
                        }
                    }

                    // 格式
                    Row {
                        spacing: 6
                        Text {
                            text: "EXT"
                            color: Styles.ThemeManager.textMuted
                            font.pixelSize: 10
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        Text {
                            text: "MP4"
                            color: Styles.ThemeManager.textSecondary
                            font.pixelSize: 11
                            font.weight: Font.Bold
                        }
                    }
                }

                Text {
                    text: "OUTPUT: " + (outputPath || "C:/Users/Administrator/Videos")
                    color: Styles.ThemeManager.textMuted
                    font.pixelSize: 10
                    opacity: 0.5
                }
            }
        }

        // ========== 底部控制栏 ==========
        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 120

            // 控制按钮容器
            Rectangle {
                anchors.centerIn: parent
                width: 320
                height: 72
                radius: 36
                color: Styles.ThemeManager.bgSurface
                border.width: 1
                border.color: Styles.ThemeManager.border

                RowLayout {
                    id: controlRow
                    anchors.fill: parent
                    anchors.leftMargin: 24
                    anchors.rightMargin: 24
                    spacing: 0

                    // 截图按钮
                    Item {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 44

                        Rectangle {
                            anchors.centerIn: parent
                            width: 44
                            height: 44
                            radius: 22
                            color: screenshotMA.containsMouse ? Styles.ThemeManager.bgInput : "transparent"

                            Behavior on color {
                                ColorAnimation {
                                    duration: 200
                                }
                            }

                            Canvas {
                                anchors.centerIn: parent
                                width: 20
                                height: 20
                                onPaint: {
                                    var ctx = getContext("2d");
                                    ctx.reset();
                                    ctx.strokeStyle = Styles.ThemeManager.text;
                                    ctx.lineWidth = 1.5;
                                    ctx.strokeRect(2, 4, 16, 12);
                                    ctx.beginPath();
                                    ctx.arc(10, 10, 3, 0, 2 * Math.PI);
                                    ctx.stroke();
                                    ctx.strokeRect(14, 2, 2, 2);
                                }
                            }

                            MouseArea {
                                id: screenshotMA
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: console.log("Screenshot")
                            }
                        }
                    }

                    // 主录制按钮
                    Item {
                        Layout.preferredWidth: 80
                        Layout.preferredHeight: 80

                        Rectangle {
                            anchors.centerIn: parent
                            width: 64
                            height: 64
                            radius: 32
                            color: "transparent"
                            border.width: 4
                            border.color: Styles.ThemeManager.border

                            // 内部内容
                            Rectangle {
                                id: recordButtonInner
                                anchors.centerIn: parent
                                width: isRecording ? 24 : 48
                                height: isRecording ? 24 : 48
                                radius: isRecording ? 4 : 24
                                color: isRecording ? Styles.ThemeManager.accent : Styles.ThemeManager.primary

                                Behavior on width {
                                    NumberAnimation {
                                        duration: 300
                                        easing.type: Easing.InOutCubic
                                    }
                                }
                                Behavior on height {
                                    NumberAnimation {
                                        duration: 300
                                        easing.type: Easing.InOutCubic
                                    }
                                }
                                Behavior on radius {
                                    NumberAnimation {
                                        duration: 300
                                    }
                                }
                                Behavior on color {
                                    ColorAnimation {
                                        duration: 300
                                    }
                                }
                            }

                            // 呼吸动画 (未录制时)
                            Rectangle {
                                visible: !isRecording
                                anchors.centerIn: parent
                                width: 48
                                height: 48
                                radius: 24
                                color: Styles.ThemeManager.primary
                                opacity: 0.3
                                SequentialAnimation on opacity {
                                    loops: Animation.Infinite
                                    NumberAnimation {
                                        to: 0.6
                                        duration: 1500
                                    }
                                    NumberAnimation {
                                        to: 0.3
                                        duration: 1500
                                    }
                                }
                                SequentialAnimation on scale {
                                    loops: Animation.Infinite
                                    NumberAnimation {
                                        to: 1.2
                                        duration: 1500
                                    }
                                    NumberAnimation {
                                        to: 1.0
                                        duration: 1500
                                    }
                                }
                            }

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    recorderViewModel.toggleRecording();
                                    if (!isRecording) {
                                        recordingTime = 0;
                                    }
                                }
                            }
                        }
                    }

                    // 暂停/恢复按钮
                    Item {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 44
                        opacity: isRecording ? 1 : 0.3

                        Rectangle {
                            anchors.centerIn: parent
                            width: 44
                            height: 44
                            radius: 22
                            color: pauseMA.containsMouse ? Styles.ThemeManager.bgInput : "transparent"

                            Behavior on color {
                                ColorAnimation {
                                    duration: 200
                                }
                            }

                            Canvas {
                                anchors.centerIn: parent
                                width: 20
                                height: 20
                                onPaint: {
                                    var ctx = getContext("2d");
                                    ctx.reset();
                                    ctx.fillStyle = Styles.ThemeManager.text;
                                    if (isPaused) {
                                        // Play triangle
                                        ctx.beginPath();
                                        ctx.moveTo(6, 4);
                                        ctx.lineTo(16, 10);
                                        ctx.lineTo(6, 16);
                                        ctx.closePath();
                                        ctx.fill();
                                    } else {
                                        // Pause bars
                                        ctx.fillRect(6, 4, 3, 12);
                                        ctx.fillRect(11, 4, 3, 12);
                                    }
                                }
                            }

                            MouseArea {
                                id: pauseMA
                                anchors.fill: parent
                                enabled: isRecording
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: recorderViewModel.togglePause()
                            }
                        }
                    }
                }
            }
        }
    }

    // ==================== 辅助函数 ====================

    function formatTime(seconds) {
        var mins = Math.floor(seconds / 60);
        var secs = seconds % 60;
        return mins.toString().padStart(2, '0') + ":" + secs.toString().padStart(2, '0');
    }
}
