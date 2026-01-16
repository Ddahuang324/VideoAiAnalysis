// ThemeManager.qml - 主题管理器
// 参考 videosys-ai 设计风格
// 暗色主题 + zinc 色调系统 + 毛玻璃效果

pragma Singleton
import QtQuick

QtObject {
    id: root

    // ==================== 主题模式 ====================
    property bool isDark: true  // 默认暗色主题

    // ==================== 颜色系统 (Zinc Palette) ====================

    // 背景色 - zinc 暗色系
    readonly property color bg: isDark ? "#09090b" : "#ffffff"      // zinc-950 / white
    readonly property color bgSurface: isDark ? "#18181b" : "#fafafa"  // zinc-900 / zinc-50
    readonly property color bgCard: isDark ? "#18181b" : "#ffffff"  // zinc-900 / white
    readonly property color bgInput: isDark ? "#27272a" : "#f4f4f5"  // zinc-800 / zinc-100

    // 文本色
    readonly property color text: isDark ? "#fafafa" : "#09090b"     // zinc-50 / zinc-950
    readonly property color textSecondary: isDark ? "#a1a1aa" : "#71717a"  // zinc-400 / zinc-500
    readonly property color textMuted: isDark ? "#71717a" : "#a1a1aa"  // zinc-500 / zinc-400

    // 主色调 - 紫色调 (更现代)
    readonly property color primary: isDark ? "#a78bfa" : "#7c3aed"  // violet-400 / violet-600
    readonly property color primaryHover: isDark ? "#c4b5fd" : "#8b5cf6"
    readonly property color primaryBg: isDark ? "#4c1d95" : "#ddd6fe"  // violet-900 / violet-200

    // 强调色 - 红色 (录制状态)
    readonly property color accent: "#ef4444"  // red-500
    readonly property color accentGlow: "#dc2626"  // red-600

    // 边框色
    readonly property color border: isDark ? "#27272a" : "#e4e4e7"  // zinc-800 / zinc-200
    readonly property color borderLight: isDark ? "#3f3f46" : "#f4f4f5"  // zinc-700 / zinc-100

    // 状态色
    readonly property color success: "#22c55e"  // green-500
    readonly property color warning: "#f59e0b"  // amber-500
    readonly property color error: "#ef4444"    // red-500
    readonly property color info: "#3b82f6"     // blue-500

    // 半透明覆盖层
    readonly property color overlayDark: "#80000000"
    readonly property color overlayLight: "#40ffffff"
    readonly property color glassDark: "#c018181b"  // 75% opaque zinc-900
    readonly property color glassLight: "#c0fafafa"  // 75% opaque zinc-50

    // ==================== 字体系统 ====================

    readonly property string fontFamily: isDark ? "'Inter', 'Segoe UI', Roboto, Helvetica, Arial, sans-serif" : "'Inter', 'Segoe UI', Roboto, Helvetica, Arial, sans-serif"

    readonly property int fontSizeH1: 36
    readonly property int fontSizeH2: 28
    readonly property int fontSizeH3: 20
    readonly property int fontSizeBody: 14
    readonly property int fontSizeSmall: 12
    readonly property int fontSizeTiny: 10

    // ==================== 间距系统 ====================

    readonly property int spacingXs: 4
    readonly property int spacingSm: 8
    readonly property int spacingMd: 16
    readonly property int spacingLg: 24
    readonly property int spacingXl: 32
    readonly property int spacing2xl: 48

    // ==================== 圆角系统 ====================

    readonly property int radiusSm: 8    // rounded-lg
    readonly property int radiusMd: 12   // rounded-xl
    readonly property int radiusLg: 16   // rounded-2xl
    readonly property int radiusXl: 24   // rounded-3xl
    readonly property int radiusFull: 9999

    // ==================== 布局尺寸 ====================

    readonly property int sidebarWidth: 80      // 窄侧边栏 (参考项目设计)
    readonly property int headerHeight: 0       // 无传统标题栏
    readonly property int controlBarHeight: 96  // 底部控制栏高度

    // ==================== 动画时长 ====================

    readonly property int animFast: 150
    readonly property int animNormal: 300
    readonly property int animSlow: 500

    // ==================== 渐变 ====================

    readonly property Gradient backgroundGradient: Gradient {
        GradientStop {
            position: 0.0
            color: "transparent"
        }
        GradientStop {
            position: 1.0
            color: root.isDark ? "#10000000" : "#10ffffff"
        }
    }

    readonly property Gradient primaryGradient: Gradient {
        GradientStop {
            position: 0.0
            color: root.primary
        }
        GradientStop {
            position: 1.0
            color: root.primaryHover
        }
    }

    // ==================== 阴影 ====================

    readonly property string shadowSoft: isDark ? "0 4px 24px rgba(0,0,0,0.4)" : "0 4px 24px rgba(0,0,0,0.08)"

    readonly property string shadowMedium: isDark ? "0 8px 40px rgba(0,0,0,0.5)" : "0 8px 40px rgba(0,0,0,0.12)"

    readonly property string shadowGlow: isDark ? "0 0 60px rgba(167, 139, 250, 0.15)" : "0 0 60px rgba(124, 58, 237, 0.15)"

    // ==================== 方法 ====================

    function toggleTheme() {
        isDark = !isDark;
    }

    // ==================== 便捷访问属性 (兼容旧代码) ====================

    property alias primaryColor: root.primary
    property alias textPrimary: root.text
    property alias textPrimaryColor: root.text
    property alias textSecondaryColor: root.textSecondary
    property alias bgPrimary: root.bg
    property alias bgPrimaryColor: root.bg
    property alias bgCardColor: root.bgCard
}
