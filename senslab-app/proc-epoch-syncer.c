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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "contiki.h"
#include "net/rime.h"
#include "util.h"
#include "proc-list.h"
#include "radio-arb.h"
#include "proc-epoch-syncer.h"
#include "distributions.h"
#include "size-estimator-conf.h"

#ifdef XFER_CRC16
#include "crc16.h"
#endif
#ifdef TRACK_CONNECTIONS
#include "connection-tracker.h"
#endif 


/*! 
 * \brief A struct storing the state of the epoch-syncer
 */
struct epoch_syncer {
	//! The current epoch index, a integer ranging from 0 up to 32767 (at least)
	int16_t epoch;

	//! The time at which the current epoch started in kernel ticks
	//  At a rate of 1000 ticks/s (Contiki does 128 ticks/s) a wrap-around comes after 500+ hours
	int32_t epoch_start_time;

	//! The time at which the current epoch will end in kernel ticks
	int32_t epoch_end_time;

	int32_t epoch_interval;
	int32_t epoch_sync_start;
	int32_t epoch_sync_xfer_interval;

	int32_t sum_sync_offsets;
	int32_t max_offset;
	int32_t min_offset;
	int16_t nr_offsets;
};


/*!
 * \brief This struct defines the epoch-sync packet format
 */
struct epoch_sync_packet {
#ifdef XFER_CRC16
	//! The packet 16-bit crc computed with the .crc16 field zeroed
	uint16_t crc16;
#endif
#ifdef TRACK_CONNECTIONS
	//! The sending node board-id16
	uint16_t board_id16;
#endif

	//! The sending node current epoch
	int16_t epoch;

	//! The sending node's time from the start of the current epoch (computed at send time and measedured in kernel ticks)
	int32_t time_from_epoch_start;

	//! The sending node's time to the end of the current epoch (computed at send time and measedured in kernel ticks)
	int32_t time_to_epoch_end;
};


/*!
 * Helper function to reset part of the epoch-syncer state at each epoch start.
 */
__always_inline__ void epoch_syncer_at_epoch_start(struct epoch_syncer *syncer) {
	assert(syncer != NULL);
	
	syncer->sum_sync_offsets = 0;
	syncer->nr_offsets = 0;
	syncer->max_offset = INT32_MIN;
	syncer->min_offset = INT32_MAX;
}


/*!
 * Init an epoch-syncer object.
 */
__always_inline__ void epoch_syncer_init(struct epoch_syncer *syncer) {
	assert(syncer != NULL);
	
	syncer->epoch = 0;
	syncer->epoch_start_time = -1;
	syncer->epoch_end_time = -1;

	// Setup epoch timings for the initial syncing period
	syncer->epoch_interval = EPOCH_INIT_INTERVAL;
	syncer->epoch_sync_start   = EPOCH_INIT_SYNC_START;
	syncer->epoch_sync_xfer_interval = EPOCH_INIT_SYNC_XFER_INTERVAL;
}


/*
 * The two events signalled by the epoch syncer
 */
process_event_t evt_epoch_synced;
process_event_t evt_end_of_epoch;


/*
 * The epoch-syncer object-instance.
 */
static struct epoch_syncer __epoch_syncer;


/*!
 * \brief This callback notifes us back that the sync-packet
 * transmission has come to completion: either it was successful or
 * (assuming the csma MAC layer is in use) the packet has been dropped
 * after too many retries .
 */
static void __broadcast_sent_cb(struct broadcast_conn *ptr, int status, int num_tx) {
	/*
	 * sync packet sent or dropped: release the radio so that other tasks can use it
	 */
	radio_unlock();
}


/*!
 *\brief This callback is called by the kernel when a packet is
 * received on the epoch-syncer broadcast channel. In here we compute
 * the time offsets that we use to control the epoch timing, e.g. by
 * delaying or anticipating the next epoch.
 */
