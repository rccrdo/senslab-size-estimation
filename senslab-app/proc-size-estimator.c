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
#include "ds2411.h"
#include "contiki.h"
#include "net/rime.h"
#include "math/fractional16.h"
#include "net/packet-splitter.h"
#include "radio-arb.h"
#include "proc-list.h"
#include "proc-epoch-syncer.h"
#include "size-estimators/uniform/uni-size-estimator.h"
#include "distributions.h"
#include "size-estimator-conf.h"
#ifdef XFER_CRC16
#include "crc16.h"
#endif
#ifdef TRACK_CONNECTIONS
#include "connection-tracker.h"
#endif


//#define TEST_RADIO_POWER_RADIUS 
#define TEST_POWER_OUTAGE
//#define TEST_NETWORK_SPLITTING

#ifdef TEST_RADIO_POWER_RADIUS
#define TEST_RADIO_POWER
#ifdef TEST_POWER_OUTAGE
#error select just one test
#endif
#ifdef TEST_NETWORK_SPLITTING
#error select just one test
#endif
#endif

#ifdef TEST_POWER_OUTAGE
#define TEST_RADIO_POWER
#ifdef TEST_RADIO_POWER_RADIUS
#error select just one test
#endif
#ifdef TEST_NETWORK_SPLITTING
#error select just one test
#endif
#endif

#ifdef  TEST_NETWORK_SPLITTING
#define TEST_RADIO_POWER
#ifdef TEST_POWER_OUTAGE
#error select just one test
#endif
#ifdef TEST_RADIO_POWER_RADIUS
#error select just one test
#endif
#endif

#ifdef TEST_RADIO_POWER
#include "cc2420.h"
#include "cc1100.h"
#include "cc1100-radio.h"
#include "cc2420-radio.h"
#endif

static volatile int __min_packet_id;
static volatile int __max_packet_id;

/*
 * This event is used to signal the size-estimator process that the consensus packet
 * has been sent and that, now, a new xfer can be started.
 */
static process_event_t evt_consensus_packet_sent;


/*
 * The size-estimator instance.
 */
static struct uniform_size_estimator __size_estimator;

/*!
 * \brief This callback notifes us back that the consensus-packet transmission has come to completion:
 * either it was successful or the packet has been dropped after too many retries (assuming
 * the csma MAC layer is in use).
 */
static void __broadcast_sent_cb(struct broadcast_conn *ptr, int status, int num_tx) {
	/*
	 * signal back the estimator process that the latest queued packet has been sent
	 */
	process_post(&proc_size_estimator, evt_consensus_packet_sent, NULL);
}


/*
 * Be defensive: the code depends on the PAYLOAD being a multiple of
 * sizeof(fractional16_t)=2: double check.
 */
#if PACKET_SPLITTER_PAYLOAD_LEN & 1
#error please make PAKCET_SPLITTER_PAYLOAD_LEN a multiple of 2
#endif

