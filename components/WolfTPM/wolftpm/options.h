/* options.h.in
 *
 * Copyright (C) 2006-2022 wolfSSL Inc.
 *
 * This file is part of wolfTPM.
 *
 * wolfTPM is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * wolfTPM is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1335, USA
 */

/* WolfTPM note:
 * We do not use autotools, since the configuration is known in advance.
 * Instead, we manually define all the macros that are needed.
 */

#ifndef WOLFTPM_OPTIONS_H
#define WOLFTPM_OPTIONS_H


#ifdef __cplusplus
extern "C" {
#endif

#define WOLFTPM_TRENTOS
#define WOLFTPM_SLB9670
#define TPM_TIMEOUT_TRIES 10000000
#define WOLFTPM_EXAMPLE_HAL
#define DEBUG_WOLFTPM
//#define WOLFTPM_DEBUG_IO
#define LITTLE_ENDIAN_ORDER
// TODO: HAVE_FIPS?
#define WOLFTPM2_NO_WOLFCRYPT
#undef __linux__

#ifdef __cplusplus
}
#endif


#endif /* WOLFTPM_OPTIONS_H */
