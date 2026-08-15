
####### Expanded from @PACKAGE_INIT@ by configure_package_config_file() #######
####### Any changes to this file will be overwritten by the next CMake run ####
####### The input file was NuDSPConfig.cmake.in                            ########

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

# NuDSPConfig.cmake - loaded by find_package(NuDSP CONFIG)
# This file is generated during configure step and installed under
# <prefix>/lib/cmake/NuDSP

include("${CMAKE_CURRENT_LIST_DIR}/NuDSPTargets.cmake")

# Provide imported target alias for convenience
if(NOT TARGET NuDSP::nudsp)
		add_library(NuDSP::nudsp ALIAS nudsp)
endif()

# Compute the installation prefix relative to this file so consumers get the
# correct paths regardless of their own CMAKE_INSTALL_PREFIX.
get_filename_component(_NUDSP_CURRENT_DIR "${CMAKE_CURRENT_LIST_DIR}" ABSOLUTE)
get_filename_component(_NUDSP_PREFIX "${_NUDSP_CURRENT_DIR}" PATH)
get_filename_component(_NUDSP_PREFIX "${_NUDSP_PREFIX}" PATH)
get_filename_component(_NUDSP_PREFIX "${_NUDSP_PREFIX}" PATH)
set(NuDSP_INCLUDE_DIR "${_NUDSP_PREFIX}/include")
set(NuDSP_LIB_DIR "${_NUDSP_PREFIX}/lib")

unset(_NUDSP_CURRENT_DIR)
unset(_NUDSP_PREFIX)

if(NOT DEFINED NuDSP_FOUND)
	set(NuDSP_FOUND TRUE)
endif()