static void __broadcast_recv_cb(struct broadcast_conn *ptr, const rimeaddr_t *sender) {
	uint16_t i;
	uint16_t datalen;
	fractional16_t *localdata_cur;
	fractional16_t *payload_cur;
	fractional16_t payload[PACKET_SPLITTER_PAYLOAD_LEN/sizeof(fractional16_t)];
	uint16_t nr_fractionals;
	struct split_packet_hdr packet_hdr;
	struct split_packet *packet;

	if (!uni_size_estimator_enabled(&__size_estimator))
		return;

	/*
	 * warning: packetbuf_dataptr() could be mis-aligned (worth the
	 * platform aligning requirements) dereferecing members of this
	 * struct can have undefined behaviour
	 */
	packet = packetbuf_dataptr();
	datalen = packetbuf_datalen();
	if (datalen <= sizeof(struct split_packet_hdr)) {
		/*
		 * xfer corruption; happens rarely.
		 */
		trace("@%d data xfer corruption, datalen %d\n", __size_estimator.epoch, datalen);
		return;
	}

	memcpy(&packet_hdr, packet, sizeof(struct split_packet_hdr));

#ifdef XFER_CRC16
	/*
	 * Compute the packet crc with the .crc16 field zeroed
	 */
	{
		uint16_t crc16;

		/*
		 * The .crc16 field comes first in struct split_packet
		 * ! we don't dereference packet, i.e. we don't do packet->crc16 = 0,
		 *   since packet migth not be 2-byte aligned
		 */
		((char *)packet)[0] = 0;
		((char *)packet)[1] = 0;
		crc16 = crc16_data((const unsigned char *)packet, datalen, 0);

		if (packet_hdr.crc16 != crc16) {
			printf("@%d data xfer crc mismatch\n", __size_estimator.epoch);
			return;
		}
	}
#endif

	if (__size_estimator.epoch != packet_hdr.epoch) {
		/*
		 * We can't use this packet, log and return.
		 */
		printf("size-estimator: discard packet from epoch %d at epoch %d\n", packet_hdr.epoch, __size_estimator.epoch);
		return;
	}

	/*
	 * check if the data field is mis-aligned: if it is we need to
	 * first copy the payload on a correctly aligned storage, if it is *not
	 * mis-aligned* we can spare some cycles
	 */
	payload_cur = (fractional16_t *)&(packet->data[0]);
	if ((int)payload_cur & 1) {
		printf("size-estimator: uff payload is mis-aligned\n");
		memcpy(payload, &packet->data, packet_hdr.payloadlen);
		payload_cur = payload;
	}
	  
	/*
	 * max consensus
	 *
	 * 1) we always send the matrix data in storage order irrespective of the
	 *    current matrix shift
	 * 2) all consensus packets (but possibly the last one) carry
	 *    exactly PACKET_SPLITTER_PAYLOAD_LEN/sizeof(fractional16_t) fractionals
	 * => we can use the sequential packet_id to recover the indeces to use for consensus
	 */
	nr_fractionals = packet_hdr.payloadlen/sizeof(fractional16_t);
	localdata_cur = &__size_estimator.consensus_mat.data[packet_hdr.packet_id*((PACKET_SPLITTER_PAYLOAD_LEN)/sizeof(fractional16_t))];
	for (i=0; i < nr_fractionals; i++) {
		*localdata_cur = fractional16_max(*localdata_cur, *payload_cur);
		localdata_cur++;
		payload_cur++;
	}
	
	__max_packet_id = max(__max_packet_id, packet_hdr.packet_id);
	__min_packet_id = min(__min_packet_id, packet_hdr.packet_id);

#ifdef TRACK_CONNECTIONS
	connection_track(CONNECTION_TRACK_DATA, packet_hdr.nodeid, __size_estimator.epoch);
#endif
}



