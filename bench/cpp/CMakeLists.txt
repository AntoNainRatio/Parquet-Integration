# CMakeList.txt : projet CMake pour ParquetToCsv, incluez la source et définissez
# la logique spécifique au projet ici.
#
cmake_minimum_required (VERSION 3.8)

# Activez Rechargement à chaud pour les compilateurs MSVC si cela est pris en charge.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("ParquetToCsv")

# recherche packages
find_package(Arrow REQUIRED)
find_package(Parquet REQUIRED)
find_package(DuckDB REQUIRED)

# Ajoutez une source à l'exécutable de ce projet.
add_executable (ParquetToCsv "ParquetToCsv.h" "duckdb_conversion.cpp")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET ParquetToCsv PROPERTY CXX_STANDARD 20)
endif()

# Linkage des librairies
target_link_libraries("ParquetToCsv"
						PRIVATE 
						Arrow::arrow_shared
						Parquet::parquet_shared
						DuckDB::duckdb)

# TODO: Ajoutez des tests et installez des cibles si nécessaire.
