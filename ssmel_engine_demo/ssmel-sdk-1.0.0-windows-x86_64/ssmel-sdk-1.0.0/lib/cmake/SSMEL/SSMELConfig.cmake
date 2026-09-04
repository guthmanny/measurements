
####### Expanded from @PACKAGE_INIT@ by configure_package_config_file() #######
####### Any changes to this file will be overwritten by the next CMake run ####
####### The input file was SSMELConfig.cmake.in                            ########

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

include("${CMAKE_CURRENT_LIST_DIR}/SSMELTargets.cmake")

if(TARGET SSMEL::ssmel AND NOT TARGET SSMEL::ssmel_engine)
  add_library(SSMEL::ssmel_engine ALIAS SSMEL::ssmel)
endif()

get_filename_component(_SSMEL_CURRENT_DIR "${CMAKE_CURRENT_LIST_DIR}" ABSOLUTE)
get_filename_component(_SSMEL_PREFIX "${_SSMEL_CURRENT_DIR}" PATH)
get_filename_component(_SSMEL_PREFIX "${_SSMEL_PREFIX}" PATH)
get_filename_component(_SSMEL_PREFIX "${_SSMEL_PREFIX}" PATH)
set(SSMEL_INCLUDE_DIR "${_SSMEL_PREFIX}/include")
set(SSMEL_LIB_DIR "${_SSMEL_PREFIX}/lib")

unset(_SSMEL_CURRENT_DIR)
unset(_SSMEL_PREFIX)

if(NOT DEFINED SSMEL_FOUND)
  set(SSMEL_FOUND TRUE)
endif()
