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

#include <contiki.h>
#include "math/distributions.h"
#include "matrix.h"
#include "uni-size-estimator.h"

#define __UNIFORM_SIZE_ESTIMATOR_NR_DATA_CELLS (UNIFORM_SIZE_ESTIMATOR_M*UNIFORM_SIZE_ESTIMATOR_D)

static fractional16_t __epoch_start_data_storage[__UNIFORM_SIZE_ESTIMATOR_NR_DATA_CELLS];
static fractional16_t __consensus_mat_storage[__UNIFORM_SIZE_ESTIMATOR_NR_DATA_CELLS];


/*
 * Compute the sufficient statistics \prod_{m=1}^{M} f_k,m(t) for k = 1,...,D
 */
static void __compute_sufficient_statistics(struct uniform_size_estimator *estim) {
	uint16_t col;

	assert(estim != NULL);
	for (col=0; col<UNIFORM_SIZE_ESTIMATOR_D; col++) {
		fractional16_t *cell;
		fractional48_t *suffstat;
		struct column_iter iter;

		suffstat = &estim->sufficient_stats[col];

		cell = NULL;
		column_iter_init(&iter, &estim->consensus_mat, col);
		column_iter_next(&iter, &cell);
		assert(cell != NULL);
	        fractional48_init(suffstat, fractional16_to_fixpoint32(*cell));

		while (!column_iter_next(&iter, &cell)) {
			/* 
			 * multiply the remaining UNIFOR_M-1 values
			 */
			fractional48_mul(suffstat, fractional16_to_fixpoint32(*cell));
		}
	}
}


static void _enable(struct uniform_size_estimator *estim) {
	uint16_t col;
	assert(estim != NULL);

	/*
	 * Fill the whole DxN matrix with samples from ~ U[0,1]
	 */
	for (col=0; col<UNIFORM_SIZE_ESTIMATOR_D; col++) {
		fractional16_t *cell;
		struct column_iter iter;

		column_iter_init(&iter, &estim->consensus_mat, col);
		while (!column_iter_next(&iter, &cell)) {
			*cell = fixpoint32_to_fractional16(distribution_uniform_sample());
		}
	}

	matrix_rawcopy_to_array(&estim->consensus_mat, __epoch_start_data_storage);

	estim->enabled = 1;
}


void uni_size_estimator_enable(struct uniform_size_estimator *estim) {
	assert(estim != NULL);
	assert(!estim->enabled);
	_enable(estim);
}


void uni_size_estimator_init(struct uniform_size_estimator *estim) {
	assert(estim != NULL);
	
	/* This info is used at post-processing time */
	printf("size-estimator: M=%d, D=%d\n", UNIFORM_SIZE_ESTIMATOR_M, UNIFORM_SIZE_ESTIMATOR_D);

	estim->epoch = 0;
	
	matrix_init(&estim->consensus_mat,
		    __consensus_mat_storage,
		    UNIFORM_SIZE_ESTIMATOR_M,
		    UNIFORM_SIZE_ESTIMATOR_D);

	_enable(estim);
}


void uni_size_estimator_jump_to_epoch(struct uniform_size_estimator *estim, uint16_t nr_epochs) {
	uint16_t new_epoch;
	assert(estim != NULL);

	new_epoch = estim->epoch + nr_epochs;
	
	printf("size-estimator: jumping epoch %d -> %d\n", estim->epoch, new_epoch);

	estim->epoch = new_epoch;
}


void uni_size_estimator_at_epoch_start(struct uniform_size_estimator *estim) {
	uint16_t col;
	fractional16_t *cell;
	struct column_iter iter;
		
	assert(estim != NULL);

	if (!estim->enabled) {
		estim->epoch++;
		return;
	}

	__compute_sufficient_statistics(estim);

	/*
	 * Log the sufficient statistics to the serial line
	 */
	printf("@%d stats", estim->epoch);
	for (col=0; col<UNIFORM_SIZE_ESTIMATOR_D; col++) {
		fractional48_t *stat;

		stat = &estim->sufficient_stats[col];
		printf(" %.8lx.%d", (unsigned long int)stat->value, stat->exp);
	}
	printf("\n");


	estim->epoch++;

	/* Shift one `column' out and resample the common uniform distribution */
	matrix_shift(&estim->consensus_mat);
	column_iter_init(&iter, &estim->consensus_mat, 0);

	while (!column_iter_next(&iter, &cell))
		*cell = fixpoint32_to_fractional16(distribution_uniform_sample());

	/* the new epoch_start_mat is the current epoch_end_mat */	
	matrix_rawcopy_to_array(&estim->consensus_mat, __epoch_start_data_storage);

	/* re-init the packet-splitter */
	packet_splitter_init(&estim->splitter, estim->epoch, (const char *)__epoch_start_data_storage, estim->consensus_mat.datalen);
}

