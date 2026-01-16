// SidebarButton.qml - 侧边栏按钮组件

import QtQuick
import "../styles" as Styles

Rectangle {
    id: root
    property string icon: "record"
    property bool isActive: false
    property bool hasPulse: false
    signal clicked

    width: 48
    height: 48
    radius: 12
    color: isActive ? Styles.ThemeManager.bgSurface : "transparent"

    border.width: 0

    Behavior on color {
        ColorAnimation {
            duration: 200
        }
    }

    // 活动指示器
    Rectangle {
        anchors.fill: parent
        radius: parent.radius
        color: Styles.ThemeManager.bgSurface
        opacity: isActive ? 1 : (mouseArea.containsMouse ? 0.5 : 0)

        Behavior on opacity {
            NumberAnimation {
                duration: 200
            }
        }
    }

    // 图标 (Canvas 替换 Text)
    Canvas {
        id: iconCanvas
        anchors.centerIn: parent
        width: 24
        height: 24
        onPaint: {
            var ctx = getContext("2d");
            ctx.reset();
            ctx.strokeStyle = root.isActive ? Styles.ThemeManager.primary : Styles.ThemeManager.textSecondary;
            ctx.lineWidth = 1.8;
            ctx.lineCap = "round";
            ctx.lineJoin = "round";

            var w = width;
            var h = height;

            switch (root.icon) {
            case "record":
                // Circle icon
                ctx.beginPath();
                ctx.arc(w / 2, h / 2, 9, 0, 2 * Math.PI);
                ctx.stroke();
                ctx.beginPath();
                ctx.arc(w / 2, h / 2, 3, 0, 2 * Math.PI);
                ctx.stroke();
                break;
            case "library":
                // Grid icon
                var s = 8;
                ctx.strokeRect(2, 2, s, s);
                ctx.strokeRect(14, 2, s, s);
                ctx.strokeRect(2, 14, s, s);
                ctx.strokeRect(14, 14, s, s);
                break;
            case "overview":
                // Dashboard/Pulse icon
                ctx.beginPath();
                ctx.moveTo(2, 12);
                ctx.lineTo(6, 12);
                ctx.lineTo(9, 4);
                ctx.lineTo(15, 20);
                ctx.lineTo(18, 12);
                ctx.lineTo(22, 12);
                ctx.stroke();
                break;
            case "settings":
                // Gear icon
                ctx.beginPath();
                ctx.arc(w / 2, h / 2, 5, 0, 2 * Math.PI);
                ctx.stroke();
                for (var i = 0; i < 8; i++) {
                    var angle = i * Math.PI / 4;
                    ctx.beginPath();
                    ctx.moveTo(w / 2 + 6 * Math.cos(angle), h / 2 + 6 * Math.sin(angle));
                    ctx.lineTo(w / 2 + 9 * Math.cos(angle), h / 2 + 9 * Math.sin(angle));
                    ctx.stroke();
                }
                break;
            case "prompt":
                // Document/text icon
                ctx.strokeRect(4, 2, 16, 20);
                ctx.beginPath();
                ctx.moveTo(7, 7); ctx.lineTo(17, 7);
                ctx.moveTo(7, 11); ctx.lineTo(17, 11);
                ctx.moveTo(7, 15); ctx.lineTo(13, 15);
                ctx.stroke();
                break;
            }
        }

        // 监听状态变化重绘
        Connections {
            target: root
            function onIsActiveChanged() {
                iconCanvas.requestPaint();
            }
        }
    }

    // 录制按钮脉冲指示
    Rectangle {
        visible: root.hasPulse && !root.isActive
        width: 6
        height: 6
        radius: 3
        color: Styles.ThemeManager.accent
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.topMargin: 10
        anchors.rightMargin: 10

        SequentialAnimation on opacity {
            loops: Animation.Infinite
            NumberAnimation {
                to: 1
                duration: 800
            }
            NumberAnimation {
                to: 0.3
                duration: 800
            }
        }
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        hoverEnabled: true
        onClicked: root.clicked()
    }
}
