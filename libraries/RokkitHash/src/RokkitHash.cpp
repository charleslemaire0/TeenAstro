/*
 * See header file for more information.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "RokkitHash.h"

uint32_t rokkit (const char *data, uint16_t len) {
#ifndef ROKKIT_ENABLE_8BIT_OPTIMIZATIONS
	// This is mostly Paul Hsieh's original code
	uint32_t hash, tmp;
	int rem;

	if (len <= 0 || data == 0) {
		return 0;
	}

	hash = len;
	rem = len & 3;
	len >>= 2;

	/* Main loop */
	while (len > 0) {
		hash += *((uint16_t *) data);

		/* To make a long story short, the C standard states that the
		 * shift operator's operands must be promoted to (unsigned) int,
		 * which is (usually) 32 bits wide on PC and 16 on Arduino. This
		 * results in different behaviour, since part of the result gets
		 * truncated on Arduino, so we cast the result to make sure all
		 * bits are kept.
		 */
		tmp = ((uint32_t) (*((uint16_t *) (data + 2))) << 11) ^ hash;

		hash = (hash << 16) ^ tmp;
		data += 2 * sizeof (uint16_t);
		hash += hash >> 11;
		len--;
	}

	/* Handle end cases */
	switch (rem) {
		case 3:
			hash += * ((uint16_t *) data);
			hash ^= hash << 16;
			hash ^= ((signed char) data[2]) << 18;
			hash += hash >> 11;
			break;

		case 2:
			hash += * ((uint16_t *) data);
			hash ^= hash << 11;
			hash += hash >> 17;
			break;

		case 1:
			hash += (signed char) * data;
			hash ^= hash << 10;
			hash += hash >> 1;
	}

	/* Force "avalanching" of final 127 bits */
	hash ^= hash << 3;
	hash += hash >> 5;
	hash ^= hash << 4;
	hash += hash >> 17;
	hash ^= hash << 25;
	hash += hash >> 6;

	return hash;
#else
	// Optimized code for 8-bit processors by robtillaart
	union {
		uint32_t h;
		uint16_t b[2];
		uint8_t a[4];
	}
	hash, tmp;
	uint8_t rem;
	uint16_t *p = (uint16_t *) data;

	if (data == 0) {
		return 0;
	}

	hash.h = len;
	rem = len & 3;
	len >>= 2;

	/* Main loop */
	while (len > 0) {
		// hash += *((uint16_t*)data);
		hash.h += *p++;

		// tmp = (*((uint16_t*)(data+2)) << 11) ^ hash;
		// hash = (hash << 16) ^ tmp;
		// ==>
		// hash = (hash << 16) ^ (*((uint16_t*)(data+2)) << 11) ^ hash;
		// ==>
		// hash ^= (hash << 16) ^ (*((uint16_t*)(data+2)) << 11);
		//
		// The cast is needed to make the code behave correctly
		// on platforms where sizeof (int) != 4, see the original code
		// above for an explanation.
		tmp.h = ((uint32_t) *p++) << 11;
		tmp.b[1] ^= hash.b[0];

		// hash.h ^= tmp.h;
		// hash.a[0] ^= tmp.a[0];  // not needed as tmp.a[0] == 0
		hash.a[1] ^= tmp.a[1];
		hash.a[2] ^= tmp.a[2];
		hash.a[3] ^= tmp.a[3];

		// hash  += hash >> 11;
		tmp.a[0] = hash.a[1];  // shift 8
		tmp.a[1] = hash.a[2];
		tmp.a[2] = hash.a[3];
		tmp.a[3] = 0;
		hash.h += tmp.h >> 3;

		--len;
	}

	/* Handle end cases */
	data = reinterpret_cast <char *> (p);
	switch (rem) {
		case 3:
			hash.h += *((uint16_t *) data);
			hash.h ^= hash.h << 16;  // todo
			hash.h ^= ((signed char) data[2]) << 18;
			hash.h += hash.h >> 11;  // todo
			break;

		case 2:
			hash.h += *((uint16_t *) data);
			hash.h ^= hash.h << 11;  // todo
			hash.h += hash.h >> 17;  // todo
			break;

		case 1:
			hash.h += (signed char) *data;
			hash.h ^= hash.h << 10;  // todo
			hash.h += hash.h >> 1;
	}

	/* Force "avalanching" of final 127 bits */
	hash.h ^= hash.h << 3;
	hash.h += hash.h >> 5;
	hash.h ^= hash.h << 4;

	// hash.h += hash.h >> 17;  // todo
	tmp.b[0] = hash.b[1] >> 1;
	tmp.b[1] = 0;
	hash.h += tmp.h;

	// hash.h ^= hash.h << 25;
	//  tmp.a[3] = hash.a[0] << 1; // shift 25!!
	//  tmp.a[2] = 0;
	//  tmp.a[1] = 0;
	//  tmp.a[0] = 0;
	hash.a[3] ^= hash.a[0] << 1; // shift 25!!;

	hash.h += hash.h >> 6;

	return hash.h;
