// This file is part of crc32c-sse42. It is subject to the license terms in the COPYRIGHT file found in the top-level directory of this distribution and at https://raw.githubusercontent.com/lemonrock/crc32c-sse42/master/COPYRIGHT. No part of crc32c-sse42, including this file, may be copied, modified, propagated, or distributed except according to the terms contained in the COPYRIGHT file.
// Copyright © 2016 The developers of crc32c-sse42. See the COPYRIGHT file in the top-level directory of this distribution and at https://raw.githubusercontent.com/lemonrock/crc32c-sse42/master/COPYRIGHT.
// This copyright statement refers only to changes made to the source of https://github.com/htot/crc32c/blob/cae6221df605903b0a68e2fcdac07096f3f32a34/crc32c/crc32intelc.cc ; the original authors copyrights are respected.
// This is an altered source version


// Copyright 2016 Ferry Toth, Exalon Delft BV, The Netherlands
/*
  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the author be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
    claim that you wrote the original software. If you use this software
    in a product, an acknowledgment in the product documentation would be
    appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
    misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  Ferry Toth
  ftoth@exalondelft.nl
*/

/* Use hardware CRC instruction on Intel SSE 4.2 processors.  This computes a
  CRC-32C, *not* the CRC-32 used by Ethernet and zip, gzip, etc. Where efficient
  3 crc32q instructions are used which a single core can execute in parallel.
  This compensates for the latency of a single crc32q instruction. Combining the 
  3 CRC-32C bytes is done using the pclmulqdq instruction, which has overhead of
  its own, and makes this code path only efficient for buffer sizes above 216 bytes. 
  All code requiring a crc32q instruction is done inside a macro, for which alternative
  code is generated in case of a 32 bit platform.
  
  This code is a port of Intels crc_iscsi_v_pcl.asm assembly code (which is part of
  this project as well as in a modified form the linux kernel) and reaches the same 
  throughput on 64bit platforms. The main advantage of this port is that it was
  relatively easy to port to 32bit platforms (like Intel Edison which currently has
  only 32bit support). Being written in C it is of course easier to maintain and possibly
  optimize further */

/* Version history:
  1.0  07 May 2016  Ferry Toth - First version
*/

#include "build.h"


// From logging/crc32intelc.h
#ifndef __LP64__
#define CRC_NATIVE uint32_t
#else
#define CRC_NATIVE uint64_t
#endif

#ifndef __LP64__
#define CRCtriplet(crc, buf, offset) \
    crc ## 0 = __builtin_ia32_crc32si(crc ## 0, *((uint32_t*) buf ## 0 + 2 * offset)); \
    crc ## 1 = __builtin_ia32_crc32si(crc ## 1, *((uint32_t*) buf ## 1 + 2 * offset)); \
    crc ## 2 = __builtin_ia32_crc32si(crc ## 2, *((uint32_t*) buf ## 2 + 2 * offset)); \
    crc ## 0 = __builtin_ia32_crc32si(crc ## 0, *((uint32_t*) buf ## 0 + 1 + 2 * offset)); \
    crc ## 1 = __builtin_ia32_crc32si(crc ## 1, *((uint32_t*) buf ## 1 + 1 + 2 * offset)); \
    crc ## 2 = __builtin_ia32_crc32si(crc ## 2, *((uint32_t*) buf ## 2 + 1 + 2 * offset));
#else
#define CRCtriplet(crc, buf, offset) \
    crc ## 0 = __builtin_ia32_crc32di(crc ## 0, *(buf ## 0 + offset)); \
    crc ## 1 = __builtin_ia32_crc32di(crc ## 1, *(buf ## 1 + offset)); \
    crc ## 2 = __builtin_ia32_crc32di(crc ## 2, *(buf ## 2 + offset));
#endif

#ifndef __LP64__
#define CRCduplet(crc, buf, offset) \
    crc ## 0 = __builtin_ia32_crc32si(crc ## 0, *((uint32_t*) buf ## 0 + 2 * offset)); \
    crc ## 1 = __builtin_ia32_crc32si(crc ## 1, *((uint32_t*) buf ## 1 + 2 * offset)); \
    crc ## 0 = __builtin_ia32_crc32si(crc ## 0, *((uint32_t*) buf ## 0 + 1 + 2 * offset)); \
    crc ## 1 = __builtin_ia32_crc32si(crc ## 1, *((uint32_t*) buf ## 1 + 1 + 2 * offset));
