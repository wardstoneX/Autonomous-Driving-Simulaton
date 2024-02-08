/*
 * OS libraries configurations
 *
 * Copyright (C) 2021, HENSOLDT Cyber GmbH
 */

#pragma once


//-----------------------------------------------------------------------------
// Debug
//-----------------------------------------------------------------------------
#if !defined(NDEBUG)
#   define Debug_Config_STANDARD_ASSERT
#   define Debug_Config_ASSERT_SELF_PTR
#else
#   define Debug_Config_DISABLE_ASSERT
#   define Debug_Config_NO_ASSERT_SELF_PTR
#endif

#define Debug_Config_LOG_LEVEL                  Debug_LOG_LEVEL_DEBUG
#define Debug_Config_INCLUDE_LEVEL_IN_MSG
#define Debug_Config_LOG_WITH_FILE_LINE


//-----------------------------------------------------------------------------
// Memory
//-----------------------------------------------------------------------------
/* TODO: What does this do? */
#define Memory_Config_USE_STDLIB_ALLOC




//-----------------------------------------------------------------------------
// Network Stack
//-----------------------------------------------------------------------------
#define OS_NETWORK_MAXIMUM_SOCKET_NO 1

#define SERVER_IP                 "10.0.0.10"
#define ETH_ADDR                  "10.0.0.11"
#define ETH_GATEWAY_ADDR          "10.0.0.1"
#define ETH_SUBNET_MASK           "255.255.255.0"
#define SERVER_PORT               6000


//-----------------------------------------------------------------------------
// NIC driver
//-----------------------------------------------------------------------------
#define NIC_DRIVER_RINGBUFFER_NUMBER_ELEMENTS 16
#define NIC_DRIVER_RINGBUFFER_SIZE                                             \
    (NIC_DRIVER_RINGBUFFER_NUMBER_ELEMENTS * 4096)

//-----------------------------------------------------------------------------
// Secure Communication
//-----------------------------------------------------------------------------
#define PYTHON_ADDR     "10.0.0.10"
#define CRYPTO_PORT     65432
#define COMM_PORT       1234
