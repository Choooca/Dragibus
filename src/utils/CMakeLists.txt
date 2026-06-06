cmake_minimum_required(VERSION 4.1.2)

configure_file(build_macro.h.in build_macro.h)

target_sources(Dragibus
	PRIVATE 
		debug_macro.h
		files.cpp
)