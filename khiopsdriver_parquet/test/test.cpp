#include <iostream>
#include <string>

#include "../khiopsdriver_file_parquet.h"

void dump_multifile(MultiFile* mf) {
	if (mf == nullptr) {
		std::cout << "MultiFile is null." << std::endl;
		return;
	}
	std::cout << "MultiFile details:" << std::endl;
	std::cout << "Total size: " << mf->total_size << " bytes" << std::endl;
	std::cout << "Logical position: " << mf->logical_pos << std::endl;
	std::cout << "Current file index: " << mf->current_index << std::endl;
	std::cout << "Position in current file: " << mf->pos_in_current << std::endl;
	std::cout << "Number of files: " << mf->filenames.size() << std::endl;
	for (size_t i = 0; i < mf->filenames.size(); ++i) {
		std::cout << " File " << i << ": " << mf->filenames[i]
			<< ", Size: " << mf->file_sizes[i]
			<< ", Prefix Offset: " << mf->prefix_offsets[i] << std::endl;
	}
}

int test_driver_fopen_errors() {
	std::vector<const char*> args = { nullptr, "non_existent", "parquet://C:/Users/KXFJ3896/Documents/Parquet-Integration/khiopsdriver_parquet/test/data/not_parquet.txt", "parquet://:/Users/KXFJ3896/Documents/Parquet-Integration/khiopsdriver_parquet/test/data/not_parquet.txt" };

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

	std::string parquet = "parquet://C:/Users/Public/khiops_data/samples/AccidentsMedium/Users_medium.parquet";

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

	code = driver_fclose(mf);
	if (code == -1) {
		throw std::runtime_error("driver_close error during driver_fread tests.");
	}

	free(buffer);
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

	std::string parquet = "parquet://C:/Users/Public/khiops_data/samples/AccidentsMedium/Users_medium.parquet";

	MultiFile* mf = driver_fopen(parquet.c_str(), 'r');
	if (mf == nullptr) {
		throw std::runtime_error("driver_fopen error during driver_fread tests.");
	}
	int code;

	const size_t buffer_size = 2048;
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

	failed += verify_driver_fread(mf, code, total_read);

	size_t third_read_total_size = mf->file_sizes[0];
	size_t third_read_size_in_loop = 2048;
	while (total_read < third_read_total_size) {

		code = driver_fread(buffer, 1, third_read_size_in_loop, mf);
		if (code != -1) {
			total_read += code;
		}
		else {
			failed++;
			std::cout << "driver_fread test: error reading 10 000 time 2048" << std::endl;
			break;
		}


		failed += verify_driver_fread(mf, code, total_read);
	}

	code = driver_fclose(mf);
	if (code == -1) {
		throw std::runtime_error("driver_close error during driver_fread tests.");
	}

	free(buffer);
	return failed;
}

// read all file from begin to end using driver_fread
int test_driver_fread_all_file() {
	std::string parquet = "parquet://C:/Users/Public/khiops_data/samples/AccidentsMedium/Users_medium.parquet";

	MultiFile* mf = driver_fopen(parquet.c_str(), 'r');
	if (mf == nullptr) {
		throw std::runtime_error("driver_fopen error during driver_fread all file test.");
	}
	int code;

	const size_t buffer_size = 2048;
	char* buffer = (char*)calloc(buffer_size, sizeof(char));

	size_t total_read = 0;

	size_t total_read_target = driver_getFileSize(mf);
	size_t read_size_in_loop = 2048;
	while (total_read < total_read_target) {

		code = driver_fread(buffer, sizeof(char), read_size_in_loop, mf);
		if (code != -1) {
			total_read += code;
		}
		else {
			std::cout << "driver_fread all file test: error reading the whole file." << std::endl;
			return 1;
		}
	}

	if (total_read != total_read_target) {
		std::cout << "driver_fread all file test: read more than there is in file." << std::endl;
		return 1;
	}

	code = driver_fclose(mf);
	if (code == -1) {
		throw std::runtime_error("driver_close error during driver_fread all file test.");
	}

	free(buffer);
	return 0;
}

// testing error handling in driver_fseek function
int test_driver_fseek_errors() {
	int failed = 0;

	int code;
	code = driver_fseek(NULL, 10000, MultiFileWhence::BEGIN);
	if (code != -1) {
		std::cout << "driver_fseek errors: NULL MultiFile doesn't return -1." << std::endl;
		failed++;
	}

	std::string parquet = "parquet://C:/Users/Public/khiops_data/samples/AccidentsMedium/Users_medium.parquet";

	MultiFile* mf = driver_fopen(parquet.c_str(), 'r');
	if (mf == nullptr) {
		throw std::runtime_error("driver_fopen error during driver_fseek errors.");
	}

	size_t file_size = driver_getFileSize(mf);
	code = driver_fseek(mf, file_size + 1, MultiFileWhence::BEGIN);
	if (code != -1) {
		std::cout << "driver_fseek errors: file_size+1 offset from BEGIN doesn't return -1." << std::endl;
		failed++;
	}
	code = driver_fseek(mf, file_size + 1, MultiFileWhence::CURRENT);
	if (code != -1) {
		std::cout << "driver_fseek errors: file_size+1 offset from CURRENT(BEGIN) doesn't return -1." << std::endl;
		failed++;
	}

	code = driver_fseek(mf, -10000, MultiFileWhence::CURRENT);
	if (code != -1) {
		std::cout << "driver_fseek errors: negative offset from CURRENT(BEGIN) doesn't return -1." << std::endl;
		failed++;
	}

	code = driver_fseek(mf, 10000, MultiFileWhence::END);
	if (code != -1) {
		std::cout << "driver_fseek errors: positive offset from END doesn't return -1." << std::endl;
		failed++;
	}

	code = driver_fseek(mf, -10000, MultiFileWhence::BEGIN);
	if (code != -1) {
		std::cout << "driver_fseek errors: negative offset from BEGIN doesn't return -1." << std::endl;
		failed++;
	}

	code = driver_fseek(mf, file_size + 1, MultiFileWhence::CURRENT);
	if (code != -1) {
		std::cout << "driver_fseek errors: (filesize + 1) offset from CURRENT doesn't return -1." << std::endl;
		failed++;
	}

	code = driver_fseek(mf, -(file_size + 1), MultiFileWhence::END);
	if (code != -1) {
		std::cout << "driver_fseek errors: -(filesize + 1) offset from END doesn't return -1." << std::endl;
		failed++;
	}

	code = driver_fclose(mf);
	if (code == -1) {
		throw std::runtime_error("driver_fclose error during driver_fseek random tests.");
	}

	return failed;
}

