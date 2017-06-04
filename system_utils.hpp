#pragma once
#include <string>
#include <vector>

namespace haste {

using std::vector;
using std::string;
using std::pair;

string homePath();
string baseName(string path);
size_t getmtime(const string& path);

string fullpath(string relative_path);

}
