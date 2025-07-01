
####### Expanded from @PACKAGE_INIT@ by configure_package_config_file() #######
####### Any changes to this file will be overwritten by the next CMake run ####
####### The input file was MahoganyConfig.cmake.in                            ########

get_filename_component(PACKAGE_PREFIX_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../" ABSOLUTE)

macro(set_and_check _var _file)
  set(${_var} "${_file}")
  if(NOT EXISTS "${_file}")
    message(FATAL_ERROR "File or directory ${_file} referenced by variable ${_var} does not exist !")
  endif()
endmacro()

macro(check_required_components _NAME)
  foreach(comp ${${_NAME}_FIND_COMPONENTS})
    if(NOT ${_NAME}_${comp}_FOUND)
      if(${_NAME}_FIND_REQUIRED_${comp})
        set(${_NAME}_FOUND FALSE)
      endif()
    endif()
  endforeach()
endmacro()

####################################################################################

include(CMakeFindDependencyMacro)

# Find required dependencies
find_dependency(wxWidgets REQUIRED COMPONENTS core base net html aui adv)

# Check if components were requested and are available
set(_mahogany_supported_components imap compface dspam versit)

foreach(_comp ${Mahogany_FIND_COMPONENTS})
    if(NOT _comp IN_LIST _mahogany_supported_components)
        set(Mahogany_FOUND False)
        set(Mahogany_NOTFOUND_MESSAGE "Unsupported component: ${_comp}")
    endif()
endforeach()

# Include the exported targets
include("${CMAKE_CURRENT_LIST_DIR}/MahoganyTargets.cmake")

# Provide the target names for users
set(Mahogany_LIBRARIES)
foreach(_comp ${_mahogany_supported_components})
    if(TARGET Mahogany::mahogany_${_comp})
        list(APPEND Mahogany_LIBRARIES Mahogany::mahogany_${_comp})
    endif()
endforeach()

if(TARGET Mahogany::mahogany)
    list(APPEND Mahogany_LIBRARIES Mahogany::mahogany)
endif()

check_required_components(Mahogany)