#else
#define CRCduplet(crc, buf, offset) \
    crc ## 0 = __builtin_ia32_crc32di(crc ## 0, *(buf ## 0 + offset)); \
    crc ## 1 = __builtin_ia32_crc32di(crc ## 1, *(buf ## 1 + offset));
#endif


#ifndef __LP64__
#define CRCsinglet(crc, buf, offset) \
    crc = __builtin_ia32_crc32si(crc, *(uint32_t*)(buf + offset)); \
    crc = __builtin_ia32_crc32si(crc, *(uint32_t*)(buf + offset + sizeof(uint32_t)));
#else
#define CRCsinglet(crc, buf, offset) crc = __builtin_ia32_crc32di(crc, *(uint64_t*)(buf + offset));
#endif

/*
* CombineCRC performs pclmulqdq multiplication of 2 partial CRC's and a well chosen constant 
* and xor's these with the remaining CRC. I (Ferry Toth) could not find a way to implement this in
* C, so the 64bit code following here is from Intel. As that code runs only on 64 bit (due to movq
* instructions), I am providing a 32bit variant that does the same but using movd. The 32bit
* version keeps intermediate results longer in the xmm registers to do the 2nd xor, then moves the
* longs in 2 steps for the final crc32l
* 
*/

#ifndef __LP64__
#define CombineCRC()\
asm volatile (\
"movdqu (%3), %%xmm0\n\t"\
"movd %0, %%xmm1\n\t"\
"pclmullqlqdq %%xmm0, %%xmm1\n\t"\
"movd %2, %%xmm2\n\t"\
"pclmullqhqdq %%xmm0, %%xmm2\n\t"\
"pxor %%xmm2, %%xmm1\n\t"\
"movdqu (%4), %%xmm2\n\t"\
"pxor %%xmm2, %%xmm1\n\t"\
"movd %%xmm1, %0\n\t"\
"crc32l %0, %5\n\t"\
"pextrd $1, %%xmm1, %1\n\t"\
"crc32l %1, %5\n\t"\
"movl %5, %0"\
: "=r" ( crc0 )\
: "0" ( crc0 ), "r" ( crc1 ), "r" ( K + block_size - 1 ), "r" ( ( uint64_t* ) next2 - 1 ), "r" ( crc2 )\
: "%xmm0", "%xmm1", "%xmm2"\
);
#else
#define CombineCRC()\
asm volatile (\
"movdqa (%3), %%xmm0\n\t"\
"movq %0, %%xmm1\n\t"\
"pclmullqlqdq %%xmm0, %%xmm1\n\t"\
"movq %2, %%xmm2\n\t"\
"pclmullqhqdq %%xmm0, %%xmm2\n\t"\
"pxor %%xmm2, %%xmm1\n\t"\
"movq %%xmm1, %0"\
: "=r" ( crc0 ) \
: "0" ( crc0 ), "r" ( crc1 ), "r" ( K + block_size - 1 ) \
: "%xmm0", "%xmm1", "%xmm2"\
); \
crc0 = crc0 ^ * ( ( uint64_t* ) next2 - 1 );\
crc2 = __builtin_ia32_crc32di ( crc2, crc0 );\
crc0 = crc2;
#endif


//#include "logging/crc32c.h"
//#include "logging/crc32intelc.h"
#include <x86intrin.h>

//namespace logging
//{

