set(INC 
        ${CMAKE_SOURCE_DIR}/libpcp/src
        ${CMAKE_SOURCE_DIR}/libpcp/include)
set(INCLUDE_SRC 
        ${CMAKE_SOURCE_DIR}/libpcp/src
        ${CMAKE_SOURCE_DIR}/win_utils/getopt.c
		${CMAKE_SOURCE_DIR}/win_utils/getopt_long.c
		${CMAKE_SOURCE_DIR}/libpcp/src/windows/pcp_gettimeofday.c)
set(INCLUDE_NET 
        ${CMAKE_SOURCE_DIR}/libpcp/src/net)

if (WIN32)
set(INC
        ${INC}
        ${CMAKE_SOURCE_DIR}/libpcp/src/net
        ${CMAKE_SOURCE_DIR}/libpcp/src/windows
        ${CMAKE_SOURCE_DIR}/pcp_app/windows/src
        ${CMAKE_SOURCE_DIR}/pcp_app/windows/include
        ${CMAKE_SOURCE_DIR}/win_utils
        )
endif()

include_directories(${INC} ${INCLUDE_NET})

#name collision if executable is called "pcp"
# the name of the library is "pcp" even though Cmake prepends "lib" in front
# of the name to form "libpcp"
add_executable(pcp pcp_app.c ${INCLUDE_SRC})

target_link_libraries(pcp ${LIB_LIBPCP} ${WIN_SOCK_LIBS})
