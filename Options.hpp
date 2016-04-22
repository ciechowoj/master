#pragma once
#include <string>
#include <initializer_list>

namespace haste {

using std::string;
using std::initializer_list;

struct Options {
    enum Technique { PT, PM };

    string input;
    string output;
    Technique technique = PT;
    size_t numPhotons = 1000000;
    size_t maxGather = 100;
    double maxRadius = 0.1;
    bool batch = false;
    size_t numSamples = 0;
    double numSeconds = 0.0;
    size_t numJobs = 1;
    size_t snapshot = 0;
    size_t cameraId = 0;

    bool displayHelp = false;
    bool displayVersion = false;
    string displayMessage;
};

Options parseArgs(int argc, char const* const* argv);

template <class... T> Options parseArgs2(const T&... argv) {
	char const* const table[] = { argv... };
	return parseArgs(sizeof...(T), table);
}


}
