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

// ========== Constructor ==========

StandardFrameAnalyzer::StandardFrameAnalyzer(std::shared_ptr<SceneChangeDetector> sceneDetector,
                                             std::shared_ptr<MotionDetector> motionDetector,
                                             std::shared_ptr<TextDetector> textDetector)
    : sceneDetector_(std::move(sceneDetector)),
      motionDetector_(std::move(motionDetector)),
      textDetector_(std::move(textDetector)) {}

// ========== Analyzer Interface ==========

std::string StandardFrameAnalyzer::getName() const {
    return "StandardFrameAnalyzer";
}

float StandardFrameAnalyzer::getBaseWeight() const {
    return 1.0f;
}

void StandardFrameAnalyzer::reset() {
    if (sceneDetector_) {
        sceneDetector_->reset();
        LOG_INFO("[StandardFrameAnalyzer] SceneDetector reset");
    }
    if (motionDetector_) {
        motionDetector_->reset();
        LOG_INFO("[StandardFrameAnalyzer] MotionDetector reset");
    }
    if (textDetector_) {
        textDetector_->reset();
        LOG_INFO("[StandardFrameAnalyzer] TextDetector reset");
    }
}

// ========== Frame Analysis ==========

MultiDimensionScore StandardFrameAnalyzer::analyzeFrame(std::shared_ptr<FrameResource> resource,
                                                        const AnalysisContext& context) {
    MultiDimensionScore scores;

    // Launch three detection tasks in parallel
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

    // Collect results
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
