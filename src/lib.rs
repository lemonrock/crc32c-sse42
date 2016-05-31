// This file is part of crc32c-sse42. It is subject to the license terms in the COPYRIGHT file found in the top-level directory of this distribution and at https://raw.githubusercontent.com/lemonrock/crc32c-sse42/master/COPYRIGHT. No part of crc32c-sse42, including this file, may be copied, modified, propagated, or distributed except according to the terms contained in the COPYRIGHT file.
// Copyright © 2016 The developers of crc32c-sse42. See the COPYRIGHT file in the top-level directory of this distribution and at https://raw.githubusercontent.com/lemonrock/crc32c-sse42/master/COPYRIGHT.


extern crate libc;
use libc::uint32_t;
use libc::c_void;
use libc::size_t;


#[allow(non_camel_case_types)] pub type crc32c = uint32_t;

extern "C"
{
	pub fn lemonrock_crc32c_sse42_calculate(initial: crc32c, buffer: *const c_void, length: size_t) -> crc32c;
}
