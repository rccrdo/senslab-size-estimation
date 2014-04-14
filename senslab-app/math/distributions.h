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

#ifndef __DISTRIBUTIONS_H__
#define __DISTRIBUTIONS_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "util.h"
#include "fixpoint32.h"


extern uint32_t _rng_state[5];

/*
 * Get a uniform sample in [0,1) encoded in a fixpoint32_t
 *
 * This is one of the `mother of all'-type RNG released in the public
 * domain by George Marsaglia
 *
 * ! unfortunately the rand() implementation from libc is not up to the task.
 */
__always_inline__ fixpoint32_t distribution_uniform_sample(void) {
	uint64_t sum;

	sum = (uint64_t)_rng_state[3] * 2111111111ull +
		(uint64_t)_rng_state[2] * 1492ull +
		(uint64_t)_rng_state[1] * 1776ull +
		(uint64_t)_rng_state[0] * 5115ull +
		(uint64_t)_rng_state[4];

	_rng_state[3] = _rng_state[2];
	_rng_state[2] = _rng_state[1];
	_rng_state[1] = _rng_state[0];
	_rng_state[4] = (uint32_t)(sum >> 32);
	_rng_state[0] = (uint32_t)sum;

	if (DBG_DISTRIBUTION_UNIFORM)
		dbg("uniform_sample 0x%.8lx\n", (unsigned long int)_rng_state[0]);

	return _rng_state[0];
}

/*
 * set the seed of the pseudo-random number generator
 * ! it is excidingly important that both the lower and the upper 32bits
 *   of the seed differ from node to node
 */
void distribution_seed(uint64_t seed);


#endif /* __DISTRIBUTIONS_H__ */
