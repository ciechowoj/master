#include <cstring>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <runtime_assert>
#include <sstream>
#include <stdexcept>
#include <utility.hpp>
#include <ostream>

#include <system_utils.hpp>

namespace haste {

using namespace std;

string fixedPath(string base, string scene, std::size_t samples) {
  string ext;
  std::tie(base, ext) = splitext(base);

  if (ext.empty()) {
    ext = ".exr";
  }

  string sceneBase, sceneExt;
  tie(sceneBase, sceneExt) = splitext(scene);

  std::stringstream result;

  if (!base.empty() && base[base.size() - 1] != '/') {
    result << base << "." << baseName(sceneBase) << "." << samples << ext;
  }
  else {
    result << base << baseName(sceneBase) << "." << samples << ext;
  }

  return result.str();
}

pair<string, string> splitext(string path) {
  size_t index = path.find_last_of(".");

  if (index == string::npos || index == 0) {
    return make_pair(path, string());
  }
  else {
    return make_pair(path.substr(0, index), path.substr(index, path.size()));
  }
}

string make_output_path(
  string input,
  size_t width,
  size_t height,
  size_t num_samples,
  bool snapshot,
  string technique) {
  auto split = splitext(input);

  stringstream stream;

  stream << split.first << "." << width << "." << height << "."
         << num_samples << "." << technique
         << (snapshot ? ".snapshot" : "") << ".exr";

  return fullpath(stream.str());
}

bool startswith(const std::string& s, const std::string& p) {
  const char* s_itr = s.c_str();
  const char* p_itr = p.c_str();

  while (*s_itr != '\0' && *p_itr != '\0' && *s_itr == *p_itr) {
    ++s_itr;
    ++p_itr;
  }

  return *p_itr == '\0';
}

bool endswith(const std::string& s, const std::string& p) {
  const char* s_rbegin = s.c_str() - 1;
  const char* p_rbegin = p.c_str() - 1;
  const char* s_itr = s.c_str() + s.size() - 1;
  const char* p_itr = p.c_str() + p.size() - 1;

  while (s_itr != s_rbegin && p_itr != p_rbegin && *s_itr == *p_itr) {
    --s_itr;
    --p_itr;
  }

  return p_itr == p_rbegin;
}

multimap<string, string> extract_options(int argc, char const* const* argv) {
  multimap<string, string> result;

  for (int i = 0; i < argc; ++i) {
    if (std::strcmp(argv[i], "-h") == 0) {
      result.insert(make_pair<string, string>("--help", ""));
    }
    else if (strstr(argv[i], "-") == argv[i]) {
      const char* eq = strstr(argv[i], "=");

      if (eq == nullptr) {
        result.insert(make_pair<string, string>(argv[i], ""));
      }
      else {
        result.insert(make_pair(string((const char*)argv[i], eq), string(eq + 1)));
      }
    }
    else {
      result.insert(make_pair<string, string>("--input", argv[i]));
    }
  }

  return result;
}

void split(const std::string& s, char delim, std::vector<std::string>& elems) {
  std::stringstream ss;
  ss.str(s);
  std::string item;
  while (std::getline(ss, item, delim)) {
    elems.push_back(item);
  }
}

std::vector<std::string> split(const std::string& s, char delim) {
  std::vector<std::string> elems;
  split(s, delim, elems);
  return elems;
}

std::vector<dvec4> vv3f_to_vv4d(const std::vector<vec3>& data) {
  std::vector<dvec4> result(data.size());

  for (std::size_t i = 0; i < data.size(); ++i) {
    result[i] = dvec4(data[i], 1.0f);
  }

  return result;
}

std::vector<vec3> vv4f_to_vv3f(std::size_t size, const vec4* data) {
  std::vector<vec3> result(size);

  for (std::size_t i = 0; i < size; ++i) {
    result[i] = data[i].rgb() / data[i].a;
  }

  return result;
}

std::vector<vec3> vv4d_to_vv3f(std::size_t size, const dvec4* data) {
  std::vector<vec3> result(size);

  for (std::size_t i = 0; i < size; ++i) {
    result[i] = data[i].rgb() / data[i].a;
  }

  return result;
}

double high_resolution_time() {
  auto duration = std::chrono::high_resolution_clock::now().time_since_epoch();
  return std::chrono::duration_cast<std::chrono::duration<double>>(duration)
      .count();
}
}
