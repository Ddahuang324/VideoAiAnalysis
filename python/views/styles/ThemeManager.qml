// ThemeManager.qml - 主题管理器单例
// 支持亮色/暗色主题切换

pragma Singleton
import QtQuick 2.15

QtObject {
    id: theme

    // ==================== 主题开关 ====================
    property bool isDark: false  // 默认亮色主题

    // ==================== 颜色系统 ====================

    // 主色调 (International Orange Accent)
    readonly property color primary: "#FF4D00"
    readonly property color primaryHover: "#E64500"
    readonly property color primaryActive: "#CC3D00"

    // 背景色 (Ultra Clean)
    readonly property color bgPrimary: isDark ? "#0A0A0A" : "#FFFFFF"
    readonly property color bgSecondary: isDark ? "#141414" : "#FAFAFA"
    readonly property color bgTertiary: isDark ? "#1F1F1F" : "#F4F4F5"
    readonly property color bgCard: isDark ? "#141414" : "#FFFFFF"
    readonly property color surfaceHover: isDark ? "#1A1A1A" : "#F0F0F0"

    // 文本色 (High Contrast)
    readonly property color textPrimary: isDark ? "#FFFFFF" : "#111111"
    readonly property color textSecondary: isDark ? "#888888" : "#666666"
    readonly property color textMuted: isDark ? "#555555" : "#999999"

    // 边框色 (Subtle)
    readonly property color border: isDark ? "#2A2A2A" : "#EBEBEB"
    readonly property color borderLight: isDark ? "#333333" : "#F5F5F5"

    // 状态色
    readonly property color success: "#00C853"
    readonly property color warning: "#FFAB00"
    readonly property color error: "#FF1744"
    readonly property color info: "#2979FF"

    // 侧边栏特定颜色
    readonly property color sidebarBg: isDark ? "#0A0A0A" : "#FAFAFA"
    readonly property color sidebarItemHover: isDark ? "#1F1F1F" : "#F0F0F0"
    readonly property color sidebarItemActive: isDark ? "#1F1F1F" : "#ffffff"

    // 标题栏颜色
    readonly property color titleBarBg: isDark ? "#0A0A0A" : "#FFFFFF"
    readonly property color titleBarBorder: isDark ? "#2A2A2A" : "transparent"

    // ==================== 字体系统 ====================

    readonly property string fontFamily: "Inter, 'Segoe UI', Roboto, sans-serif"

    readonly property int fontSizeH1: 48 // Bigger, Bolder
    readonly property int fontSizeH2: 32
    readonly property int fontSizeH3: 20
    readonly property int fontSizeBody: 14
    readonly property int fontSizeSmall: 12
    readonly property int fontSizeTiny: 10

    // ==================== 间距系统 ====================

    readonly property int spacingXs: 4
    readonly property int spacingSm: 12
    readonly property int spacingMd: 24
    readonly property int spacingLg: 40
    readonly property int spacingXl: 64

    // ==================== 圆角系统 (Asymmetrical Base) ====================

    readonly property int radiusSm: 4
    readonly property int radiusMd: 12
    readonly property int radiusLg: 24    // Much softer
    readonly property int radiusXl: 32
    readonly property int radiusFull: 9999

    // ==================== 动画时长 ====================

    readonly property int animFast: 200
    readonly property int animNormal: 400 // Slower, smoother
    readonly property int animSlow: 600

    // ==================== 尺寸规格 ====================

    readonly property int sidebarExpandedWidth: 260
    readonly property int sidebarCollapsedWidth: 80
    readonly property int sidebarExpandThreshold: 1000
    readonly property int titleBarHeight: 60 // Taller for cleaner look

    // ==================== 阴影 ====================

    readonly property color shadowColor: isDark ? "#AA000000" : "#1A000000"
    readonly property int shadowRadius: 30

    // ==================== 方法 ====================

    function toggleTheme() {
        isDark = !isDark;
    }
}
