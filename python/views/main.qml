import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

ApplicationWindow {
    id: root
    visible: true
    width: 800
    height: 600
    title: "AI Video Analysis - Hello World Demo"
    
    // 背景渐变
    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#1e1e2e" }
            GradientStop { position: 1.0; color: "#2d2d44" }
        }
    }
    
    ColumnLayout {
        anchors.centerIn: parent
        spacing: 30
        width: parent.width * 0.8
        
        // 标题
        Text {
            text: "🎬 AI Video Analysis System"
            font.pixelSize: 36
            font.bold: true
            color: "#ffffff"
            Layout.alignment: Qt.AlignHCenter
        }
        
        Text {
            text: "Hello World Demo - Python ↔ QML ↔ C++"
            font.pixelSize: 18
            color: "#a0a0a0"
            Layout.alignment: Qt.AlignHCenter
        }
        
        // 输入框
        Rectangle {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 400
            Layout.preferredHeight: 50
            color: "#3d3d5c"
            radius: 10
            border.color: "#5d5d7c"
            border.width: 2
            
            TextField {
                id: nameInput
                anchors.fill: parent
                anchors.margins: 10
                placeholderText: "Enter your name..."
                font.pixelSize: 16
                color: "#ffffff"
                background: Rectangle {
                    color: "transparent"
                }
            }
        }
        
        // 按钮组
        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: 20
            
            // Hello 按钮
            Button {
                text: "🚀 Call C++ Hello"
                font.pixelSize: 16
                font.bold: true
                
                contentItem: Text {
                    text: parent.text
                    font: parent.font
                    color: parent.down ? "#ffffff" : "#e0e0e0"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                
                background: Rectangle {
                    implicitWidth: 200
                    implicitHeight: 50
                    color: parent.down ? "#4a9eff" : "#3a8eef"
                    radius: 10
                    border.color: "#5aaeef"
                    border.width: 2
                    
                    // 悬停效果
                    Behavior on color {
                        ColorAnimation { duration: 200 }
                    }
                }
                
                onClicked: {
                    var name = nameInput.text || "World"
                    mainViewModel.testCppModule(name)
                }
            }
            
            // VideoProcessor 测试按钮
            Button {
                text: "🎥 Test VideoProcessor"
                font.pixelSize: 16
                font.bold: true
                
                contentItem: Text {
                    text: parent.text
                    font: parent.font
                    color: parent.down ? "#ffffff" : "#e0e0e0"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                
                background: Rectangle {
                    implicitWidth: 200
                    implicitHeight: 50
                    color: parent.down ? "#6a4aef" : "#5a3adf"
                    radius: 10
                    border.color: "#7a5aef"
                    border.width: 2
                    
                    Behavior on color {
                        ColorAnimation { duration: 200 }
                    }
                }
                
                onClicked: {
                    mainViewModel.testVideoProcessor()
                }
            }
        }
        
        // 消息显示区域
        Rectangle {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 600
            Layout.preferredHeight: 200
            color: "#2d2d44"
            radius: 15
            border.color: "#4d4d64"
            border.width: 2
            
            ScrollView {
                anchors.fill: parent
                anchors.margins: 15
                
                Text {
                    id: messageText
                    text: mainViewModel.statusMessage
                    font.pixelSize: 14
                    font.family: "Consolas"
                    color: "#00ff00"
                    wrapMode: Text.WordWrap
                    width: parent.width
                    
                    // 消息更新动画
                    Behavior on text {
                        SequentialAnimation {
                            NumberAnimation {
                                target: messageText
                                property: "opacity"
                                to: 0.3
                                duration: 100
                            }
                            NumberAnimation {
                                target: messageText
                                property: "opacity"
                                to: 1.0
                                duration: 200
                            }
                        }
                    }
                }
            }
        }
        
        // 底部信息
        Text {
            text: "💡 Tip: This demo shows Python calling C++ extension module via pybind11"
            font.pixelSize: 12
            color: "#808080"
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: 20
        }
    }
    
    // 连接信号
    Connections {
        target: mainViewModel
        function onStatusMessageChanged() {
            console.log("[QML] Status message updated:", mainViewModel.statusMessage)
        }
    }
}
