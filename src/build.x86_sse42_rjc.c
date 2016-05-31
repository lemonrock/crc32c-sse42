// This file is part of crc32c-sse42. It is subject to the license terms in the COPYRIGHT file found in the top-level directory of this distribution and at https://raw.githubusercontent.com/lemonrock/crc32c-sse42/master/COPYRIGHT. No part of crc32c-sse42, including this file, may be copied, modified, propagated, or distributed except according to the terms contained in the COPYRIGHT file.
// Copyright © 2016 The developers of crc32c-sse42. See the COPYRIGHT file in the top-level directory of this distribution and at https://raw.githubusercontent.com/lemonrock/crc32c-sse42/master/COPYRIGHT.


#include "build.h"


#if defined(IsAmd64) && defined(__SSE4_2__)
	#include <nmmintrin.h>

	#define memory(pointer, type) * ((type *) ((void *) (pointer)))

	Pure Hot static inline crc32c lemonrock_crc32c_sse42_calculateSmall(crc32c partial, uintptr_t pointer, size_t remainingLength)
	{
		uint32_t crc = partial;
	
		for(size_t index = 0; index < remainingLength; index++)
		{
		    crc = _mm_crc32_u8(crc, memory(pointer + index, uint8_t));
		}
	
		return crc;
	}
	
	Pure Hot extern inline crc32c lemonrock_crc32c_sse42_calculate(const crc32c initial, const void * buffer, const size_t length)
	{
	    crc32c crc = initial ^ UINT32_MAX;
		uintptr_t pointer = (uintptr_t) buffer;
		size_t remainingLength = length;
	
		#if defined(IsAmd64AndLp64) || defined(IsAmd64AndLlp64)
		
			if(unlikely(remainingLength < 8))
			{
				crc = lemonrock_crc32c_sse42_calculateSmall(initial, pointer, remainingLength);
			}
			else
			{
				// Handle pointers that are not 64-bit aligned
		
				switch (pointer % 8UL)
				{
					case 7:
						crc = _mm_crc32_u8(crc, memory(pointer, uint8_t));
						remainingLength -= 1;
						pointer += 1;
						break;
	
					case 6:
						crc = _mm_crc32_u16(crc, memory(pointer, uint16_t));
						remainingLength -= 2;
						pointer += 2;
						break;
	
					case 5:
						crc = _mm_crc32_u8(crc, memory(pointer, uint8_t));
						crc = _mm_crc32_u16(crc, memory(pointer + 1, uint16_t));
						remainingLength -= 3;
						pointer += 3;
						break;
	
					case 4:
						crc = _mm_crc32_u32(crc, memory(pointer, uint32_t));
						remainingLength -= 4;
						pointer += 4;
						break;
	
					case 3:
						crc = _mm_crc32_u8(crc, memory(pointer, uint8_t));
						crc = _mm_crc32_u32(crc, memory(pointer + 1, uint32_t));
						remainingLength -= 5;
						pointer += 5;
						break;
	
					case 2:
						crc = _mm_crc32_u16(crc, memory(pointer, uint16_t));
						crc = _mm_crc32_u32(crc, memory(pointer + 2, uint32_t));
						remainingLength -= 6;
						pointer += 6;
						break;
	
					case 1:
						crc = _mm_crc32_u8(crc, memory(pointer, uint8_t));
						crc = _mm_crc32_u16(crc, memory(pointer + 1, uint16_t));
						crc = _mm_crc32_u32(crc, memory(pointer + 3, uint32_t));
						remainingLength -= 7;
						pointer += 7;
						break;
				}
			
				while(likely(remainingLength >= 8))
				{
					remainingLength -= 8;
					pointer += 8;
					crc = _mm_crc32_u64(crc, memory(pointer, uint64_t));
				}
		
				switch (remainingLength)
				{
					case 7:
						crc = _mm_crc32_u32(crc, memory(pointer, uint32_t));
						crc = _mm_crc32_u16(crc, memory(pointer + 4, uint16_t));
						crc = _mm_crc32_u8(crc, memory(pointer + 6, uint8_t));
						break;
				
					case 6:
						crc = _mm_crc32_u32(crc, memory(pointer, uint32_t));
						crc = _mm_crc32_u16(crc, memory(pointer + 4, uint16_t));
						break;
				
					case 5:
						crc = _mm_crc32_u32(crc, memory(pointer, uint32_t));
						crc = _mm_crc32_u8(crc, memory(pointer + 4, uint8_t));
						break;
				
					case 4:
						crc = _mm_crc32_u32(crc, memory(pointer, uint32_t));
						break;
				
					case 3:
						crc = _mm_crc32_u16(crc, memory(pointer, uint16_t));
						crc = _mm_crc32_u8(crc, memory(pointer + 2, uint8_t));
						break;
				
					case 2:
						crc = _mm_crc32_u16(crc, memory(pointer, uint16_t));
						break;
				
					case 1:
						crc = _mm_crc32_u8(crc, memory(pointer, uint8_t));
						break;
				}
			}
		
		#else
		
			if(unlikely(remainingLength < 4))
			{
				crc = lemonrock_crc32c_sse42_calculateSmall(crc, pointer, remainingLength);
			}
			else
			{
				// Handle pointers that are not 32-bit aligned
		
				switch (pointer % 4UL)
				{
					case 3:
						crc = _mm_crc32_u8(crc, memory(pointer, uint8_t));
						remainingLength -= 1;
						pointer += 1;
						break;
	
					case 2:
						crc = _mm_crc32_u16(crc, memory(pointer, uint16_t));
						remainingLength -= 2;
						pointer += 2;
						break;
	
					case 1:
						crc = _mm_crc32_u8(crc, memory(pointer, uint8_t));
						crc = _mm_crc32_u16(crc, memory(pointer + 1, uint16_t));
						remainingLength -= 3;
						pointer += 3;
						break;
				}
		
				while(likely(remainingLength >= 4))
				{
					remainingLength -= 4;
					pointer += 4;
					crc = _mm_crc32_u32(crc, memory(pointer, uint32_t));
				}

				switch (remainingLength)
				{
					case 3:
						crc = _mm_crc32_u16(crc, memory(pointer));
						crc = _mm_crc32_u8(crc, memory(pointer + 2, uint8_t));
						break;
				
					case 2:
						crc = _mm_crc32_u16(crc, memory(pointer, uint16_t));
						break;
				
					case 1:
						crc = _mm_crc32_u8(crc, memory(pointer, uint8_t));
						break;
				}
			}
		
		#endif
		
	    return crc ^ UINT32_MAX;
	}
#endif
