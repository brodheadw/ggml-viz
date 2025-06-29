#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "glfw" for configuration "Debug"
set_property(TARGET glfw APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(glfw PROPERTIES
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib/libglfw.3.5.dylib"
  IMPORTED_SONAME_DEBUG "@rpath/libglfw.3.dylib"
  )

list(APPEND _cmake_import_check_targets glfw )
list(APPEND _cmake_import_check_files_for_glfw "${_IMPORT_PREFIX}/lib/libglfw.3.5.dylib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
