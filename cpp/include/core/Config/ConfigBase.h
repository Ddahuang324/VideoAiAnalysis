#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>

#include "Log.h"

namespace Config {

// 配置验证结果
struct ValidationResult {
    bool isValid = true;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;

    void addError(const std::string& error) {
        isValid = false;
        errors.push_back(error);
    }

    void addWarning(const std::string& warning) {
        warnings.push_back(warning);
    }

    std::string toString() const {
        std::string result;
        if (!errors.empty()) {
            result += "Errors:\n";
            for (const auto& err : errors) {
                result += "  - " + err + "\n";
            }
        }
        if (!warnings.empty()) {
            result += "Warnings:\n";
            for (const auto& warn : warnings) {
                result += "  - " + warn + "\n";
            }
        }
        if (errors.empty() && warnings.empty()) {
            result = "No issues found.";
        }
        return result;
    }

    bool hasWarnings() const { return !warnings.empty(); }
    bool hasErrors() const { return !errors.empty(); }
};

// 配置基类接口
class IConfigBase {
public:
    virtual ~IConfigBase() = default;

    // 从 JSON 加载配置
    virtual void fromJson(const nlohmann::json& j) = 0;

    // 转换为 JSON
    virtual nlohmann::json toJson() const = 0;

    // 验证配置有效性
    virtual ValidationResult validate() const = 0;

    // 从文件加载
    virtual bool loadFromFile(const std::string& filepath);

    // 保存到文件
    virtual bool saveToFile(const std::string& filepath) const;

    // 合并配置 (用于配置继承)
    virtual void merge(const IConfigBase& other) = 0;

    // 获取配置名称 (用于日志)
    virtual std::string getConfigName() const = 0;
};

// CRTP 基类,提供默认实现
template<typename Derived>
class ConfigBase : public IConfigBase {
public:
    bool loadFromFile(const std::string& filepath) override {
        try {
            std::ifstream file(filepath);
            if (!file.is_open()) {
                LOG_ERROR("Failed to open config file: " + filepath);
                return false;
            }

            nlohmann::json j;
            file >> j;

            static_cast<Derived*>(this)->fromJson(j);

            // 验证配置
            auto result = validate();
            if (!result.isValid) {
                LOG_ERROR("Config validation failed for " + getConfigName() + ":\n" + result.toString());
                return false;
            }

            if (!result.warnings.empty()) {
                LOG_WARNING("Config warnings for " + getConfigName() + ":\n" + result.toString());
            }

            LOG_INFO("Config loaded successfully from " + filepath);
            return true;

        } catch (const std::exception& e) {
            LOG_ERROR("Exception loading config: " + std::string(e.what()));
            return false;
        }
    }

    bool saveToFile(const std::string& filepath) const override {
        try {
            nlohmann::json j = toJson();
            std::ofstream file(filepath);
            if (!file.is_open()) {
                LOG_ERROR("Failed to open file for writing: " + filepath);
                return false;
            }
            file << j.dump(4);  // 4 空格缩进
            LOG_INFO("Config saved to " + filepath);
            return true;
        } catch (const std::exception& e) {
            LOG_ERROR("Exception saving config: " + std::string(e.what()));
            return false;
        }
    }
};

}  // namespace Config
