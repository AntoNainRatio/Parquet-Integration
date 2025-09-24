#include "khiopsdriver_parquet.h"
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
	std::vector<const char*> files = { 
		"parquet://C:/Users/KXFJ3896/Documents/Parquet-Integration/khiopsdriver_parquet/test/toto/toto_0.txt",
		"parquet://C:/Users/KXFJ3896/Documents/Parquet-Integration/khiopsdriver_parquet/test/toto/toto_1.txt",
		"parquet://C:/Users/KXFJ3896/Documents/Parquet-Integration/khiopsdriver_parquet/test/toto/toto_2.txt",
	};

	MultiFile* mf = driver_fopen(files, 'r');
	if (mf->error_state == MultiFileError::OK) {
		std::cout << "Files opened successfully." << std::endl;
	}
	else {
		std::cout << "Error opening files. Error code: " << static_cast<int>(mf->error_state) << std::endl;
	}
	dump_multifile(mf);


	void* buffer = calloc(100, sizeof(char));
	
	read_test(buffer, 40, mf);

	dump_multifile(mf);

	if (driver_fseek(mf, -1, MultiFileWhence::CURRENT) != 0) {
		std::cout << "Error seeking." << std::endl;
	}
	else {
		read_test(buffer, 50, mf);
	}


	free(buffer);

	int code = driver_fclose(mf);
	if (code == 0) {
		std::cout << "Files closed successfully." << std::endl;
	} else {
		std::cout << "Error closing files. Error code: " << code << std::endl;
	}

	dump_multifile(mf); // mf is deleted in driver_fclose, this is just to show the function usage
	return 0;
}