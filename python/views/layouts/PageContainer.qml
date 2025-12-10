// PageContainer.qml - 页面容器组件
// 使用 Loader 缓存模式，页面实例不销毁

import QtQuick 2.15
import "../styles" as Styles

Item {
    id: root

    // ==================== 公共 API ====================

    property int currentIndex: 0

    // ==================== 页面 Loaders ====================

    // 首页 Loader
    Loader {
        id: homeLoader
        anchors.fill: parent
        active: true  // 默认激活
        visible: root.currentIndex === 0
        source: "../pages/HomePage.qml"
    }

    // 录制页 Loader
    Loader {
        id: recordLoader
        anchors.fill: parent
        active: root.currentIndex === 1 || recordLoader.item !== null  // 激活后保持
        visible: root.currentIndex === 1
        source: "../pages/RecordPage.qml"
    }

    // 历史页 Loader
    Loader {
        id: archiveLoader
        anchors.fill: parent
        active: root.currentIndex === 2 || archiveLoader.item !== null
        visible: root.currentIndex === 2
        source: "../pages/ArchivePage.qml"
    }

    // 设置页 Loader
    Loader {
        id: settingsLoader
        anchors.fill: parent
        active: root.currentIndex === 3 || settingsLoader.item !== null
        visible: root.currentIndex === 3
        source: "../pages/SettingsPage.qml"
    }
}
