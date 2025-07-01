#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "Mahogany::mahogany_imap" for configuration "Release"
set_property(TARGET Mahogany::mahogany_imap APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(Mahogany::mahogany_imap PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libimap.a"
  )

list(APPEND _cmake_import_check_targets Mahogany::mahogany_imap )
list(APPEND _cmake_import_check_files_for_Mahogany::mahogany_imap "${_IMPORT_PREFIX}/lib/libimap.a" )

# Import target "Mahogany::mahogany_compface" for configuration "Release"
set_property(TARGET Mahogany::mahogany_compface APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(Mahogany::mahogany_compface PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libcompface.a"
  )

list(APPEND _cmake_import_check_targets Mahogany::mahogany_compface )
list(APPEND _cmake_import_check_files_for_Mahogany::mahogany_compface "${_IMPORT_PREFIX}/lib/libcompface.a" )

# Import target "Mahogany::mahogany_dspam" for configuration "Release"
set_property(TARGET Mahogany::mahogany_dspam APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(Mahogany::mahogany_dspam PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libdspam.a"
  )

list(APPEND _cmake_import_check_targets Mahogany::mahogany_dspam )
list(APPEND _cmake_import_check_files_for_Mahogany::mahogany_dspam "${_IMPORT_PREFIX}/lib/libdspam.a" )

# Import target "Mahogany::mahogany_versit" for configuration "Release"
set_property(TARGET Mahogany::mahogany_versit APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(Mahogany::mahogany_versit PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libversit.a"
  )

list(APPEND _cmake_import_check_targets Mahogany::mahogany_versit )
list(APPEND _cmake_import_check_files_for_Mahogany::mahogany_versit "${_IMPORT_PREFIX}/lib/libversit.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
