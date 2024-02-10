cmake_minimum_required(VERSION 3.7.2)

function(SecureCommunication_DeclareCAmkESComponent name)
  DeclareCAmkESComponent(
    ${name}
    SOURCES
        ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/src/SecureCommunication.c
        ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/src/sc_helpers.c
    INCLUDES
        interfaces/headers/
    C_FLAGS
        -Wall
        -Werror
    LIBS
        system_config
        lib_debug
        os_core_api
        os_socket_client
        os_crypto
  )
endfunction()