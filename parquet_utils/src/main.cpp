#include <iostream>
#include "parquet_utils.h"

#define DELETION false
#define MERGE false

int main(int argc, char** argv) {

	//const char* filename = "C:/Users/KXFJ3896/Documents/parquet_reader/data/toto.parquet";
	const char* filename = "C:/Users/Public/khiops_data/samples/AccidentsMedium/Vehicles.parquet";
	//const char* filename = "gs://test-parquet-khiops/parquet_data/Medium/Users_medium.parquet";

	const char* output_dir = "C:/Users/KXFJ3896/Documents/Parquet-Integration/parquet_utils/Vehicles";
	//const char* output_dir = "gs://test-parquet-khiops/output_data/Users_medium";

	const char* prefix = "toto";
	const char* merged_prefix = "Users_conversion";


	int return_code = parquetToCsv(filename, output_dir, prefix);
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

	/*if (argc < 4 || argc > 5) {
		if (argc == 2 && strcmp(argv[1],"-h") ==0) {
			std::cout << "This program converts a Parquet file to CSV files." << std::endl;
			std::cout << "Usage: " << argv[0] << " <parquet_file> <output_prefix> <output_dir> [-m]" << std::endl;
			std::cout << "  <parquet_file>: Path to the input Parquet file." << std::endl;
			std::cout << "  <output_dir>: Directory where the output CSV files will be saved." << std::endl;
			std::cout << "  <output_prefix>: Prefix for the output CSV files." << std::endl;
			std::cout << "  -m (optional): If provided, merges all CSV files into a single file after conversion. It creates the files \"<output_dir>/<output_prefix>_merged.txt\"." << std::endl;
		}
		else {
			std::cout << "Invalid number of arguments." << std::endl;
			std::cout << "Usage: " << argv[0] << " <parquet_file> <output_prefix> <output_dir> [-m]" << std::endl;
			std::cout << "Use -h for help." << std::endl;
			return 1;
		}
	}

	const char* filename = argv[1];
	
	const char* output_dir = argv[2];
	
	const char* prefix = argv[3];


	int return_code = parquetToCsv(filename, output_dir, prefix);
	if (return_code == 0) {
		std::cout << "Conversion suceed: parquet file (" << filename << ") saved in csv in " << output_dir << "/" << prefix << "*.txt" << std::endl;
		if (argc == 5 && strcmp(argv[4], "-m") == 0) {
			std::string output_file = std::string(output_dir) + "/" + std::string(prefix) + "_merged.txt";
			merge_csv_files(output_dir, prefix, output_file.c_str());
			std::cout << "Merged csv file created and saved at: " << output_file << std::endl;
		}
	}
	else {
		std::cout << "Conversion failed" << std::endl;
	}
	return 0;*/
}