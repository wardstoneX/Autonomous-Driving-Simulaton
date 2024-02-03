# Demo TUM Course
# Copyright (C) 2021, Hensoldt Cyber GmbH

cmake_minimum_required(VERSION 3.7.2)

include(components/WolfTPM/CMakeLists.txt)

os_set_config_file(
  "system_config.h"
  CONFIG_PROJECT_NAME "system_config"
)

project(demo_tpm C)

WolfTPM_DeclareCAmkESComponent(WolfTPM)

DeclareCAmkESComponent(
  TestApp
  SOURCES
      components/TestApp/src/TestApp.c
  INCLUDES
      interfaces/headers
  C_FLAGS
      -Wall
      -Werror
  LIBS
      lib_debug
      os_core_api
)

# Include the custom interfaces
CAmkESAddCPPInclude(interfaces/camkes)
CAmkESAddImportPath(interfaces/camkes)

DeclareAndCreateCamkESSystem(demo_tpm.camkes)
GenerateSimulateScript()
