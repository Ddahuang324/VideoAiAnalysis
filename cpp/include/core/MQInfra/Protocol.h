#pragma once

#include <cstdint>
#include <vector>

struct FrameMessage {
    uint32_t magic_num;               // 4 bytes 魔数，用于标识协议
    uint8_t version;                  // 1 byte  协议版本号
    uint8_t message_type;             // 1 byte  消息类型
    uint32_t frameID;                 // 4 bytes 帧ID，用于标识消息帧
    uint64_t timestamp;               // 8 bytes 时间戳，记录消息创建时间
    uint32_t width;                   // 4 bytes 图像宽度
    uint32_t height;                  // 4 bytes 图像高度
    uint8_t channels;                 // 1 byte  图像通道数
    uint32_t data_size;               // 4 bytes 图像数据大小
    std::vector<uint8_t> image_data;  // 可变长图像数据，大小为 data_size
    uint32_t Crc32;                   // 4 bytes CRC32校验码
};

struct KeyFrameMetaDataMessage {
    uint32_t magic_num;       // 4 bytes 魔数，用于标识协议
    uint8_t version;          // 1 byte  协议版本号
    uint8_t message_type;     // 1 byte  消息类型
    uint32_t frameID;         // 4 bytes 帧ID，对应的原始帧ID
    uint64_t timestamp;       // 8 bytes 时间戳，记录消息创建时间
    float Final_Score;        // 4 bytes 关键帧评分
    float Sence_Score;        // 4 bytes 场景评分
    float Motion_Score;       // 4 bytes 运动评分
    float Text_Score;         // 4 bytes 文字评分
    uint8_t is_Scene_Change;  // 1 byte 是否场景变化标志
    uint32_t Crc32;           // 4 bytes CRC32校验码
};