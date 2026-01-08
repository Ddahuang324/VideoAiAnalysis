#include "StandardFrameAnalyzer.h"

#include <future>
#include <memory>
#include <opencv2/core/mat.hpp>
#include <string>
#include <utility>

#include "FrameResource.h"
#include "IFrameAnalyzer.h"
#include "Log.h"
#include "MotionDetector.h"
#include "SceneChangeDetector.h"
#include "TextDetector.h"

namespace KeyFrame {

StandardFrameAnalyzer::StandardFrameAnalyzer(std::shared_ptr<SceneChangeDetector> sceneDetector,
                                             std::shared_ptr<MotionDetector> motionDetector,
                                             std::shared_ptr<TextDetector> textDetector)
    : sceneDetector_(std::move(sceneDetector)),
      motionDetector_(std::move(motionDetector)),
      textDetector_(std::move(textDetector)) {}

std::string StandardFrameAnalyzer::getName() const {
    return "StandardFrameAnalyzer";
}

float StandardFrameAnalyzer::getBaseWeight() const {
    return 1.0f;  // 标准分析器的基础权重为1.0
}

void StandardFrameAnalyzer::reset() {
    if (sceneDetector_) {
        sceneDetector_->reset();
        LOG_INFO("ScenceDetector reset Successfully.");
    }
    if (motionDetector_) {
        motionDetector_->reset();
        LOG_INFO("MotionDetector reset Successfully.");
    }
    if (textDetector_) {
        textDetector_->reset();
        LOG_INFO("TextDetector reset Successfully.");
    }
}

MultiDimensionScore StandardFrameAnalyzer::analyzeFrame(std::shared_ptr<FrameResource> resource,
                                                        const AnalysisContext& context) {
    MultiDimensionScore scores;

    // 使用 std::async 并行启动三个检测任务
    auto sceneFuture = std::async(std::launch::async, [&]() {
        if (sceneDetector_) {
            auto result = sceneDetector_->detect(resource);
            float score = result.isSceneChange ? 1.0f : static_cast<float>(result.score);
            return std::make_pair(result, score);
        }
        return std::make_pair(SceneChangeDetector::Result{}, 0.0f);
    });

    auto motionFuture = std::async(std::launch::async, [&]() {
        if (motionDetector_) {
            auto result = motionDetector_->detect(resource);
            float score = static_cast<float>(result.score);
            return std::make_pair(result, score);
        }
        return std::make_pair(MotionDetector::Result{}, 0.0f);
    });

    auto textFuture = std::async(std::launch::async, [&]() {
        if (textDetector_) {
            auto result = textDetector_->detect(resource);
            float score = static_cast<float>(result.score);
            return std::make_pair(result, score);
        }
        return std::make_pair(TextDetector::Result{}, 0.0f);
    });

    // 等待所有任务完成并收集结果
    auto sceneRes = sceneFuture.get();
    scores.sceneChangeResult = sceneRes.first;
    scores.sceneScore = sceneRes.second;

    auto motionRes = motionFuture.get();
    scores.motionResult = motionRes.first;
    scores.motionScore = motionRes.second;

    auto textRes = textFuture.get();
    scores.textResult = textRes.first;
    scores.textScore = textRes.second;

    return scores;
}

}  // namespace KeyFrame