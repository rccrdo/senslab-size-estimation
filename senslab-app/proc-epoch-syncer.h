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

#ifndef __PROC_EPOCH_SYNCER_H__
#define __PROC_EPOCH_SYNCER_H__

#include "contiki.h"


/**
 * The epoch-syncer can posts two type of events
 *
 * - the first one is used to tell consumer processes tha the epoch synchro is now stable
 *   troughout the network. This event is signalled only once at startup as soon as the
 *   epoch-syncer has achieved sufficient syncronization between the nodes.
 *   The size estimator process will start running its algo only aftert this event has
 *   been signalled.
 *
 * - the second one is used to signal the end of an epoch: the size estimator process
 *   uses this event to sync its internal logic.
 */
extern process_event_t evt_epoch_synced;
extern process_event_t evt_end_of_epoch;


/*
 * We partition each epoch in this way
 *
 * ts  td1        tsyncs     td2    tsyncd   te
 * |---|-------------|--------|-------|------|
 * |   |             |        |       |      \----> epoch end
 * |   |             |        |       \-----------> from this time no sync packets can be sent anymore
 * |   |             |        \-------------------> nodes can't start a new consensus-related radio transmission after this time
 * |   |             \----------------------------> from this time the epoch-syncer can send sync packets: this time must as near as possible
 * |   |                                              to `te`
 * |   \------------------------------------------> nodes can start sharing information with their radios after td1
 * \----------------------------------------------> epoch starts, nodes update the various information with the packets received during
 *                                                    the previous epoch and generate the new numbers
 *
 * 
 * We call 'epoch start delay' the (constant) time td1-ts.
 * Introducing a small delay is necessary in order to allow the nodes
 * to finish the computation of the sufficient statistics, put them on the serial line etc,
 * all before data from the next epoch starts being transmitted.
 *
 * We call 'epoch end delay' the (constant) time te-td2.
 * A tx started near td2 will require some time to finish ...
 *
 * We call 'epoch interval' the (constant) time te-ts.
 */


/*
 * The number of epochs from startup that are necessary for all nodes to be synced
 */
#define EPOCHS_UNTIL_SYNCED      (10)

/*
 * Epoch timings to be used from reset until EPOCHS_UNTIL_SYNCED epochs elapsed
 */
#define EPOCH_INIT_INTERVAL           (CLOCK_SECOND*10)
#define EPOCH_INIT_SYNC_START         (CLOCK_SECOND*4)
#define EPOCH_INIT_SYNC_END           (CLOCK_SECOND*8)
#define EPOCH_INIT_SYNC_XFER_INTERVAL (EPOCH_INIT_SYNC_END - EPOCH_INIT_SYNC_START)


/*
 * `Run-time` epoch timings
 */
#define EPOCH_START_DELAY        (CLOCK_SECOND/2)
#define EPOCH_END_DELAY          (CLOCK_SECOND)
#define EPOCH_INTERVAL           (CLOCK_SECOND*10)
#define EPOCH_SYNC_START         (CLOCK_SECOND*3)
#define EPOCH_SYNC_END           (CLOCK_SECOND*9 + CLOCK_SECOND/2)

#define EPOCH_SYNC_XFER_INTERVAL (EPOCH_INIT_SYNC_END - EPOCH_INIT_SYNC_START)
#define EPOCH_XFER_INTERVAL      (EPOCH_INTERVAL - EPOCH_START_DELAY - EPOCH_END_DELAY)


/*
 * Sanity checks
 */
#if EPOCH_INIT_INTERVAL < 2*CLOCK_SECOND
#error choose an higher value for EPOCH_INIT_INTERVAL
#endif

#if EPOCH_INIT_SYNC_START < CLOCK_SECOND/2
#error choose an higher value for EPOCH_INIT_SYNC_START
#endif

#if EPOCH_INIT_SYNC_XFER_INTERVAL <= 2*CLOCK_SECOND
#error choose a longer EPOCH_INIT_SYNC_XFER_INTERVAL.
#endif


#if EPOCH_START_DELAY < CLOCK_SECOND/2
#error choose an higher value for EPOCH_START_DELAY
#endif

#if EPOCH_END_DELAY < CLOCK_SECOND/4
#error choose an higher value for EPOCH_END_DELAY.
#endif

#if EPOCH_XFER_INTERVAL <= CLOCK_SECOND
#error choose a longer EPOCH_INTERVAL.
#endif


#endif /* __PROC_EPOCH_SYNCER_H__ */

