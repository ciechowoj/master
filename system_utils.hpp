#pragma once
#include <string>
#include <vector>

namespace haste {

using std::vector;
using std::string;
using std::pair;

string homePath();
string baseName(string path);
string fixedPath(string base, string scene, std::size_t samples);
pair<string, string> splitext(string path);
size_t getmtime(const string& path);

}
