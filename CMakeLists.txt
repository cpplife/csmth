cmake_minimum_required (VERSION 2.6)
project(cSMTH)


###############################################################################
# Set project output dirs
###############################################################################
set(PROJECT_OUT_LIB ${CMAKE_BINARY_DIR}/lib)
set(PROJECT_OUT_BIN ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_OUT_LIB})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_OUT_LIB})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_OUT_BIN})


include_directories(. ./src ./tinyxml2 ./libcurl/include)

add_subdirectory( mbedtls )
add_subdirectory( libcurl )

set(HEADERS ${HEADERS} 
    )

#set(TINYXML_SRCS ./tinyxml2/tinyxml2.cpp)

set(SRCS ${TINYXML_SRCS} ${SRCS}
	./src/net_util.cpp
	./src/smth.cpp
	./src/main.cpp
    )

set(ALL ${SRCS} ${HEADERS} ${RESOURCES})

###############################################################################
# set link dirs for project
###############################################################################
link_directories(
	${PROJECT_OUT_LIB}
	)

#message(${SRCS})
add_executable(csmth ${ALL})
add_dependencies(csmth mbedtls libcurl)

if(MSVC)
	target_link_libraries(csmth PRIVATE ws2_32 wldap32 crypt32 mbedtls libcurl)
else()
	target_link_libraries(csmth PRIVATE -lmbedtls -llibcurl -lpthread -luuid )
endif()