static void __broadcast_recv_cb(struct broadcast_conn *ptr, const rimeaddr_t *sender) {
	int datalen;
	int distance_nr_epochs;
	long int now, offset;
	struct epoch_sync_packet packet;

	/*
	 * Get the current time first-thing; this will be used in the
	 * computation of the epoch offset
	 */
	now = clock_time();

	/*
	 * TODO: we could use the packet rssi to seed tha random number generator
	 *
	 * ! iff the radio operates on lower power modes: otherwise the rssi is the same
	 *   constant all the time
	 *
	 * if (0) {
	 *	uint16_t rssi;
         *
	 *	rssi = packetbuf_attr(PACKETBUF_ATTR_RSSI);
	 *	printf("rssi=0x%.4x\n", rssi);
	 * }
	 */

	datalen = packetbuf_datalen();
	if (datalen != sizeof(struct epoch_sync_packet)) {
		/*
		 * xfer corruption; happens rarely.
		 */
		trace("@%d sync xfer corruption, datalen %d\n", __epoch_syncer.epoch, datalen);
		return;
	}

	/*
	 * Copy the received packet into local storage
	 *
	 * ! the pointer returned by packetbuf_dataptr() is not necesserely aligned
	 *   wrt the mcu platform requirements, thus we cannot simply dereference
	 *   it into a 'struct epoch_sync_packet'. The msp430, in particular, can
	 *    have undefined behaviour when loading from unaligned addresses.
	 */
	memcpy(&packet, packetbuf_dataptr(), datalen);

#ifdef XFER_CRC16
	{
		uint16_t recv_crc16, crc16;

		/*
		 * Compute the received packet crc with the .crc16 field zeroed
		 */
		recv_crc16 = packet.crc16;

		packet.crc16 = 0;
		crc16 = crc16_data((const unsigned char *)&packet, sizeof(struct epoch_sync_packet), 0);

		if (recv_crc16 != crc16) {
			/*
			 * xfer corruption; happens rarely.
			 */
			trace("@%d sync xfer crc mismatch\n", __epoch_syncer.epoch);
			return;
		}
	}
#endif

	/*
	 * The packet is valid: compute the offset between our and the
	 * other node's `time to end of epoch`.
	 *
	 * ! If we are sufficiently out-of-sync than this node and the
	 *   sender node are currently in different epochs and we need to
	 *   special case a bit.
	 */
	offset = 0;
	distance_nr_epochs = __epoch_syncer.epoch - packet.epoch;
	if (distance_nr_epochs > 0) {
		/*
		 * We are going too fast but we don't care, we simply let slower
		 * nodes adjust to us.
		 *
		 * ! don't trace the sender and return.
		 */
		printf("epoch-syncer: discarding packet from epoch %d at epoch %d\n", packet.epoch, __epoch_syncer.epoch);
		return;
	} else if (distance_nr_epochs < 0) {
		long int time_to_epoch_end;
		/*
		 * We are going too slow !
		 */
		if (distance_nr_epochs == -1) {
			assert(__epoch_syncer.epoch_end_time > now);
			time_to_epoch_end = (long int)__epoch_syncer.epoch_end_time - now;
			offset = time_to_epoch_end + packet.time_from_epoch_start;
		} else {
			offset = __epoch_syncer.epoch_interval*(packet.epoch - __epoch_syncer.epoch);
		}
	} else {
		long int time_from_epoch_start, time_to_epoch_end;

		/* 
		 * Both this node and the sender node are in the same epoch.
		 */
		if (now > __epoch_syncer.epoch_end_time) {
			/*
			 * The epoch is expired but the epoch counter has not been updated yet.
			 * If this happens there is a *bug*: something is delaying the epoch_timer `is-expired`
			 * check in the process main loop.
			 *
			 * ! we can't and don't want to recover from this situation: go fix your changes in the code :)
			 */
			printf("@%d BUG epoch-syncer: packet received after end-of-epoch %ld\n", __epoch_syncer.epoch, now - (long int)__epoch_syncer.epoch_end_time);
			return;
		}

		/*
		 * compute this node's `time from epoch start` and `time to epoch end`
		 */
		assert(now <= __epoch_syncer.epoch_end_time);
		assert(__epoch_syncer.epoch_start_time <= now);
		assert(__epoch_syncer.epoch_end_time >__epoch_syncer.epoch_start_time);
		time_from_epoch_start = now - __epoch_syncer.epoch_start_time;

		assert(time_from_epoch_start >= 0);
		assert(time_from_epoch_start < (__epoch_syncer.epoch_end_time -__epoch_syncer.epoch_start_time));
		time_to_epoch_end = __epoch_syncer.epoch_end_time - now;

		/*
		 * compute the `time to epoch end` offset between this node and the sender node
		 */
		offset = time_to_epoch_end - packet.time_to_epoch_end;

		/*
		 * linearly interpolate to guess the eventual offset at end of this node epoch
		 */
		offset = (offset*__epoch_syncer.epoch_interval)/time_from_epoch_start;
	}

	/* update offset statistics */
	__epoch_syncer.max_offset = max(offset, __epoch_syncer.max_offset);
	__epoch_syncer.min_offset = min(offset, __epoch_syncer.min_offset);

	/*
	 * Accumulate the offsets: from this sum the `offset-average` can be computed.
	 * This average is then used to adjust the timing of the next epoch.
	 */
	__epoch_syncer.sum_sync_offsets += offset;
	__epoch_syncer.nr_offsets++;

#ifdef TRACK_CONNECTIONS
	/* trace the xfer */
	connection_track(CONNECTION_TRACK_SYNC, packet.board_id16, __epoch_syncer.epoch);
#endif
}



