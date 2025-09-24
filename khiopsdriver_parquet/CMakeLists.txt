# CMakeList.txt : projet CMake pour khiopsdriver_parquet, incluez la source et définissez
# la logique spécifique au projet ici.
#
cmake_minimum_required (VERSION 3.8)

# Activez Rechargement à chaud pour les compilateurs MSVC si cela est pris en charge.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("khiopsdriver_parquet")

# recherche packages
find_package(Arrow REQUIRED)
find_package(Parquet REQUIRED)

# Ajoutez une source à l'exécutable de ce projet.
add_library (khiopsdriver_parquet "khiopsdriver_parquet.cpp" "khiopsdriver_parquet.h" "multifile.h" "globbing.h" "file_finder.cpp" "file_finder.h" "parquet_to_csv.cpp" "parquet_to_csv.h")
add_executable (main "main.cpp")
add_subdirectory(test)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET khiopsdriver_parquet PROPERTY CXX_STANDARD 20)
endif()

# Linkage des librairies
target_link_libraries("khiopsdriver_parquet"
						PRIVATE 
						Arrow::arrow_shared
						Parquet::parquet_shared)

target_link_libraries("main"
						PRIVATE 
						khiopsdriver_parquet)

# TODO: Ajoutez des tests et installez des cibles si nécessaire.