extern __v2di K[];
__v2di K[] = {
        {0x14cd00bd6, 0x105ec76f0},
        {0x0ba4fc28e, 0x14cd00bd6},
        {0x1d82c63da, 0x0f20c0dfe},
        {0x09e4addf8, 0x0ba4fc28e},
        {0x039d3b296, 0x1384aa63a},
        {0x102f9b8a2, 0x1d82c63da},
        {0x14237f5e6, 0x01c291d04},
        {0x00d3b6092, 0x09e4addf8},
        {0x0c96cfdc0, 0x0740eef02},
        {0x18266e456, 0x039d3b296},
        {0x0daece73e, 0x0083a6eec},
        {0x0ab7aff2a, 0x102f9b8a2},
        {0x1248ea574, 0x1c1733996},
        {0x083348832, 0x14237f5e6},
        {0x12c743124, 0x02ad91c30},
        {0x0b9e02b86, 0x00d3b6092},
        {0x018b33a4e, 0x06992cea2},
        {0x1b331e26a, 0x0c96cfdc0},
        {0x17d35ba46, 0x07e908048},
        {0x1bf2e8b8a, 0x18266e456},
        {0x1a3e0968a, 0x11ed1f9d8},
        {0x0ce7f39f4, 0x0daece73e},
        {0x061d82e56, 0x0f1d0f55e},
        {0x0d270f1a2, 0x0ab7aff2a},
        {0x1c3f5f66c, 0x0a87ab8a8},
        {0x12ed0daac, 0x1248ea574},
        {0x065863b64, 0x08462d800},
        {0x11eef4f8e, 0x083348832},
        {0x1ee54f54c, 0x071d111a8},
        {0x0b3e32c28, 0x12c743124},
        {0x0064f7f26, 0x0ffd852c6},
        {0x0dd7e3b0c, 0x0b9e02b86},
        {0x0f285651c, 0x0dcb17aa4},
        {0x010746f3c, 0x018b33a4e},
        {0x1c24afea4, 0x0f37c5aee},
        {0x0271d9844, 0x1b331e26a},
        {0x08e766a0c, 0x06051d5a2},
        {0x093a5f730, 0x17d35ba46},
        {0x06cb08e5c, 0x11d5ca20e},
        {0x06b749fb2, 0x1bf2e8b8a},
        {0x1167f94f2, 0x021f3d99c},
        {0x0cec3662e, 0x1a3e0968a},
        {0x19329634a, 0x08f158014},
        {0x0e6fc4e6a, 0x0ce7f39f4},
        {0x08227bb8a, 0x1a5e82106},
        {0x0b0cd4768, 0x061d82e56},
        {0x13c2b89c4, 0x188815ab2},
        {0x0d7a4825c, 0x0d270f1a2},
        {0x10f5ff2ba, 0x105405f3e},
        {0x00167d312, 0x1c3f5f66c},
        {0x0f6076544, 0x0e9adf796},
        {0x026f6a60a, 0x12ed0daac},
        {0x1a2adb74e, 0x096638b34},
        {0x19d34af3a, 0x065863b64},
        {0x049c3cc9c, 0x1e50585a0},
        {0x068bce87a, 0x11eef4f8e},
        {0x1524fa6c6, 0x19f1c69dc},
        {0x16cba8aca, 0x1ee54f54c},
        {0x042d98888, 0x12913343e},
        {0x1329d9f7e, 0x0b3e32c28},
        {0x1b1c69528, 0x088f25a3a},
        {0x02178513a, 0x0064f7f26},
        {0x0e0ac139e, 0x04e36f0b0},
        {0x0170076fa, 0x0dd7e3b0c},
        {0x141a1a2e2, 0x0bd6f81f8},
        {0x16ad828b4, 0x0f285651c},
        {0x041d17b64, 0x19425cbba},
        {0x1fae1cc66, 0x010746f3c},
        {0x1a75b4b00, 0x18db37e8a},
        {0x0f872e54c, 0x1c24afea4},
        {0x01e41e9fc, 0x04c144932},
        {0x086d8e4d2, 0x0271d9844},
        {0x160f7af7a, 0x052148f02},
        {0x05bb8f1bc, 0x08e766a0c},
        {0x0a90fd27a, 0x0a3c6f37a},
        {0x0b3af077a, 0x093a5f730},
        {0x04984d782, 0x1d22c238e},
        {0x0ca6ef3ac, 0x06cb08e5c},
        {0x0234e0b26, 0x063ded06a},
        {0x1d88abd4a, 0x06b749fb2},
        {0x04597456a, 0x04d56973c},
        {0x0e9e28eb4, 0x1167f94f2},
        {0x07b3ff57a, 0x19385bf2e},
        {0x0c9c8b782, 0x0cec3662e},
        {0x13a9cba9e, 0x0e417f38a},
        {0x093e106a4, 0x19329634a},
        {0x167001a9c, 0x14e727980},
        {0x1ddffc5d4, 0x0e6fc4e6a},
        {0x00df04680, 0x0d104b8fc},
        {0x02342001e, 0x08227bb8a},
        {0x00a2a8d7e, 0x05b397730},
        {0x168763fa6, 0x0b0cd4768},
        {0x1ed5a407a, 0x0e78eb416},
        {0x0d2c3ed1a, 0x13c2b89c4},
        {0x0995a5724, 0x1641378f0},
        {0x19b1afbc4, 0x0d7a4825c},
        {0x109ffedc0, 0x08d96551c},
        {0x0f2271e60, 0x10f5ff2ba},
        {0x00b0bf8ca, 0x00bf80dd2},
        {0x123888b7a, 0x00167d312},
        {0x1e888f7dc, 0x18dcddd1c},
        {0x002ee03b2, 0x0f6076544},
        {0x183e8d8fe, 0x06a45d2b2},
        {0x133d7a042, 0x026f6a60a},
        {0x116b0f50c, 0x1dd3e10e8},
        {0x05fabe670, 0x1a2adb74e},
        {0x130004488, 0x0de87806c},
        {0x000bcf5f6, 0x19d34af3a},
        {0x18f0c7078, 0x014338754},
        {0x017f27698, 0x049c3cc9c},
        {0x058ca5f00, 0x15e3e77ee},
        {0x1af900c24, 0x068bce87a},
        {0x0b5cfca28, 0x0dd07448e},
        {0x0ded288f8, 0x1524fa6c6},
        {0x059f229bc, 0x1d8048348},
        {0x06d390dec, 0x16cba8aca},
        {0x037170390, 0x0a3e3e02c},
        {0x06353c1cc, 0x042d98888},
        {0x0c4584f5c, 0x0d73c7bea},
        {0x1f16a3418, 0x1329d9f7e},
        {0x0531377e2, 0x185137662},
        {0x1d8d9ca7c, 0x1b1c69528},
        {0x0b25b29f2, 0x18a08b5bc},
        {0x19fb2a8b0, 0x02178513a},
        {0x1a08fe6ac, 0x1da758ae0},
        {0x045cddf4e, 0x0e0ac139e},
        {0x1a91647f2, 0x169cf9eb0},
        {0x1a0f717c4, 0x0170076fa}
};

