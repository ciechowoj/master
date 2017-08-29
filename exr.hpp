#pragma once
#include <glm>
#include <map>
#include <string>
#include <vector>

namespace haste {

using std::map;
using std::size_t;
using std::string;
using std::vector;

void save_exr(
  const string& path,
  const map<string, string>& metadata,
  size_t width,
  size_t height,
  const vec3* data);

void save_exr(
  const string& path,
  const map<string, string>& metadata,
  size_t width,
  size_t height,
  const vector<vec3>& data);

void load_exr(
  const string& path,
  map<string, string>& metadata,
  size_t& width,
  size_t& height,
  vector<vec3>& data);

void load_exr(
  const string& path,
  vector<vec3>& data);

void save_exr(
  const string& path,
  const map<string, string>& metadata,
  size_t width,
  size_t height,
  const vec4* data);

void save_exr(
  const string& path,
  const map<string, string>& metadata,
  size_t width,
  size_t height,
  const vector<vec4>& data);

void load_exr(
  const string& path,
  map<string, string>& metadata,
  size_t& width,
  size_t& height,
  vector<vec4>& data);

void load_exr(
  const string& path,
  vector<vec4>& data);

map<string, string> load_metadata(const string& path);

vec3 exr_average(string path);
void subtract_exr(string dst, string fst, string snd);
void filter_exr(string dst, string src);

string compute_errors(string fst, string snd);
string query_time(string path);

}
