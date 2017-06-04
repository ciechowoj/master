#include <chrono>
#include <iomanip>
#include <iostream>
#include <runtime_assert>
#include <sstream>
#include <stdexcept>
#include <tuple>
#include <sstream>
#include <system_utils.hpp>

#if defined _MSC_VER
#undef FLOAT
#undef UINT
#include <ShlObj.h>
#else
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>
#endif

namespace haste {

string homePath() {
	const char* home = getenv("HOME");

	if (home == nullptr) {
		#if defined _MSC_VER
		char path[MAX_PATH];
		SHGetFolderPathA(NULL, CSIDL_PROFILE, NULL, 0, path);
		return path;
		#else
		home = getpwuid(getuid())->pw_dir;
		#endif
	}

	return home;
}

string baseName(string path) {
	size_t index = path.find_last_of("/");

	if (index != string::npos) {
		return path.substr(index + 1, path.size());
	}
	else {
		return path;
	}
}

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
}

#include <sys/stat.h>

namespace haste {

size_t getmtime(const string& path) {
	struct stat buf;
	stat(path.c_str(), &buf);
	return buf.st_mtime;
}

}
