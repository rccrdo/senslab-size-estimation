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

#ifndef __FRACTIONAL16_H__
#define __FRACTIONAL16_H__

#include <stdint.h>
#include <stdio.h>
#include "util.h"
#include "network.h"
#include "fixpoint32.h"

/*
 * 16bit representation of numbers in [0,1)
 *
 * A 15bit significand spanning two complementary ranges
 * (the range is encoded with 1 bit)
 *   range 0 -> significand in [0,0.9375)
 *   range 1 -> significand in [0.9375,1)
 *
 * The mappings from fixpoint32_t to fractional16_t and viceversa do not
 * garantee the smallest possible approximation-error but are very fast
 * (involving only shifts and summations in place of multiplications and
 * divisions) and none the less sufficiently accurate for our purposes.
 */
typedef uint16_t fractional16_t;


#define FRACTIONAL16_MAX (0xffff)


__always_inline__ uint16_t __fractional16_range(fractional16_t f16) {
	return f16 >> 15;
}


__always_inline__ uint16_t __fractional16_value(fractional16_t f16) {
	return f16 & 0x7fff;
}


__always_inline__ fractional16_t fixpoint32_to_fractional16(fixpoint32_t fix32) {
	fractional16_t ret;
	uint16_t range, value;
	
	/*
	 * 0xf8000000 = ((FIXPOINT32_MAX+1)/32)*31)
	 */
	if (fix32 < (uint32_t)0xf8000000) {
		range = 0;
		value = (fix32 >> 17) & 0x0000ffff;
	} else {
		range = 1;
		value = (((uint32_t)fix32 - (uint32_t)0xf8000000) >> 12) & 0x0000ffff;
	}

	assert(range == 0 || range == 1);
	assert(value <= 0x7fff);

	ret = (range << 15) | value;
	
	if (DBG_MATH)
		dbg("fix32->frac16 0x%.8lx -> 0x%.4x\n", (unsigned long int)fix32, ret);

	return ret;
}


__always_inline__ fixpoint32_t fractional16_to_fixpoint32(fractional16_t f16) {
	uint16_t range, value;
	fixpoint32_t fix32;

	range = __fractional16_range(f16);
	value = __fractional16_value(f16);
	
	if (range == 0) {
		fix32 = ((uint32_t)value) << 17;
	} else {
		fix32 = 0xf8000000 | (((uint32_t)value) << 12);
	}

	if (DBG_MATH)
		dbg("frac16->fix32 0x%.4x -> 0x%.8lx\n", f16, (unsigned long int)fix32);

	return fix32;
}


__always_inline__ fractional16_t fractional16_max(fractional16_t frac1, fractional16_t frac2) {
	fractional16_t ret;

	ret = frac1;
	if (frac1 < frac2)
		ret = frac2;

	if (DBG_MATH)
		dbg("max-frac16 0x%.4x 0x%.4x -> 0x%.4x\n", frac1, frac2, ret);

	return ret;
}


#endif /* __FRACTIONAL16_H__ */

