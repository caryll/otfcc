clang-cl-debug-x64 :
	premake5 vs2015
	cmd /c _vcbuild64.bat /property:Configuration=Debug

clang-cl-debug-x86 :
	premake5 vs2015
	cmd /c _vcbuild32.bat /property:Configuration=Debug /property:Platform=win32

clang-cl-release-x64 :
	premake5 vs2015
	cmd /c _vcbuild64.bat /property:Configuration=Release

clang-cl-release-x86 :
	premake5 vs2015
	cmd /c _vcbuild32.bat /property:Configuration=Release /property:Platform=win32

mingw-debug-x64 :
	premake5 gmake
	cd build && make config=debug_x64
	
mingw-debug-x86 :
	premake5 gmake
	cd build && make config=debug_x64

mingw-release-x64 :
	premake5 gmake
	cd build && make config=release_x64
	
mingw-release-x86 :
	premake5 gmake
	cd build && make config=release_x64
