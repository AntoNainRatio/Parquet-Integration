# CMakeList.txt : projet CMake pour parquet_utils, incluez la source et définissez
# la logique spécifique au projet ici.
#
cmake_minimum_required (VERSION 3.8)

# Activez Rechargement à chaud pour les compilateurs MSVC si cela est pris en charge.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("parquet_utils" VERSION 1.0.0 LANGUAGES CXX)

find_package(Arrow CONFIG REQUIRED)
find_package(Parquet CONFIG REQUIRED)

# Ajoutez une source à l'exécutable de ce projet.
add_library (parquet_utils "parquet_utils.cpp" "parquet_utils.h"
)

target_link_libraries(parquet_utils PRIVATE
        Arrow::arrow_shared
        Parquet::parquet_shared
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