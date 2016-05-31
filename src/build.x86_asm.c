// This file is part of crc32c-sse42. It is subject to the license terms in the COPYRIGHT file found in the top-level directory of this distribution and at https://raw.githubusercontent.com/lemonrock/crc32c-sse42/master/COPYRIGHT. No part of crc32c-sse42, including this file, may be copied, modified, propagated, or distributed except according to the terms contained in the COPYRIGHT file.
// Copyright © 2016 The developers of crc32c-sse42. See the COPYRIGHT file in the top-level directory of this distribution and at https://raw.githubusercontent.com/lemonrock/crc32c-sse42/master/COPYRIGHT.


#include "build.h"


#if defined(IsAmd64) && defined(__SSE4_2__)

	// Defined in crc_iscsi_v_pcl.asm
	extern unsigned int crc_pcl(unsigned char * buffer, int len, unsigned int crc_init);

	Pure Hot extern inline crc32c lemonrock_crc32c_sse42_calculate(const crc32c initial, const void * buffer, const size_t length)
	{
		return (crc32c) crc_pcl((unsigned char *) buffer, (int) length, (unsigned int) initial);
	}
	
#endif
