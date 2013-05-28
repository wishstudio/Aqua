#include <cstdlib>

#include "String.h"

/* Grab low "bits" bits of x */
#define LOWBITS(x, bits) ((x) & ((1 << ((bits) + 1)) - 1))
/* UTF-8 to UTF-16 */
uint32 utf8ToUtf16(const char *data, uint32 length, uint16 *outdata)
{
	uint32 outlength = 0;
	for (uint32 i = 0; i < length;)
	{
		uint32 codepoint = 0;
		if ((data[i] & 0xF8) == 0xF0) // 11110xxx
		{
			codepoint += LOWBITS(data[i++], 3) << 18;
			codepoint += LOWBITS(data[i++], 6) << 12;
			codepoint += LOWBITS(data[i++], 6) << 6;
			codepoint += LOWBITS(data[i++], 6);
		}
		else if ((data[i] & 0xF0) == 0xE0) // 1110xxxx
		{
			codepoint += LOWBITS(data[i++], 4) << 12;
			codepoint += LOWBITS(data[i++], 6) << 6;
			codepoint += LOWBITS(data[i++], 6);
		}
		else if ((data[i] & 0xE0) == 0xC0) // 110xxxxx
		{
			codepoint += LOWBITS(data[i++], 5) << 6;
			codepoint += LOWBITS(data[i++], 6);
		}
		else // 0xxxxxxx
			codepoint += data[i++];
		if (codepoint <= 0xFFFF)
		{
			if (outdata)
				*outdata++ = codepoint;
			outlength += 1;
		}
		else
		{
			if (outdata)
			{
				*outdata++ = 0xD800 + ((codepoint - 0x10000) >> 10);
				*outdata++ = 0xDC00 + ((codepoint - 0x10000) & 0x3FF);
			}
			outlength += 2;
		}
	}
	return outlength;
}

uint32 utf16ToUtf8(const uint16 *data, uint32 length, char *outdata)
{
	uint32 outlength = 0;
	for (uint32 i = 0; i < length;)
	{
		uint32 codepoint;
		if (data[i] >= 0xD800 && data[i] <= 0xDFFF)
		{
			codepoint = 0x10000;
			codepoint += (data[i++] - 0xD800) << 10;
			codepoint += data[i++] - 0xDC00;
		}
		else
			codepoint = data[i++];
		if (outdata)
		{
			if (codepoint <= 0x7F)
				outdata[outlength++] = codepoint;
			else if (codepoint <= 0x07FF)
			{
				outdata[outlength++] = 0x80000000 | (codepoint & 0x3F);
				outdata[outlength++] = 0xC0000000 | (codepoint >> 6);
			}
			else if (codepoint <= 0xFFFF)
			{
				outdata[outlength++] = 0x80000000 | (codepoint & 0x3F);
				outdata[outlength++] = 0x80000000 | ((codepoint >> 6) & 0x3F);
				outdata[outlength++] = 0xE0000000 | (codepoint >> 12);
			}
			else /* <= 0x10FFFF */
			{
				outdata[outlength++] = 0x80000000 | (codepoint & 0x3F);
				outdata[outlength++] = 0x80000000 | ((codepoint >> 6) & 0x3F);
				outdata[outlength++] = 0x80000000 | ((codepoint >> 12) & 0x3F);
				outdata[outlength++] = 0xF0000000 | (codepoint >> 18);
			}
		}
		else if (codepoint <= 0x7F)
			outlength += 1;
		else if (codepoint <= 0x07FF)
			outlength += 2;
		else if (codepoint <= 0xFFFF)
			outlength += 3;
		else /* <= 0x10FFFF */
			outlength += 4;
	}
	return outlength;
}

/* MurmurHash 3 */

static __forceinline uint32 fmix(uint32 h)
{
	h ^= h >> 16;
	h *= 0x85ebca6b;
	h ^= h >> 13;
	h *= 0xc2b2ae35;
	h ^= h >> 16;

	return h;
}

static uint32 MurmurHash3(const void *key, int len, uint32 seed)
{
	const uint8 *data = (const uint8 *) key;
	const int nblocks = len / 4;

	uint32 h1 = seed;

	const uint32 c1 = 0xcc9e2d51;
	const uint32 c2 = 0x1b873593;

	//----------
	// body

	const uint32 *blocks = (const uint32 *) (data + nblocks * 4);

	for (int i = -nblocks; i; i++)
	{
		uint32 k1 = blocks[i];

		k1 *= c1;
		k1 = _rotl(k1, 15);
		k1 *= c2;
    
		h1 ^= k1;
		h1 = _rotl(h1, 13); 
		h1 = h1 * 5 + 0xe6546b64;
	}

	//----------
	// tail

	const uint8 *tail = (const uint8 *) (data + nblocks * 4);

	uint32 k1 = 0;

	switch(len & 3)
	{
	case 3: k1 ^= tail[2] << 16;
	case 2: k1 ^= tail[1] << 8;
	case 1: k1 ^= tail[0];
			k1 *= c1; k1 = _rotl(k1, 15); k1 *= c2; h1 ^= k1;
	};

	//----------
	// finalization

	h1 ^= len;

	h1 = fmix(h1);

	return h1;
}

uint32 hashString(const void *string, uint32 length)
{
	return MurmurHash3(string, length, 42);
}
