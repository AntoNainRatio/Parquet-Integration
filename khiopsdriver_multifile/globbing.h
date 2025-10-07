#include <string>
#include <vector>

const std::vector<std::string> get_matching_files(const char* pattern);

bool gitignore_glob_match(const std::string& text, const std::string& glob);