/* Compute CRC-32C using the Intel hardware instruction. */
// uint32_t crc32cIntelC ( uint32_t crc, const void *buf, size_t len )
Pure Hot extern inline crc32c lemonrock_crc32c_sse42_calculate(const crc32c initial, const void * buffer, const size_t length)
{
        // const unsigned char *next = ( const unsigned char * ) buf;
	    const unsigned char *next = ( const unsigned char * ) buffer;
		// RJC
		size_t len = length;
        unsigned long count;
        CRC_NATIVE crc0, crc1, crc2;
        crc0 = initial;

        if ( len >= 8 ) {
                // if len > 216 then align and use triplets
                if ( len > 216 ) {
                        {
                                uint32_t crc32bit = crc0;                                       // create this block actually prevent 2 asignments
                                unsigned long align = ( 8 - ( uintptr_t ) next ) % 8;           // byte to boundary
                                len -= align;
                                if ( align & 0x04 ) {
                                        crc32bit = __builtin_ia32_crc32si ( crc32bit, * ( uint32_t* ) next );
                                        next += sizeof(uint32_t);
                                };
                                if ( align & 0x02 ) {
                                        crc32bit = __builtin_ia32_crc32hi ( crc32bit, * ( uint16_t* ) next );
                                        next += sizeof(uint16_t);
                                };

                                if ( align & 0x01 ) {
                                        crc32bit = __builtin_ia32_crc32qi ( crc32bit, * ( next ) );
                                        next++;
                                };
                                crc0 = crc32bit;
                        };

                        // use Duff's device, a for() loop inside a switch() statement. This is Legal
                        // needs to execute at least once, round len down to nearast triplet multiple
                        count = len / 24;			// number of triplets
                        len %= 24;				// bytes remaining
                        unsigned long n = count / 128;		// #blocks = first block + full blocks
                        unsigned long block_size = count % 128;
                        if ( block_size == 0 ) {
                                block_size = 128;
                        } else {
                                n++;
                        };
                        const uint64_t *next0 = ( uint64_t* ) next + block_size; // points to the first byte of the next block
                        const uint64_t *next1 = next0 + block_size;
                        const uint64_t *next2 = next1 + block_size;

                        crc1 = crc2 = 0;
                        switch ( block_size ) {
                        case 128:
                                do {
                                        CRCtriplet ( crc, next, -128 );	// jumps here for a full block of len 128
                                case 127:
                                        CRCtriplet ( crc, next, -127 );	// jumps here or below for the first block smaller
                                case 126:
                                        CRCtriplet ( crc, next, -126 );	// than 128
                                case 125:
                                        CRCtriplet ( crc, next, -125 );
                                case 124:
                                        CRCtriplet ( crc, next, -124 );
                                case 123:
                                        CRCtriplet ( crc, next, -123 );
                                case 122:
                                        CRCtriplet ( crc, next, -122 );
                                case 121:
                                        CRCtriplet ( crc, next, -121 );
                                case 120:
                                        CRCtriplet ( crc, next, -120 );
                                case 119:
                                        CRCtriplet ( crc, next, -119 );
                                case 118:
                                        CRCtriplet ( crc, next, -118 );
                                case 117:
                                        CRCtriplet ( crc, next, -117 );
                                case 116:
                                        CRCtriplet ( crc, next, -116 );
                                case 115:
                                        CRCtriplet ( crc, next, -115 );
                                case 114:
                                        CRCtriplet ( crc, next, -114 );
                                case 113:
                                        CRCtriplet ( crc, next, -113 );
                                case 112:
                                        CRCtriplet ( crc, next, -112 );
                                case 111:
                                        CRCtriplet ( crc, next, -111 );
                                case 110:
                                        CRCtriplet ( crc, next, -110 );
                                case 109:
                                        CRCtriplet ( crc, next, -109 );
                                case 108:
                                        CRCtriplet ( crc, next, -108 );
                                case 107:
                                        CRCtriplet ( crc, next, -107 );
                                case 106:
                                        CRCtriplet ( crc, next, -106 );
                                case 105:
                                        CRCtriplet ( crc, next, -105 );
                                case 104:
                                        CRCtriplet ( crc, next, -104 );
                                case 103:
                                        CRCtriplet ( crc, next, -103 );
                                case 102:
                                        CRCtriplet ( crc, next, -102 );
                                case 101:
                                        CRCtriplet ( crc, next, -101 );
                                case 100:
                                        CRCtriplet ( crc, next, -100 );
                                case 99:
                                        CRCtriplet ( crc, next, -99 );
                                case 98:
                                        CRCtriplet ( crc, next, -98 );
                                case 97:
                                        CRCtriplet ( crc, next, -97 );
                                case 96:
                                        CRCtriplet ( crc, next, -96 );
                                case 95:
                                        CRCtriplet ( crc, next, -95 );
                                case 94:
                                        CRCtriplet ( crc, next, -94 );
                                case 93:
                                        CRCtriplet ( crc, next, -93 );
                                case 92:
                                        CRCtriplet ( crc, next, -92 );
                                case 91:
                                        CRCtriplet ( crc, next, -91 );
                                case 90:
                                        CRCtriplet ( crc, next, -90 );
                                case 89:
                                        CRCtriplet ( crc, next, -89 );
                                case 88:
                                        CRCtriplet ( crc, next, -88 );
                                case 87:
                                        CRCtriplet ( crc, next, -87 );
                                case 86:
                                        CRCtriplet ( crc, next, -86 );
                                case 85:
                                        CRCtriplet ( crc, next, -85 );
                                case 84:
                                        CRCtriplet ( crc, next, -84 );
                                case 83:
                                        CRCtriplet ( crc, next, -83 );
                                case 82:
                                        CRCtriplet ( crc, next, -82 );
                                case 81:
                                        CRCtriplet ( crc, next, -81 );
                                case 80:
                                        CRCtriplet ( crc, next, -80 );
                                case 79:
                                        CRCtriplet ( crc, next, -79 );
                                case 78:
                                        CRCtriplet ( crc, next, -78 );
                                case 77:
                                        CRCtriplet ( crc, next, -77 );
                                case 76:
                                        CRCtriplet ( crc, next, -76 );
                                case 75:
                                        CRCtriplet ( crc, next, -75 );
                                case 74:
                                        CRCtriplet ( crc, next, -74 );
                                case 73:
                                        CRCtriplet ( crc, next, -73 );
                                case 72:
                                        CRCtriplet ( crc, next, -72 );
                                case 71:
                                        CRCtriplet ( crc, next, -71 );
                                case 70:
                                        CRCtriplet ( crc, next, -70 );
                                case 69:
                                        CRCtriplet ( crc, next, -69 );
                                case 68:
                                        CRCtriplet ( crc, next, -68 );
                                case 67:
                                        CRCtriplet ( crc, next, -67 );
                                case 66:
                                        CRCtriplet ( crc, next, -66 );
                                case 65:
                                        CRCtriplet ( crc, next, -65 );
                                case 64:
                                        CRCtriplet ( crc, next, -64 );
                                case 63:
                                        CRCtriplet ( crc, next, -63 );
                                case 62:
                                        CRCtriplet ( crc, next, -62 );
                                case 61:
                                        CRCtriplet ( crc, next, -61 );
                                case 60:
                                        CRCtriplet ( crc, next, -60 );
                                case 59:
                                        CRCtriplet ( crc, next, -59 );
                                case 58:
                                        CRCtriplet ( crc, next, -58 );
                                case 57:
                                        CRCtriplet ( crc, next, -57 );
                                case 56:
                                        CRCtriplet ( crc, next, -56 );
                                case 55:
                                        CRCtriplet ( crc, next, -55 );
                                case 54:
                                        CRCtriplet ( crc, next, -54 );
                                case 53:
                                        CRCtriplet ( crc, next, -53 );
                                case 52:
                                        CRCtriplet ( crc, next, -52 );
                                case 51:
                                        CRCtriplet ( crc, next, -51 );
                                case 50:
                                        CRCtriplet ( crc, next, -50 );
                                case 49:
                                        CRCtriplet ( crc, next, -49 );
                                case 48:
                                        CRCtriplet ( crc, next, -48 );
                                case 47:
                                        CRCtriplet ( crc, next, -47 );
                                case 46:
                                        CRCtriplet ( crc, next, -46 );
                                case 45:
                                        CRCtriplet ( crc, next, -45 );
                                case 44:
                                        CRCtriplet ( crc, next, -44 );
                                case 43:
                                        CRCtriplet ( crc, next, -43 );
                                case 42:
                                        CRCtriplet ( crc, next, -42 );
                                case 41:
                                        CRCtriplet ( crc, next, -41 );
                                case 40:
                                        CRCtriplet ( crc, next, -40 );
                                case 39:
                                        CRCtriplet ( crc, next, -39 );
                                case 38:
                                        CRCtriplet ( crc, next, -38 );
                                case 37:
                                        CRCtriplet ( crc, next, -37 );
                                case 36:
                                        CRCtriplet ( crc, next, -36 );
                                case 35:
                                        CRCtriplet ( crc, next, -35 );
                                case 34:
                                        CRCtriplet ( crc, next, -34 );
                                case 33:
                                        CRCtriplet ( crc, next, -33 );
                                case 32:
                                        CRCtriplet ( crc, next, -32 );
                                case 31:
                                        CRCtriplet ( crc, next, -31 );
                                case 30:
                                        CRCtriplet ( crc, next, -30 );
                                case 29:
                                        CRCtriplet ( crc, next, -29 );
                                case 28:
                                        CRCtriplet ( crc, next, -28 );
                                case 27:
                                        CRCtriplet ( crc, next, -27 );
                                case 26:
                                        CRCtriplet ( crc, next, -26 );
                                case 25:
                                        CRCtriplet ( crc, next, -25 );
                                case 24:
                                        CRCtriplet ( crc, next, -24 );
                                case 23:
                                        CRCtriplet ( crc, next, -23 );
                                case 22:
                                        CRCtriplet ( crc, next, -22 );
                                case 21:
                                        CRCtriplet ( crc, next, -21 );
                                case 20:
                                        CRCtriplet ( crc, next, -20 );
                                case 19:
                                        CRCtriplet ( crc, next, -19 );
                                case 18:
                                        CRCtriplet ( crc, next, -18 );
                                case 17:
                                        CRCtriplet ( crc, next, -17 );
                                case 16:
                                        CRCtriplet ( crc, next, -16 );
                                case 15:
                                        CRCtriplet ( crc, next, -15 );
                                case 14:
                                        CRCtriplet ( crc, next, -14 );
                                case 13:
                                        CRCtriplet ( crc, next, -13 );
                                case 12:
                                        CRCtriplet ( crc, next, -12 );
                                case 11:
                                        CRCtriplet ( crc, next, -11 );
                                case 10:
                                        CRCtriplet ( crc, next, -10 );
                                case 9:
                                        CRCtriplet ( crc, next, -9 );
                                case 8:
                                        CRCtriplet ( crc, next, -8 );
                                case 7:
                                        CRCtriplet ( crc, next, -7 );
                                case 6:
                                        CRCtriplet ( crc, next, -6 );
                                case 5:
                                        CRCtriplet ( crc, next, -5 );
                                case 4:
                                        CRCtriplet ( crc, next, -4 );
                                case 3:
                                        CRCtriplet ( crc, next, -3 );
                                case 2:
                                        CRCtriplet ( crc, next, -2 );
                                case 1:
                                        CRCduplet ( crc, next, -1 );		        // the final triplet is actually only 2
                                        CombineCRC();
                                        if ( --n > 0 ) {
                                                crc1 = crc2 = 0;
                                                block_size = 128;
                                                next0 = next2 + 128;			// points to the first byte of the next block
                                                next1 = next0 + 128;			// from here on all blocks are 128 long
                                                next2 = next1 + 128;
                                        };
                                case 0:
                                        ;
                                } while ( n > 0 );
                        };
                        next = ( const unsigned char* ) next2;
                };
                unsigned count = len / 8;                                               // 216 of less bytes is 27 or less singlets
                len %= 8;
                next += ( count * 8 );
                switch ( count ) {
                case 27:
                        CRCsinglet ( crc0, next, -27 * 8 );
                case 26:
                        CRCsinglet ( crc0, next, -26 * 8 );
                case 25:
                        CRCsinglet ( crc0, next, -25 * 8 );
                case 24:
                        CRCsinglet ( crc0, next, -24 * 8 );
                case 23:
                        CRCsinglet ( crc0, next, -23 * 8 );
                case 22:
                        CRCsinglet ( crc0, next, -22 * 8 );
                case 21:
                        CRCsinglet ( crc0, next, -21 * 8 );
                case 20:
                        CRCsinglet ( crc0, next, -20 * 8 );
                case 19:
                        CRCsinglet ( crc0, next, -19 * 8 );
                case 18:
                        CRCsinglet ( crc0, next, -18 * 8 );
                case 17:
                        CRCsinglet ( crc0, next, -17 * 8 );
                case 16:
                        CRCsinglet ( crc0, next, -16 * 8 );
                case 15:
                        CRCsinglet ( crc0, next, -15 * 8 );
                case 14:
                        CRCsinglet ( crc0, next, -14 * 8 );
                case 13:
                        CRCsinglet ( crc0, next, -13 * 8 );
                case 12:
                        CRCsinglet ( crc0, next, -12 * 8 );
                case 11:
                        CRCsinglet ( crc0, next, -11 * 8 );
                case 10:
                        CRCsinglet ( crc0, next, -10 * 8 );
                case 9:
                        CRCsinglet ( crc0, next, -9 * 8 );
                case 8:
                        CRCsinglet ( crc0, next, -8 * 8 );
                case 7:
                        CRCsinglet ( crc0, next, -7 * 8 );
                case 6:
                        CRCsinglet ( crc0, next, -6 * 8 );
                case 5:
                        CRCsinglet ( crc0, next, -5 * 8 );
                case 4:
                        CRCsinglet ( crc0, next, -4 * 8 );
                case 3:
                        CRCsinglet ( crc0, next, -3 * 8 );
                case 2:
                        CRCsinglet ( crc0, next, -2 * 8 );
                case 1:
                        CRCsinglet ( crc0, next, -1 * 8 );
                case 0:
                        ;
                };

        };
        {
                uint32_t crc32bit = crc0;
                // less than 8 bytes remain
                /* compute the crc for up to seven trailing bytes */
                if ( len & 0x04 ) {
                        crc32bit = __builtin_ia32_crc32si ( crc32bit, * ( uint32_t* ) next );
                        next += 4;
                };
                if ( len & 0x02 ) {
                        crc32bit = __builtin_ia32_crc32hi ( crc32bit, * ( uint16_t* ) next );
                        next += 2;
                };

                if ( len & 0x01 ) {
                        crc32bit = __builtin_ia32_crc32qi ( crc32bit, * ( next ) );
                };
                return ( uint32_t ) crc32bit;
        };
};

//}  // namespace logging
// kate: indent-mode cstyle; indent-width 8; replace-tabs on; 
