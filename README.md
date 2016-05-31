[](This file is part of crc32c-sse42. It is subject to the license terms in the COPYRIGHT file found in the top-level directory of this distribution and at https://raw.githubusercontent.com/lemonrock/crc32c-sse42/master/COPYRIGHT. No part of crc32c-sse42, including this file, may be copied, modified, propagated, or distributed except according to the terms contained in the COPYRIGHT file.)
[](Copyright © 2016 The developers of crc32c-sse42. See the COPYRIGHT file in the top-level directory of this distribution and at https://raw.githubusercontent.com/lemonrock/crc32c-sse42/master/COPYRIGHT.)

# crc32c-sse42

[crc32c-sse42] is a rust crate providing, where possible, hardware optimised (using SSE4.2 and the CLMUL instruction set (specifically, `PCLMULQDQ`)). Since SSE 4.2 is not (as of 2016) ubiquitous, a non-hardware-optimised version is supplied. Wherever possible, an assembler variant is used. This only works on 64-bit X86-64 Linux and Windows. A slightly less optimal solution that works on 64-bit and 32-bit (including x32 x86-64) x86 on any major OS is used secondarily (originally developed by Ferry Toth). Finally, the fallback logic is to do everything entirely in software.

For interest, a 32-bit and 64-bit implementation that uses SSE 4.2 but not CLMUL is included but not built (`src/build.sse42_rjc.c`).


## Compiling and Installing

* The feature flag `sse42` is set by default; disable it if installing on x86 platforms that lack SSE4.2.
* The program `yasm` must be present at build time if the feature flag `sse42` is set.


## Future Optimisations

* It is possible to introduce a fallback optimisation for 64-bit and 32-bit X86 CPUs which lack SSE 4.2 but have the CLMUL instruction set by using Intel's Slicing-by-8 code on SourceForge.


## Licensing

The license for this project is MIT but some code is licensed under compatible licenses:-

* The file `src/crc_iscsi_v_pcl.asm` is licensed separately by Intel and was originally found in the GitHub repository [crc32c](https://github.com/htot/crc32c).
* The file `src/build.software.c` is derived from posts by Mark Adler and Robert Važan on [StackOverflow](https://stackoverflow.com/questions/17645167/implementing-sse-4-2s-crc32c-in-software).

[crc32c-sse42]: https://github.com/lemonrock/crc32c-sse42 "crc32c-sse42 GitHub page"
