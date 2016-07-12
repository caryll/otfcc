require "dep/premake-modules/xcode-alt"

-- Premake 5 configurations
workspace "otfcc"
	configurations { "Debug", "Release" }
	platforms { "x32", "x64" }
	
	defines {
		'_CARYLL_USE_PRE_SERIALIZED',
		'MAIN_VER=0',
		"SECONDARY_VER=2",
		"PATCH_VER=1"
	}
	
	location "build"
	includedirs { "dep", "lib" }
	
	filter "action:vs2015"
		location "build/vs"
		toolset "msc-LLVM-vs2014"
		defines { '_CRT_SECURE_NO_WARNINGS', '_CRT_NONSTDC_NO_DEPRECATE' }
		buildoptions { '/MP', '/Wall', '-Wno-unused-parameter', '-Wshorten-64-to-32', '-Qunused-arguments' }
		flags { "StaticRuntime" }
		includedirs { "dep/polyfill-msvc" }
	filter {}
	
	filter "action:gmake"
		location "build/gmake"
		buildoptions { '-std=gnu11', '-Wall', '-Wno-multichar' }
	filter {}

	filter "action:xcode4"
		location "build/xcode"
		includedirs { "dep/extern", "dep/extern/**", "lib/**" }
		buildoptions { '-std=gnu11', '-Wall', '-Wno-multichar' }
	filter {}
	
	filter "configurations:Debug"
		defines { "DEBUG" }
		flags { "Symbols" }
	filter "configurations:Release"
		defines { "NDEBUG" }
		optimize "Full"

project "deps"
	kind "StaticLib"
	language "C"
	files {
		"dep/extern/**.h",
		"dep/extern/**.c"
	}
	filter "action:vs*"
	files {
		"dep/polyfill-msvc/**.h",
		"dep/polyfill-msvc/**.c"
	}
	buildoptions { '-Wno-unused-const-variable', '-Wno-shorten-64-to-32' }
	filter {}

project "libotfcc"
	kind "StaticLib"
	language "C"
	files {
		"lib/**.h",
		"lib/**.c"
	}

project "otfccdump"
	kind "ConsoleApp"
	language "C"
	targetdir "bin/%{cfg.buildcfg}-%{cfg.platform}"
	
	links { "libotfcc", "deps" }
	
	filter "action:gmake"
		links "m"
	filter {}

	filter "action:xcode4"
		links "m"
	filter {}
	
	files {
		"src/**.c",
		"src/**.h"
	}
	removefiles {
		"src/otfccbuild.c"
	}

project "otfccbuild"
	kind "ConsoleApp"
	language "C"
	targetdir "bin/%{cfg.buildcfg}-%{cfg.platform}"
	
	links { "libotfcc", "deps" }
	
	filter "action:gmake"
		links "m"
	filter {}

	filter "action:xcode4"
		links "m"
	filter {}
	
	files {
		"src/**.c",
		"src/**.h"
	}
	removefiles {
		"src/otfccdump.c"
	}
