#include "core/MQInfra/Protocol.h"

#include <cstdint>
#include <cstring>  // for memcpy
#include <optional>
#include <stdexcept>  // for std::runtime_error
#include <vector>
#include <zmq.hpp>

std::vector<uint8_t> Protocol::serializeFrameMessage(const FrameMessage& frame) {
    std::vector<uint8_t> buffer;
    buffer.reserve(sizeof(FrameHeader) + frame.image_data.size() + sizeof(uint32_t));

    const uint8_t* header_ptr = reinterpret_cast<const uint8_t*>(&frame.header);
    buffer.insert(buffer.end(), header_ptr, header_ptr + sizeof(FrameHeader));

    buffer.insert(buffer.end(), frame.image_data.begin(), frame.image_data.end());

    uint32_t crc = calculateCrc32(buffer.data(), buffer.size()) ^ 0xFFFFFFFF;
    const uint8_t* crc_ptr = reinterpret_cast<const uint8_t*>(&crc);
    buffer.insert(buffer.end(), crc_ptr, crc_ptr + sizeof(uint32_t));

    return buffer;
}

Protocol::FrameMessage Protocol::deserializeFrameMessage(const std::vector<uint8_t>& buffer) {
    if (buffer.size() < sizeof(FrameHeader) + sizeof(uint32_t)) {
        throw std::runtime_error("Buffer too small for FrameMessage");
    }

    FrameMessage frame;
    std::memcpy(&frame.header, buffer.data(), sizeof(FrameHeader));

    size_t offset = sizeof(FrameHeader);
    frame.image_data.assign(buffer.begin() + offset,
                            buffer.begin() + offset + frame.header.data_size);
    offset += frame.header.data_size;

    std::memcpy(&frame.crc32, buffer.data() + offset, sizeof(uint32_t));

    size_t data_size = buffer.size() - sizeof(uint32_t);
    std::vector<uint8_t> data_part(buffer.begin(), buffer.begin() + data_size);
    if (!verifyCrc32(data_part.data(), data_part.size(), frame.crc32)) {
        throw std::runtime_error("CRC32 verification failed");
    }

    return frame;
}

std::vector<uint8_t> Protocol::serializeKeyFrameMetaDataMessage(
    const KeyFrameMetaDataMessage& meta) {
    std::vector<uint8_t> buffer;
    buffer.reserve(sizeof(KeyFrameMetaDataHeader) + sizeof(uint32_t));

    auto append = [&](const void* data, size_t size) {
        const uint8_t* ptr = reinterpret_cast<const uint8_t*>(data);
        buffer.insert(buffer.end(), ptr, ptr + size);
    };

    // 顺序序列化所有 Header 字段
    append(&meta.header.magic_num, sizeof(meta.header.magic_num));
    append(&meta.header.version, sizeof(meta.header.version));
    append(&meta.header.message_type, sizeof(meta.header.message_type));
    append(&meta.header.frameID, sizeof(meta.header.frameID));
    append(&meta.header.timestamp, sizeof(meta.header.timestamp));
    append(&meta.header.Final_Score, sizeof(meta.header.Final_Score));
    append(&meta.header.Sence_Score, sizeof(meta.header.Sence_Score));
    append(&meta.header.Motion_Score, sizeof(meta.header.Motion_Score));
    append(&meta.header.Text_Score, sizeof(meta.header.Text_Score));
    append(&meta.header.is_Scene_Change, sizeof(meta.header.is_Scene_Change));

    // 计算并追加 CRC32
    uint32_t crc = calculateCrc32(buffer.data(), buffer.size()) ^ 0xFFFFFFFF;
    append(&crc, sizeof(crc));

    return buffer;
}

