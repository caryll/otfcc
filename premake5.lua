require "dep/premake-modules/xcode-alt"

function cbuildoptions()
	filter "action:vs2015"
		buildoptions { '/MP', '/Wall', '-Wno-unused-parameter', '-Qunused-arguments' }
	filter { "action:vs2015", "platforms:x64" }
		buildoptions {'-Wshorten-64-to-32'}
	filter "action:gmake"
		buildoptions { '-std=gnu11', '-Wall', '-Wno-multichar' }
	filter "action:xcode4"
		buildoptions { '-std=gnu11', '-Wall', '-Wno-multichar' }
	filter {}
end

function externcbuildoptions()
	filter "action:vs2015"
		buildoptions { '/MP', '-Qunused-arguments', '-Wno-unused-const-variable' }
	filter "action:gmake"
		buildoptions { '-std=gnu11', '-Wno-unused-const-variable', '-Wno-shorten-64-to-32' }
	filter "action:xcode4"
		buildoptions { '-std=gnu11', '-Wno-unused-const-variable', '-Wno-shorten-64-to-32' }
	filter {}
end

function cxxbuildoptions()
	filter "action:vs2015"
		buildoptions { '/MP', '-Qunused-arguments' }
	filter "action:gmake"
		buildoptions { '-std=gnu++11' }
	filter "action:xcode4"
		buildoptions { '-std=gnu++11' }
	filter {}
end

-- Premake 5 configurations
workspace "otfcc"
	configurations { "Debug", "Release" }
	platforms { "x32", "x64" }
	
	defines {
		'_CARYLL_USE_PRE_SERIALIZED',
		'MAIN_VER=0',
		"SECONDARY_VER=2",
		"PATCH_VER=4"
	}
	
	location "build"
	includedirs { "dep", "lib" }
	
	filter "action:vs2015"
		location "build/vs"
		toolset "msc-LLVM-vs2014"
		defines { '_CRT_SECURE_NO_WARNINGS', '_CRT_NONSTDC_NO_DEPRECATE' }
		flags { "StaticRuntime" }
		includedirs { "dep/polyfill-msvc" }
	filter "action:gmake"
		location "build/gmake"
	filter "action:xcode4"
		location "build/xcode"
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
	externcbuildoptions()
	files {
		"dep/extern/**.h",
		"dep/extern/**.c"
	}
	filter "action:vs*"
	files {
		"dep/polyfill-msvc/**.h",
		"dep/polyfill-msvc/**.c"
	}
	filter {}

project "libotfcc"
	kind "StaticLib"
	language "C"
	cbuildoptions()
	files {
		"lib/**.h",
		"lib/**.c"
	}

project "otfccdump"
	kind "ConsoleApp"
	language "C"
	cbuildoptions()
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
	cbuildoptions()
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
