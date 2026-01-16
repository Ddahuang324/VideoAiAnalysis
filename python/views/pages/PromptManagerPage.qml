// PromptManagerPage.qml - 提示词模板管理页面

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import "../styles" as Styles

Rectangle {
    id: root
    anchors.fill: parent
    color: "transparent"

    property var selectedTemplate: null

    RowLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 24

        // ========== 左侧模板列表 ==========
        Rectangle {
            Layout.preferredWidth: 300
            Layout.fillHeight: true
            radius: Styles.ThemeManager.radiusLg
            color: Styles.ThemeManager.bgSurface
            border.width: 1
            border.color: Styles.ThemeManager.border

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 12

                // 标题栏
                RowLayout {
                    Layout.fillWidth: true
                    Text {
                        text: "Prompt Templates"
                        color: Styles.ThemeManager.text
                        font.pixelSize: 16
                        font.weight: Font.Bold
                        font.family: Styles.ThemeManager.fontFamily
                    }
                    Item {
                        Layout.fillWidth: true
                    }
                    Rectangle {
                        width: 32
                        height: 32
                        radius: 8
                        color: addBtnArea.containsMouse ? Styles.ThemeManager.bgInput : "transparent"
                        Text {
                            anchors.centerIn: parent
                            text: "+"
                            color: Styles.ThemeManager.text
                            font.pixelSize: 18
                        }
                        MouseArea {
                            id: addBtnArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                root.selectedTemplate = null;
                                nameField.text = "";
                                descField.text = "";
                                contentField.text = "";
                                categoryCombo.currentIndex = 0;
                            }
                        }
                    }
                }

                // 模板列表
                ListView {
                    id: templateList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    spacing: 8
                    model: promptViewModel ? promptViewModel.templates : []

                    delegate: Rectangle {
                        width: templateList.width
                        height: 64
                        radius: 8
                        color: root.selectedTemplate && root.selectedTemplate.promptId === modelData.promptId ? Styles.ThemeManager.bgInput : (itemArea.containsMouse ? Styles.ThemeManager.overlayLight : "transparent")
                        border.width: modelData.isDefault ? 1 : 0
                        border.color: Styles.ThemeManager.accent

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 12
                            spacing: 4

                            RowLayout {
                                Layout.fillWidth: true
                                Text {
                                    text: modelData.name
                                    color: Styles.ThemeManager.text
                                    font.pixelSize: 13
                                    font.weight: Font.Medium
                                    font.family: Styles.ThemeManager.fontFamily
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }
                                Text {
                                    visible: modelData.isDefault
                                    text: "DEFAULT"
                                    color: Styles.ThemeManager.accent
                                    font.pixelSize: 9
                                    font.weight: Font.Bold
                                    font.family: Styles.ThemeManager.fontFamily
                                }
                            }
                            Text {
                                text: modelData.description || modelData.category
                                color: Styles.ThemeManager.textMuted
                                font.pixelSize: 11
                                font.family: Styles.ThemeManager.fontFamily
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }
                        }

                        MouseArea {
                            id: itemArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                root.selectedTemplate = modelData;
                                nameField.text = modelData.name;
                                descField.text = modelData.description;
                                contentField.text = modelData.content;
                                var idx = categoryCombo.model.indexOf(modelData.category);
                                categoryCombo.currentIndex = idx >= 0 ? idx : 0;
                            }
                        }
                    }
                }
            }
        }

        // ========== 右侧编辑区域 ==========
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: Styles.ThemeManager.radiusLg
            color: Styles.ThemeManager.bgSurface
            border.width: 1
            border.color: Styles.ThemeManager.border

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 24
                spacing: 16

                Text {
                    text: root.selectedTemplate ? "Edit Template" : "New Template"
                    color: Styles.ThemeManager.text
                    font.pixelSize: 18
                    font.weight: Font.Bold
                    font.family: Styles.ThemeManager.fontFamily
                }

                // 名称
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    Text {
                        text: "Name"
                        color: Styles.ThemeManager.textSecondary
                        font.pixelSize: 12
                        font.family: Styles.ThemeManager.fontFamily
                    }
                    TextField {
                        id: nameField
                        Layout.fillWidth: true
                        placeholderText: "Template name"
                        color: Styles.ThemeManager.text
                        font.family: Styles.ThemeManager.fontFamily
                        renderType: Text.NativeRendering
                        background: Rectangle {
                            radius: 8
                            color: Styles.ThemeManager.bgInput
                            border.width: 1
                            border.color: Styles.ThemeManager.border
                        }
                    }
                }

                // 分类
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    Text {
                        text: "Category"
                        color: Styles.ThemeManager.textSecondary
                        font.pixelSize: 12
                        font.family: Styles.ThemeManager.fontFamily
                    }
                    ComboBox {
                        id: categoryCombo
                        Layout.fillWidth: true
                        model: ["general", "algorithm", "knowledge", "code", "technical"]
                        font.family: Styles.ThemeManager.fontFamily
                        background: Rectangle {
                            radius: 8
                            color: Styles.ThemeManager.bgInput
                            border.width: 1
                            border.color: Styles.ThemeManager.border
                        }
                        contentItem: Text {
                            text: categoryCombo.displayText
                            color: Styles.ThemeManager.text
                            leftPadding: 12
                            verticalAlignment: Text.AlignVCenter
                            font.family: Styles.ThemeManager.fontFamily
                        }
                    }
                }

                // 描述
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    Text {
                        text: "Description"
                        color: Styles.ThemeManager.textSecondary
                        font.pixelSize: 12
                        font.family: Styles.ThemeManager.fontFamily
                    }
                    TextField {
                        id: descField
                        Layout.fillWidth: true
                        placeholderText: "Brief description"
                        color: Styles.ThemeManager.text
                        font.family: Styles.ThemeManager.fontFamily
                        renderType: Text.NativeRendering
                        background: Rectangle {
                            radius: 8
                            color: Styles.ThemeManager.bgInput
                            border.width: 1
                            border.color: Styles.ThemeManager.border
                        }
                    }
                }

                // 内容
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 6
                    Text {
                        text: "Prompt Content"
                        color: Styles.ThemeManager.textSecondary
                        font.pixelSize: 12
                        font.family: Styles.ThemeManager.fontFamily
                    }
                    ScrollView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        TextArea {
                            id: contentField
                            placeholderText: "Enter your prompt template here..."
                            color: Styles.ThemeManager.text
                            font.family: Styles.ThemeManager.fontFamily
                            renderType: Text.NativeRendering
                            wrapMode: TextArea.Wrap
                            background: Rectangle {
                                radius: 8
                                color: Styles.ThemeManager.bgInput
                                border.width: 1
                                border.color: Styles.ThemeManager.border
                            }
                        }
                    }
                }

                // 操作按钮
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    Rectangle {
                        visible: root.selectedTemplate !== null
                        width: 100
                        height: 36
                        radius: 8
                        color: setDefaultArea.containsMouse ? Styles.ThemeManager.overlayLight : "transparent"
                        border.width: 1
                        border.color: Styles.ThemeManager.border
                        Text {
                            anchors.centerIn: parent
                            text: "Set Default"
                            color: Styles.ThemeManager.textSecondary
                            font.pixelSize: 12
                        }
                        MouseArea {
                            id: setDefaultArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                if (root.selectedTemplate && promptViewModel) {
                                    promptViewModel.setAsDefault(root.selectedTemplate.promptId);
                                }
                            }
                        }
                    }

                    Rectangle {
                        visible: root.selectedTemplate !== null
                        width: 80
                        height: 36
                        radius: 8
                        color: deleteArea.containsMouse ? "#ef4444" : "transparent"
                        border.width: 1
                        border.color: deleteArea.containsMouse ? "#ef4444" : Styles.ThemeManager.border
                        Text {
                            anchors.centerIn: parent
                            text: "Delete"
                            color: deleteArea.containsMouse ? "white" : Styles.ThemeManager.textSecondary
                            font.pixelSize: 12
                        }
                        MouseArea {
                            id: deleteArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                if (root.selectedTemplate && promptViewModel) {
                                    promptViewModel.deleteTemplate(root.selectedTemplate.promptId);
                                    root.selectedTemplate = null;
                                }
                            }
                        }
                    }

                    Item {
                        Layout.fillWidth: true
                    }

                    Rectangle {
                        width: 80
                        height: 36
                        radius: 8
                        color: saveArea.containsMouse ? Styles.ThemeManager.primary : Styles.ThemeManager.accent
                        Text {
                            anchors.centerIn: parent
                            text: "Save"
                            color: "white"
                            font.pixelSize: 12
                            font.weight: Font.Medium
                        }
                        MouseArea {
                            id: saveArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                if (!promptViewModel)
                                    return;
                                if (root.selectedTemplate) {
                                    promptViewModel.updateTemplate(root.selectedTemplate.promptId, nameField.text, contentField.text, descField.text, categoryCombo.currentText);
                                } else {
                                    promptViewModel.createTemplate(nameField.text, contentField.text, descField.text, categoryCombo.currentText);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    Component.onCompleted: {
        if (promptViewModel) {
            promptViewModel.loadTemplates();
        }
    }
}
