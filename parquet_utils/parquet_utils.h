#include <vector>

int parquetToCsv(const char* parquet_file, const char* prefix, const char* output_dir);
void delete_chunk_files(const std::vector<std::string>& chunk_files);