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

#ifndef __FIXPOINT32_H__
#define __FIXPOINT32_H__

#include <stdint.h>
#include <assert.h>
#include "util.h"
#include "network.h"


/*
 * 32bit fixed-point representation of numbers in [0,1)
 */
typedef uint32_t fixpoint32_t;

#define FIXPOINT32_MAX                  0xffffffff
#define ERR_FIXPOINT32_OVERFLOW		(-1)
#define ERR_FIXPOINT32_UNDERFLOW	(-2)

typedef struct {
	uint16_t num;
	uint16_t den;
} fraction_t;


__always_inline__ fixpoint32_t fixpoint32_max(fixpoint32_t x, fixpoint32_t y) {
	if (x > y)
		return x;
		
	return y;
}


__always_inline__ fixpoint32_t fixpoint32_average(fixpoint32_t x, fixpoint32_t y) {
	uint64_t _x = (uint64_t)x;
	uint64_t _y = (uint64_t)y;

	return (_x + _y)/2;
}


__always_inline__ char fixpoint32_add(fixpoint32_t *dest, fixpoint32_t op1, fixpoint32_t op2) {
	uint64_t _op1, _op2, res;
	
	_op1 = (uint64_t)op1;
	_op2 = (uint64_t)op2;
	res = _op1 + _op2;
	*dest = (fixpoint32_t) res;

	if (res > FIXPOINT32_MAX)
		return ERR_FIXPOINT32_OVERFLOW;
		
	return 0;
}


__always_inline__ char fixpoint32_mul(fixpoint32_t *dest, fixpoint32_t op1, fixpoint32_t op2) {
	uint64_t _op1, _op2, res;
	
	_op1 = (uint64_t)op1;
	_op2 = (uint64_t)op2;
	res = (_op1*_op2) >> (sizeof(fixpoint32_t)*8);

	*dest = (fixpoint32_t) res;

	if (op1 && op2 && !res)
		return ERR_FIXPOINT32_UNDERFLOW;
		
	return 0;
}


__always_inline__ fixpoint32_t fixpoint32_from_fraction(uint16_t num, uint16_t den) {
	uint32_t n = (uint32_t)num;
	uint32_t d = (uint32_t)den;

	assert(num <= den);
	assert(den > 0);
	/* this `*2` is needed for fraction_from_fixpoint32 */
	assert(num <= NETWORK_MAX_SIZE*2);
	assert(den <= NETWORK_MAX_SIZE*2);
	
	if (num == den)
		return FIXPOINT32_MAX;

	/* 
	 * Approximate num/den
	 * ! the following code works iff
	 *   num <= (1024*2-1)=0x000007ff
	 */
	assert(num <= (1024*2-1));
	return ((n << 21)/d) << 11;
}


/*
 * Return the best approximation of fp in the form of a coprime
 * fraction with maximum denominator of NETWORK_MAX_SIZE
 */
fraction_t fixpoint32_to_coprime_fraction(fixpoint32_t fp);


#endif /* __FIXPOINT32_H__ */

