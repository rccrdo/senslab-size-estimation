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

#ifndef __FRACTIONAL48_H__
#define __FRACTIONAL48_H__

#include <stdint.h>
#include <stdio.h>
#include "fixpoint32.h"


typedef struct {
	fixpoint32_t value;
	int16_t exp;
} fractional48_t;


__always_inline__ void fractional48_init(fractional48_t *f48, fixpoint32_t fix32) {
	f48->value = fix32;
	f48->exp = 0;

	if (f48->value != 0) {
		while (!(f48->value & 0x80000000)) {
			f48->value <<= 1;
			f48->exp--;
		}
	}

	if (DBG_MATH)
		dbg("fix32->frac48 0x%.8lx -> 0x%.8lx.%d\n", (unsigned long int)fix32, (unsigned long int)f48->value, f48->exp);
}


__always_inline__ void fractional48_mul(fractional48_t *f48, fixpoint32_t fix32) {
	uint64_t res;
	assert(f48 != NULL);

	if (f48->value == 0) {
		assert(f48->exp == 0);
	} else {
		assert(f48->value & (unsigned long int)0x80000000);
	}

	if (DBG_MATH)
		dbg("f48*f32 %.8lx.%d * 0x%.8lx = ", (unsigned long int)f48->value, f48->exp, fix32);

	res = (uint64_t)f48->value * (uint64_t)fix32;

	if (res == 0) {
		f48->value = 0;
		f48->exp = 0;
	} else if (res > 0xfffffffful) {
		f48->exp -= 32;
		while (res > 0xfffffffful) {
			res >>= 1;
			f48->exp++;
		}
		f48->value = res;
	} else { // res <= 0xffffffff
		f48->value = res;
		while (!(f48->value & 0x80000000ul)) {
			f48->value <<= 1;
			f48->exp--;
		}
	}

	if (DBG_MATH)
		dbg("%.8lx.%d\n", (unsigned long int)f48->value, f48->exp);
}


#endif /* __FRACTIONAL48_H__ */

