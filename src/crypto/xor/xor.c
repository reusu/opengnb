/*
   Copyright (C) gnbdev

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdint.h>
#include <string.h>

#if defined(__AVX2__)
#define XOR_USE_AVX2 1
#include <immintrin.h>
#elif defined(__SSE2__) || (defined(_MSC_VER) && defined(_M_X64))
#define XOR_USE_SSE2 1
#include <emmintrin.h>
#elif defined(__ARM_NEON) || defined(__ARM_NEON__)
#define XOR_USE_NEON 1
#include <arm_neon.h>
#endif

void xor_crypto(unsigned char *crypto_key, unsigned char *data, unsigned int len) {
    unsigned int offset = 0;
    unsigned int j;

#if defined(XOR_USE_AVX2)
    __m256i k0 = _mm256_loadu_si256((const __m256i *)(crypto_key));
    __m256i k1 = _mm256_loadu_si256((const __m256i *)(crypto_key + 32));

    while (offset + 64 <= len) {
        __m256i d0 = _mm256_loadu_si256((__m256i *)(data + offset));
        __m256i d1 = _mm256_loadu_si256((__m256i *)(data + offset + 32));
        d0 = _mm256_xor_si256(d0, k0);
        d1 = _mm256_xor_si256(d1, k1);
        _mm256_storeu_si256((__m256i *)(data + offset), d0);
        _mm256_storeu_si256((__m256i *)(data + offset + 32), d1);
        offset += 64;
    }

#elif defined(XOR_USE_SSE2)
    __m128i k0 = _mm_loadu_si128((const __m128i *)(crypto_key));
    __m128i k1 = _mm_loadu_si128((const __m128i *)(crypto_key + 16));
    __m128i k2 = _mm_loadu_si128((const __m128i *)(crypto_key + 32));
    __m128i k3 = _mm_loadu_si128((const __m128i *)(crypto_key + 48));

    while (offset + 64 <= len) {
        __m128i d0 = _mm_loadu_si128((__m128i *)(data + offset));
        __m128i d1 = _mm_loadu_si128((__m128i *)(data + offset + 16));
        __m128i d2 = _mm_loadu_si128((__m128i *)(data + offset + 32));
        __m128i d3 = _mm_loadu_si128((__m128i *)(data + offset + 48));
        d0 = _mm_xor_si128(d0, k0);
        d1 = _mm_xor_si128(d1, k1);
        d2 = _mm_xor_si128(d2, k2);
        d3 = _mm_xor_si128(d3, k3);
        _mm_storeu_si128((__m128i *)(data + offset), d0);
        _mm_storeu_si128((__m128i *)(data + offset + 16), d1);
        _mm_storeu_si128((__m128i *)(data + offset + 32), d2);
        _mm_storeu_si128((__m128i *)(data + offset + 48), d3);
        offset += 64;
    }

#elif defined(XOR_USE_NEON)
    uint8x16_t k0 = vld1q_u8(crypto_key);
    uint8x16_t k1 = vld1q_u8(crypto_key + 16);
    uint8x16_t k2 = vld1q_u8(crypto_key + 32);
    uint8x16_t k3 = vld1q_u8(crypto_key + 48);

    while (offset + 64 <= len) {
        uint8x16_t d0 = vld1q_u8(data + offset);
        uint8x16_t d1 = vld1q_u8(data + offset + 16);
        uint8x16_t d2 = vld1q_u8(data + offset + 32);
        uint8x16_t d3 = vld1q_u8(data + offset + 48);
        d0 = veorq_u8(d0, k0);
        d1 = veorq_u8(d1, k1);
        d2 = veorq_u8(d2, k2);
        d3 = veorq_u8(d3, k3);
        vst1q_u8(data + offset, d0);
        vst1q_u8(data + offset + 16, d1);
        vst1q_u8(data + offset + 32, d2);
        vst1q_u8(data + offset + 48, d3);
        offset += 64;
    }

#else
    {
        const uint64_t *k = (const uint64_t *)crypto_key;
        uint64_t *dp;
        while (offset + 64 <= len) {
            dp = (uint64_t *)(data + offset);
            dp[0] ^= k[0]; dp[1] ^= k[1];
            dp[2] ^= k[2]; dp[3] ^= k[3];
            dp[4] ^= k[4]; dp[5] ^= k[5];
            dp[6] ^= k[6]; dp[7] ^= k[7];
            offset += 64;
        }
    }
#endif

    j = 0;
    while (offset < len) {
        data[offset] ^= crypto_key[j];
        offset++;
        j++;
    }
}

void xor_crypto_copy(unsigned char *crypto_key, unsigned char *dest, unsigned char *src, unsigned int len) {
    int i;
    int j = 0;
    for ( i=0; i<len; i++ ) {
        *dest = *src ^ crypto_key[j];
        src++;
        dest++;
        j++;
        if ( j >= 64 ) {
            j = 0;
        }
    }
}