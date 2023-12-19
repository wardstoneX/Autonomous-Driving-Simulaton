/*
 * OS libraries configurations
 *
 * Copyright (C) 2021, HENSOLDT Cyber GmbH
 */

#pragma once

#if !defined(NDEBUG)
#  define Debug_Config_STANDARD_ASSERT
#  define Debug_Config_ASSERT_SELF_PTR
#else
#  define Debug_Config_DISABLE_ASSERT
#  define Debug_Config_NO_ASSERT_SELF_PTR
#endif

#define Debug_Config_LOG_LEVEL			Debug_LOG_LEVEL_DEBUG
#define Debug_Config_INCLUDE_LEVEL_IN_MSG
#define Debug_Config_LOG_WITH_FILE_LINE

/* TODO: What does this do? */
#define Memory_Config_USE_STDLIB_ALLOC
