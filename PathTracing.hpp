#pragma once
#include <Technique.hpp>

namespace haste {

class PathTracing : public Technique {
public:
    PathTracing();

    void updateInteractive(double timeQuantum) override;

    string stageName() const override;
    double stageProgress() const override;
private:
    double _progress = 0.0;
};

}
