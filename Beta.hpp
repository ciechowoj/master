#pragma once
#include <string>
#include <glm>

namespace haste {

template <int C> class FixedBeta {
public:
    float beta(float x);
    std::string name() const;
};

class VariableBeta {
public:
    float beta(float x);
    std::string name() const;
    void init(float beta);
private:
    float _beta = 1.0f;
};

template <> inline float FixedBeta<0>::beta(float x) {
    return 1.0f;
}

template <> inline float FixedBeta<1>::beta(float x) {
    return x;
}

template <> inline float FixedBeta<2>::beta(float x) {
    return x * x;
}

inline float VariableBeta::beta(float x) {
    return pow(x, _beta);
}

}
