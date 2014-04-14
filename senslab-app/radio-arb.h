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

#ifndef __RADIO_ARB_H__
#define __RADIO_ARB_H__

#include <stdint.h>


/*
 * Different tasks are assigned different broadcasting channels
 *
 * ! in Contiki a channel is any number in [0,2**16-1] and by convention
 *   the range [0,150] is reserved to system services.
 */
#define BROADCAST_CHANNEL_TIMESYNC 150
#define BROADCAST_CHANNEL_ESTIMATOR 151



/*
 * The radio-lock is used to arbiter access to the radio and the kernel's "rime"
 * packet buffer.
 *
 * Tasks must make sure they acquire the lock when they want to transmit data with
 * the radio. The lock must be acquired before manupulating the rime packet buffer.
 * The lock is to be released at xfer completion.
 *
 * ! the radio rx operation is not arbitered.
 */


/* 
 * Lock the radio (for tx)
 *
 * The return value is -1 on locking-failure (eg. because the radio lock is
 * already held by another process) or 0 when locking is successful.
 */
int radio_trylock(void);


/*
 * Unlock the radio
 *
 * This function should be called only by the current lock holder.
 */
void radio_unlock(void);


#endif /* __RADIO_ARB_H__ */

