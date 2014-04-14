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


/*
 * This one comes from Contiki
 */
void _xassert(const char *filename, int line) {
	printf("assert %s::%d\n", filename, line);
}


void printhex(const char *mem, uint16_t len) {
	uint16_t i;

	printf("[0x%.4x-0x%.4x] ", (int)mem, len + (int)mem);
	for (i=0; i < len; i++)
		printf("%.2x", mem[i]);
	printf("\n");
}


void printhex2(const char *mem, uint16_t len) {
	uint16_t i;
	const uint16_t _len = 10;


	if (len <= _len) {
		printhex((const char*)mem, len);
		return;
	}

	printf("[0x%.4x-0x%.4x] ", (int)mem, len + (int)mem);
	for (i=0; i < _len; i++)
		printf("%.2x", mem[i]);

	printf("... + %d bytes\n", len-_len);
}

