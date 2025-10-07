# CMakeList.txt�: projet CMake pour khiopsdriver_parquet, incluez la source et d�finissez
# la logique sp�cifique au projet ici.
#
cmake_minimum_required (VERSION 3.8)

# Activez Rechargement � chaud pour les compilateurs MSVC si cela est pris en charge.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

# Ajoutez une source � l'ex�cutable de ce projet.
add_executable (matching_test "test_matching.cpp")


if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET matching_test PROPERTY CXX_STANDARD 20)
endif()


target_link_libraries("matching_test"
						PRIVATE 
						khiopsdriver_file_multifile)

add_custom_command(TARGET matching_test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:khiopsdriver_file_multifile>
        $<TARGET_FILE_DIR:matching_test>
)