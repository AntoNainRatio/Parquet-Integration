#include <vector>

extern "C" __declspec(dllexport)
int parquetToCsv(const char* parquet_file, const char* output_dir, const char* prefix);
bool delete_dir(const char* output_dir, const char* prefix);

extern "C" __declspec(dllexport)
void merge_csv_files(const char* dir, const char* prefix, const char* output_file);