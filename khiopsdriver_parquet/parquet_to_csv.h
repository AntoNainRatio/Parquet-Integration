std::vector<std::string> parquetToCsv(const char* parquet_file, const char* prefix);
void delete_chunk_files(const std::vector<std::string>& chunk_files);