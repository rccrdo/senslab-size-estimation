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
#include <stdio.h>
#include "contiki.h"
#include "net/packetbuf.h"
#include "packet-splitter.h"

#ifdef XFER_CRC16
#include "crc16.h"
#endif

#ifdef TRACK_CONNECTIONS
#include "util.h"
#endif

void packet_splitter_init(struct packet_splitter *splitter, uint16_t epoch, const char *data, uint16_t datalen) {
	assert(splitter != NULL);
	assert(data != NULL);
	assert(datalen > 0);

	/*
	 * We are using an uint8_t to store the sequential packet id and thus 
	 * cannot handle blocks of data that can't be split in <= 256 packets
	 */
	assert((datalen / PACKET_SPLITTER_PAYLOAD_LEN) <= 256);

	splitter->data = data;
	splitter->epoch = epoch;
	splitter->packet_id = 0;
	splitter->nr_bytes_queued = 0;
	splitter->nr_bytes_remaining = datalen;
}


uint16_t packet_splitter_queue(struct packet_splitter *splitter) {
	const char *src_data_cur;
	uint16_t i;
	uint16_t packetlen;
	uint16_t nr_to_send;

	assert(splitter != NULL);
	assert(splitter->data != NULL);
	assert(splitter->nr_bytes_remaining > 0);
	
	nr_to_send = PACKET_SPLITTER_PAYLOAD_LEN;
	if (nr_to_send > splitter->nr_bytes_remaining)
		nr_to_send = splitter->nr_bytes_remaining;

	/* setup header */
#ifdef TRACK_CONNECTIONS
	splitter->packet.hdr.nodeid = board_get_id16();
#endif
	splitter->packet.hdr.epoch = splitter->epoch;
	splitter->packet.hdr.packet_id = splitter->packet_id;
	splitter->packet.hdr.payloadlen = nr_to_send;

	/* setup payload */
	src_data_cur = &splitter->data[splitter->nr_bytes_queued];
	for (i=0; i<nr_to_send; i++) {
		splitter->packet.data[i] = *src_data_cur;
		src_data_cur++;
	}

        /*
	 * `push` our packet to contiki's packetbuf 
	 *
	 * ! The payload of the last packet in each consensus transaction is not full in general
	 *   and we can spare sending some bytes.
	 */
	packetlen = sizeof(struct split_packet) - (PACKET_SPLITTER_PAYLOAD_LEN - nr_to_send);

#ifdef XFER_CRC16
	/*
	 * Compute the crc with the .crc16 field zeroed
	 */
	{
		uint16_t crc16;

		splitter->packet.hdr.crc16 = 0;
		crc16 = crc16_data((const unsigned char *)&splitter->packet, packetlen, 0);
		splitter->packet.hdr.crc16 = crc16;
	}
#endif
        packetbuf_reference((void *)&splitter->packet, packetlen);

	splitter->nr_bytes_queued += nr_to_send;
	splitter->nr_bytes_remaining -= nr_to_send;
	splitter->packet_id++;
	
	assert(splitter->nr_bytes_remaining >= 0);
	
	return splitter->nr_bytes_remaining;	
}

