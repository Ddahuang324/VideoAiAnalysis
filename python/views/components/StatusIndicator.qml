// StatusIndicator.qml - 状态指示器组件
// 用于显示录制状态（呼吸灯效果）

import QtQuick 2.15
import "../styles" as Styles

Rectangle {
    id: root
    width: 12
    height: 12
    radius: width / 2
    color: statusColor

    // ==================== 公共 API ====================

    property string status: "idle"  // "idle" | "recording" | "processing" | "success" | "error"
    property bool animated: status === "recording"

    // ==================== 状态颜色映射 ====================

    readonly property color statusColor: {
        switch (status) {
        case "recording":
            return Styles.ThemeManager.error;
        case "processing":
            return Styles.ThemeManager.warning;
        case "success":
            return Styles.ThemeManager.success;
        case "error":
            return Styles.ThemeManager.error;
        default:
            return Styles.ThemeManager.textMuted;
        }
    }

    // ==================== 呼吸动画 ====================

    SequentialAnimation {
        id: breatheAnimation
        running: root.animated
        loops: Animation.Infinite

        NumberAnimation {
            target: root
            property: "opacity"
            from: 1.0
            to: 0.4
            duration: 800
            easing.type: Easing.InOutSine
        }

        NumberAnimation {
            target: root
            property: "opacity"
            from: 0.4
            to: 1.0
            duration: 800
            easing.type: Easing.InOutSine
        }
    }

    // ==================== 动画控制 ====================

    onAnimatedChanged: {
        if (!animated) {
            breatheAnimation.stop();
            opacity = 1.0;
        }
    }

    Behavior on color {
        ColorAnimation {
            duration: Styles.ThemeManager.animNormal
        }
    }
}
