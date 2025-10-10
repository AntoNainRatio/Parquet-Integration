#include <iostream>
#include "parquet_utils.h"

#define DELETION false

int main() {
	const char* filename = "C:/Users/Public/khiops_data/samples/AccidentsMedium/Users_medium.parquet";
	const char* output_dir = "C:/Users/KXFJ3896/Documents/Parquet-Integration/parquet_utils/output_dir";
	const char* prefix = "toto";
	int return_code = parquetToCsv(filename, prefix, output_dir);
	if (return_code == 0) {
		std::cout << "Conversion suceed: parquet file (" << filename << ") saved in csv in " << output_dir << "/" << prefix << "*.txt" << std::endl;
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