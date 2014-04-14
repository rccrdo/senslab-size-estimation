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

#ifndef __UNIFORM_SIZE_ESTIMATOR_H__
#define __UNIFORM_SIZE_ESTIMATOR_H__

#include <stdlib.h>
#include "util.h"
#include "network.h"
#include "fixpoint32.h"
#include "fractional16.h"
#include "fractional48.h"
#include "matrix.h"
#include "packet-splitter.h"


/*
 * Parameters of the size estimator
 *
 * M, the number of scalars resampled at the start of each new epoch
 * D, the farthest k-steps neighborhood we consider
 */
#define UNIFORM_SIZE_ESTIMATOR_M 	100
#define UNIFORM_SIZE_ESTIMATOR_D	7


struct uniform_size_estimator {
	char enabled;
	uint16_t epoch;
	struct matrix consensus_mat;
	fractional48_t sufficient_stats[UNIFORM_SIZE_ESTIMATOR_D];
	
	/* The embedded packet-splitter object */
	struct packet_splitter splitter;
};


void uni_size_estimator_init(struct uniform_size_estimator *estim);

void uni_size_estimator_jump_to_epoch(struct uniform_size_estimator *estim, uint16_t nr_epochs);

void uni_size_estimator_at_epoch_start(struct uniform_size_estimator *estim);


__always_inline__ uint16_t uni_size_estimator_queue_packet(struct uniform_size_estimator *estim) {
	assert(estim != NULL);
	
	return packet_splitter_queue(&estim->splitter);
}


__always_inline__ uint16_t uni_size_estimator_get_current_epoch(struct uniform_size_estimator *estim) {
	assert(estim != NULL);
	
	return estim->epoch;
}


void uni_size_estimator_enable(struct uniform_size_estimator *estim);


__always_inline__ void uni_size_estimator_disable(struct uniform_size_estimator *estim) {
	assert(estim != NULL);
	assert(estim->enabled);
	estim->enabled = 0;
}


__always_inline__ char uni_size_estimator_enabled(struct uniform_size_estimator *estim) {
	assert(estim != NULL);
	return estim->enabled;
}

#endif /* __UNIFORM_SIZE_ESTIMATOR_H__ */

