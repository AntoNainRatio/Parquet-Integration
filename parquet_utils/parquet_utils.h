#include <vector>

extern "C" __declspec(dllexport)
int parquetToCsv(const char* parquet_file, const char* prefix, const char* output_dir);
bool delete_dir(const char* output_dir, const char* prefix);