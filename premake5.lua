require "dep/premake-modules/xcode-alt"
require "dep/premake-modules/ninja"

MAIN_VER = '0'
SECONDARY_VER = '5'
PATCH_VER = '0'

function cbuildoptions()
	filter "action:vs2015"
		buildoptions { '/MP', '/Wall', '-Wno-unused-parameter', '-Qunused-arguments' }
	filter { "action:vs2015", "platforms:x64" }
		buildoptions {'-Wshorten-64-to-32'}
	filter {"system:windows", "action:ninja"}
		buildoptions { '/Wall', '-Wextra', '-Wno-unused-parameter', '-Qunused-arguments' }
	filter "action:gmake"
		buildoptions { '-std=gnu11', '-Wall', '-Wno-multichar' }
	filter "action:xcode4"
		buildoptions { '-std=gnu11', '-Wall', '-Wno-multichar' }
	filter {}
end

function externcbuildoptions()
	filter "action:vs2015"
		buildoptions { '/MP', '-Qunused-arguments', '-Wno-unused-const-variable' }
	filter {"system:windows", "action:ninja"}
		buildoptions { '-Wno-unused-parameter', '-Qunused-arguments' }
	filter "action:gmake"
		buildoptions { '-std=gnu11', '-Wno-unused-const-variable', '-Wno-shorten-64-to-32' }
	filter "action:xcode4"
		buildoptions { '-std=gnu11', '-Wno-unused-const-variable', '-Wno-shorten-64-to-32' }
	filter {}
end

function cxxbuildoptions()
	filter "action:vs2015"
		buildoptions { '/MP', '-Qunused-arguments' }
	filter {"system:windows", "action:ninja"}
		buildoptions { '-Qunused-arguments' }
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
	
	location "build"
	includedirs { "include" }
	
	defines {
		'_CARYLL_USE_PRE_SERIALIZED',
		('MAIN_VER=' .. MAIN_VER),
		("SECONDARY_VER=" .. SECONDARY_VER),
		("PATCH_VER=" .. PATCH_VER)
	}
	
	filter "action:vs2015"
		location "build/vs"
		toolset "msc-LLVM-vs2014"
		defines { '_CRT_SECURE_NO_WARNINGS', '_CRT_NONSTDC_NO_DEPRECATE' }
		flags { "StaticRuntime" }
		includedirs { "dep/polyfill-msvc" }
	filter "action:ninja"
		location "build/ninja"
	filter {"system:windows", "action:ninja"}
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
		symbols "on"
	filter "configurations:Release"
		defines { "NDEBUG" }
		optimize "Full"

project "deps"
	kind "StaticLib"
	language "C"
	externcbuildoptions()
	includedirs { "include/dep" }
	files {
		"dep/extern/**.h",
		"dep/extern/**.c"
	}
	filter "action:vs*"
	files {
		"dep/polyfill-msvc/**.h",
		"dep/polyfill-msvc/**.c"
	}
	filter {"system:windows", "action:ninja"}
	files {
		"dep/polyfill-msvc/**.h",
		"dep/polyfill-msvc/**.c"
	}
	filter {}

project "libotfcc"
	kind "StaticLib"
	language "C"
	cbuildoptions()

	links { "deps" }
	includedirs{ "lib" }
	filter "action:gmake"
		links "m"
	filter {}

	filter "action:xcode4"
		links "m"
	filter {}

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
