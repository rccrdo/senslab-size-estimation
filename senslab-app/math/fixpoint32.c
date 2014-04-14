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

#include "fixpoint32.h"


/*
 * This function returns the best approximation of fp in the form of a coprime
 * fraction with maximum denominator of NETWORK_MAX_SIZE
 *
 * ! The algorithm is related to Farey sequence:
 *   http://www.johndcook.com/blog/2010/10/20/best-rational-approximation/
 */
fraction_t fixpoint32_to_coprime_fraction(fixpoint32_t fp) {
	uint16_t a, b, c, d;
	fraction_t frac;

	a = 0; 
	b = 1;
	c = 1;
	d = 1;
	while (b <= NETWORK_MAX_SIZE && d <= NETWORK_MAX_SIZE) {
		fixpoint32_t  mediant;

		mediant = fixpoint32_from_fraction(a + c, b + d);
		if (fp == mediant) {
			if (b + d <= NETWORK_MAX_SIZE) {
				frac.num = a + c;
				frac.den = b + d;
				return frac;
			} else if (d > b) {
				frac.num = c;
				frac.den = d;
				return frac;
			} else  {
				frac.num = a;
				frac.den = b;
				return frac;
			}
		} else if (fp > mediant) {
			a = a + c;
			b = b + d;
		} else {
			c = a + c;
			d = b + d;
		}
	}

	if (b  > NETWORK_MAX_SIZE) {
		frac.num = c;
		frac.den = d;
	} else {
		frac.num = a;
		frac.den = b;
	}

	return frac;
}

