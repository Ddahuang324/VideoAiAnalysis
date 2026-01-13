#pragma once

#include "ConfigBase.h"
#include <filesystem>
#include <cmath>
#include <sstream>

namespace Config {

// 验证辅助宏
#define VALIDATE_RANGE(result, value, min, max, name) \
    do { \
        if ((value) < (min) || (value) > (max)) { \
            std::ostringstream oss; \
            oss << (name) << " must be in range [" << (min) << ", " << (max) << "], got " << (value); \
            result.addError(oss.str()); \
        } \
    } while(0)

#define VALIDATE_POSITIVE(result, value, name) \
    do { \
        if ((value) <= 0) { \
            std::ostringstream oss; \
            oss << (name) << " must be positive, got " << (value); \
            result.addError(oss.str()); \
        } \
    } while(0)

#define VALIDATE_NON_NEGATIVE(result, value, name) \
    do { \
        if ((value) < 0) { \
            std::ostringstream oss; \
            oss << (name) << " must be non-negative, got " << (value); \
            result.addError(oss.str()); \
        } \
    } while(0)

#define VALIDATE_NOT_EMPTY(result, value, name) \
    do { \
        if ((value).empty()) { \
            result.addError(std::string(name) + " must not be empty"); \
        } \
    } while(0)

#define VALIDATE_FILE_EXISTS(result, path, name) \
    do { \
        if (!std::filesystem::exists(path)) { \
            result.addError(std::string(name) + " file does not exist: " + path); \
        } \
    } while(0)

#define VALIDATE_CONDITION(result, condition, message) \
    do { \
        if (!(condition)) { \
            result.addError(message); \
        } \
    } while(0)

#define WARN_IF(result, condition, message) \
    do { \
        if (condition) { \
            result.addWarning(message); \
        } \
    } while(0)

// 验证权重和是否接近 1.0
inline void ValidateWeightSum(ValidationResult& result, float sum, const std::string& name,
                              float tolerance = 0.01f) {
    if (std::abs(sum - 1.0f) > tolerance) {
        std::ostringstream oss;
        oss << name << " sum to " << sum << ", expected 1.0";
        result.addWarning(oss.str());
    }
}

// 验证容器大小
#define VALIDATE_VECTOR_SIZE(result, vec, expected, name) \
    do { \
        if ((vec).size() != (expected)) { \
            std::ostringstream oss; \
            oss << (name) << " must have exactly " << (expected) << " elements, got " << (vec).size(); \
            result.addError(oss.str()); \
        } \
    } while(0)

#define VALIDATE_VECTOR_MIN_SIZE(result, vec, min, name) \
    do { \
        if ((vec).size() < (min)) { \
            std::ostringstream oss; \
            oss << (name) << " must have at least " << (min) << " elements, got " << (vec).size(); \
            result.addError(oss.str()); \
        } \
    } while(0)

// 验证范围关系
#define VALIDATE_LESS_THAN(result, value, reference, valueName, referenceName) \
    do { \
        if ((value) >= (reference)) { \
            std::ostringstream oss; \
            oss << (valueName) << " must be less than " << (referenceName) \
                << " (" << (value) << " >= " << (reference) << ")"; \
            result.addError(oss.str()); \
        } \
    } while(0)

#define VALIDATE_LESS_THAN_OR_EQUAL(result, value, reference, valueName, referenceName) \
    do { \
        if ((value) > (reference)) { \
            std::ostringstream oss; \
            oss << (valueName) << " must be less than or equal to " << (referenceName) \
                << " (" << (value) << " > " << (reference) << ")"; \
            result.addError(oss.str()); \
        } \
    } while(0)

}  // namespace Config
