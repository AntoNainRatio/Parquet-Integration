#include <iostream>
#include <string>

#include "../khiopsdriver_parquet.h"

int test_driver_fopen_errors() {
	std::vector<const char*> args = { nullptr, "non_existent", "parquet//:C:/Users/KXFJ3896/Documents/Parquet-Integration/khiopsdriver_parquet/test/data/not_parquet.txt"};

	int failed = 0;

	for (const char* arg : args) {
		MultiFile* mf = driver_fopen(arg, 'r');
		if (mf != nullptr) {
			failed++;
		}
	}

	return failed;
}

int test_driver_fclose_erros() {
	std::vector<void*> args = { nullptr/*, (void*)"non_existent" , (void*)2*/};

	int failed = 0;

	for (void* arg : args) {
		int code = driver_fclose(arg);
		if (code != EOF) {
			failed++;
		}
	}

	return failed;
}

int test_driver_fread_errors() {
	int failed = 0;

	int code;

	std::string parquet = "C:/Users/Public/khiops_data/samples/AccidentsMedium/Users_medium.parquet";

	// opening file
	MultiFile* mf = driver_fopen(parquet.c_str(), 'r');
	if (mf == nullptr) {
		throw std::runtime_error("driver_fopen error during driver_fread errors.");
	}

	// init buffer
	const size_t buffer_size = 1024;
	char* buffer = (char*)malloc(buffer_size * sizeof(char));

	size_t size = sizeof(char);
	size_t count = 500;

	code = driver_fread(NULL, size, count, mf);
	if (code != -1) {
		std::cout << "driver_fread errors: NULL ptr doesn't return -1" << std::endl;
		failed++;
	}

	code = driver_fread(buffer, size, count, NULL);
	if (code != -1) {
		std::cout << "driver_fread errors: NULL MultiFile doesn't return -1" << std::endl;
		failed++;
	}

	code = driver_fread(buffer, 0, count, mf);
	if (code != -1) {
		std::cout << "driver_fread errors: size=0 doesn't return -1" << std::endl;
		failed++;
	}

	code = driver_fread(buffer, size, 0, mf);
	if (code != -1) {
		std::cout << "driver_fread errors: count=0 doesn't return -1" << std::endl;
		failed++;
	}

	return failed;
}

int verify_driver_fread(MultiFile* mf, int return_code, size_t total_read) {
	if (return_code == -1) {
		std::cout << "Test driver_fread: Invalid return code" << std::endl;
		return 1;
	}
	if (mf->logical_pos != total_read) {
		std::cout << "Test driver_fread: logical_pos != total bytes read" << std::endl;
		return 1;
	}
	size_t i = 0;
	for (; i < mf->prefix_offsets.size() - 1; i++) {
		if (total_read < mf->prefix_offsets[i+1]) {
			break;
		}
	}
	if (i != mf->current_index) {
		std::cout << "Test driver_fread: Invalid file index" << std::endl;
		return 1;
	}
	size_t offset_in_curr = total_read - mf->prefix_offsets[i];
	if (offset_in_curr != mf->pos_in_current) {
		std::cout << "Test driver_fread: Invalid position in current file" << std::endl;
		return 1;
	}

	return 0;
}

int test_driver_fread() {
	int failed = 0;

	std::string parquet = "C:/Users/Public/khiops_data/samples/AccidentsMedium/Users_medium.parquet";

	MultiFile* mf = driver_fopen(parquet.c_str(), 'r');
	if (mf == nullptr) {
		std::cout << "driver_fopen error." << std::endl;
		return -1;
	}
	int code;

	const size_t buffer_size = 1024;
	char* buffer = (char*)malloc(buffer_size * sizeof(char));

	size_t total_read = 0;

	size_t first_read_size = 500;

	code = driver_fread(buffer, sizeof(char), first_read_size, mf);
	if (code != -1) {
		total_read += code;
	}

	failed += verify_driver_fread(mf, code, total_read);

	size_t second_read_size = 1000;
	
	code = driver_fread(buffer, sizeof(char), second_read_size, mf);
	if (code != -1) {
		total_read += code;
	}

	return failed;
}

// return 0 if pass, 1 otherwise
int verify_code(MultiFile* mf, MultiFileError expected) {
	if (mf->error_state != expected) {
		std::cerr << "Error on state:" << std::endl
			<< "\tGot  " << (int)mf->error_state << std::endl
			<< "\tExpected  " << (int)expected << std::endl;
		return 1;
	}
	return 0;
}

void error_resume(int failed) {
	std::cout << "Tests finished:" << std::endl;
	if (failed == 0 || failed == 1) {
		std::cout << "\t" << failed << " error";
	}
	else {
		std::cout << "\t" << failed << " errors";

	}
}

int main() {
	// redirecting stderr to null
	std::cerr.rdbuf(nullptr);

	int failed = 0;

	failed += test_driver_fopen_errors();

	failed += test_driver_fclose_erros();

	failed += test_driver_fread();
	failed += test_driver_fread_errors();

	error_resume(failed);

	return 0;
}