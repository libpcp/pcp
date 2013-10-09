cmake_minimum_required(VERSION 2.8)

set(WITH_EXPERIMENTAL ON CACHE BOOL "Experimental extensions suppport")
set(USE_IPV6_SOCKET ON CACHE BOOL "Use IPv6 socket")

if(WITH_EXPERIMENTAL)
add_definitions(-DPCP_SADSCP -DPCP_EXPERIMENTAL -DPCP_FLOW_PRIORITY)
endif()

if(USE_IPV6_SOCKET)
add_definitions(-DPCP_USE_IPV6_SOCKET)
endif()

if (WIN32)
	# In Visual Studio, forces the set of available configurations to be CMAKE_BUILD_TYPE only.
    set(CMAKE_CONFIGURATION_TYPES ${CMAKE_BUILD_TYPE} CACHE STRING "" FORCE)
	
	# These two libs are needed by windows, for socket operations
	set (WIN_SOCK_LIBS
	${WIN_SOCK_LIBS}ws2_32.lib
	${WIN_SOCK_LIBS}Iphlpapi.lib)

	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()


project(pcp-client)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin )

# LINUX is not a pre-defined like WIN32 or APPLE, so let's set it ourselves.
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
set(LINUX 1)
	message("Linux detected.")
endif()

# Build project in different directory than where the sources are
if(CMAKE_BINARY_DIR STREQUAL CMAKE_SOURCE_DIR)
    message(FATAL_ERROR
"
Attempt to build in-source detected.

Please do the following:
1. Remove the in-source generated files:
    rm -rf CMakeFiles CMakeCache.txt
2. Build out-of-source as follows:
    mkdir build (or whatever you want to call your build directory)
    cd build && cmake ..
Now you can execute make as many times as you want, without the need to re-execute cmake.")
endif()

# for custom find modules
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
    message("${CMAKE_SOURCE_DIR}/cmake")
    message("my binary dir ${CMAKE_BINARY_DIR}")

# set the name of the static library at top level
set(LIB_LIBPCP pcp_library)

add_subdirectory(${CMAKE_SOURCE_DIR}/libpcp)
add_subdirectory(${CMAKE_SOURCE_DIR}/pcp_app)
add_subdirectory(${CMAKE_SOURCE_DIR}/pcp_server)
add_subdirectory(${CMAKE_SOURCE_DIR}/tests)
