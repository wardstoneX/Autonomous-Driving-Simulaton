cmake_minimum_required(VERSION 3.7.2)

function(WolfTPM_DeclareCAmkESComponent name)
  DeclareCAmkESComponent(
    ${name}
    SOURCES
        ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/trentos.c
        ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/hal/bcm2837/bcm2837_gpio.c
        ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/hal/bcm2837/bcm2837_spi.c
        ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/hal/tpm_io.c
        ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/hal/tpm_io_trentos.c
        ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/src/tpm2.c
        ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/src/tpm2_cryptocb.c
        ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/src/tpm2_packet.c
        ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/src/tpm2_param_enc.c
        ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/src/tpm2_tis.c
        ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/src/tpm2_wrap.c
    INCLUDES
        ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/hal
        ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/
	interfaces/headers/
    C_FLAGS
        -Wall
	-Werror
    LIBS
        lib_debug
	os_core_api
  )
endfunction()
