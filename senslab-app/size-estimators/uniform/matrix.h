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

#ifndef __MATRIX_H__
#define __MATRIX_H__

#include <assert.h>
#include <stdint.h>
#include "fractional16.h"

/*
 * Error returned when trying to read a column iterator past the column end
 */
#define ERR_COLUMN_ITER_END -1


struct matrix {
	fractional16_t *data;
	uint16_t nr_rows;
	uint16_t nr_cols;
	uint16_t datalen;
	uint16_t start_col;
};


struct column_iter {
	struct matrix *mat;
	fractional16_t *column;
	int row;
};


__always_inline__ uint16_t __column_index(struct matrix *mat, uint16_t col) {
	int _col;
	assert(mat != NULL);
	assert(col >= 0);
	assert(col < mat->nr_cols);
	assert(mat->start_col >=0);
	assert(mat->start_col < mat->nr_cols);

	_col = mat->start_col - col;
	if (_col < 0)
		_col += mat->nr_cols;
	
	assert(_col >= 0);
	assert(_col < mat->nr_cols);

	col =  (uint16_t)_col;

	return (uint16_t)col;
}


__always_inline__ void column_iter_init(struct column_iter *iter, struct matrix *mat, uint16_t col) {
	uint16_t _col;

	assert(mat != NULL);
	assert(iter != NULL);
	assert(col >= 0);
	assert(col < mat->nr_cols);

	_col = __column_index(mat, col);
	
	iter->mat = mat;
	iter->row = 0;
	iter->column = &mat->data[_col*mat->nr_rows];
}


__always_inline__ uint16_t column_iter_next(struct column_iter *iter, fractional16_t **cell_ptr) {
	assert(iter != NULL);
	assert(cell_ptr != NULL);
	
	if (iter->row == iter->mat->nr_rows)
		return ERR_COLUMN_ITER_END;

	*cell_ptr = &iter->column[iter->row];
	iter->row++;

	return 0;
}


__always_inline__ void matrix_init(struct matrix *mat, fractional16_t *data, uint16_t nr_rows, uint16_t nr_cols) {
	assert(mat != NULL);
	assert(data != NULL);
	assert(nr_rows > 0);
	assert(nr_cols > 0);	

	mat->data = data;
	mat->nr_rows = nr_rows;
	mat->nr_cols = nr_cols;
	mat->datalen = nr_rows*nr_cols*sizeof(fractional16_t);
	mat->start_col = 0;
}


__always_inline__ fractional16_t matrix_get(struct matrix *mat, uint16_t row, uint16_t col) {
	uint16_t _col;

	assert(mat != NULL);
	assert(row >= 0);
	assert(row < mat->nr_rows);
	assert(col >= 0);
	assert(col < mat->nr_cols);

	_col = __column_index(mat, col);

	return mat->data[_col*mat->nr_rows + row];
}


__always_inline__ void matrix_set(struct matrix *mat, uint16_t row, uint16_t col, fractional16_t value) {
	uint16_t _col;

	assert(mat != NULL);
	assert(row >= 0);
	assert(row < mat->nr_rows);
	assert(col >= 0);
	assert(col < mat->nr_cols);

	_col = __column_index(mat, col);

	mat->data[_col*mat->nr_rows + row] = value;
}


__always_inline__ void matrix_shift(struct matrix *mat) {
	assert(mat != NULL);
	assert(mat->start_col >= 0);
	assert(mat->start_col < mat->nr_cols);

	mat->start_col++;
	if (mat->start_col == mat->nr_cols)
		mat->start_col = 0;
}


__always_inline__ void matrix_rawcopy_to_array(struct matrix *mat, fractional16_t *dest) {
	uint16_t i;

	assert(mat != NULL);
	assert(mat->data != NULL);
	assert(dest != NULL);
	
	for (i=0; i < mat->nr_rows*mat->nr_cols; i++)
		dest[i] = mat->data[i];
}

#endif /* __MATRIX_H__ */

