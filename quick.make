ifndef PREMAKE5
PREMAKE5=premake5
endif
ifndef BD_NINJA
NINJA_EXEC=ninja
else
NINJA_EXEC=../../$(BD_NINJA)
endif

mf-vs2015 :
	@$(PREMAKE5) vs2015
mf-gmake :
	@$(PREMAKE5) gmake
mf-ninja-windows :
	@$(PREMAKE5) ninja --os=windows
mf-ninja-linux :
	@$(PREMAKE5) ninja --os=linux --cc=$(CC)

mingw-debug-x64 : mf-gmake
	@cd build/gmake && make config=debug_x64
mingw-debug-x86 : mf-gmake
	@cd build/gmake && make config=debug_x32
mingw-release-x64 : mf-gmake
	@cd build/gmake && make config=release_x64
mingw-release-x86 : mf-gmake
	@cd build/gmake && make config=release_x32

linux-debug-x64 : mf-ninja-linux
	@cd build/ninja && $(NINJA_EXEC) otfccdump_Debug_x64 otfccbuild_Debug_x64
linux-debug-x86 : mf-ninja-linux
	@cd build/ninja && $(NINJA_EXEC) otfccdump_Debug_x32 otfccbuild_Debug_x32
linux-release-x64 : mf-ninja-linux
	@cd build/ninja && $(NINJA_EXEC) otfccdump_Release_x64 otfccbuild_Release_x64
linux-release-x86 : mf-ninja-linux
	@cd build/ninja && $(NINJA_EXEC) otfccdump_Release_x32 otfccbuild_Release_x32

# Clang-cl does not support debugging well
# It is used for release versions only
clang-cl-release-x64 : mf-vs2015
	@cmd /c _vcbuild64.bat /property:Configuration=Release
clang-cl-release-x86 : mf-vs2015
	@cmd /c _vcbuild32.bat /property:Configuration=Release /property:Platform=win32

ninja-win-x64 : mf-ninja-windows
	@cmd /c _vcbuildNinja.bat otfccdump_Release_x64 otfccbuild_Release_x64
ninja-win-x86 : mf-ninja-windows
	@cmd /c _vcbuildNinja.bat otfccdump_Release_x32 otfccbuild_Release_x32
ninja-win-debug-x64 : mf-ninja-windows
	@cmd /c _vcbuildNinja.bat otfccdump_Debug_x64 otfccbuild_Debug_x64
ninja-win-debug-x86 : mf-ninja-windows
	@cmd /c _vcbuildNinja.bat otfccdump_Debug_x32 otfccbuild_Debug_x32

TEST_OPCODES = abs add div drop dup eq.(mul) exch ifelse index.(roll,drop) mul neg not or.(mul) put.get roll.(drop) sqrt.(mul) sub
TEST_OPCODES_TARGETS = $(foreach op,$(TEST_OPCODES),cffopcodetest-$(op))

cffopcodetest : $(TEST_OPCODES_TARGETS)
$(TEST_OPCODES_TARGETS) : cffopcodetest-% : tests/payload/cffspecial/cff.%.otf
	@bin/Release-x64/otfccdump '$<' | node tests/cffdump-opcode-check

TTF_ROUNDTRIP_PAYLOADS = NotoNastaliqUrdu-Regular iosevka-r
TTF_ROUNDTRIP_TARGETS = $(foreach f,$(TTF_ROUNDTRIP_PAYLOADS),ttfroundtriptest-$(f))

ttfroundtriptest: $(TTF_ROUNDTRIP_TARGETS)
$(TTF_ROUNDTRIP_TARGETS) : ttfroundtriptest-% : tests/payload/%.ttf
	@bin/Release-x64/otfccdump $< -o build/$(basename $(notdir $<)).1.json --pretty
	@bin/Release-x64/otfccbuild build/$(basename $(notdir $<)).1.json -o build/$(basename $(notdir $<)).2.ttf --keep-average-char-width --keep-modified-time
	@bin/Release-x64/otfccdump build/$(basename $(notdir $<)).2.ttf -o build/$(basename $(notdir $<)).3.json --pretty
	@bin/Release-x64/otfccbuild build/$(basename $(notdir $<)).3.json -o build/$(basename $(notdir $<)).4.ttf --keep-average-char-width --keep-modified-time
	@bin/Release-x64/otfccdump build/$(basename $(notdir $<)).4.ttf -o build/$(basename $(notdir $<)).5.json --pretty
	@node tests/ttf-roundtrip-test.js build/$(basename $(notdir $<)).5.json build/$(basename $(notdir $<)).3.json
	-@rm build/$(basename $(notdir $<)).2.ttf build/$(basename $(notdir $<)).3.json build/$(basename $(notdir $<)).4.ttf build/$(basename $(notdir $<)).5.json
	@bin/Release-x64/otfccbuild -O3 build/$(basename $(notdir $<)).1.json -o build/$(basename $(notdir $<)).2o3.ttf --keep-average-char-width --keep-modified-time
	@bin/Release-x64/otfccdump build/$(basename $(notdir $<)).2o3.ttf -o build/$(basename $(notdir $<)).3o3.json --pretty
	@bin/Release-x64/otfccbuild -O3 build/$(basename $(notdir $<)).3o3.json -o build/$(basename $(notdir $<)).4o3.ttf --keep-average-char-width --keep-modified-time
	@bin/Release-x64/otfccdump build/$(basename $(notdir $<)).4o3.ttf -o build/$(basename $(notdir $<)).5o3.json --pretty
	@node tests/ttf-roundtrip-test.js build/$(basename $(notdir $<)).5o3.json build/$(basename $(notdir $<)).3o3.json
	-@rm build/$(basename $(notdir $<)).1.json build/$(basename $(notdir $<)).2o3.ttf build/$(basename $(notdir $<)).3o3.json build/$(basename $(notdir $<)).4o3.ttf build/$(basename $(notdir $<)).5o3.json


