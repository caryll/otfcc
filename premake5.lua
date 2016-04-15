-- Premake 5 configurations
workspace "otfcc"
	configurations { "Debug", "Release" }
	platforms { "x32", "x64" }
	
	defines {
		'_CARYLL_USE_PRE_SERIALIZED',
		'MAIN_VER=0',
		"SECONDARY_VER=1",
		"PATCH_VER=1"
	}
	
	location "build"
	
	filter "action:vs2015"
		location "build/vs"
		toolset "msc-LLVM-vs2014"
		defines { '_CRT_SECURE_NO_WARNINGS' }
		buildoptions { '/MP', '/Wall', '-Wno-unused-parameter', '-Qunused-arguments' }
		flags { "StaticRuntime" }
		includedirs { "extern-msvc" }
	filter {}
	
	filter "action:gmake"
		location "build/gmake"
		buildoptions { '-Wall', '-Wno-multichar' }
	filter {}
	
	filter "configurations:Debug"
		defines { "DEBUG" }
		flags { "Symbols" }
	filter "configurations:Release"
		defines { "NDEBUG" }
		optimize "Full"

project "externals"
	kind "StaticLib"
	language "C"
	files {
		"extern/**.h",
		"extern/**.c"
	}

project "extern-msvc"
	kind "StaticLib"
	language "C"
	files {
		"extern-msvc/**.h",
		"extern-msvc/**.c"
	}

project "libotfcc"
	kind "StaticLib"
	language "C"
	files {
		"src/**.h",
		"src/**.c"
	}

	removefiles {
		"src-cli/**"
	}

project "otfccdump"
	kind "ConsoleApp"
	language "C"
	targetdir "bin/%{cfg.buildcfg}-%{cfg.platform}"
	
	links { "libotfcc", "externals" }
	filter "action:vs*"
		links "extern-msvc"
	filter {}
	
	files {
		"src-cli/**.c",
		"src-cli/**.h"
	}
	removefiles {
		"src-cli/otfccbuild.c"
	}

project "otfccbuild"
	kind "ConsoleApp"
	language "C"
	targetdir "bin/%{cfg.buildcfg}-%{cfg.platform}"
	
	links { "libotfcc", "externals" }
	filter "action:vs*"
		links "extern-msvc"
	filter {}
	
	files {
		"src-cli/**.c",
		"src-cli/**.h"
	}
	removefiles {
		"src-cli/otfccdump.c"
	}
