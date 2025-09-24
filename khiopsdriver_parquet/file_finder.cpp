#include "globbing.h"
#include "file_finder.h"

#include <filesystem>
#include <iostream>
#include <vector>
#include <string>

const std::vector<const char*> get_matching_files(const std::string& pattern) {
	std::vector<const char*> matched_files;
	std::filesystem::path search_path = std::filesystem::path(pattern).parent_path();
	std::string filename_pattern = std::filesystem::path(pattern).filename().string();
	if (!std::filesystem::exists(search_path) || !std::filesystem::is_directory(search_path)) {
		return matched_files; // Return empty if the directory doesn't exist
	}
	for (const auto& entry : std::filesystem::directory_iterator(search_path)) {
		if (entry.is_regular_file()) {
			const std::string& filepath = entry.path().string();
			const std::string& filename = entry.path().filename().string();
			if (utils::gitignore_glob_match(filename, filename_pattern)) {
				matched_files.push_back(filepath.c_str());
			}
		}
	}
	return matched_files;
}