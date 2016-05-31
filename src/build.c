// This file is part of crc32c-sse42. It is subject to the license terms in the COPYRIGHT file found in the top-level directory of this distribution and at https://raw.githubusercontent.com/lemonrock/crc32c-sse42/master/COPYRIGHT. No part of crc32c-sse42, including this file, may be copied, modified, propagated, or distributed except according to the terms contained in the COPYRIGHT file.
// Copyright © 2016 The developers of crc32c-sse42. See the COPYRIGHT file in the top-level directory of this distribution and at https://raw.githubusercontent.com/lemonrock/crc32c-sse42/master/COPYRIGHT.


#include "c/compatibility.h"


#if defined(__SSE4_2__)
	#if (defined(IsAmd64AndLp64) || defined(IsAmd64AndLlp64) ) && (defined(IsWindows) || defined(IsLinuxOrAndroid))
		#include "build.x86_asm.c"
	#else
		#include "build.x86_sse42.c"
	#endif
#else
	#include "build.software.c"
#endif
