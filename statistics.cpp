#include <statistics.hpp>
#include <iostream>

namespace haste {

statistics_t::statistics_t(const map<string, string>& dict) {
	num_samples = stoll(dict.find("num_samples")->second);
	num_basic_rays = stoll(dict.find("num_basic_rays")->second);
	num_shadow_rays = stoll(dict.find("num_shadow_rays")->second);
	num_tentative_rays = stoll(dict.find("num_tentative_rays")->second);
	num_photons = stoll(dict.find("num_photons")->second);
	num_scattered = stoll(dict.find("num_scattered")->second);
	total_time = stoll(dict.find("total_time")->second);
	scatter_time = stoll(dict.find("scatter_time")->second);
	build_time = stod(dict.find("build_time")->second);
	gather_time = stod(dict.find("gather_time")->second);
	merge_time = stod(dict.find("merge_time")->second);
	density_time = stod(dict.find("density_time")->second);
	intersect_time = stod(dict.find("intersect_time")->second);
	trace_eye_time = stod(dict.find("trace_eye_time")->second);
	trace_light_time = stod(dict.find("trace_light_time")->second);
}

map<string, string> statistics_t::to_dict() const {
	map<string, string> result;

	result["num_samples"] = std::to_string(num_samples);
  result["num_basic_rays"] = std::to_string(num_basic_rays);
  result["num_shadow_rays"] = std::to_string(num_shadow_rays);
  result["num_tentative_rays"] = std::to_string(num_tentative_rays);
  result["num_photons"] = std::to_string(num_photons);
  result["num_scattered"] = std::to_string(num_scattered);
  result["total_time"] = std::to_string(total_time);
  result["scatter_time"] = std::to_string(scatter_time);
  result["build_time"] = std::to_string(build_time);
  result["gather_time"] = std::to_string(gather_time);
  result["merge_time"] = std::to_string(merge_time);
  result["density_time"] = std::to_string(density_time);
  result["intersect_time"] = std::to_string(intersect_time);
  result["trace_eye_time"] = std::to_string(trace_eye_time);
  result["trace_light_time"] = std::to_string(trace_light_time);

  return result;
}

std::ostream& operator<<(std::ostream& stream, const statistics_t& meta) {
  double connection_time =
      meta.trace_eye_time - meta.trace_light_time - meta.gather_time;
  double query_time = meta.gather_time - meta.merge_time - meta.intersect_time;
  double generate_time = meta.scatter_time - meta.build_time;
  double merge_remaining_time = meta.merge_time - meta.density_time;

  stream << "num basic rays: " << meta.num_basic_rays << "\n"
         << "num shadow rays: " << meta.num_shadow_rays << "\n"
         << "num tentative rays: " << meta.num_tentative_rays << "\n"
         << "num photons: " << meta.num_photons << "\n"
         << "num scattered: " << meta.num_scattered / meta.num_samples << " ("
         << (meta.num_scattered / meta.num_samples + meta.num_photons - 1) /
                std::max(size_t(1), meta.num_photons)
         << "x)\n"
         << "total time: " << meta.total_time << "s\n"
         << "time per sample:        " << meta.total_time / meta.num_samples
         << "s\n"
         << "    trace eye time:       "
         << int(meta.trace_eye_time / meta.total_time * 100) << "%% ("
         << meta.trace_eye_time / meta.num_samples << "s)\n"
         << "        gather time:        "
         << int(meta.gather_time / meta.total_time * 100) << "%% ("
         << meta.gather_time / meta.num_samples << "s)\n"
         << "            query time:       "
         << int(query_time / meta.total_time * 100) << "%% ("
         << query_time / meta.num_samples << "s)\n"
         << "            merge time:       "
         << int(meta.merge_time / meta.total_time * 100) << "%% ("
         << meta.merge_time / meta.num_samples << "s)\n"
         << "                density time:   "
         << int(meta.density_time / meta.total_time * 100) << "%% ("
         << meta.density_time / meta.num_samples << "s)\n"
         << "                rest time:      "
         << int(merge_remaining_time / meta.total_time * 100) << "%% ("
         << merge_remaining_time / meta.num_samples << "s)\n"
         << "        connection time:    "
         << int(connection_time / meta.total_time * 100) << "%% ("
         << connection_time / meta.num_samples << "s)\n"
         << "    scatter time:         "
         << int(meta.scatter_time / meta.total_time * 100) << "%% ("
         << meta.scatter_time / meta.num_samples << "s)\n"
         << "        generate time:      "
         << int(generate_time / meta.total_time * 100) << "%% ("
         << generate_time / meta.num_samples << "s)\n"
         << "        build time:         "
         << int(meta.build_time / meta.total_time * 100) << "%% ("
         << meta.build_time / meta.num_samples << "s)";

  return stream;
}

}
