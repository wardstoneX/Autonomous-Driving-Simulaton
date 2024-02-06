#
# Demo TUM Course
#
# Copyright (C) 2021-2023, Hensoldt Cyber GmbH
#

cmake_minimum_required(VERSION 3.7.2)

#-------------------------------------------------------------------------------
os_set_config_file(
    "system_config.h"
    CONFIG_PROJECT_NAME "system_config"
)

#-------------------------------------------------------------------------------
project(demo_tum_course C)

#include("plat/${PLATFORM}/plat_nic.cmake")

TimeServer_DeclareCAmkESComponent(
    TimeServer
)

NetworkStack_PicoTcp_DeclareCAmkESComponent(
    NetworkStack_PicoTcp
    C_FLAGS
        -DNetworkStack_PicoTcp_USE_HARDCODED_IPADDR
        -DDEV_ADDR=ETH_ADDR
        -DGATEWAY_ADDR=ETH_GATEWAY_ADDR
        -DSUBNET_MASK=ETH_SUBNET_MASK
)

DeclareCAmkESComponents_for_UARTs()

NIC_ChanMux_DeclareCAmkESComponent(
    NIC_ChanMux
    CHANMUX_CHANNEL_NIC_CTRL
    CHANMUX_CHANNEL_NIC_DATA
)

ChanMux_UART_DeclareCAmkESComponents(
    ChanMux_UART
    components/ChanMux/ChanMux_config.c
    system_config
)


DeclareCAmkESComponent(
    TestApp
    INCLUDES
        include/scv
    SOURCES
        components/TestApp/src/TestApp.c         
        include/scv/scv.c
    C_FLAGS
        -Wall
        -Werror
    LIBS
        system_config
        lib_debug
        os_core_api
        os_socket_client
)

DeclareAndCreateCamkESSystem(demo_tum_course.camkes)
GenerateSimulateScript()


