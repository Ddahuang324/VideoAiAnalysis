#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>
#include <zmq.hpp>

class Protocol {
public:
    static constexpr uint32_t FRAME_MAGIC = 0xABCD1234;
    static constexpr uint32_t METADATA_MAGIC = 0xDCBA4321;

#pragma pack(push, 1)
    struct FrameHeader {
        uint32_t magic_num = FRAME_MAGIC;  // 4 bytes 魔数，用于标识协议
        uint8_t version = 1;               // 1 byte  协议版本号
        uint8_t message_type = 1;          // 1 byte  消息类型
        uint32_t frameID = 0;              // 4 bytes 帧ID，用于标识消息帧
        uint64_t timestamp = 0;            // 8 bytes 时间戳，记录消息创建时间
        uint32_t width = 0;                // 4 bytes 图像宽度
        uint32_t height = 0;               // 4 bytes 图像高度
        uint8_t channels = 0;              // 1 byte  图像通道数
        uint32_t data_size = 0;            // 4 bytes 图像数据大小
    };

    struct KeyFrameMetaDataHeader {
        uint32_t magic_num = METADATA_MAGIC;  // 4 bytes 魔数，用于标识协议
        uint8_t version = 1;                  // 1 byte  协议版本号
        uint8_t message_type = 2;             // 1 byte  消息类型
        uint32_t frameID = 0;                 // 4 bytes 帧ID，对应的原始帧ID
        uint64_t timestamp = 0;               // 8 bytes 时间戳，记录消息创建时间
        float Final_Score = 0.0f;             // 4 bytes 关键帧评分
        float Sence_Score = 0.0f;             // 4 bytes 场景评分
        float Motion_Score = 0.0f;            // 4 bytes 运动评分
        float Text_Score = 0.0f;              // 4 bytes 文字评分
        uint8_t is_Scene_Change = 0;          // 1 byte 是否场景变化标志
    };
#pragma pack(pop)

    struct FrameMessage {
        FrameHeader header;
        std::vector<uint8_t> image_data;  // 可变长图像数据
        uint32_t crc32;                   // 4 bytes CRC32校验码
    };

    struct KeyFrameMetaDataMessage {
        KeyFrameMetaDataHeader header;
        uint32_t crc32;  // 4 bytes CRC32校验码
    };

    static std::vector<uint8_t> serializeFrameMessage(const FrameMessage& frame);
    static FrameMessage deserializeFrameMessage(const std::vector<uint8_t>& buffer);

    static std::vector<uint8_t> serializeKeyFrameMetaDataMessage(
        const KeyFrameMetaDataMessage& meta);
    static KeyFrameMetaDataMessage deserializeKeyFrameMetaDataMessage(
        const std::vector<uint8_t>& buffer);
    static uint32_t calculateCrc32(const void* data, size_t size,
                                   uint32_t initial_crc = 0xFFFFFFFF);
    static bool verifyCrc32(const void* data, size_t size, uint32_t expected_crc,
                            uint32_t initial_crc = 0xFFFFFFFF);

    static bool sendFrameMessageZeroCopy(zmq::socket_t& socket, const FrameMessage& frame);
    static bool sendFrameRawZeroCopy(zmq::socket_t& socket, const FrameHeader& header,
                                     const void* data, size_t data_size, uint32_t crc);
    static std::optional<FrameMessage> receiveFrameMessageZeroCopy(zmq::socket_t& socket,
                                                                   int timeout_ms = 100);
};
