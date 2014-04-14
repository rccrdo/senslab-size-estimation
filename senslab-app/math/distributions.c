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

#include <stdint.h>
#include <assert.h>
#include <stdlib.h>
#include "distributions.h"


uint32_t _rng_state[5];

void distribution_seed(uint64_t seed) {
	int i;
	uint32_t _seed = seed & 0xffffffffull;
	uint32_t s = _seed;

	assert(_seed);

	// make random numbers and put them into the buffer
	for (i = 0; i < 5; i++) {
		s = s * 29943829ull - 1;
		_rng_state[i] = s;
	}

	// randomize some more
	for (i=0; i<50; i++)
		distribution_uniform_sample();
}



