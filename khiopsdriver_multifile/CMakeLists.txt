# CMakeList.txt : projet CMake pour khiopsdriver_multifile, incluez la source et définissez
# la logique spécifique au projet ici.
#
cmake_minimum_required (VERSION 3.8)

# Activez Rechargement à chaud pour les compilateurs MSVC si cela est pris en charge.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("khiopsdriver_file_multifile" VERSION 1.0.0 LANGUAGES CXX)

# Ajoutez une source à l'exécutable de ce projet.
add_library (khiopsdriver_file_multifile SHARED 
		"khiopsdriver_file_multifile.cpp"
		"khiopsdriver_file_multifile.h"
		"multifile.h"
		"globbing.cpp"
		"globbing.h"
)

set_target_properties(khiopsdriver_file_multifile PROPERTIES
    CXX_STANDARD 20
    PUBLIC_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/khiopsdriver_file_multifile.h
    SOVERSION ${PROJECT_VERSION_MAJOR}
    VERSION ${PROJECT_VERSION}
)

if(WIN32)
  set_target_properties(khiopsdriver_file_multifile PROPERTIES
    WINDOWS_EXPORT_ALL_SYMBOLS ON
    RUNTIME_OUTPUT_NAME "libkhiopsdriver_file_multifile"
    ARCHIVE_OUTPUT_NAME "libkhiopsdriver_file_multifile"
  )
endif()

add_subdirectory("tests")