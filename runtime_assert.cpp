#include <runtime_assert>
#include <sstream>
#include <stdexcept>

using namespace std;

void _runtime_assert(std::string file, std::size_t line) {
	stringstream message;
	message << file << ":" << line << ":1:" << " Assertion failed.";
	throw logic_error(message.str());
}


