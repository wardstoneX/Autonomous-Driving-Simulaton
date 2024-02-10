# Demo TUM Course
# Copyright (C) 2021, Hensoldt Cyber GmbH

cmake_minimum_required(VERSION 3.7.2)

include(components/WolfTPM/CMakeLists.txt)
include("plat/${PLATFORM}/plat_nic.cmake")

os_set_config_file(
  "system_config.h"
  CONFIG_PROJECT_NAME "system_config"
)

project(demo_tum_course C)

WolfTPM_DeclareCAmkESComponent(WolfTPM)

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

DeclareCAmkESComponent(
    SecureCommunication
    INCLUDES
        interfaces/headers
    SOURCES
        components/SecureCommunication/src/SecureCommunication.c
        components/SecureCommunication/src/sc_helpers.c
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

DeclareCAmkESComponent(
  TestApp
  SOURCES
        components/TestApp/src/TestApp.c
	components/TestApp/src/scv/scv.c
        components/TestApp/src/utils.c
  INCLUDES
      interfaces/headers
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

# Include the custom interfaces
CAmkESAddCPPInclude(interfaces/camkes)
CAmkESAddImportPath(interfaces/camkes)

DeclareAndCreateCamkESSystem(demo_tum_course.camkes)
GenerateSimulateScript()
