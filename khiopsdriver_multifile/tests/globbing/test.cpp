#include <iostream>

#include "../../globbing.h"

int verify_glob_match(const std::string& text, const std::string& glob, bool expected)
{
	bool result = gitignore_glob_match(text, glob);
	if (result != expected)
	{
		std::cout << "Mismatch: text='" << text << "' glob='" << glob << "' expected=" << expected << " got=" << result << std::endl;
		return 1;
	}
	return 0;
}

int glob_matchs() {
	int failed = 0;

	failed += verify_glob_match("file.txt", "file.txt", true);
	failed += verify_glob_match("file.txt", "*", true);
	failed += verify_glob_match("file.txt", "*.txt", true);
	failed += verify_glob_match("data/file.csv", "data/*.csv", true);
	failed += verify_glob_match("file\?.txt", "file1.txt", true);
	failed += verify_glob_match("file\?.txt", "file2.txt", true);

	failed += verify_glob_match("a", "b", false);
	failed += verify_glob_match("file.txt", "*.csv", false);	
	failed += verify_glob_match("data/file.csv", "data/*.txt", false);
	failed += verify_glob_match("data/subdir/file.csv", "data/*.csv", false);
	failed += verify_glob_match("file\?.txt", "file.txt", false);
	failed += verify_glob_match("file\?.txt", "file12.txt", false);



	return failed;
}

int main() {
	std::cout << "Tests for globbing:" << std::endl;
	int failed = glob_matchs();
	if (failed)
	{
		std::cout << "FAILED: " << failed << " tests failed" << std::endl;
		return 1;
	}
	std::cout << "PASSED: All tests passed" << std::endl;
	return 0;
}