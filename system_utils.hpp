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
string temppath(string extension = "");
vector<string> temppaths(size_t num_paths, string extension = "");
bool isfile(string path);

void move_file(string old_path, string new_path);

}
