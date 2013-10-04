set(INC
        ${CMAKE_SOURCE_DIR}/libpcp/src
		${CMAKE_SOURCE_DIR}/libpcp/windows
        ${CMAKE_SOURCE_DIR}/libpcp/include
		${CMAKE_SOURCE_DIR}/libpcp/src/net/pcp_socket.h
		)

set(INCLUDE_SRC
        ${CMAKE_SOURCE_DIR}/tests
		)
set(INCLUDE_NET
		${CMAKE_SOURCE_DIR}/libpcp/src/net)
#include_directories(${INC} ${INCLUDE_SRC} ${INCLUDE_NET})

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

include_directories(${INC} ${INCLUDE_SRC} ${INCLUDE_NET})
# include_directories(${INC})

# name collision if executable is called "pcp"
# the name of the library is "pcp" even though Cmake prepends "lib" in front
# of the name to form "libpcp"
add_executable(test_flow_notify 			test_flow_notify.c ${INCLUDE_SRC})
add_executable(test_event_handler 			test_event_handler.c ${INCLUDE_SRC})
add_executable(test_gateway 				test_gateway.c ${INCLUDE_SRC})
add_executable(test_lifetime_renewal 		test_lifetime_renewal.c ${INCLUDE_SRC})
add_executable(test_pcp_api 				test_pcp_api.c ${INCLUDE_SRC})
add_executable(test_pcp_client_db 			test_pcp_client_db.c ${INCLUDE_SRC})
add_executable(test_pcp_client_map_opcode 	test_pcp_client_map_opcode.c ${INCLUDE_SRC})
add_executable(test_pcp_client_peer_opcode 	test_pcp_client_peer_opcode.c ${INCLUDE_SRC})
add_executable(test_pcp_logger 				test_pcp_logger.c ${INCLUDE_SRC})
add_executable(test_pcp_msg 				test_pcp_msg.c ${INCLUDE_SRC})
add_executable(test_ping_gws 				test_server_discovery.c ${INCLUDE_SRC})
add_executable(test_server_reping 			test_server_reping.c ${INCLUDE_SRC})
add_executable(test_server_restart 			test_server_restart.c ${INCLUDE_SRC})
add_executable(test_sock_ntop 				test_sock_ntop.c ${INCLUDE_SRC})
add_executable(test_version_negotiation 	test_version_negotiation.c ${INCLUDE_SRC})

target_link_libraries(test_flow_notify 				${LIB_LIBPCP} ${WIN_SOCK_LIBS})
target_link_libraries(test_event_handler 			${LIB_LIBPCP} ${WIN_SOCK_LIBS})
target_link_libraries(test_gateway 					${LIB_LIBPCP} ${WIN_SOCK_LIBS})
target_link_libraries(test_lifetime_renewal 		${LIB_LIBPCP} ${WIN_SOCK_LIBS})
target_link_libraries(test_pcp_api 					${LIB_LIBPCP} ${WIN_SOCK_LIBS})
target_link_libraries(test_pcp_client_db 			${LIB_LIBPCP} ${WIN_SOCK_LIBS})
target_link_libraries(test_pcp_client_map_opcode 	${LIB_LIBPCP} ${WIN_SOCK_LIBS})
target_link_libraries(test_pcp_client_peer_opcode 	${LIB_LIBPCP} ${WIN_SOCK_LIBS})
target_link_libraries(test_pcp_logger 				${LIB_LIBPCP} ${WIN_SOCK_LIBS})
target_link_libraries(test_pcp_msg 					${LIB_LIBPCP} ${WIN_SOCK_LIBS})
target_link_libraries(test_ping_gws 				${LIB_LIBPCP} ${WIN_SOCK_LIBS})
target_link_libraries(test_server_reping 			${LIB_LIBPCP} ${WIN_SOCK_LIBS})
target_link_libraries(test_server_restart 			${LIB_LIBPCP} ${WIN_SOCK_LIBS})
target_link_libraries(test_sock_ntop 				${LIB_LIBPCP} ${WIN_SOCK_LIBS})
target_link_libraries(test_version_negotiation 		${LIB_LIBPCP} ${WIN_SOCK_LIBS})

