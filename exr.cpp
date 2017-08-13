#include <runtime_assert>
#include <cstring>
#include <iomanip>
#include <sstream>

#include <exr.hpp>

#include <ImfArray.h>
#include <ImfChannelList.h>
#include <ImfDoubleAttribute.h>
#include <ImfInputFile.h>
#include <ImfOutputFile.h>
#include <ImfStringAttribute.h>
#include <ImfVecAttribute.h>

namespace haste {

using namespace std;
using namespace Imf;

void save_exr(
  const string& path,
  const map<string, string>& metadata,
  size_t width,
  size_t height,
  const vec3* data)
{
  runtime_assert(width < INT_MAX);
  runtime_assert(height < INT_MAX);

  int iwidth = int(width);
  int iheight = int(height);

  Header header(iwidth, iheight);
  header.channels().insert("R", Channel(Imf::FLOAT));
  header.channels().insert("G", Channel(Imf::FLOAT));
  header.channels().insert("B", Channel(Imf::FLOAT));

  for (auto&& entry : metadata) {
    header.insert(entry.first, StringAttribute(entry.second));
  }

  OutputFile file(path.c_str(), header);

  FrameBuffer framebuffer;

  size_t size = width * height * 3;
  vector<float> data_copy(size);

  for (size_t y = 0; y < height; ++y) {
    for (size_t x = 0; x < width; ++x) {
      data_copy[(y * width + x) * 3 + 0] = data[(height - y - 1) * width + x].x;
      data_copy[(y * width + x) * 3 + 1] = data[(height - y - 1) * width + x].y;
      data_copy[(y * width + x) * 3 + 2] = data[(height - y - 1) * width + x].z;
    }
  }

  auto R = Slice(Imf::FLOAT, (char*)(data_copy.data() + 0), sizeof(float) * 3,
                 sizeof(float) * width * 3);
  auto G = Slice(Imf::FLOAT, (char*)(data_copy.data() + 1), sizeof(float) * 3,
                 sizeof(float) * width * 3);
  auto B = Slice(Imf::FLOAT, (char*)(data_copy.data() + 2), sizeof(float) * 3,
                 sizeof(float) * width * 3);

  framebuffer.insert("R", R);
  framebuffer.insert("G", G);
  framebuffer.insert("B", B);

  file.setFrameBuffer(framebuffer);
  file.writePixels(iheight);
}

void save_exr(
  const string& path,
  const map<string, string>& metadata,
  size_t width,
  size_t height,
  const vector<vec3>& data)
{
  runtime_assert(data.size() == width * height);
  save_exr(path, metadata, width, height, data.data());
}

void load_metadata(const InputFile& file, map<string, string>& metadata) {
  for (auto itr = file.header().begin(); itr != file.header().end(); ++itr) {
    auto typeName = itr.attribute().typeName();

    if (std::strcmp(typeName, "string") == 0) {
      metadata[itr.name()] = static_cast<const StringAttribute&>(itr.attribute()).value();
    }
  }
}

void load_exr(
  const string& path,
  map<string, string>& metadata,
  size_t& width,
  size_t& height,
  vector<vec3>& data)
{
  InputFile file(path.c_str());

  load_metadata(file, metadata);

  auto window = file.header().dataWindow();
  int iwidth = window.max.x - window.min.x + 1;
  int iheight = window.max.y - window.min.y + 1;

  runtime_assert(iwidth >= 0);
  runtime_assert(iheight >= 0);

  width = size_t(iwidth);
  height = size_t(iheight);

  FrameBuffer framebuffer;

  auto hasDenom = file.header().channels().findChannel("denom") != nullptr;

  vector<vec3> data_copy;
  vector<float> denom;

  data_copy.resize(iwidth * iheight);

  if (hasDenom) {
    denom.resize(iwidth * iheight);
  }

  float* fdata = (float*)data_copy.data();

  auto R = Slice(Imf::FLOAT, (char*)(fdata + 0), sizeof(float) * 3,
                 sizeof(float) * width * 3);
  auto G = Slice(Imf::FLOAT, (char*)(fdata + 1), sizeof(float) * 3,
                 sizeof(float) * width * 3);
  auto B = Slice(Imf::FLOAT, (char*)(fdata + 2), sizeof(float) * 3,
                 sizeof(float) * width * 3);

  framebuffer.insert("R", R);
  framebuffer.insert("G", G);
  framebuffer.insert("B", B);

  if (hasDenom) {
    auto D = Slice(Imf::FLOAT, (char*)(denom.data()), sizeof(float),
                 sizeof(float) * width);
    framebuffer.insert("denom", D);
  }

  file.setFrameBuffer(framebuffer);
  file.readPixels(window.min.y, window.max.y);

  data.resize(width * height);

  if (hasDenom) {
    for (size_t i = 0; i < height; ++i) {
      for (size_t j = 0; j < width; ++j) {
        data[i * width + j] = data_copy[(height - i - 1) * width + j] / denom[(height - i - 1) * width + j];
      }
    }
  }
  else {
    for (size_t i = 0; i < height; ++i) {
      for (size_t j = 0; j < width; ++j) {
        data[i * width + j] = data_copy[(height - i - 1) * width + j];
      }
    }
  }
}

void load_exr(
  const string& path,
  vector<vec3>& data) {
  map<string, string> metadata;
  size_t width, height;
  load_exr(path, metadata, width, height, data);
}

void save_exr(
  const string& path,
  const map<string, string>& metadata,
  size_t width,
  size_t height,
  const vec4* data)
{
  runtime_assert(width < INT_MAX);
  runtime_assert(height < INT_MAX);

  int iwidth = int(width);
  int iheight = int(height);

  Header header(iwidth, iheight);
  header.channels().insert("R", Channel(Imf::FLOAT));
  header.channels().insert("G", Channel(Imf::FLOAT));
  header.channels().insert("B", Channel(Imf::FLOAT));
  header.channels().insert("denom", Channel(Imf::FLOAT));

  for (auto&& entry : metadata) {
    header.insert(entry.first, StringAttribute(entry.second));
  }

  OutputFile file(path.c_str(), header);

  FrameBuffer framebuffer;

  size_t size = width * height * 4;
  vector<float> data_copy(size);

  for (size_t y = 0; y < height; ++y) {
    for (size_t x = 0; x < width; ++x) {
      data_copy[(y * width + x) * 4 + 0] = data[(height - y - 1) * width + x].x;
      data_copy[(y * width + x) * 4 + 1] = data[(height - y - 1) * width + x].y;
      data_copy[(y * width + x) * 4 + 2] = data[(height - y - 1) * width + x].z;
      data_copy[(y * width + x) * 4 + 3] = data[(height - y - 1) * width + x].w;
    }
  }

  auto R = Slice(Imf::FLOAT, (char*)(data_copy.data() + 0), sizeof(float) * 4,
                 sizeof(float) * width * 4);
  auto G = Slice(Imf::FLOAT, (char*)(data_copy.data() + 1), sizeof(float) * 4,
                 sizeof(float) * width * 4);
  auto B = Slice(Imf::FLOAT, (char*)(data_copy.data() + 2), sizeof(float) * 4,
                 sizeof(float) * width * 4);
  auto D = Slice(Imf::FLOAT, (char*)(data_copy.data() + 3), sizeof(float) * 4,
                 sizeof(float) * width * 4);

  framebuffer.insert("R", R);
  framebuffer.insert("G", G);
  framebuffer.insert("B", B);
  framebuffer.insert("denom", D);

  file.setFrameBuffer(framebuffer);
  file.writePixels(iheight);
}

void save_exr(
  const string& path,
  const map<string, string>& metadata,
  size_t width,
  size_t height,
  const vector<vec4>& data)
{
  runtime_assert(data.size() == width * height);
  save_exr(path, metadata, width, height, data.data());
}

void load_exr(
  const string& path,
  map<string, string>& metadata,
  size_t& width,
  size_t& height,
  vector<vec4>& data)
{
  InputFile file(path.c_str());

  load_metadata(file, metadata);

  auto window = file.header().dataWindow();
  int iwidth = window.max.x - window.min.x + 1;
  int iheight = window.max.y - window.min.y + 1;

  runtime_assert(iwidth >= 0);
  runtime_assert(iheight >= 0);

  width = size_t(iwidth);
  height = size_t(iheight);

  FrameBuffer framebuffer;

  vector<vec4> data_copy;
  data_copy.resize(width * height);

  float* fdata = (float*)data_copy.data();

  auto R = Slice(Imf::FLOAT, (char*)(fdata + 0), sizeof(float) * 4,
                 sizeof(float) * width * 4);
  auto G = Slice(Imf::FLOAT, (char*)(fdata + 1), sizeof(float) * 4,
                 sizeof(float) * width * 4);
  auto B = Slice(Imf::FLOAT, (char*)(fdata + 2), sizeof(float) * 4,
                 sizeof(float) * width * 4);
  auto D = Slice(Imf::FLOAT, (char*)(fdata + 3), sizeof(float) * 4,
                 sizeof(float) * width * 4);

  framebuffer.insert("R", R);
  framebuffer.insert("G", G);
  framebuffer.insert("B", B);
  framebuffer.insert("denom", D);

  file.setFrameBuffer(framebuffer);
  file.readPixels(window.min.y, window.max.y);

  data.resize(width * height);

  for (size_t i = 0; i < height; ++i) {
    for (size_t j = 0; j < width; ++j) {
      data[i * width + j] = data_copy[(height - i - 1) * width + j];
    }
  }
}

map<string, string> load_metadata(const string& path)
{
  map<string, string> metadata;
  InputFile file(path.c_str());
  load_metadata(file, metadata);
  return metadata;
}

vec3 exr_average(string path) {
  vector<vec3> data;
  load_exr(path, data);

  dvec3 result = dvec3(0.0f);

  for (size_t i = 0; i < data.size(); ++i) {
    result += data[i];
  }

  return result / double(data.size());
}

void subtract_exr(string dst, string fst, string snd) {
  vector<vec4> dst_data, fst_data, snd_data;
  map<string, string> fst_metadata, snd_metadata;

  size_t fst_width = 0, snd_width = 0, fst_height = 0, snd_height = 0;

  load_exr(fst, fst_metadata, fst_width, fst_height, fst_data);
  load_exr(snd, snd_metadata, snd_width, snd_height, snd_data);

  if (fst_width != snd_width || fst_height != snd_height) {
    throw std::runtime_error("Sizes of '" + fst + "' and '" + snd +
                             "' doesn't match.");
  }

  dst_data.resize(fst_data.size());

  for (size_t i = 0; i < fst_data.size(); ++i) {
    dst_data[i] = fst_data[i] - snd_data[i];
  }

  map<string, string> dst_metadata;
  dst_metadata["technique"] = "difference (subtract_exr)";
  dst_metadata["first path"] = fst;
  dst_metadata["second path"] = snd;

  save_exr(dst, dst_metadata, fst_width, fst_height, dst_data);
}

void filter_exr(string dst, string src) {
  vector<vec4> dst_data, src_data;
  map<string, string> metadata;

  size_t width = 0, height = 0;

  load_exr(src, metadata, width, height, src_data);

  dst_data.resize(src_data.size());

  for (size_t y = 0; y < height; ++y) {
    for (size_t x = 0; x < width; ++x) {
      if (any(isnan(src_data[y * width + x]))) {
        float num = 0.0f;
        vec4 sum = vec4(0.0f);
        for (size_t j = glm::max(size_t(0), y - 1); j < glm::min(height - 1, y + 1); ++j) {
          for (size_t i = glm::max(size_t(0), x - 1); i < glm::min(width - 1, x + 1); ++i) {
            if (!any(isnan(src_data[j * width + i]))) {
              sum += src_data[j * width + i];
              num += 1.0f;
            }
          }
        }

        dst_data[y * width + x] = sum / num;
      } else {
        dst_data[y * width + x] = src_data[y * width + x];
      }
    }
  }

  save_exr(dst, metadata, width, height, dst_data);
}

string compute_errors(string fst, string snd) {
  vector<vec3> dst_data, fst_data, snd_data;
  map<string, string> fst_metadata, snd_metadata;

  size_t fst_width = 0, snd_width = 0, fst_height = 0, snd_height = 0;

  load_exr(fst, fst_metadata, fst_width, fst_height, fst_data);
  load_exr(snd, snd_metadata, snd_width, snd_height, snd_data);

  if (fst_width != snd_width || fst_height != snd_height) {
    throw std::runtime_error("Sizes of '" + fst + "' and '" + snd +
                             "' doesn't match.");
  }

  double num = 0.0f;
  double abs_sum = 0.0;
  double rms_sum = 0.0;

  for (size_t i = 0; i < fst_data.size(); ++i) {
    if (!any(isnan(fst_data[i])) && !any(isnan(snd_data[i]))) {
      auto diff = fst_data[i] - snd_data[i];
      rms_sum += l1Norm(diff * diff);
      abs_sum += l1Norm(diff);
      num += 3.0f;
    }
  }

  stringstream stream;

  stream << std::setprecision(10)
    << abs_sum / num << " "
    << sqrt(rms_sum / num) << " "
    << fst_metadata.find("total_time")->second << " "
    << snd_metadata.find("total_time")->second << " "
    << fst_metadata.find("technique")->second << " "
    << snd_metadata.find("technique")->second;

  return stream.str();
}

string query_time(string path) {
  map<string, string> metadata = load_metadata(path);
  return metadata.find("total_time")->second;
}

}
