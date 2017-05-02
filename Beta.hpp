#pragma once
#include <string>
#include <glm>

namespace haste {

template <int C> class FixedBeta {
public:
    float beta(float x);
    float beta_exp() const;
    std::string name() const;
};

class VariableBeta {
public:
    float beta(float x);
    float beta_exp() const;
    std::string name() const;
    void init(float beta);
private:
    float _beta = 1.0f;
};

template <> inline float FixedBeta<0>::beta(float x) {
    return x == 0.0f ? 0.0f : 1.0f;
}

template <> inline float FixedBeta<1>::beta(float x) {
    return x;
}

template <> inline float FixedBeta<2>::beta(float x) {
    return x * x;
}

template <int C> inline float FixedBeta<C>::beta_exp() const {
    return float(C);
}

inline float VariableBeta::beta(float x) {
    return pow(x, _beta);
}

inline float VariableBeta::beta_exp() const {
    return _beta;
}

}
