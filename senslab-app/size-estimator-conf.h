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


/*
 * The name of this macro is `important`: PROJECT_CONF_H, CONTIKI_CONF_H etc
 * are reserved by Contiki
 */
#ifndef __SIZE_ESTIMATOR_CONF_H__
#define __SIZE_ESTIMATOR_CONF_H__


#define __CONTIKI_NETSTACK_RDC_NULL  0
#define __CONTIKI_NETSTACK_RDC_CXMAC 1


/* -------------------------------------------------------------------------- */


/*
 *
 * Setup the RDC to be used
 * ! an overview on MACs and RDCs in Contiki is available here
 *   http://www.sics.se/contiki/wiki/index.php/Change_MAC_or_Radio_Duty_Cycling_Protocols
 */
#define __CONTIKI_NETSTACK_RDC __CONTIKI_NETSTACK_RDC_NULL


/*
 * Define this macro to track connections
 *
 * When this macro is defined each radio transmission stores also the board-id
 * of the sender node. This feature can be used to track and gather statistics
 * on the network connectivity. 
 */
#define TRACK_CONNECTIONS


/*
 * Define this macro to use CRC16 filtering of incoming packets
 *
 * When this macro is defined each radio transmission stores also the crc of 
 * the packet data. This feature is used to filter out corrupted packets. 
 */
#define XFER_CRC16


/* -------------------------------------------------------------------------- */


/*
 * Define the correct RDC in the way Contiki wants it
 */
#undef NETSTACK_CONF_RDC
#if __CONTIKI_NETSTACK_RDC==__CONTIKI_NETSTACK_RDC_NULL
#define NETSTACK_CONF_RDC nullrdc_driver
#elif __CONTIKI_NETSTACK_RDC==__CONTIKI_NETSTACK_RDC_CXMAC
#define NETSTACK_CONF_RDC cxmac_driver
#else
#error please configure the RDC to be used.
#endif

#endif /* __SIZE_ESTIMATOR_CONF_H__ */

