cmake_minimum_required(VERSION 3.20)
project(ParquetIntegration CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


add_subdirectory(parquet_utils)

add_subdirectory(khiopsdriver_multifile)

# add_subdirectory(bench)
# add_subdirectory(test_driver)