Protocol::KeyFrameMetaDataMessage Protocol::deserializeKeyFrameMetaDataMessage(
    const std::vector<uint8_t>& buffer) {
    if (buffer.size() < sizeof(KeyFrameMetaDataHeader) + sizeof(uint32_t)) {
        throw std::runtime_error("Buffer too small for KeyFrameMetaDataMessage");
    }

    KeyFrameMetaDataMessage meta;
    size_t offset = 0;
    auto read = [&](void* dst, size_t size) {
        std::memcpy(dst, buffer.data() + offset, size);
        offset += size;
    };

    read(&meta.header.magic_num, sizeof(meta.header.magic_num));
    read(&meta.header.version, sizeof(meta.header.version));
    read(&meta.header.message_type, sizeof(meta.header.message_type));
    read(&meta.header.frameID, sizeof(meta.header.frameID));
    read(&meta.header.timestamp, sizeof(meta.header.timestamp));
    read(&meta.header.Final_Score, sizeof(meta.header.Final_Score));
    read(&meta.header.Sence_Score, sizeof(meta.header.Sence_Score));
    read(&meta.header.Motion_Score, sizeof(meta.header.Motion_Score));
    read(&meta.header.Text_Score, sizeof(meta.header.Text_Score));
    read(&meta.header.is_Scene_Change, sizeof(meta.header.is_Scene_Change));
    read(&meta.crc32, sizeof(meta.crc32));

    // 验证 CRC32
    size_t data_size = buffer.size() - sizeof(uint32_t);
    std::vector<uint8_t> data_part(buffer.begin(), buffer.begin() + data_size);
    if (!verifyCrc32(data_part.data(), data_part.size(), meta.crc32)) {
        throw std::runtime_error("CRC32 verification failed for KeyFrameMetaDataMessage");
    }

    return meta;
}

bool Protocol::sendFrameMessageZeroCopy(zmq::socket_t& socket, const FrameMessage& frame) {
    return sendFrameRawZeroCopy(socket, frame.header, frame.image_data.data(),
                                frame.image_data.size(), frame.crc32);
}

bool Protocol::sendFrameRawZeroCopy(zmq::socket_t& socket, const FrameHeader& header,
                                    const void* data, size_t data_size, uint32_t crc) {
    try {
        // Send Header
        socket.send(zmq::buffer(&header, sizeof(header)), zmq::send_flags::sndmore);

        // Send Image Data (Zero Copy)
        // 注意：调用者必须确保 data 在发送完成前有效
        zmq::message_t img_msg(const_cast<void*>(data), data_size, nullptr, nullptr);
        socket.send(img_msg, zmq::send_flags::sndmore);

        // Send CRC
        socket.send(zmq::buffer(&crc, sizeof(crc)), zmq::send_flags::none);

        return true;
    } catch (const zmq::error_t& e) {
        return false;
    }
}

std::optional<Protocol::FrameMessage> Protocol::receiveFrameMessageZeroCopy(zmq::socket_t& socket,
                                                                            int timeout_ms) {
    socket.set(zmq::sockopt::rcvtimeo, timeout_ms);

    FrameMessage frame;
    zmq::message_t header_msg, data_msg, crc_msg;

    try {
        auto res1 = socket.recv(header_msg, zmq::recv_flags::none);
        if (!res1 || header_msg.size() != sizeof(FrameHeader))
            return std::nullopt;

        std::memcpy(&frame.header, header_msg.data(), sizeof(FrameHeader));

        auto res2 = socket.recv(data_msg, zmq::recv_flags::none);
        if (!res2)
            return std::nullopt;

        frame.image_data.assign(data_msg.data<uint8_t>(),
                                data_msg.data<uint8_t>() + data_msg.size());

        auto res3 = socket.recv(crc_msg, zmq::recv_flags::none);
        if (!res3 || crc_msg.size() != sizeof(uint32_t))
            return std::nullopt;
        std::memcpy(&frame.crc32, crc_msg.data(), sizeof(uint32_t));

        uint32_t computed_crc = calculateCrc32(&frame.header, sizeof(frame.header));
        computed_crc =
            calculateCrc32(frame.image_data.data(), frame.image_data.size(), computed_crc);

        if (!verifyCrc32(nullptr, 0, frame.crc32, computed_crc)) {
            return std::nullopt;
        }

    } catch (const zmq::error_t& e) {
        return std::nullopt;
    }

    return frame;
}

uint32_t Protocol::calculateCrc32(const void* data, size_t size, uint32_t initial_crc) {
    uint32_t crc = initial_crc;
    const uint8_t* p = static_cast<const uint8_t*>(data);
    for (size_t i = 0; i < size; ++i) {
        crc ^= p[i];
        for (int j = 0; j < 8; ++j) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0xEDB88320;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

bool Protocol::verifyCrc32(const void* data, size_t size, uint32_t expected_crc,
                           uint32_t initial_crc) {
    return (calculateCrc32(data, size, initial_crc) ^ 0xFFFFFFFF) == expected_crc;
}
