#include <chrono>
#include <iomanip>
#include <iostream>
#include <runtime_assert>
#include <sstream>
#include <stdexcept>
#include <tuple>
#include <sstream>
#include <algorithm>
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

}

#include <sys/stat.h>

namespace haste {

size_t getmtime(const string& path) {
	struct stat buf;
	stat(path.c_str(), &buf);
	return buf.st_mtime;
}

string fullpath(string relative_path) {
  #ifdef _MSC_VER
  auto buffer = _fullpath(nullptr, relative_path.c_str(), 0);
  auto result = std::string(buffer);
  std::free(buffer);
  return result;
  #else
  return relative_path;
  #endif
}

string tempdir() {
  #if defined _MSC_VER
  char buffer[MAX_PATH + 2];
  GetTempPathA(MAX_PATH, buffer);
  return buffer;
  #else
  return P_tmpdir;
  #endif
}

string temppath(string extension) {
  #if defined _MSC_VER
  char buffer[MAX_PATH + 2];
  GetTempPathA(MAX_PATH, buffer);

  RPC_CSTR uuid_str;
  UUID uuid;
  UuidCreate(&uuid);
  UuidToStringA(&uuid, &uuid_str);
  string result = buffer + ((LPCSTR)uuid_str + extension);
  RpcStringFreeA(&uuid_str);

  return result;
  #else
  string tempdir = haste::tempdir();
  string result = string(tempdir.size() + 7 + extension.size() + 1, '/');

  std::copy_n(tempdir.begin(), tempdir.size(), result.begin());
  std::fill_n(result.begin() + tempdir.size() + 1, 6, 'X');
  std::copy_n(extension.begin(), extension.size(), result.begin() + tempdir.size() + 7);
  result[result.size() - 1] = '\0';

  int fd = mkstemps(&result[0], extension.size());

  if (fd == -1) {
    throw std::runtime_error("failed to create temporary file");
  }

  close(fd);

  result.resize(result.size() - 1);
  return result;
  #endif
}

bool isfile(string path) {
  #ifdef _MSC_VER
  DWORD attributes = GetFileAttributesA(path.c_str());

  if (attributes == 0xffffffff) {
    DWORD error = GetLastError();

    if (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND) {
      return false;
    }
    else if (error == ERROR_ACCESS_DENIED) {
      throw std::runtime_error("access denied");
    }
    else {
      throw std::runtime_error("unknown error");
    }
  }
  else if (attributes & FILE_ATTRIBUTE_DIRECTORY) {
    return false;
  }
  else {
    return true;
  }
  #else
  struct stat path_stat;
  stat(path.c_str(), &path_stat);
  return S_ISREG(path_stat.st_mode);
  #endif
}

void move_file(string old_path, string new_path) {
  #if defined _MSC_VER
  if (MoveFileExA(old_path.c_str(), new_path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) == 0) {
    throw std::runtime_error("failed to move file");
  }
  #else
  if (rename(old_path.c_str(), new_path.c_str()) != 0) {
    throw std::runtime_error("failed to move file");
  }
  #endif
}

}
