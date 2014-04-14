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

#ifndef __PACKET_SPLITTER_H__
#define __PACKET_SPLITTER_H__

#include <stdint.h>
#include "size-estimator-conf.h"

/*
 * The following macros define the size in bytes of the payload (actual
 * application-data bytes) sent in each split packet.
 *
 * ! Contiki core will reject larger packets
 */

#ifdef XFER_CRC16

#ifdef TRACK_CONNECTIONS
/*
 * reserve 2 bytes for the 16bit board-id and 2 bytes for the crc
 */
#define PACKET_SPLITTER_PAYLOAD_LEN (104)
#else
/*
 * reserve 2 bytes for the crc
 */
#define PACKET_SPLITTER_PAYLOAD_LEN (106)
#endif

#else /* XFER_CRC16 */

#ifdef TRACK_CONNECTIONS
/*
 * reserve 2 bytes for the 16bit board-id
 */
#define PACKET_SPLITTER_PAYLOAD_LEN (106)
#else
#define PACKET_SPLITTER_PAYLOAD_LEN (108)
#endif

#endif  /* XFER_CRC16 */


/*
 * make sure PACKET_SPLITTER_PAYLOAD_SIZE is a multiple of 2 
 * 
 * ! In principle this would no be needed, but in our application it really helps
 *   not having to split fractional16_t variables (which have a 2 byte storage)
 */
#if (PACKET_SPLITTER_PAYLOAD_LEN & 1) != 0
#error PACKET_SPLITTER_PAYLOAD_SIZE must be a multiple of two !
#endif


/*
 * The header of each split packet
 */
struct split_packet_hdr {

#ifdef XFER_CRC16
	/*
	 * It is important that this field is the first struct member when
	 * XFER_CRC16 is defined.
	 */
	uint16_t crc16;
#endif

#ifdef TRACK_CONNECTIONS
	uint16_t nodeid;
#endif

	uint16_t epoch;
	uint8_t packet_id;
	uint8_t payloadlen;
};


/*
 * We are using an uint8_t to store the payload length (in bytes) and thus 
 * cannot handle packets with payload bigger than that.
 */
#if PACKET_SPLITTER_PAYLOAD_LEN > 255
#error please choose PACKET_SPLITTER_PAYLOAD_LEN < 256
#endif


/*
 * The split packet format
 */
struct split_packet {
	struct split_packet_hdr hdr;

#if __CONTIKI_NETSTACK_RDC==__CONTIKI_NETSTACK_RDC_CXMAC
	/*
	 * When the cxmac RDC is in use, Contiki will prepend to these packets
	 * an header with an odd number of bytes. This placeholder is here so that
	 * (given that sizeof(split_packet_hdr) is even) on the receiver side we can
	 * dereference the following data field using 16bit-loads on 2-byte 
	 * aligned adresses (which would otherwise have undefined behaviour on msp430).
	 */
	char placeholder;
#endif

	/*
	 * ! This struct defines also the on-wire format of the packet and 
	 *   the code assumes that `data` is the last field in this struct
	 */
	char data[PACKET_SPLITTER_PAYLOAD_LEN];
};


/*
 * The packet splitter `class'
 */
struct packet_splitter {
	uint16_t epoch;
	const char *data;
	uint16_t packet_id;
	uint16_t nr_bytes_queued;
	uint16_t nr_bytes_remaining;
	struct split_packet packet;
};


/*
 * Called at every beginning of each epoch: receives the epoch index,
 * the address of the data array to send and its length
 * 
 * It does no more than an initialization, i.e., saves internally where
 * the data is.
 */
void packet_splitter_init(struct packet_splitter *splitter, uint16_t epoch, const char *data, uint16_t datalen);


/*
 * Called whenever you want to send the data
 * 
 * ! since we have a big matrix of data to be sent, we need to split everything
 * before sending.
 * 
 * Will queeu approx PACKET_SPLITTER_PAYLOAD_LEN bytes
 */
uint16_t packet_splitter_queue(struct packet_splitter *splitter);


#endif /* __PACKET_SPLITTER_H__ */

