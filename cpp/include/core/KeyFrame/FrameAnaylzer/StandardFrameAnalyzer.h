#pragma once
#include <memory>
#include <opencv2/core/mat.hpp>
#include <string>

#include "IFrameAnalyzer.h"
#include "MotionDetector.h"
#include "SceneChangeDetector.h"
#include "TextDetector.h"

namespace KeyFrame {

class StandardFrameAnalyzer : public IFrameAnalyzer {
public:
    StandardFrameAnalyzer(std::shared_ptr<SceneChangeDetector> sceneDetector,
                          std::shared_ptr<MotionDetector> motionDetector,
                          std::shared_ptr<TextDetector> textDetector);

    MultiDimensionScore analyzeFrame(std::shared_ptr<FrameResource> resource,
                                     const AnalysisContext& context) override;

    float getBaseWeight() const override;
    std::string getName() const override;
    void reset() override;

private:
    std::shared_ptr<SceneChangeDetector> sceneDetector_;
    std::shared_ptr<MotionDetector> motionDetector_;
    std::shared_ptr<TextDetector> textDetector_;
};

}  // namespace KeyFrame
