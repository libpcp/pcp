set(INC 
    ${CMAKE_SOURCE_DIR}/libpcp/src
    ${CMAKE_SOURCE_DIR}/libpcp/include
    )
    
if (WIN32)

set(PCP_SERVER_SOURCES
    ${CMAKE_SOURCE_DIR}/win_utils/getopt.c
    ${CMAKE_SOURCE_DIR}/win_utils/getopt_long.c
    ${CMAKE_SOURCE_DIR}/libpcp/src/net/pcp_socket.c
    ${CMAKE_SOURCE_DIR}/libpcp/src/windows/pcp_gettimeofday.c
    ${CMAKE_SOURCE_DIR}/libpcp/src/pcp_logger.c
	)
    
set(INC 
    ${INC}
	${CMAKE_SOURCE_DIR}/libpcp/src/net
	${CMAKE_SOURCE_DIR}/pcp_client/win_utils
    ${CMAKE_SOURCE_DIR}/libpcp/src/windows
    ${CMAKE_SOURCE_DIR}/win_utils
   )
endif()

include_directories(${INC})
add_executable(pcp-server pcp_server.c ${PCP_SERVER_SOURCES})
target_link_libraries(pcp-server ${WIN_SOCK_LIBS})
