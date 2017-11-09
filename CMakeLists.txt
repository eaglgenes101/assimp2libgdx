cmake_minimum_required (VERSION 3.02)
project (assimp2libgdx)

set(CMAKE_CXX_FLAGS "-std=c++11")

# Options supported by assimp2libgdx (assimp supports a superset of these)
OPTION ( ASSIMP_BUILD_TESTS
  "If the test suite for Assimp is built in addition to the library."
  OFF
)
OPTION ( ASSIMP_WERROR
  "Treat warnings as errors."
  OFF
)
OPTION ( ASSIMP_ASAN
  "Enable AddressSanitizer."
  OFF
)
OPTION ( ASSIMP_UBSAN
  "Enable Undefined Behavior sanitizer."
  OFF
)

# The version number.
set (ASSIMP2LIBGDX_VERSION_MAJOR 0)
set (ASSIMP2LIBGDX_VERSION_MINOR 1)
set (ASSIMP2LIBGDX_VERSION_PATCH 0)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake-modules")
 
# configure a header file to pass some of the CMake settings
# to the source code
configure_file (
  "${PROJECT_SOURCE_DIR}/assimp2libgdx/version.h.in"
  "${PROJECT_SOURCE_DIR}/assimp2libgdx/version.h"
)

include_directories("assimp/include")

set ( ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL
	"If the supplementary tools for Assimp are built in addition to the library."
)

add_subdirectory (assimp)
set (EXTRA_LIBS ${EXTRA_LIBS} assimp)

IF ( ASSIMP_BUILD_TESTS )
  #ADD_SUBDIRECTORY( test bin )
ENDIF ( ASSIMP_BUILD_TESTS )

# ASSIMP flags are useful here, and are inherited
if (ASSIMP_WERROR)
  MESSAGE(STATUS "Treating warnings as errors")
  IF (MSVC)
    add_compile_options(/WX)
  ELSE()
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror")
  ENDIF()
endif()

if (ASSIMP_ASAN)
    MESSAGE(STATUS "AddressSanitizer enabled")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address")
endif()

if (ASSIMP_UBSAN)
    MESSAGE(STATUS "Undefined Behavior sanitizer enabled")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined -fno-sanitize-recover=all")
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=undefined -fno-sanitize-recover=all")
endif()

IF ( CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR )
  SET( CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_HOME_DIRECTORY}/lib" )
  SET( CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_HOME_DIRECTORY}/lib" )
  SET( CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_HOME_DIRECTORY}/bin" )
ENDIF ( CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR )

add_executable(assimp2libgdx assimp2libgdx/main.cpp assimp2libgdx/json_exporter.cpp assimp2libgdx/mesh_splitter.h assimp2libgdx/mesh_splitter.cpp)
target_link_libraries (assimp2libgdx ${EXTRA_LIBS})

if( MSVC_IDE )
   add_custom_command(
      TARGET assimp2libgdx
      POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/assimp/code/$(ConfigurationName)/assimp.dll ${CMAKE_BINARY_DIR}/$(ConfigurationName)/assimp.dll
   )
endif( MSVC_IDE )

INSTALL( TARGETS assimp2libgdx 
	 LIBRARY DESTINATION ${ASSIMP_LIB_INSTALL_DIR}
	 RUNTIME DESTINATION ${ASSIMP_BIN_INSTALL_DIR})
