#include <iostream>

#include "../../globbing.h"

bool VERBOSE = false;

int verify(int actual, int expected) {
	if (actual != expected) {
		std::cout << "Error: exp = " << expected << " and got = " << actual << std::endl;
		return 1;
	}
	return 0;
}

int launch_test(std::string glob, int expected_count) {
	auto files = get_matching_files(glob.c_str());
	if (VERBOSE) {
		std::cout << "Pattern: '" << glob << "' matched " << files.size() << " files" << std::endl;
		for (const auto& f : files) {
			std::cout << "  " << f << std::endl;
		}
	}
	int val = verify(static_cast<int>(files.size()), expected_count);
	if (val != 0) {
		std::cout << "Test failed for pattern: " << glob << std::endl;
	}
	return val;
}

int main() {

	std::cout << "Matching tests  :" << std::endl;

	int failed = 0;

	failed += launch_test("C:/Users/KXFJ3896/Documents/Parquet-Integration/khiopsdriver_multifile/tests/files/*.txt", 8);
	failed += launch_test("C:/Users/KXFJ3896/Documents/Parquet-Integration/khiopsdriver_multifile/tests/files/*.csv", 1);
	failed += launch_test("C:/Users/KXFJ3896/Documents/Parquet-Integration/khiopsdriver_multifile/tests/files/file?.txt", 2);
	failed += launch_test("C:/Users/KXFJ3896/Documents/Parquet-Integration/khiopsdriver_multifile/tests/files/file*.txt", 3);
	failed += launch_test("C:/Users/KXFJ3896/Documents/Parquet-Integration/khiopsdriver_multifile/tests/files/file.txt", 1);
	failed += launch_test("C:/Users/KXFJ3896/Documents/Parquet-Integration/khiopsdriver_multifile/tests/files/ehoh.txt", 0);

	if (failed == 0) {
		std::cout << "PASSED: All tests passed" << std::endl;
	}
	else {
		std::cout << "FAILED: " << failed << " tests failed" << std::endl;
	}

	return 0;
}