PROCESS_THREAD(proc_epoch_syncer, ev, data) {
	static struct etimer send_timer;
	static struct etimer epoch_timer;
	static const struct broadcast_callbacks broadcast_cbs = {__broadcast_recv_cb, __broadcast_sent_cb};
	static struct broadcast_conn conn;

	PROCESS_EXITHANDLER(broadcast_close(&conn));

	PROCESS_BEGIN();


#ifdef TRACK_CONNECTIONS
	/* Log the node id */
	printf("board-id64 0x%.16llx\n", board_get_id64());
#endif
#ifdef XFER_CRC16
	/* Log the node id */
	printf("xfer crc16\n");
#endif
	printf("epoch interval %ld ticks\n", EPOCH_INTERVAL);

	/*
	 * Alloc the two syncer events
	 */
	evt_epoch_synced = process_alloc_event();
	evt_end_of_epoch = process_alloc_event();

	/*
	 * Open a `connection` on the syncer broadcasting channel
	 */
	broadcast_open(&conn, BROADCAST_CHANNEL_TIMESYNC, &broadcast_cbs);

	/*
	 * init the epoch-syncer instance
	 */
	epoch_syncer_init(&__epoch_syncer);

	/*
	 * This is the main syncer loop. Initially we try to sync the
	 * epoch between nodes without concurrently running any other
	 * algo. After a period, at which time the network is synced,
	 * we start generating epoch events which can be
	 * consumed by, e.g., the estimator process.
	 */
	etimer_set(&epoch_timer, __epoch_syncer.epoch_interval);
	__epoch_syncer.epoch_start_time = clock_time();
	__epoch_syncer.epoch_end_time = etimer_expiration_time(&epoch_timer);
	while (1) {
		/*
		 * The start of a new epoch !
		 */
		epoch_syncer_at_epoch_start(&__epoch_syncer);


		clock_time_t now;
		clock_time_t time_to_epoch_end;
			
		now = clock_time();

		assert(__epoch_syncer.epoch_end_time == etimer_expiration_time(&epoch_timer));
		assert(__epoch_syncer.epoch_end_time > now);
		time_to_epoch_end = __epoch_syncer.epoch_end_time - now;

		/* 
		 * Setup a random wait time before sending the sync packet
		 *
		 * ! we cannot let send_timer delay the epoch_timer, especially
		 *   when the next `end-of-epoch-time` has been anticipated by a lot
		 *   (this can happen at startup)
		 */
		if (time_to_epoch_end > __epoch_syncer.epoch_sync_start) {
			long int send_wait;
			long int send_wait_rnd;
			long int rnd;

			rnd = rand();
			send_wait_rnd = (unsigned)rnd % (unsigned) __epoch_syncer.epoch_sync_xfer_interval;
			send_wait = __epoch_syncer.epoch_sync_start + send_wait_rnd;
			assert(send_wait >= __epoch_syncer.epoch_sync_start);
			assert(send_wait <= __epoch_syncer.epoch_sync_start + __epoch_syncer.epoch_sync_xfer_interval);

			if (send_wait > time_to_epoch_end)
				send_wait = __epoch_syncer.epoch_sync_start;

			assert(send_wait < time_to_epoch_end);
			etimer_set(&send_timer, send_wait);

			PROCESS_WAIT_UNTIL(etimer_expired(&send_timer));

			/*
			 * Acquire the radio lock
			 *
			 * ! we don't use WAIT/YIELD_UNTIL() because
			 *   1) we do not want to yield if we can acquire the lock on the first try
			 *   2) no kernel signal is generated when the lock is released (we would `deadlock')
			 */
			do {
				if (!radio_trylock())
					break;

				PROCESS_PAUSE();
			} while (1);


			{
				clock_time_t now;
				struct epoch_sync_packet packet;
					
				/*
				 * broadcast the sync packet
				 *
				 * ! We put this part into its own block since non static stack
				 * variables/allocations in the parent block wouldn't get preserved trough
				 * kernel calls (e.g. the PROCESS_PAUSE() a few lines above)
				 */
#ifdef TRACK_CONNECTIONS
				packet.board_id16 = board_get_id16();
#endif
				packet.epoch = __epoch_syncer.epoch;

				now = clock_time();
				assert(now > __epoch_syncer.epoch_start_time);
				assert(__epoch_syncer.epoch_end_time > now);
				packet.time_from_epoch_start = now - __epoch_syncer.epoch_start_time;
				packet.time_to_epoch_end = __epoch_syncer.epoch_end_time - now;

				
#ifdef XFER_CRC16
				/*
				 * Compute the packet crc with the .crc16 field zeroed
				 */
				{
					uint16_t crc16;

					packet.crc16 = 0;
					crc16 = crc16_data((const unsigned char *)&packet,  sizeof(struct epoch_sync_packet), 0);

					packet.crc16 = crc16;
				}
#endif
				packetbuf_copyfrom(&packet, sizeof(struct epoch_sync_packet));
				broadcast_send(&conn);
			}
		} else {
			printf("epoch-syncer: skipping sync send\n");
		}
			

		/*
		 * We cannot YIELD here: if epoch_timer has already expired there won't be
		 * any event to wake us up.
		 *
		 * FIXME: if we get here and the epoch timer has fired
		 * already print by how much we are late: this can be terribly useful
		 * to trace bugs in the epoch sync code or the kernel.
		 */
		if (etimer_expired(&epoch_timer)) {
			long int now;

			now = clock_time();
			assert(now > __epoch_syncer.epoch_end_time);
			
		} else {
			char do_wait;
			do_wait = 1;

			if (__epoch_syncer.sum_sync_offsets) {
				long int avg_offset = __epoch_syncer.sum_sync_offsets / __epoch_syncer.nr_offsets;
				const long int threshold = CLOCK_SECOND;

				if (avg_offset > threshold) {
					/*
					 * if we are late don't wait until the timer expires
					 * ! this migth give us the opportunity to re-enter the right sync_xfer_interval
					 */
					do_wait = 0;
				} else if (avg_offset < -threshold) {
					/*
					 * we are too fast, delay end of epoch
					 */
					clock_time_t now;
					clock_time_t time_to_epoch_end;
			
					now = clock_time();
					assert(__epoch_syncer.epoch_end_time == etimer_expiration_time(&epoch_timer));
					assert(__epoch_syncer.epoch_end_time > now);
					time_to_epoch_end = __epoch_syncer.epoch_end_time - now;

					long int delay = time_to_epoch_end + (-avg_offset/2);

					static struct etimer delay_timer;
					trace("epoch-syncer: delaying end-of-epoch by %ld ticks\n", (-avg_offset/2));
					etimer_set(&delay_timer, delay);
					__epoch_syncer.epoch_end_time += (-avg_offset/2);

					PROCESS_WAIT_UNTIL(etimer_expired(&delay_timer));
				}
			}

			if (do_wait) {
				PROCESS_WAIT_UNTIL(etimer_expired(&epoch_timer));
			} else {
				trace("epoch-syncer: not waiting for end-of-epoch\n");
			}
		}
		trace("epoch-syncer: epoch %d ended\n",  __epoch_syncer.epoch);

#ifdef TRACK_CONNECTIONS
		connection_print_and_zero(CONNECTION_TRACK_SYNC, __epoch_syncer.epoch);
#endif

		/*
		 * Re-Set the end-of-epoch timer
		 */
		if (__epoch_syncer.epoch == EPOCHS_UNTIL_SYNCED) {
			/*
			 * We have hopefully achieved sync at this point
			 *
			 * 1) update the epoch timings, and set the epoch timer
			 *
			 * 2) signal the size-estimator process that the epoch is now synced
			 */
			__epoch_syncer.epoch_interval = EPOCH_INTERVAL;
			__epoch_syncer.epoch_sync_start = EPOCH_SYNC_START;
			__epoch_syncer.epoch_sync_xfer_interval = EPOCH_SYNC_XFER_INTERVAL;

			etimer_stop(&epoch_timer);
			etimer_set(&epoch_timer, __epoch_syncer.epoch_interval);
			/*
			 * The epoch timer has been re-set: update the time until the next epoch end
			 * Increase the epoch count.
			 * ! these operations must happen in a block which cannot block in kernel calls
			 */
			__epoch_syncer.epoch_start_time = clock_time();
			__epoch_syncer.epoch_end_time = etimer_expiration_time(&epoch_timer);
			__epoch_syncer.epoch++;

			process_post(&proc_size_estimator, evt_epoch_synced, NULL);
		} else {
			/*
			 * Re-set and adjust the epoch timer using the data received trough sync packets
			 * (in this epoch)
			 *
			 * ! using re-set (instead of, e.g., restart) is important here in order to avoid
			 *   drifting
			 */ 
			etimer_reset(&epoch_timer);

			/*
			 * The epoch timer has been re-set: update the time until the next epoch end
			 * Increase the epoch count.
			 * ! these operations must happen in a block which cannot block in kernel calls
			 */
			//__epoch_syncer.epoch_start_time = epoch_timer.timer.start;
			__epoch_syncer.epoch_start_time = clock_time();
			__epoch_syncer.epoch_end_time = etimer_expiration_time(&epoch_timer);
			__epoch_syncer.epoch++;
			if (__epoch_syncer.sum_sync_offsets) {
				long int avg_offset = __epoch_syncer.sum_sync_offsets / __epoch_syncer.nr_offsets;
				const long int threshold = 1;//(CLOCK_SECOND/32);//*3;

#if __CONTIKI_NETSTACK_RDC==__CONTIKI_NETSTACK_RDC_NULL
				const int tx_delay = 0;
#elif __CONTIKI_NETSTACK_RDC==__CONTIKI_NETSTACK_RDC_CXMAC
				/*
				 * When the cxmac RDC is used we must consider an added delay due to the fact that when
				 * other nodes radios are turned off the sync packet must be re-sent.
				 */
				const int tx_delay = 8;
#endif

				/*
				 * estimate the avg tx delay
				 */
				avg_offset += tx_delay;

				trace("epoch-syncer: sync offsets %d ~ %ld < %ld < %ld\n", __epoch_syncer.nr_offsets,  __epoch_syncer.min_offset + tx_delay, avg_offset, __epoch_syncer.max_offset+tx_delay);
				
				if ((avg_offset < -threshold) || (avg_offset > threshold)) {
					clock_time_t new_expiration_time;

					const long int adjust_threshold = CLOCK_SECOND/2;
					long int adjust;
		
					/*
					 * feedback control the next expiration time
					 */
					adjust = -avg_offset/2;
					adjust = min(adjust, adjust_threshold);
					adjust = max(adjust, -adjust_threshold);

					if (adjust)
						etimer_adjust(&epoch_timer, adjust);
						
					new_expiration_time = etimer_expiration_time(&epoch_timer);
					__epoch_syncer.epoch_end_time = new_expiration_time;
				}
			}

			if (__epoch_syncer.epoch > EPOCHS_UNTIL_SYNCED) {
				/*
				 * Signal the estimator-process that this epoch has ended
				 */
				process_post(&proc_size_estimator, evt_end_of_epoch, NULL);
			}
		}
	}

	PROCESS_END();
}
