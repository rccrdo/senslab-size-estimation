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

#include <assert.h>
#include "radio-arb.h"


static int __radio_locked = 0;


int radio_trylock(void) {
	if (__radio_locked)
		return -1;

	__radio_locked = 1;

	return 0;
}


void radio_unlock(void) {
	/*
	 * Defensive check against
	 * - double unlocks
	 * - unlocking from a `thread` not holding the lock
	 */
	assert(__radio_locked);

	__radio_locked = 0;
}


