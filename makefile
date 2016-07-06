VERSION=0.1.9

ifndef PREMAKE5
PREMAKE5=premake5
endif

default: linux-release-x64

mingw-debug-x64 : mf-gmake
	@cd build/gmake && make config=debug_x64
mingw-debug-x86 : mf-gmake
	@cd build/gmake && make config=debug_x32
mingw-release-x64 : mf-gmake
	@cd build/gmake && make config=release_x64
mingw-release-x86 : mf-gmake
	@cd build/gmake && make config=release_x32

linux-debug-x64 : mf-gmake
	@cd build/gmake && make config=debug_x64
linux-debug-x86 : mf-gmake
	@cd build/gmake && make config=debug_x32
linux-release-x64 : mf-gmake
	@cd build/gmake && make config=release_x64
linux-release-x86 : mf-gmake
	@cd build/gmake && make config=release_x32

# Clang-cl does not support debugging well
# It is used for release versions only
clang-cl-release-x64 : mf-vs2015
	@cmd /c _vcbuild64.bat /property:Configuration=Release
clang-cl-release-x86 : mf-vs2015
	@cmd /c _vcbuild32.bat /property:Configuration=Release /property:Platform=win32

win : clang-cl-release-x86 clang-cl-release-x64
	cd bin/Release-x64 && 7z a ../otfcc-win64-$(VERSION).zip ./* -y
	cd bin/Release-x32 && 7z a ../otfcc-win32-$(VERSION).zip ./* -y

mf-vs2015 :
	@$(PREMAKE5) vs2015
mf-gmake :
	@$(PREMAKE5) gmake


cffopcodetest : linux-release-x64
	@bin/Release-x64/otfccdump 'tests/payload/cffspecial/cff.abs.otf' | node tests/cffdump-opcode-check
	@bin/Release-x64/otfccdump 'tests/payload/cffspecial/cff.add.otf' | node tests/cffdump-opcode-check
	@bin/Release-x64/otfccdump 'tests/payload/cffspecial/cff.div.otf' | node tests/cffdump-opcode-check
	@bin/Release-x64/otfccdump 'tests/payload/cffspecial/cff.drop.otf' | node tests/cffdump-opcode-check
	@bin/Release-x64/otfccdump 'tests/payload/cffspecial/cff.dup.otf' | node tests/cffdump-opcode-check
	@bin/Release-x64/otfccdump 'tests/payload/cffspecial/cff.eq.(mul).otf' | node tests/cffdump-opcode-check
	@bin/Release-x64/otfccdump 'tests/payload/cffspecial/cff.exch.otf' | node tests/cffdump-opcode-check
	@bin/Release-x64/otfccdump 'tests/payload/cffspecial/cff.ifelse.otf' | node tests/cffdump-opcode-check
	@bin/Release-x64/otfccdump 'tests/payload/cffspecial/cff.index.(roll,drop).otf' | node tests/cffdump-opcode-check
	@bin/Release-x64/otfccdump 'tests/payload/cffspecial/cff.mul.otf' | node tests/cffdump-opcode-check
	@bin/Release-x64/otfccdump 'tests/payload/cffspecial/cff.neg.otf' | node tests/cffdump-opcode-check
	@bin/Release-x64/otfccdump 'tests/payload/cffspecial/cff.not.otf' | node tests/cffdump-opcode-check
	@bin/Release-x64/otfccdump 'tests/payload/cffspecial/cff.or.(mul).otf' | node tests/cffdump-opcode-check
	@bin/Release-x64/otfccdump 'tests/payload/cffspecial/cff.put.get.otf' | node tests/cffdump-opcode-check
	@bin/Release-x64/otfccdump 'tests/payload/cffspecial/cff.roll.(drop).otf' | node tests/cffdump-opcode-check
	@bin/Release-x64/otfccdump 'tests/payload/cffspecial/cff.sqrt.(mul).otf' | node tests/cffdump-opcode-check
	@bin/Release-x64/otfccdump 'tests/payload/cffspecial/cff.sub.otf' | node tests/cffdump-opcode-check

test: cffopcodetest
