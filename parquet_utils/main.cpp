#include <iostream>
#include "parquet_utils.h"

#define DELETION false
#define MERGE true

int main() {
	const char* filename = "C:/Users/Public/khiops_data/samples/AccidentsMedium/Users_medium.parquet";
	const char* output_dir = "C:/Users/KXFJ3896/Documents/Parquet-Integration/parquet_utils/Users_medium";
	const char* prefix = "toto";
	const char* merged_prefix = "Users_medium_conversion";
	int return_code = parquetToCsv(filename, prefix, output_dir);
	if (return_code == 0) {
		std::cout << "Conversion suceed: parquet file (" << filename << ") saved in csv in " << output_dir << "/" << prefix << "*.txt" << std::endl;
		if (MERGE) {
			std::string output_file = std::string(output_dir) + "/" + std::string(merged_prefix) + ".txt";
			merge_csv_files(output_dir, prefix, output_file.c_str());
			std::cout << "Merged csv file created: " << output_file << std::endl;
		}
		if (DELETION && delete_dir(output_dir, prefix)) {
			std::cout << "Output directory cleaned" << std::endl;
		}
		else {
			std::cout << "Output directory not cleaned" << std::endl;
		}
	}
	else {
		std::cout << "Conversion failed" << std::endl;
	}
	return 0;
}