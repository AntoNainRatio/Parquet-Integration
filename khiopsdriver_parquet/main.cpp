#include "khiopsdriver_parquet.h"
#include "file_finder.h"
#include <iostream>

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

int read_test(void* buffer, size_t count, MultiFile* mf) {
	int test = driver_fread(buffer, sizeof(char), count, mf);
	if (test == -1) {
		std::cout << "driver_fread error." << std::endl;
	}
	else {
		std::cout << "driver_fread succeed, returned: " << test << " and buffer contains :" << static_cast<char*>(buffer) << std::endl;
	}
	return test;
}

int main() {
	std::string parquet = "C:/Users/Public/khiops_data/samples/AccidentsMedium/Users_medium.parquet";

	MultiFile* mf = driver_fopen(parquet.c_str(), 'r');
	if (mf == nullptr) {
		std::cout << "driver_fopen error." << std::endl;
		return -1;
	}
	else {
		dump_multifile(mf);
	}
	

	return 0;
}