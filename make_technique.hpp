#pragma once

#include <statistics.hpp>
#include <Options.hpp>

namespace haste {

template <class T> using shared = std::shared_ptr<T>;

class Technique;
class Scene;

void print_traces_tabular(std::ostream& stream, const map<string, string>& metadata);

shared<Technique> makeTechnique(const shared<const Scene>& scene, Options& options);
shared<Scene> loadScene(const Options& options);

using options_t = Options;

}
