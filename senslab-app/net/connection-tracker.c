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
#include "connection-tracker.h"


struct connection_tracker_node {
	uint16_t board_id16;
	uint8_t count[CONNECTION_TRACKER_NR_TYPES];
};

struct connection_tracker {
	struct connection_tracker_node nodes[CONNECTION_TRACKER_MAX_NODES];
};


static char _initialized = 0;

/*
 * The connection tracker `singletone` instance
 */
static struct connection_tracker _tracker;


static void connection_tracker_init(void) {
	int i;

	if (_initialized)
		return;

	for (i=0; i < CONNECTION_TRACKER_MAX_NODES; i++) {
		int j;
		for (j=0; j < CONNECTION_TRACKER_NR_TYPES; j++) {
			_tracker.nodes[i].count[j] = 0;
		}
	}
	_initialized = 1;
}


static void _connection_track_print(int type, uint16_t board_id16, uint16_t epoch) {
	if (type == CONNECTION_TRACK_SYNC) {
		printf("@%d track sync %.4x:1\n", epoch, board_id16);
	} else if (type == CONNECTION_TRACK_DATA) {
		printf("@%d track data %.4x:1\n", epoch, board_id16);
	} else {
		printf("@%d track unknown %.4x:1\n", epoch, board_id16);
	}
}


void connection_track(int type, uint16_t board_id16, uint16_t epoch) {
	int i;

	assert(type >= __CONNECTION_TRACK_FIRST);
	assert(type <= __CONNECTION_TRACK_LAST);

	if (!_initialized)
		connection_tracker_init();

	if ((type < __CONNECTION_TRACK_FIRST) || (type > __CONNECTION_TRACK_LAST)) {
		_connection_track_print(type, board_id16, epoch);
		return;
	}

	for (i=0; i < CONNECTION_TRACKER_MAX_NODES; i++) {
		if (_tracker.nodes[i].board_id16 == board_id16) {
			/*
			 * we found an entry with this id
			 */
			if (_tracker.nodes[i].count[type] < 0xff) {
				_tracker.nodes[i].count[type]++;
			} else {
				/*
				 * we have space to count up to 0xff hits per epoch, track this
				 * connection in the log. This should happen less then rarely.
				 */
				_connection_track_print(type, board_id16, epoch);
			}
			return;
		}
	}

	/*
	 * no entry with the given board_id16 was found
	 *
	 * check if we can reuse an empty slot, i.e. an entry that has `.count`
	 * equal to zero for all tracked types
	 */
	for (i=0; i < CONNECTION_TRACKER_MAX_NODES; i++) {
		int j;
		char found;
		found = 1;
		for (j=0; j < CONNECTION_TRACKER_NR_TYPES; j++) {
			if (_tracker.nodes[i].count[j]) {
				found = 0;
				break;
			}
		}

		if (found) {
			_tracker.nodes[i].board_id16 = board_id16;
			_tracker.nodes[i].count[type] = 1;
			return;
		}
	}

	_connection_track_print(type, board_id16, epoch);
}


void connection_print_and_zero(int type, uint16_t epoch) {
	int i;

	assert(type >= __CONNECTION_TRACK_FIRST);
	assert(type <= __CONNECTION_TRACK_LAST);

	if (!_initialized)
		return;

	/*
	 * `type` is used later as an index into an array, be defensive
	 */
	if ((type < __CONNECTION_TRACK_FIRST) || (type > __CONNECTION_TRACK_LAST))
		return;

	if (type == CONNECTION_TRACK_SYNC) {
		printf("@%d track sync", epoch);
	} else if (type == CONNECTION_TRACK_DATA) {
		printf("@%d track data", epoch);
	}

	for (i=0; i < CONNECTION_TRACKER_MAX_NODES; i++) {
		if (_tracker.nodes[i].count[type]) {
			printf(" %.4x:%d", _tracker.nodes[i].board_id16, _tracker.nodes[i].count[type]);
			_tracker.nodes[i].count[type] = 0;
		}
	}
	printf("\n");
}

