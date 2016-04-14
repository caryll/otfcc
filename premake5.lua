-- Premake 5 configurations
workspace "otfcc"
	configurations {"Debug", "Release"}
	platforms {"x32", "x64"}
	location "build"
	
	defines {'_CARYLL_USE_PRE_SERIALIZED'}
	
	filter "action:vs2015"
		toolset "msc-LLVM-vs2014"
		defines {'_CRT_SECURE_NO_WARNINGS'}
		buildoptions { '/Wall', '-Wno-unused-parameter', '-Qunused-arguments' }
		flags { "StaticRuntime" }
	filter {}
	
	filter "action:gmake"
		buildoptions { '-Wall', '-Wno-multichar' }
	filter {}
	
	filter "configurations:Debug"
		defines { "DEBUG" }
		flags { "Symbols" }

	filter "configurations:Release"
		defines { "NDEBUG" }
		optimize "Full"
	
project "libotfcc"
	kind "StaticLib"
	language "C"
	files {
		"src/**.h",
		"src/**.c",
		"extern/**.h",
		"extern/**.c"
	}

	removefiles {
		"src/otfccdump.c",
		"src/otfccbuild.c"
	}

project "otfccdump"
	kind "ConsoleApp"
	language "C"
	targetdir "bin/%{cfg.buildcfg}-%{cfg.platform}"
	links {"libotfcc"}
	files {
		"src/otfccdump.c"
	}


project "otfccbuild"
	kind "ConsoleApp"
	language "C"
	targetdir "bin/%{cfg.buildcfg}-%{cfg.platform}"
	links {"libotfcc"}
	files {
		"src/otfccbuild.c"
	}

