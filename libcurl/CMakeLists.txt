cmake_minimum_required (VERSION 3.10.1)

# project must be set after reset the compilers.
project(libcurl)

###############################################################################
# Set project output dirs
###############################################################################
set(PROJECT_OUT_LIB ${CMAKE_BINARY_DIR}/lib)
set(PROJECT_OUT_BIN ${CMAKE_BINARY_DIR}/bin)

if( DEFINED PROJECT_UNIFIED_OUT_LIB )
	set(PROJECT_OUT_LIB ${PROJECT_UNIFIED_OUT_LIB})
endif()
if( DEFINED PROJECT_UNIFIED_OUT_BIN )
	set(PROJECT_OUT_BIN ${PROJECT_UNIFIED_OUT_BIN})
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_OUT_LIB})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_OUT_LIB})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_OUT_BIN})

# Set Project dir
set(PROJECT_DIR ${CMAKE_CURRENT_LIST_DIR})

###############################################################################
# libcurl code
###############################################################################
include(${PROJECT_DIR}/libcurl.cmake)

# Import variables:
include(${PROJECT_DIR}/../mbedtls/mbedtls.cmake)

###############################################################################
# all codes
###############################################################################
set(PROJECT_SRCS 
	${SRCS_LIBCURL}
    )

###############################################################################
# set include dirs for project
###############################################################################
include_directories(
	${INCLUDES_LIBCURL}
	${INCLUDES_MBEDTLS}
	${PROJECT_DIR}/code
	)

###############################################################################
# set link dirs for project
###############################################################################
link_directories(
	${PROJECT_OUT_LIB}
	)

###############################################################################
# build project.
###############################################################################
add_definitions( -DCURL_STATICLIB -DBUILDING_LIBCURL -DUSE_MBEDTLS )
set(CMAKE_C_FLAGS_DEBUG   "${CMAKE_C_FLAGS_DEBUG} -DDEBUG -D_DEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG -D_DEBUG")
add_library(libcurl ${PROJECT_SRCS})
