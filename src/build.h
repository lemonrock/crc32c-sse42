// This file is part of crc32c-sse42. It is subject to the license terms in the COPYRIGHT file found in the top-level directory of this distribution and at https://raw.githubusercontent.com/lemonrock/crc32c-sse42/master/COPYRIGHT. No part of crc32c-sse42, including this file, may be copied, modified, propagated, or distributed except according to the terms contained in the COPYRIGHT file.
// Copyright © 2016 The developers of crc32c-sse42. See the COPYRIGHT file in the top-level directory of this distribution and at https://raw.githubusercontent.com/lemonrock/crc32c-sse42/master/COPYRIGHT.


#ifndef LEMONROCK_CRC32C_SSE42_BUILD_H
	
	#define LEMONROCK_CRC32C_SSE42_BUILD_H	

	#include "c/compatibility.h"
	#include "c/macros.h"
	#include "c/functions.h"
	#include <inttypes.h>
	#include <stdint.h>
	
	C_START


			typedef uint32_t crc32c;
	
			Pure Hot extern inline crc32c lemonrock_crc32c_sse42_calculate(const crc32c initial, const void * buffer, const size_t length);
	
		
	C_END

#endif
