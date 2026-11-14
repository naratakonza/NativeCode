// SA-MP authentication transform from Arita's samp_auth.cpp.
#include "samp_auth.h"

#include <cstdint>
#include <cstring>

namespace
{
#define ROTL(value, shift) ((value << shift) | (value >> (32 - shift)))

void SHA1(const char* message, uint32_t* out)
{	
	uint32_t h0 = 0x67452301;
	uint32_t h1 = 0xEFCDAB89;
	uint32_t h2 = 0x98BADCFE;
	uint32_t h3 = 0x10325476;
	uint32_t h4 = 0xC3D2E1F0;

	uint32_t len = 0;
	uint64_t bitlen = 0;
	
	while (message[len])
	{
		len++;
		bitlen += 8;
	}
	
	uint32_t complement = (55 - (len%56)) + 8*(((len+8)/64));
	uint32_t newlen = len + complement + 8 + 1;
    uint8_t pMessage[352]{};
    std::memcpy(pMessage, message, len);
    pMessage[len] = 0x80;
    for (uint32_t i = 0; i < 8; ++i)
    {
        pMessage[len + 1 + complement + i] = static_cast<uint8_t>(bitlen >> ((7 - i) * 8));
    }

	uint32_t chunks = newlen/64;
	uint32_t w[80];

	for (uint32_t x = 0; x < chunks; x++)
	{
		for (uint32_t i = 0; i < 16; i++)
			{
            const uint8_t* word = &pMessage[x * 64 + i * 4];
            w[i] = (static_cast<uint32_t>(word[0]) << 24) |
                (static_cast<uint32_t>(word[1]) << 16) |
                (static_cast<uint32_t>(word[2]) << 8) | word[3];
        }

		memset(&w[16], 0, 64*4);

		for (uint32_t i = 16; i <= 79; i++)
			w[i] = ROTL((w[i-3] ^ w[i-8] ^ w[i-14] ^ w[i-16]), 1);

		uint32_t a = h0;
		uint32_t b = h1;
		uint32_t c = h2;
		uint32_t d = h3;
		uint32_t e = h4;

		for (uint32_t i = 0; i <= 79; i++)
		{
			uint32_t f;
			uint32_t k;

			if (i <= 19)
			{
				f = (b & c) | ((~b) & d);
				k = 0x5A827999;
			}
			else if (i <= 39)
			{
				f = b ^ c ^ d;
				k = 0x6ED9EBA1;
			}
			else if (i <= 59)
			{
				f = (b & c) | (b & d) | (c & d);
				k = 0x8F1BBCDC;
			}
			else
			{
				f = b ^ c ^ d;
				k = 0xCA62C1D6;
			}

			uint32_t temp = (ROTL(a, 5) + f + e + k + w[i]);
			e = d;
			d = c;
			c = ROTL(b, 30);
			b = a;
			a = temp;
		}

		h0 = (h0 + a);
		h1 = (h1 + b);
		h2 = (h2 + c);
		h3 = (h3 + d);
		h4 = (h4 + e);
	}

	out[0] = h0;
	out[1] = h1;
	out[2] = h2;
	out[3] = h3;
	out[4] = h4;
}

#undef ROTL

const static uint8_t auth_hash_transform_table[100] = {
	0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 0x00, 0x00, 0x00, 0x0D,
	0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x80,
	0x08, 0x06, 0x00, 0x00, 0x00, 0xE4, 0xB5, 0xB7, 0x0A, 0x00, 0x00, 0x00,
	0x09, 0x70, 0x48, 0x59, 0x73, 0x00, 0x00, 0x0B, 0x13, 0x00, 0x00, 0x0B,
	0x13, 0x01, 0x00, 0x9A, 0x9C, 0x18, 0x00, 0x00, 0x00, 0x04, 0x67, 0x41,
	0x4D, 0x41, 0x00, 0x00, 0xB1, 0x8E, 0x7C, 0xFB, 0x51, 0x93, 0x00, 0x00,
	0x00, 0x20, 0x63, 0x48, 0x52, 0x4D, 0x00, 0x00, 0x7A, 0x25, 0x00, 0x00,
	0x80, 0x83, 0x00, 0x00, 0xF9, 0xFF, 0x00, 0x00, 0x80, 0xE9, 0x00, 0x00,
	0x75, 0x30, 0x00, 0x00
};

uint8_t transform_auth_sha1(uint8_t value, uint8_t _xor)
{
	uint8_t result = value;

	for(uint8_t i = 0; i < 100; i++)
	{
		result = result ^ auth_hash_transform_table[i] ^ _xor;
	}

	return result;
}

const static uint8_t code_from_CAnimManager_AddAnimation[20] = {
	0xFF, 0x25, 0x34, 0x39, // gta_sa.exe + 0x4D3AA0
	0x4D, 0x00, 0x90, 0x90, // gta_sa.exe + 0x4D3AA4
	0x90, 0x90, 0x56, 0x57, // gta_sa.exe + 0x4D3AAC
	0x50, 0x8B, 0x44, 0x24, // gta_sa.exe + 0x4D3AA8
	0x14, 0x8D, 0x0C, 0x80  // gta_sa.exe + 0x4D3AB0
};

char samp_sub_100517E0(uint8_t a1)
{
	char result = a1 + '0';

	if(a1 + '0' > '9')
	{
		result = a1 + '7';
	}

	return result;
}

void auth_stringify(char *out, uint8_t* hash)
{
	uint8_t i = 0;
	uint8_t* j = hash;

	do
	{
		out[i] = samp_sub_100517E0(*j >> 4); i++;
		out[i] = samp_sub_100517E0(*j & 0xF); i++;

		j++;
	}
	while(i < 40);

	out[i] = '\0';
}
}

void gen_auth_key(char buf[260], char* auth_in)
{
    if (!auth_in || std::strlen(auth_in) > 255) {
        return;
    }

    uint32_t words[5];
    SHA1(auth_in, words);
    uint8_t out[20];
    // Arita applies its transform to the little-endian bytes of the words.
    for (unsigned int i = 0; i < sizeof(out); ++i)
    {
        out[i] = static_cast<uint8_t>(words[i / 4] >> ((i % 4) * 8));
    }

    constexpr uint8_t transforms[]{0x2F, 0x45, 0x6F, 0xDB};
    for (unsigned int i = 0; i < sizeof(out); ++i)
    {
        out[i] = transform_auth_sha1(out[i], transforms[i / 5]) ^ code_from_CAnimManager_AddAnimation[i];
    }

    auth_stringify(buf, out);
}