// calls to driver_fseek with offset between 0 and file_size 'times' times
int test_driver_fseek_random(int times = 20) {
	int failed = 0;

	std::string parquet = "parquet://C:/Users/Public/khiops_data/samples/AccidentsMedium/Users_medium.parquet";

	MultiFile* mf = driver_fopen(parquet.c_str(), 'r');
	if (mf == nullptr) {
		throw std::runtime_error("driver_fopen error during driver_fseek errors.");
	}
	int code;

	size_t file_size = driver_getFileSize(mf);

	int iter = 0;
	while (iter < times) {
		int rdm = rand() % file_size;

		code = driver_fseek(mf, rdm, MultiFileWhence::BEGIN);
		if (code == -1) {
			std::cout << "driver_fseek random test: error seeking from " << mf->logical_pos << " to " << rdm << "." << std::endl;
			failed++;
		}
		iter++;
	}

	code = driver_fclose(mf);
	if (code == -1) {
		throw std::runtime_error("driver_fclose error during driver_fseek random tests.");
	}
	return failed;
}

// crossing the file from begin to end using driver_fseek
int test_driver_fseek_all_file() {
	std::string parquet = "parquet://C:/Users/Public/khiops_data/samples/AccidentsMedium/Users_medium.parquet";

	MultiFile* mf = driver_fopen(parquet.c_str(), 'r');
	if (mf == nullptr) {
		throw std::runtime_error("driver_fopen error during driver_fseek all file test.");
	}
	int code;

	size_t total_seek = 0;

	size_t total_seek_target = driver_getFileSize(mf);
	size_t offset = 5678;
	while (total_seek + offset < total_seek_target) {

		code = driver_fseek(mf, offset, MultiFileWhence::CURRENT);
		if (code != -1) {
			total_seek += offset;
		}
		else {
			std::cout << "driver_fseek all file test: error seeking the whole file." << std::endl;
			return 1;
		}
	}

	size_t final_offset = total_seek_target - total_seek - 1;
	code = driver_fseek(mf, final_offset, MultiFileWhence::CURRENT);
	if (code != -1) {
		total_seek += offset;
	}
	else {
		std::cout << "driver_fseek all file test: error seeking the whole file (last call)." << std::endl;
		std::cout << driver_getlasterror() << std::endl;
		return 1;
	}

	code = driver_fclose(mf);
	if (code == -1) {
		throw std::runtime_error("driver_close error during driver_fseek all file test.");
	}
	return 0;
}

// crossing the file from end to begin using driver_fseek
int test_driver_fseek_all_file_reverse() {
	std::string parquet = "parquet://C:/Users/Public/khiops_data/samples/AccidentsMedium/Users_medium.parquet";

	MultiFile* mf = driver_fopen(parquet.c_str(), 'r');
	if (mf == nullptr) {
		throw std::runtime_error("driver_fopen error during driver_fseek all file test.");
	}
	int code;

	size_t total_seek = 0;
	code = driver_fseek(mf, 0, MultiFileWhence::END);
	if (code == -1) {
		std::cout << "driver_fseek all file reverse test: error seeking the whole file." << std::endl;
		std::cout << driver_getlasterror() << std::endl;
		return 1;
	}
	//dump_multifile(mf);

	size_t total_seek_target = driver_getFileSize(mf);
	size_t offset = 5678;
	while (total_seek + offset < total_seek_target) {

		code = driver_fseek(mf, -offset, MultiFileWhence::CURRENT);
		if (code != -1) {
			total_seek += offset;
		}
		else {
			std::cout << "driver_fseek all file reverse test: error seeking the whole file." << std::endl;
			std::cout << driver_getlasterror() << std::endl;
			return 1;
		}
	}

	size_t final_offset = total_seek_target - total_seek;
	code = driver_fseek(mf, -final_offset, MultiFileWhence::CURRENT);
	if (code != -1) {
		total_seek += offset;
	}
	else {
		std::cout << "driver_fseek all file reverse test: error seeking the whole file (last call)." << std::endl;
		std::cout << driver_getlasterror() << std::endl;
		return 1;
	}
	// dump_multifile(mf);

	code = driver_fclose(mf);
	if (code == -1) {
		throw std::runtime_error("driver_close error during driver_fseek all file test.");
	}
	return 0;
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
	failed += test_driver_fread_all_file();
	failed += test_driver_fread_errors();

	failed += test_driver_fseek_random();
	failed += test_driver_fseek_all_file();
	failed += test_driver_fseek_all_file_reverse();
	failed += test_driver_fseek_errors();

	error_resume(failed);

	return 0;
}