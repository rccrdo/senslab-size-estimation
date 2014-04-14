/*
 * Copyright (c) 2012 Riccardo Lucchese, lucchese at dei.unipd.it
 *               2012 Damiano Varagnolo, varagnolo at dei.unipd.it
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 *    1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 *
 *    2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 *
 *    3. This notice may not be removed or altered from any source
 *    distribution.
 */

#ifndef __UTIL_H__
#define __UTIL_H__

#include <stdint.h>
#include <stdio.h>
#include "ds2411.h"


/*
 * Force function inlining
 */
#define __always_inline__ static inline __attribute__((always_inline))


#define max(a,b) ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a >= _b ? _a : _b;})
#define min(a,b) ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a <= _b ? _a : _b;})


/*
 * Return the full 64-bit board-id (=node-uid + crc) from the ds2411 chip
 */
__always_inline__ uint64_t board_get_id64(void) {
	return *((uint64_t *)ds2411_id.raw);
}


/*
 * This will return a 16-bit board-id/node-uid from the ds2411 chip
 * This is the same 16bit id used in the csv files listing the node's positions
 * which are distributed on the senslab website
 */
__always_inline__ uint16_t board_get_id16(void) {
	return (ds2411_id.serial1<<8) | ds2411_id.serial0;
}


/*
 * Helper routines to printf in hexadecimal the chunk of memory pointed by
 * mem and with length `len` bytes
 */
void printhex(const char *mem, uint16_t len);
void printhex2(const char *mem, uint16_t len);


/*
 * Define this macro to enable tracing
 *
 * ! Enabling tracing can be very verbose and interfere with the test 
 * (eg. by introducing delays).
 */
#define DBG_TRACE
#ifdef DBG_TRACE
#define trace(...) printf(__VA_ARGS__)
#else
#define trace(...) {}
#endif


/*
 * NDEBUG enables/disables debug globally: assert() and dbg() expand to empty
 * blocks when defined.
 * ! if you want to disable debugging pass -DNDEBUG to make or append -DNDEBUG in
 * the CFLAGS environment variable inside the project Makefile
 */
#ifdef NDEBUG
#define dbg(...) {}
#else
#define dbg(...) printf(__VA_ARGS__)
#endif


/*
 * Enable/disable debugging of certain parts of the code
 *
 * ! NDEBUG takes precedence
 */

/*
 * Enabling debug output can produce lots of output and interfere with the test
 * (e.g. by introducing delays).
 */
#define DBG_DISTRIBUTION_UNIFORM	0
#define DBG_MATH	                0

#endif /* __UTIL_H__ */

