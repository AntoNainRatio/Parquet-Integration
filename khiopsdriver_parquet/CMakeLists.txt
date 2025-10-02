cmake_minimum_required(VERSION 3.14)

project("khiopsdriver_file_parquet" VERSION 1.0.0 LANGUAGES CXX)

find_package(Arrow CONFIG REQUIRED)
find_package(Parquet CONFIG REQUIRED)
find_package(boost_uuid CONFIG REQUIRED)

add_library(khiopsdriver_file_parquet SHARED
    khiopsdriver_file_parquet.cpp
    khiopsdriver_file_parquet.h
    multifile.h
    parquet_to_csv.cpp
    parquet_to_csv.h
)

target_link_libraries(khiopsdriver_file_parquet
    PRIVATE
      Arrow::arrow_shared
      Parquet::parquet_shared
      Boost::uuid
)

set_target_properties(khiopsdriver_file_parquet PROPERTIES
    CXX_STANDARD 20
    PUBLIC_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/khiopsdriver_file_parquet.h
    SOVERSION ${PROJECT_VERSION_MAJOR}
    VERSION ${PROJECT_VERSION}
)

if(WIN32)
  set_target_properties(khiopsdriver_file_parquet PROPERTIES
    WINDOWS_EXPORT_ALL_SYMBOLS ON
    RUNTIME_OUTPUT_NAME "libkhiopsdriver_file_parquet"
    ARCHIVE_OUTPUT_NAME "libkhiopsdriver_file_parquet"
  )
endif()



target_include_directories(khiopsdriver_file_parquet PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(test)
