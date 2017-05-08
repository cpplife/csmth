cmake_minimum_required (VERSION 2.6)
project(cSMTH)

include_directories(. ./src ./tinyxml2 ./libcurl/include)


set(HEADERS ${HEADERS} 
    )

set(TINYXML_SRCS ./tinyxml2/tinyxml2.cpp)

set(SRCS ${TINYXML_SRCS} ${SRCS}
	./src/net_util.cpp
	./src/smth.cpp
	./src/main.cpp
    )

set(ALL ${SRCS} ${HEADERS} ${RESOURCES})

#message(${SRCS})
#add_library(bf ${SRCS})
add_executable(csmth ${ALL})