#endif
}

#ifdef ROKKIT_ENABLE_FLASH_FUNCTIONS

#include <avr/pgmspace.h>

uint32_t rokkit (const __FlashStringHelper *data, uint16_t len) {
	union {
		uint32_t h;
		uint16_t b[2];
		uint8_t a[4];
	} hash, tmp;
	uint8_t rem;

	if (data == NULL) {
		return 0;
	}

	PGM_P p = reinterpret_cast <PGM_P> (data);
	hash.h = len;
	rem = len & 3;
	len >>= 2;

	/* Main loop */
	while (len > 0) {
		tmp.h = pgm_read_dword (p);
		hash.h += tmp.b[0];
		uint16_t val = tmp.b[1];
		p += 4;

		tmp.h = ((uint32_t) val) << 11;
		tmp.b[1] ^= hash.b[0];

		// hash.h ^= tmp.h;
		// hash.a[0] ^= tmp.a[0];  // not needed as tmp.a[0] == 0
		hash.a[1] ^= tmp.a[1];
		hash.a[2] ^= tmp.a[2];
		hash.a[3] ^= tmp.a[3];

		// hash  += hash >> 11;
		tmp.a[0] = hash.a[1];  // shift 8
		tmp.a[1] = hash.a[2];
		tmp.a[2] = hash.a[3];
		tmp.a[3] = 0;
		hash.h += tmp.h >> 3;

		--len;
	}

	/* Handle end cases */
	uint16_t val = pgm_read_word (p);
	uint16_t val2 = pgm_read_word (p + 2);
	switch (rem) {
		case 3:
			hash.h += val;
			hash.h ^= hash.h << 16;  // todo
			hash.h ^= ((signed char) val2) << 18;
			hash.h += hash.h >> 11;  // todo
			break;

		case 2:
			hash.h += val2;
			hash.h ^= hash.h << 11;  // todo
			hash.h += hash.h >> 17;  // todo
			break;

		case 1:
			hash.h += val2;
			hash.h ^= hash.h << 10;  // todo
			hash.h += hash.h >> 1;
	}

	/* Force "avalanching" of final 127 bits */
	hash.h ^= hash.h << 3;
	hash.h += hash.h >> 5;
	hash.h ^= hash.h << 4;

	// hash.h += hash.h >> 17;  // todo
	tmp.b[0] = hash.b[1] >> 1;
	tmp.b[1] = 0;
	hash.h += tmp.h;

	// hash.h ^= hash.h << 25;
	//  tmp.a[3] = hash.a[0] << 1; // shift 25!!
	//  tmp.a[2] = 0;
	//  tmp.a[1] = 0;
	//  tmp.a[0] = 0;
	hash.a[3] ^= hash.a[0] << 1; // shift 25!!;

	hash.h += hash.h >> 6;

	return hash.h;
}

#endif