PROCESS_THREAD(proc_size_estimator, ev, data) {
	static struct etimer send_timer;
	static const struct broadcast_callbacks broadcast_cbs = {__broadcast_recv_cb, __broadcast_sent_cb};
	static struct broadcast_conn conn;
	
	PROCESS_EXITHANDLER(broadcast_close(&conn));

	PROCESS_BEGIN();


#if defined(TEST_POWER_OUTAGE) || defined(TEST_NETWORK_SPLITTING)
	/* for all nodes */
	do {
		if (!radio_trylock())
			break;
		PROCESS_PAUSE();
	} while (1);

#ifdef WITH_CC1100
	{
		/*
		 * tx power [dbm]    -30  | -20  | -15 |  -10 |  -5  |   0  |  5   |  7   |  10
		 * patable setting   0x03 | 0x0d | 0x1c| 0x34 | 0x57 | 0x8e | 0x85 | 0xcc | 0xc3
		 */
		uint8_t tx_power = 0xc3;

		printf("@%d setting cc1100 tx-power with %d\n", __size_estimator.epoch, tx_power);

		cc1100_radio_init_with_power(tx_power);
	}
#else
#error Changing the radio tx power is supported only on CC1100
#endif

	radio_unlock();
#endif

	/*
	 * initialize the seed of the random-number-generator
	 */
	distribution_seed(board_get_id64());

	/*
	 * Init the size-estimator object
	 */
	uni_size_estimator_init(&__size_estimator);
	uni_size_estimator_jump_to_epoch(&__size_estimator, EPOCHS_UNTIL_SYNCED);

	/*
	 * Allocate the `consensus packet sent` event
	 */ 
	evt_consensus_packet_sent = process_alloc_event();

	/*
	 * Open a `connection` on the estimator broadcasting channel
	 */
	broadcast_open(&conn, BROADCAST_CHANNEL_ESTIMATOR, &broadcast_cbs);

	/*
	 * Now wait until the epoch syncer gives us the `start`
	 */
	PROCESS_WAIT_EVENT_UNTIL(ev == evt_epoch_synced);

	/*
	 * Enter the main estimator loop
	 */
	do {

#ifdef TEST_NETWORK_SPLITTING
		/*
		 * Test: Network `splitting`
		 */ 
		if (1) {
			if (__size_estimator.epoch == 59 || __size_estimator.epoch == 119 || __size_estimator.epoch == 179)
				uni_size_estimator_disable(&__size_estimator);

			if (__size_estimator.epoch == 89 || __size_estimator.epoch == 149)
				uni_size_estimator_enable(&__size_estimator);
		}

#endif

		/************************************************************************/
#ifdef TEST_POWER_OUTAGE
		/*
		 * Test: `Power Outage`
		 */ 
		if (1) { // for failing nodes only
			if (__size_estimator.epoch == 59 || __size_estimator.epoch == 119 || __size_estimator.epoch == 179) {
				do {
					if (!radio_trylock())
						break;
					PROCESS_PAUSE();
				} while (1);

#ifdef WITH_CC1100
				{
					/*
					 * tx power [dbm]    -30  | -20  | -15 |  -10 |  -5  |   0  |  5   |  7   |  10
					 * patable setting   0x03 | 0x0d | 0x1c| 0x34 | 0x57 | 0x8e | 0x85 | 0xcc | 0xc3
					 */
					uint8_t tx_power;

					if (__size_estimator.epoch == 60)
						tx_power = 0x34;
					else if (__size_estimator.epoch == 120)
						tx_power = 0x1c;
					else 
						tx_power = 0x0d;

					printf("@%d setting cc1100 tx-power with %d\n", __size_estimator.epoch, tx_power);

					cc1100_radio_init_with_power(tx_power);
				}
#else
#error Changing the radio tx power is supported only on CC1100
#endif

				radio_unlock();
			}

			if (__size_estimator.epoch == 89 || __size_estimator.epoch == 149) {
				do {
					if (!radio_trylock())
						break;
					PROCESS_PAUSE();
				} while (1);

#ifdef WITH_CC1100
				{
					/*
					 * tx power [dbm]    -30  | -20  | -15 |  -10 |  -5  |   0  |  5   |  7   |  10
					 * patable setting   0x03 | 0x0d | 0x1c| 0x34 | 0x57 | 0x8e | 0x85 | 0xcc | 0xc3
					 */
					uint8_t tx_power = 0xc3;
				
					printf("@%d setting cc1100 tx-power with %d\n", __size_estimator.epoch, tx_power);

					cc1100_radio_init_with_power(tx_power);
				}
#else
#error Changing the radio tx power is supported only on CC1100
#endif

				radio_unlock();
			}
		}
		/************************************************************************/
#endif
			
#ifdef TEST_RADIO_POWER_RADIUS
		/*
		 * Test: `Communication Radius`
		 */ 
		const int test_set_power_every_nr_epochs = 10;
		const int test_start_epoch = EPOCHS_UNTIL_SYNCED + test_set_power_every_nr_epochs;

		if ((__size_estimator.epoch >= test_start_epoch) &&  (__size_estimator.epoch % test_set_power_every_nr_epochs == 0)) {

			/*
			 * lock the radio
			 */
			do {
				if (!radio_trylock())
					break;
				PROCESS_PAUSE();
			} while (1);

#ifdef WITH_CC1100
			{
				/*
				 * tx power [dbm]    -30  | -20  | -15 |  -10 |  -5  |   0  |  5   |  7   |  10
				 * patable setting   0x03 | 0x0d | 0x1c| 0x34 | 0x57 | 0x8e | 0x85 | 0xcc | 0xc3
				 */
				static uint8_t radio_power_table[] = {0xc3, 0xcc, 0x85, 0x8e, 0x57, 0x34, 0x1c, 0x0d, 0x03};
				int index = (__size_estimator.epoch - test_start_epoch)/test_set_power_every_nr_epochs;


				if (index < sizeof(radio_power_table)) {
					uint8_t tx_power = radio_power_table[index];

					printf("@%d setting cc1100 tx-power with %d\n", __size_estimator.epoch, tx_power);

					cc1100_radio_init_with_power(tx_power);
				}
			}
#elif defined(WITH_CC2420)
			/*
			 * param value     : 31 | 27 | 23 | 19 | 15 |  11 |  7  |  3
			 * output power (dBm): 0  | -1 | -3 | -5 | -7 | -10 | -15 | -25
			 */
			unsigned char tx_power;
			tx_power = 31 - 2*(__size_estimator.epoch - test_start_epoch)/test_set_power_every_nr_epochs;

			if (tx_power >= 3) {
				printf("@%d setting cc2420 tx-power to %d\n", __size_estimator.epoch, tx_power);
				cc2420_radio_reinit_with_power(tx_power);
			}
#else
#error no radio ?
#endif

			/*
			 * unlock the radio
			 */
			radio_unlock();
		}
		/************************************************************************/
#endif
		
		/*
		 * During the EPOCH_START_DELAY we compute the estimator
		 * sufficient statistics and log them on the serial line.
		 *
		 * ! if the estimator is not enabled this will simply update
		 * the epoch count and return.
		 */
		uni_size_estimator_at_epoch_start(&__size_estimator);


		if (uni_size_estimator_enabled(&__size_estimator)) {
			static long int tx_start;
			static long int send_time;

			/* 
			 * Setup a random wait time before starting transmission
			 */
			send_time = EPOCH_START_DELAY + ((unsigned)rand()) % EPOCH_XFER_INTERVAL;
			assert(send_time >= EPOCH_START_DELAY);
			assert(send_time <= EPOCH_START_DELAY + EPOCH_XFER_INTERVAL);
			etimer_set(&send_timer, send_time);

			/*
			 * Wait wait ...
			 */
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

			/* 
			 * Transmit consensus data
			 */
			__max_packet_id = 0;
			__min_packet_id = 0xff;
			tx_start = clock_time();
			do {
				static volatile uint16_t bytes_remaining;

				/*
				 * Prepare and send a new consensus packet.
				 */
				bytes_remaining = uni_size_estimator_queue_packet(&__size_estimator);
				broadcast_send(&conn);

				/*
				 * Wait until we are signalled back from the `broadcast_sent`-callback: the 
				 * xfer was either successful or the packet has been dropped after too many
				 * retries (assuming the csma MAC layer is in use).
				 */
				PROCESS_WAIT_EVENT_UNTIL(ev == evt_consensus_packet_sent);
			
				/*
				 * Break-out the tx loop when we are done sending our data
				 */
				if (!bytes_remaining)
					break;

				if ((clock_time()-tx_start) > EPOCH_END_DELAY) {
					/*
					 * bail, we are already too late
					 */
					printf("size-estimator: tx took too long, bailing !\n");
					break;
				}
			} while (1);
			trace("size-estimator: data sent in %ld ticks\n", clock_time()-tx_start);

			/*
			 * We are mostly done for this epoch: we just have to handle 
			 * incoming consensus packets and this is done in the
			 * `broadcast_recv`-callback.
			 */
			radio_unlock();
		}
		PROCESS_WAIT_EVENT_UNTIL(ev == evt_end_of_epoch);
		trace("@%d size-estimator recv packet ids %d-%d\n", __size_estimator.epoch, __min_packet_id, __max_packet_id);

#ifdef TRACK_CONNECTIONS
		connection_print_and_zero(CONNECTION_TRACK_DATA, __size_estimator.epoch);
#endif
	} while (1);

	PROCESS_END();
}


