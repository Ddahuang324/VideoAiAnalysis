// ThemeManager.qml - 主题管理器单例
// 支持亮色/暗色主题切换

pragma Singleton
import QtQuick 2.15

QtObject {
    id: theme

    // ==================== 主题开关 ====================
    property bool isDark: false  // 默认亮色主题

    // ==================== 颜色系统 ====================

    // 主色调
    readonly property color primary: "#6366f1"
    readonly property color primaryHover: "#4f46e5"
    readonly property color primaryActive: "#4338ca"

    // 背景色
    readonly property color bgPrimary: isDark ? "#0f0f23" : "#ffffff"
    readonly property color bgSecondary: isDark ? "#1a1a2e" : "#f5f5f7"
    readonly property color bgTertiary: isDark ? "#16213e" : "#eaeaec"
    readonly property color bgCard: isDark ? "#1e1e32" : "#ffffff"

    // 文本色
    readonly property color textPrimary: isDark ? "#ffffff" : "#1a1a2e"
    readonly property color textSecondary: isDark ? "#a0a0b0" : "#6b7280"
    readonly property color textMuted: isDark ? "#6b7280" : "#9ca3af"

    // 边框色
    readonly property color border: isDark ? "#2d2d44" : "#e5e7eb"
    readonly property color borderLight: isDark ? "#3d3d5c" : "#d1d5db"

    // 状态色
    readonly property color success: "#22c55e"
    readonly property color warning: "#f59e0b"
    readonly property color error: "#ef4444"
    readonly property color info: "#3b82f6"

    // 侧边栏特定颜色
    readonly property color sidebarBg: isDark ? "#1a1a2e" : "#f8f9fa"
    readonly property color sidebarItemHover: isDark ? "#2d2d44" : "#e9ecef"
    readonly property color sidebarItemActive: isDark ? "#6366f120" : "#6366f115"

    // 标题栏颜色
    readonly property color titleBarBg: isDark ? "#1e1e2e" : "#ffffff"
    readonly property color titleBarBorder: isDark ? "#2d2d44" : "#e5e7eb"

    // ==================== 字体系统 ====================

    readonly property string fontFamily: "Inter, Segoe UI, sans-serif"

    readonly property int fontSizeH1: 32
    readonly property int fontSizeH2: 24
    readonly property int fontSizeH3: 18
    readonly property int fontSizeBody: 14
    readonly property int fontSizeSmall: 12
    readonly property int fontSizeTiny: 10

    // ==================== 间距系统 ====================

    readonly property int spacingXs: 4
    readonly property int spacingSm: 8
    readonly property int spacingMd: 16
    readonly property int spacingLg: 24
    readonly property int spacingXl: 32

    // ==================== 圆角系统 ====================

    readonly property int radiusSm: 4
    readonly property int radiusMd: 8
    readonly property int radiusLg: 12
    readonly property int radiusXl: 16
    readonly property int radiusFull: 9999

    // ==================== 动画时长 ====================

    readonly property int animFast: 150
    readonly property int animNormal: 250
    readonly property int animSlow: 400

    // ==================== 尺寸规格 ====================

    readonly property int sidebarExpandedWidth: 240
    readonly property int sidebarCollapsedWidth: 64
    readonly property int sidebarExpandThreshold: 960
    readonly property int titleBarHeight: 48

    // ==================== 阴影 ====================

    readonly property color shadowColor: isDark ? "#80000000" : "#20000000"
    readonly property int shadowRadius: 12

    // ==================== 方法 ====================

    function toggleTheme() {
        isDark = !isDark;
    }
}
