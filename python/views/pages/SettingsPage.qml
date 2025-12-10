// SettingsPage.qml - 设置页面
// 管理 API Key、主题和其他配置

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../styles" as Styles

Rectangle {
    id: root
    color: Styles.ThemeManager.bgPrimary

    // ==================== 设置数据 ====================

    property string apiKey: ""
    property bool autoSave: true

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Styles.ThemeManager.spacingLg
        spacing: Styles.ThemeManager.spacingLg

        // 标题
        Text {
            text: "⚙️ 设置"
            color: Styles.ThemeManager.textPrimary
            font.pixelSize: Styles.ThemeManager.fontSizeH2
            font.weight: Font.Bold
        }

        // 设置卡片容器
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            ColumnLayout {
                width: parent.width
                spacing: Styles.ThemeManager.spacingMd

                // ==================== API 配置 ====================

                Rectangle {
                    Layout.fillWidth: true
                    height: apiSection.height + Styles.ThemeManager.spacingLg * 2
                    radius: Styles.ThemeManager.radiusLg
                    color: Styles.ThemeManager.bgCard
                    border.width: 1
                    border.color: Styles.ThemeManager.border

                    ColumnLayout {
                        id: apiSection
                        anchors.fill: parent
                        anchors.margins: Styles.ThemeManager.spacingLg
                        spacing: Styles.ThemeManager.spacingMd

                        Text {
                            text: "🔑 API 配置"
                            color: Styles.ThemeManager.textPrimary
                            font.pixelSize: Styles.ThemeManager.fontSizeH3
                            font.weight: Font.Medium
                        }

                        // API Key 输入
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: Styles.ThemeManager.spacingXs

                            Text {
                                text: "Google Gemini API Key"
                                color: Styles.ThemeManager.textSecondary
                                font.pixelSize: Styles.ThemeManager.fontSizeSmall
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                height: 44
                                radius: Styles.ThemeManager.radiusMd
                                color: Styles.ThemeManager.bgSecondary
                                border.width: 1
                                border.color: apiKeyInput.activeFocus ? Styles.ThemeManager.primary : Styles.ThemeManager.border

                                TextInput {
                                    id: apiKeyInput
                                    anchors.fill: parent
                                    anchors.margins: Styles.ThemeManager.spacingSm
                                    color: Styles.ThemeManager.textPrimary
                                    font.pixelSize: Styles.ThemeManager.fontSizeBody
                                    echoMode: TextInput.Password
                                    verticalAlignment: TextInput.AlignVCenter
                                    text: root.apiKey
                                    onTextChanged: root.apiKey = text

                                    Text {
                                        anchors.fill: parent
                                        anchors.leftMargin: 0
                                        verticalAlignment: Text.AlignVCenter
                                        text: "请输入 API Key..."
                                        color: Styles.ThemeManager.textMuted
                                        font.pixelSize: Styles.ThemeManager.fontSizeBody
                                        visible: !parent.text && !parent.activeFocus
                                    }
                                }
                            }
                        }
                    }
                }

                // ==================== 外观设置 ====================

                Rectangle {
                    Layout.fillWidth: true
                    height: themeSection.height + Styles.ThemeManager.spacingLg * 2
                    radius: Styles.ThemeManager.radiusLg
                    color: Styles.ThemeManager.bgCard
                    border.width: 1
                    border.color: Styles.ThemeManager.border

                    ColumnLayout {
                        id: themeSection
                        anchors.fill: parent
                        anchors.margins: Styles.ThemeManager.spacingLg
                        spacing: Styles.ThemeManager.spacingMd

                        Text {
                            text: "🎨 外观"
                            color: Styles.ThemeManager.textPrimary
                            font.pixelSize: Styles.ThemeManager.fontSizeH3
                            font.weight: Font.Medium
                        }

                        // 主题切换
                        RowLayout {
                            Layout.fillWidth: true

                            Text {
                                text: "深色主题"
                                color: Styles.ThemeManager.textPrimary
                                font.pixelSize: Styles.ThemeManager.fontSizeBody
                            }

                            Item {
                                Layout.fillWidth: true
                            }

                            // 开关
                            Rectangle {
                                width: 50
                                height: 26
                                radius: 13
                                color: Styles.ThemeManager.isDark ? Styles.ThemeManager.primary : Styles.ThemeManager.bgTertiary

                                Behavior on color {
                                    ColorAnimation {
                                        duration: Styles.ThemeManager.animNormal
                                    }
                                }

                                Rectangle {
                                    width: 22
                                    height: 22
                                    radius: 11
                                    color: "#ffffff"
                                    anchors.verticalCenter: parent.verticalCenter
                                    x: Styles.ThemeManager.isDark ? parent.width - width - 2 : 2

                                    Behavior on x {
                                        NumberAnimation {
                                            duration: Styles.ThemeManager.animNormal
                                            easing.type: Easing.OutQuad
                                        }
                                    }
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: Styles.ThemeManager.toggleTheme()
                                }
                            }
                        }
                    }
                }

                // ==================== 录制设置 ====================

                Rectangle {
                    Layout.fillWidth: true
                    height: recordSection.height + Styles.ThemeManager.spacingLg * 2
                    radius: Styles.ThemeManager.radiusLg
                    color: Styles.ThemeManager.bgCard
                    border.width: 1
                    border.color: Styles.ThemeManager.border

                    ColumnLayout {
                        id: recordSection
                        anchors.fill: parent
                        anchors.margins: Styles.ThemeManager.spacingLg
                        spacing: Styles.ThemeManager.spacingMd

                        Text {
                            text: "🎥 录制"
                            color: Styles.ThemeManager.textPrimary
                            font.pixelSize: Styles.ThemeManager.fontSizeH3
                            font.weight: Font.Medium
                        }

                        // 自动保存开关
                        RowLayout {
                            Layout.fillWidth: true

                            ColumnLayout {
                                spacing: 2

                                Text {
                                    text: "自动保存录制"
                                    color: Styles.ThemeManager.textPrimary
                                    font.pixelSize: Styles.ThemeManager.fontSizeBody
                                }

                                Text {
                                    text: "录制结束后自动保存到本地"
                                    color: Styles.ThemeManager.textSecondary
                                    font.pixelSize: Styles.ThemeManager.fontSizeSmall
                                }
                            }

                            Item {
                                Layout.fillWidth: true
                            }

                            Rectangle {
                                width: 50
                                height: 26
                                radius: 13
                                color: root.autoSave ? Styles.ThemeManager.primary : Styles.ThemeManager.bgTertiary

                                Behavior on color {
                                    ColorAnimation {
                                        duration: Styles.ThemeManager.animNormal
                                    }
                                }

                                Rectangle {
                                    width: 22
                                    height: 22
                                    radius: 11
                                    color: "#ffffff"
                                    anchors.verticalCenter: parent.verticalCenter
                                    x: root.autoSave ? parent.width - width - 2 : 2

                                    Behavior on x {
                                        NumberAnimation {
                                            duration: Styles.ThemeManager.animNormal
                                            easing.type: Easing.OutQuad
                                        }
                                    }
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: root.autoSave = !root.autoSave
                                }
                            }
                        }
                    }
                }

                // 底部留白
                Item {
                    Layout.fillWidth: true
                    height: Styles.ThemeManager.spacingXl
                }
            }
        }
    }
}
