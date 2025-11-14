
find_package(Arrow CONFIG REQUIRED)
find_package(Parquet CONFIG REQUIRED)
find_package(google_cloud_cpp_storage CONFIG REQUIRED)

add_library (parquet_utils SHARED "parquet_utils.cpp" "parquet_utils.h"
                        "job_handler.h" 
                        "io_backend.cpp" "io_backend.h"  
                        "gcs_stream.cpp" "gcs_stream.h")

#target_include_directories(parquet_utils PRIVATE
#    ${ARROW_INCLUDE_DIR}
#    ${CMAKE_CURRENT_SOURCE_DIR}/include
#)

#include_directories("${ARROW_HOME}/include")
#link_directories("${ARROW_HOME}/lib")

target_link_libraries(parquet_utils PRIVATE
        Arrow::arrow_shared
        Parquet::parquet_shared
        google-cloud-cpp::storage
)

set_target_properties(parquet_utils PROPERTIES
    CXX_STANDARD 20
    PUBLIC_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/parquet_utils.h
    SOVERSION ${PROJECT_VERSION_MAJOR}
    VERSION ${PROJECT_VERSION}
)

if(WIN32)
  set_target_properties(parquet_utils PROPERTIES
    WINDOWS_EXPORT_ALL_SYMBOLS ON
    RUNTIME_OUTPUT_NAME "libparquet_utils"
    ARCHIVE_OUTPUT_NAME "libparquet_utils"
  )
endif()



target_include_directories(parquet_utils PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(main "main.cpp")
target_link_libraries(main PRIVATE parquet_utils)
target_link_libraries(main PRIVATE
        Arrow::arrow_shared
        Parquet::parquet_shared
)