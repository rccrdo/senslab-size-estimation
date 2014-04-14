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

#ifndef __CONNECTION_TRACKER_H__
#define __CONNECTION_TRACKER_H__

#include <stdint.h>

/*
 * !
 * 1) start from 0
 * 2) do not make holes in the numbering
 * 3) update _FIRST and _LAST if you change something
 */
#define CONNECTION_TRACK_SYNC     0
#define CONNECTION_TRACK_DATA     1
#define __CONNECTION_TRACK_FIRST          CONNECTION_TRACK_SYNC
#define __CONNECTION_TRACK_LAST           CONNECTION_TRACK_DATA

#define CONNECTION_TRACKER_NR_TYPES (__CONNECTION_TRACK_LAST + 1)

#define CONNECTION_TRACKER_MAX_NODES 40

#if __CONNECTION_TRACK_FIRST != 0
#error The connection tracker types must start at zero (and cannot contain holes).
#endif


void connection_track(int type, uint16_t board_id16, uint16_t epoch);
void connection_print_and_zero(int type, uint16_t epoch);


#endif /* __CONNECTION_TRACKER_H__ */

