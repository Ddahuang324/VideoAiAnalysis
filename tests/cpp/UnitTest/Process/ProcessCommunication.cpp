#include <gtest/gtest.h>

#include <chrono>
#include <memory>
#include <string>
#include <thread>

#include "Process/IPC/AnalyzerProcessClient.h"
#include "Process/IPC/CommandProtocol.h"
#include "Process/IPC/IPCServer.h"
#include "nlohmann/json_fwd.hpp"

using namespace IPC;

class ProcessCommunicationTest : public ::testing::Test {
protected:
    const std::string test_endpoint = "tcp://127.0.0.1:5555";
    std::unique_ptr<IPCServer> server;
    std::unique_ptr<AnalyzerProcessClient> client;

    void SetUp() override {
        server = std::make_unique<IPCServer>(test_endpoint);
        client = std::make_unique<AnalyzerProcessClient>(test_endpoint);
    }

    void TearDown() override {
        if (client) {
            client->disconnect();
        }
        if (server) {
            server->stop();
        }
        // Wait a bit for sockets to clean up
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
};

TEST_F(ProcessCommunicationTest, ServerStartAndStop) {
    EXPECT_TRUE(server->start());
    EXPECT_TRUE(server->isRunning());
    server->stop();
    EXPECT_FALSE(server->isRunning());
}

TEST_F(ProcessCommunicationTest, ClientConnect) {
    EXPECT_TRUE(server->start());
    EXPECT_TRUE(client->connect());
    EXPECT_TRUE(client->isConnected());
}

TEST_F(ProcessCommunicationTest, BasicCommandExchange) {
    server->registerHandler(CommandType::PING, [](const CommandRequest& req) {
        CommandResponse resp;
        resp.code = ResponseCode::SUCCESS;
        resp.message = "PONG";
        resp.data = "Pong Data";
        return resp;
    });

    ASSERT_TRUE(server->start());
    ASSERT_TRUE(client->connect());

    CommandRequest req;
    req.command = CommandType::PING;
    auto response = client->sendCommand(req);
    EXPECT_EQ(response.code, ResponseCode::SUCCESS);
    EXPECT_EQ(response.message, "PONG");
    EXPECT_EQ(response.data.get<std::string>(), "Pong Data");
}

TEST_F(ProcessCommunicationTest, CommandWithParameters) {
    server->registerHandler(CommandType::ANALYZER_CONFIG_SET, [](const CommandRequest& req) {
        CommandResponse resp;
        if (req.parameters.contains("bitrate") && req.parameters["bitrate"] == 5000) {
            resp.code = ResponseCode::SUCCESS;
            resp.message = "Config applied";
        } else {
            resp.code = ResponseCode::ERROR_INVALID_PRMS;
            resp.message = "Invalid bitrate";
        }
        return resp;
    });

    ASSERT_TRUE(server->start());
    ASSERT_TRUE(client->connect());

    nlohmann::json config;
    config["bitrate"] = 5000;
    auto response = client->setConfig(config);
    EXPECT_EQ(response.code, ResponseCode::SUCCESS);
    EXPECT_EQ(response.message, "Config applied");

    nlohmann::json bad_config;
    bad_config["bitrate"] = 1000;
    response = client->setConfig(bad_config);
    EXPECT_EQ(response.code, ResponseCode::ERROR_INVALID_PRMS);
}

TEST_F(ProcessCommunicationTest, UnhandledCommand) {
    // No handler registered for START_ANALYZER
    ASSERT_TRUE(server->start());
    ASSERT_TRUE(client->connect());

    auto response = client->startAnalyzer();
    EXPECT_EQ(response.code, ResponseCode::ERROR_INVALID_CMD);
}

TEST_F(ProcessCommunicationTest, ClientTimeout) {
    server->registerHandler(CommandType::GET_STATUS, [](const CommandRequest& req) {
        // Simulate a long processing time
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        CommandResponse resp;
        resp.code = ResponseCode::SUCCESS;
        return resp;
    });

    ASSERT_TRUE(server->start());
    ASSERT_TRUE(client->connect());  // connect normally

    CommandRequest req;
    req.command = CommandType::GET_STATUS;
    auto response = client->sendCommand(req, 500);  // short timeout
    EXPECT_EQ(response.code, ResponseCode::ERROR_INTERNAL);
    EXPECT_NE(response.message.find("Timeout"), std::string::npos);
}
