#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "SSMEL::ssmel" for configuration "Release"
set_property(TARGET SSMEL::ssmel APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(SSMEL::ssmel PROPERTIES
  IMPORTED_IMPLIB_RELEASE "${_IMPORT_PREFIX}/lib/ssmel.lib"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/ssmel.dll"
  )

list(APPEND _cmake_import_check_targets SSMEL::ssmel )
list(APPEND _cmake_import_check_files_for_SSMEL::ssmel "${_IMPORT_PREFIX}/lib/ssmel.lib" "${_IMPORT_PREFIX}/bin/ssmel.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
