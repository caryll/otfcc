-- Premake 5 configurations
workspace "otfcc"
	configurations { "Debug", "Release" }
	platforms { "x32", "x64" }
	
	defines {
		'_CARYLL_USE_PRE_SERIALIZED',
		'MAIN_VER=0',
		"SECONDARY_VER=1",
		"PATCH_VER=5"
	}
	
	location "build"
	includedirs { "dep", "src" }
	
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
		buildoptions { '-std=gnu11', '-Wall', '-Wno-multichar' }
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
		"dep/extern/**.h",
		"dep/extern/**.c"
	}

project "extern-msvc"
	kind "StaticLib"
	language "C"
	files {
		"extern-msvc/**.h",
		"extern-msvc/**.c"
	}

project "libotfcc-support"
	kind "StaticLib"
	language "C"
	files {
		"src/support/**.h",
		"src/support/**.c"
	}

project "libotfcc-tables"
	kind "StaticLib"
	language "C"
	files {
		"src/tables/**.h",
		"src/tables/**.c"
	}

project "libotfcc-font"
	kind "StaticLib"
	language "C"
	files {
		"src/font/**.h",
		"src/font/**.c"
	}

project "libotfcc-fontops"
	kind "StaticLib"
	language "C"
	files {
		"src/fontops/**.h",
		"src/fontops/**.c"
	}

project "otfccdump"
	kind "ConsoleApp"
	language "C"
	targetdir "bin/%{cfg.buildcfg}-%{cfg.platform}"
	
	links { "libotfcc-fontops", "libotfcc-font", "libotfcc-tables", "libotfcc-support", "externals" }
	
	filter "action:vs*"
		links "extern-msvc"
	filter {}
	filter "action:gmake"
		links "m"
	filter {}
	
	files {
		"src/cli/**.c",
		"src/cli/**.h"
	}
	removefiles {
		"src/cli/otfccbuild.c"
	}

project "otfccbuild"
	kind "ConsoleApp"
	language "C"
	targetdir "bin/%{cfg.buildcfg}-%{cfg.platform}"
	
	links { "libotfcc-fontops", "libotfcc-font", "libotfcc-tables", "libotfcc-support", "externals" }
	
	filter "action:vs*"
		links "extern-msvc"
	filter {}
	filter "action:gmake"
		links "m"
	filter {}
	
	files {
		"src/cli/**.c",
		"src/cli/**.h"
	}
	removefiles {
		"src/cli/otfccdump.c"
	}
