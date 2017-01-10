#include <Beta.hpp>
#include <sstream>

namespace haste {

using namespace std;
using namespace glm;

template <int C> float FixedBeta<C>::beta(float x) const {
    return pow(x, float(C));
}

template <int C> string FixedBeta<C>::name() const {
    std::stringstream stream;
    stream << u8"Bidirectional Path Tracing (β = " << C << ")";
    return stream.str();
}

string VariableBeta::name() const {
    std::stringstream stream;
    stream << u8"Bidirectional Path Tracing (β = " << _beta << ")";
    return stream.str();
}

void VariableBeta::init(float beta) {
    _beta = beta;
}

template class FixedBeta<0>;
template class FixedBeta<1>;
template class FixedBeta<2>;

}
