// SettingsPage.qml - 设置页面
// 管理 API Key、主题和其他配置

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../styles" as Styles

Rectangle {
    id: root
    anchors.fill: parent
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
            text: "Settings"
            color: Styles.ThemeManager.textPrimary
            font.pixelSize: Styles.ThemeManager.fontSizeH2
            font.weight: Font.ExtraBold
            font.family: Styles.ThemeManager.fontFamily
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
                    Layout.preferredHeight: apiSection.implicitHeight + Styles.ThemeManager.spacingLg * 2
                    radius: Styles.ThemeManager.radiusLg
                    color: Styles.ThemeManager.bgCard
                    border.width: 1
                    border.color: Styles.ThemeManager.border

                    ColumnLayout {
                        id: apiSection
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.margins: Styles.ThemeManager.spacingLg
                        spacing: Styles.ThemeManager.spacingMd

                        Text {
                            text: "API Configuration"
                            color: Styles.ThemeManager.textPrimary
                            font.pixelSize: Styles.ThemeManager.fontSizeH3
                            font.weight: Font.Bold
                            font.family: Styles.ThemeManager.fontFamily
                        }

                        // API Key 输入
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: Styles.ThemeManager.spacingXs

                            Text {
                                text: "Google Gemini API Key"
                                color: Styles.ThemeManager.textSecondary
                                font.pixelSize: Styles.ThemeManager.fontSizeSmall
                                font.family: Styles.ThemeManager.fontFamily
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
                                    font.family: Styles.ThemeManager.fontFamily
                                    echoMode: TextInput.Password
                                    verticalAlignment: TextInput.AlignVCenter
                                    text: root.apiKey
                                    onTextChanged: root.apiKey = text

                                    Text {
                                        anchors.fill: parent
                                        anchors.leftMargin: 0
                                        verticalAlignment: Text.AlignVCenter
                                        text: "Enter your API Key..."
                                        color: Styles.ThemeManager.textMuted
                                        font.pixelSize: Styles.ThemeManager.fontSizeBody
                                        font.family: Styles.ThemeManager.fontFamily
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
                    Layout.preferredHeight: themeSection.implicitHeight + Styles.ThemeManager.spacingLg * 2
                    radius: Styles.ThemeManager.radiusLg
                    color: Styles.ThemeManager.bgCard
                    border.width: 1
                    border.color: Styles.ThemeManager.border

                    ColumnLayout {
                        id: themeSection
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.margins: Styles.ThemeManager.spacingLg
                        spacing: Styles.ThemeManager.spacingMd

                        Text {
                            text: "Appearance"
                            color: Styles.ThemeManager.textPrimary
                            font.pixelSize: Styles.ThemeManager.fontSizeH3
                            font.weight: Font.Bold
                            font.family: Styles.ThemeManager.fontFamily
                        }

                        // 主题切换
                        RowLayout {
                            Layout.fillWidth: true

                            Text {
                                text: "Dark Mode"
                                color: Styles.ThemeManager.textPrimary
                                font.pixelSize: Styles.ThemeManager.fontSizeBody
                                font.family: Styles.ThemeManager.fontFamily
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
                    Layout.preferredHeight: recordSection.implicitHeight + Styles.ThemeManager.spacingLg * 2
                    radius: Styles.ThemeManager.radiusLg
                    color: Styles.ThemeManager.bgCard
                    border.width: 1
                    border.color: Styles.ThemeManager.border

                    ColumnLayout {
                        id: recordSection
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.margins: Styles.ThemeManager.spacingLg
                        spacing: Styles.ThemeManager.spacingMd

                        Text {
                            text: "Recording"
                            color: Styles.ThemeManager.textPrimary
                            font.pixelSize: Styles.ThemeManager.fontSizeH3
                            font.weight: Font.Bold
                            font.family: Styles.ThemeManager.fontFamily
                        }

                        // 自动保存开关
                        RowLayout {
                            Layout.fillWidth: true

                            ColumnLayout {
                                spacing: 2

                                Text {
                                    text: "Auto-Save Recordings"
                                    color: Styles.ThemeManager.textPrimary
                                    font.pixelSize: Styles.ThemeManager.fontSizeBody
                                    font.family: Styles.ThemeManager.fontFamily
                                }

                                Text {
                                    text: "Automatically save files when recording stops"
                                    color: Styles.ThemeManager.textSecondary
                                    font.pixelSize: Styles.ThemeManager.fontSizeSmall
                                    font.family: Styles.ThemeManager.fontFamily
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
