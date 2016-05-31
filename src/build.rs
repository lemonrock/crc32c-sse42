// This file is part of c. It is subject to the license terms in the COPYRIGHT file found in the top-level directory of this distribution and at https://raw.githubusercontent.com/lemonrock/c/master/COPYRIGHT. No part of c, including this file, may be copied, modified, propagated, or distributed except according to the terms contained in the COPYRIGHT file.
// Copyright © 2016 The developers of c. See the COPYRIGHT file in the top-level directory of this distribution and at https://raw.githubusercontent.com/lemonrock/c/master/COPYRIGHT.


use std::process::Command;
use std::process::Stdio;
use std::env;
extern crate gcc;
use gcc::Config;


fn main()
{
	let out_dir = env::var("OUT_DIR").unwrap();
	let mut config = Config::new();
	config.flag("-Wall");
	config.flag("-Werror");

	if cfg!(feature = "sse42")
	{
		if cfg!(target_arch = "x86_64") || cfg!(target_arch = "x86")
		{
			config.flag("-mpclmul");
			config.flag("-msse4.2");
		}
				
		// We check target_pointer_width for when (if ever) Rust adds x32 (ILP32) support on Linux
		if cfg!(target_arch = "x86_64") && cfg!(target_pointer_width = "64") && (cfg!(windows) || cfg!(any(target_os = "linux", target_os = "android")))
		{
			if has_yasm()
			{
				let source_file = "src/crc_iscsi_v_pcl.asm";
				let object_file = format!("{}/crc_iscsi_v_pcl.o", out_dir);
				let arguments;
				if cfg!(windows)
				{
					arguments = vec!["-X", "vc", "-f", "x64", "-rnasm", "-pnasm", "-o", &object_file, &source_file];
				}
				else if cfg!(any(target_os = "linux", target_os = "android"))
				{
					arguments = vec!["-X", "gnu", "-f", "x64", "-f", "elf64", "-g", "dwarf2", "-D", "LINUX", "-o", &object_file, &source_file];
				}
				else
				{
					panic!("Can not build for this configuration");
				}
			    Command::new("yasm").args(&arguments).status().unwrap();
		
				config.flag(&object_file);
			}
			else
			{
				println!("Using SSE 4.2 and CLMUL but slightly suboptimally without yasm");
			}
		}
	}

	config.file("src/build.c");
	config.compile("libbuild.a");
}

fn has_yasm() -> bool
{
	match Command::new("yasm").args(&["-h"]).stdin(Stdio::null()).stdout(Stdio::null()).stderr(Stdio::null()).status()
	{
		Ok(_) => true,
		Err(_) => false,
	}
}