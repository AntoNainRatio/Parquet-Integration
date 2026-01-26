cmake_minimum_required(VERSION 3.20)
project(parquet_utils LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Arrow CONFIG REQUIRED)
find_package(Parquet CONFIG REQUIRED)
find_package(google_cloud_cpp_storage CONFIG REQUIRED)

add_library (parquet_utils SHARED 
             "src/parquet_utils.cpp" "src/parquet_utils.h"
             "src/job_handler.h" 
             "src/io_backend.cpp" "src/io_backend.h"  
             "src/gcs_stream.cpp" "src/gcs_stream.h"
            )

target_link_libraries(parquet_utils 
    PRIVATE
        Arrow::arrow_shared
        Parquet::parquet_shared
        google-cloud-cpp::storage
)

if(WIN32)
  set_target_properties(parquet_utils PROPERTIES
    WINDOWS_EXPORT_ALL_SYMBOLS ON
    RUNTIME_OUTPUT_NAME "libparquet_utils"
    ARCHIVE_OUTPUT_NAME "libparquet_utils"
  )
endif()

add_executable(parquet_utils_main "src/main.cpp")
target_link_libraries(parquet_utils_main PRIVATE parquet_utils)