# Demo TUM Course
# Copyright (C) 2021, Hensoldt Cyber GmbH

cmake_minimum_required(VERSION 3.7.2)

os_set_config_file(
  "system_config.h"
  CONFIG_PROJECT_NAME "system_config"
)

project(demo_tpm C)

DeclareCAmkESComponent(
  TPM
  SOURCES
      components/TPM/src/TPM.c
      components/TPM/src/TPM_if.c
      components/TPM/src/bcm2837/bcm2837_gpio.c
      components/TPM/src/bcm2837/bcm2837_spi.c
  INCLUDES
      components/TPM/src/bcm2837
  C_FLAGS
      -Wall
      -Werror
  LIBS
      lib_debug
      os_core_api
)

DeclareCAmkESComponent(
  TestApp
  SOURCES
      components/TestApp/src/TestApp.c
  C_FLAGS
      -Wall
      -Werror
  LIBS
      lib_debug
      os_core_api
)

DeclareAndCreateCamkESSystem(demo_tpm.camkes)
GenerateSimulateScript()
