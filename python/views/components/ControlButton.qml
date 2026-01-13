// ControlButton.qml - 控制按钮组件

import QtQuick
import "../styles" as Styles

Rectangle {
    id: root
    property string icon: "⏸"
    property string toolTip: ""
    signal clicked()

    width: 44
    height: 44
    radius: 22
    color: mouseArea.containsMouse ? Styles.ThemeManager.bgInput : "transparent"

    Behavior on color {
        ColorAnimation {
            duration: 200
        }
    }

    Text {
        anchors.centerIn: parent
        text: root.icon
        color: root.enabled ? Styles.ThemeManager.text : Styles.ThemeManager.textMuted
        font.pixelSize: 18
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        hoverEnabled: true
        onClicked: root.clicked()
    }
}