CFF_ROUNDTRIP_PAYLOADS = Cormorant-Medium WorkSans-Regular
CFF_ROUNDTRIP_PAYLOADS_FJ = WorkSans-Regular kltf-bugfont1
CFF_ROUNDTRIP_TARGETS = $(foreach f,$(CFF_ROUNDTRIP_PAYLOADS),cffroundtriptest-$(f))
CFF_ROUNDTRIP_TARGETS_FJ = $(foreach f,$(CFF_ROUNDTRIP_PAYLOADS_FJ),cffroundtriptest-fj-$(f))
cffroundtriptest: $(CFF_ROUNDTRIP_TARGETS) $(CFF_ROUNDTRIP_TARGETS_FJ)

$(CFF_ROUNDTRIP_TARGETS) : cffroundtriptest-% : tests/payload/%.otf
	@bin/Release-x64/otfccdump $< -o build/$(basename $(notdir $<)).1.json --pretty
	@bin/Release-x64/otfccbuild build/$(basename $(notdir $<)).1.json -o build/$(basename $(notdir $<)).2.otf --keep-average-char-width --keep-modified-time
	@bin/Release-x64/otfccdump build/$(basename $(notdir $<)).2.otf -o build/$(basename $(notdir $<)).3.json --pretty
	@bin/Release-x64/otfccbuild build/$(basename $(notdir $<)).3.json -o build/$(basename $(notdir $<)).4.otf --keep-average-char-width --keep-modified-time
	@bin/Release-x64/otfccdump build/$(basename $(notdir $<)).4.otf -o build/$(basename $(notdir $<)).5.json --pretty
	@node tests/ttf-roundtrip-test.js build/$(basename $(notdir $<)).5.json build/$(basename $(notdir $<)).3.json
	-@rm build/$(basename $(notdir $<)).2.otf build/$(basename $(notdir $<)).3.json build/$(basename $(notdir $<)).4.otf build/$(basename $(notdir $<)).5.json
	@bin/Release-x64/otfccbuild -O3 build/$(basename $(notdir $<)).1.json -o build/$(basename $(notdir $<)).2o3.otf --keep-average-char-width --keep-modified-time
	@bin/Release-x64/otfccdump build/$(basename $(notdir $<)).2o3.otf -o build/$(basename $(notdir $<)).3o3.json --pretty
	@bin/Release-x64/otfccbuild -O3 build/$(basename $(notdir $<)).3o3.json -o build/$(basename $(notdir $<)).4o3.otf --keep-average-char-width --keep-modified-time
	@bin/Release-x64/otfccdump build/$(basename $(notdir $<)).4o3.otf -o build/$(basename $(notdir $<)).5o3.json --pretty
	@node tests/ttf-roundtrip-test.js build/$(basename $(notdir $<)).5o3.json build/$(basename $(notdir $<)).3o3.json
	-@rm build/$(basename $(notdir $<)).1.json build/$(basename $(notdir $<)).2o3.otf build/$(basename $(notdir $<)).3o3.json build/$(basename $(notdir $<)).4o3.otf build/$(basename $(notdir $<)).5o3.json

$(CFF_ROUNDTRIP_TARGETS_FJ) : cffroundtriptest-fj-% : tests/payload/%.json
	@bin/Release-x64/otfccbuild $< -o build/fj-$(basename $(notdir $<)).2.otf --keep-average-char-width --keep-modified-time
	@bin/Release-x64/otfccdump build/fj-$(basename $(notdir $<)).2.otf -o build/fj-$(basename $(notdir $<)).3.json --pretty
	@bin/Release-x64/otfccbuild build/fj-$(basename $(notdir $<)).3.json -o build/fj-$(basename $(notdir $<)).4.otf --keep-average-char-width --keep-modified-time
	@bin/Release-x64/otfccdump build/fj-$(basename $(notdir $<)).4.otf -o build/fj-$(basename $(notdir $<)).5.json --pretty
	@node tests/ttf-roundtrip-test.js build/fj-$(basename $(notdir $<)).5.json build/fj-$(basename $(notdir $<)).3.json
	-@rm build/fj-$(basename $(notdir $<)).2.otf build/fj-$(basename $(notdir $<)).3.json build/fj-$(basename $(notdir $<)).4.otf build/fj-$(basename $(notdir $<)).5.json
	@bin/Release-x64/otfccbuild -O3 $< -o build/fj-$(basename $(notdir $<)).2o3.otf --keep-average-char-width --keep-modified-time
	@bin/Release-x64/otfccdump build/fj-$(basename $(notdir $<)).2o3.otf -o build/fj-$(basename $(notdir $<)).3o3.json  --pretty
	@bin/Release-x64/otfccbuild -O3 build/fj-$(basename $(notdir $<)).3o3.json -o build/fj-$(basename $(notdir $<)).4o3.otf --keep-average-char-width --keep-modified-time
	@bin/Release-x64/otfccdump build/fj-$(basename $(notdir $<)).4o3.otf -o build/fj-$(basename $(notdir $<)).5o3.json  --pretty
	@node tests/ttf-roundtrip-test.js build/fj-$(basename $(notdir $<)).5o3.json build/fj-$(basename $(notdir $<)).3o3.json
	-@rm build/fj-$(basename $(notdir $<)).2o3.otf build/fj-$(basename $(notdir $<)).3o3.json build/fj-$(basename $(notdir $<)).4o3.otf build/fj-$(basename $(notdir $<)).5o3.json

test: ttfroundtriptest cffroundtriptest cffopcodetest
