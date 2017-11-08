cmake_minimum_required (VERSION 2.6)
project (assimp2libgdx)
set(CMAKE_CXX_FLAGS "-std=c++11")


# The version number.
set (ASSIMP2LIBGDX_VERSION_MAJOR 0)
set (ASSIMP2LIBGDX_VERSION_MINOR 1)
set (ASSIMP2LIBGDX_VERSION_PATCH 0)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake-modules")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)
 
# configure a header file to pass some of the CMake settings
# to the source code
configure_file (
  "${PROJECT_SOURCE_DIR}/assimp2libgdx/version.h.in"
  "${PROJECT_SOURCE_DIR}/assimp2libgdx/version.h"
  )

set ( ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL
	"If the supplementary tools for Assimp are built in addition to the library."
)

add_subdirectory (assimp)
set (EXTRA_LIBS ${EXTRA_LIBS} assimp)

include_directories("assimp/include")

add_executable(assimp2libgdx assimp2libgdx/main.cpp assimp2libgdx/json_exporter.cpp assimp2libgdx/cencode.c assimp2libgdx/mesh_splitter.h assimp2libgdx/mesh_splitter.cpp)
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
