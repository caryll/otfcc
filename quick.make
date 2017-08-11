ifndef PREMAKE5
PREMAKE5=premake5
endif
ifndef BD_NINJA
NINJA_EXEC=ninja
else
NINJA_EXEC=../../$(BD_NINJA)
endif

mf-vs2017 :
	@$(PREMAKE5) vs2017
mf-gmake :
	@$(PREMAKE5) gmake
mf-ninja-windows :
	@$(PREMAKE5) ninja --os=windows
mf-ninja-linux :
	@$(PREMAKE5) ninja --os=linux --cc=$(CC)
mf-ninja-macosx :
	@$(PREMAKE5) ninja --os=macosx

mingw-debug-x64 : mf-gmake
	@cd build/gmake && make config=debug_x64
mingw-debug-x86 : mf-gmake
	@cd build/gmake && make config=debug_x86
mingw-release-x64 : mf-gmake
	@cd build/gmake && make config=release_x64
mingw-release-x86 : mf-gmake
	@cd build/gmake && make config=release_x86

linux-debug-x64 : mf-ninja-linux
	@cd build/ninja && $(NINJA_EXEC) otfccdump_debug_x64 otfccbuild_debug_x64 otfccdll_debug_x64
linux-debug-x86 : mf-ninja-linux
	@cd build/ninja && $(NINJA_EXEC) otfccdump_debug_x86 otfccbuild_debug_x86 otfccdll_debug_x86
linux-release-x64 : mf-ninja-linux
	@cd build/ninja && $(NINJA_EXEC) otfccdump_release_x64 otfccbuild_release_x64 otfccdll_release_x64
linux-release-x86 : mf-ninja-linux
	@cd build/ninja && $(NINJA_EXEC) otfccdump_release_x86 otfccbuild_release_x86 otfccdll_release_x86

macosx-debug-x64 : mf-ninja-macosx
	@cd build/ninja && $(NINJA_EXEC) otfccdump_debug_x64 otfccbuild_debug_x64 otfccdll_debug_x64
macosx-debug-x86 : mf-ninja-macosx
	@cd build/ninja && $(NINJA_EXEC) otfccdump_debug_x86 otfccbuild_debug_x86 otfccdll_debug_x86
macosx-release-x64 : mf-ninja-macosx
	@cd build/ninja && $(NINJA_EXEC) otfccdump_release_x64 otfccbuild_release_x64 otfccdll_release_x64
macosx-release-x86 : mf-ninja-macosx
	@cd build/ninja && $(NINJA_EXEC) otfccdump_release_x86 otfccbuild_release_x86 otfccdll_release_x86

# VC does not support debugging well
# It is used for release versions only
vc-release-x64 : mf-vs2017
	@./_vc2017.bat build/vs/otfcc.sln /property:Configuration=release /property:Platform=x64
vc-release-x86 : mf-vs2017
	@./_vc2017.bat build/vs/otfcc.sln /property:Configuration=release /property:Platform=win32

ninja-win-x64 : mf-ninja-windows
	@./_vcbuildNinja.bat otfccdump_release_x64 otfccbuild_release_x64 otfccdll_release_x64
ninja-win-x86 : mf-ninja-windows
	@./_vcbuildNinja.bat otfccdump_release_x86 otfccbuild_release_x86 otfccdll_release_x86
ninja-win-debug-x64 : mf-ninja-windows
	@./_vcbuildNinja.bat otfccdump_debug_x64 otfccbuild_debug_x64 otfccdll_debug_x64
ninja-win-debug-x86 : mf-ninja-windows
	@./_vcbuildNinja.bat otfccdump_debug_x86 otfccbuild_debug_x86 otfccdll_debug_x86

TEST_OPCODES = abs add div drop dup eq.(mul) exch ifelse index.(roll,drop) mul neg not or.(mul) put.get roll.(drop) sqrt.(mul) sub
TEST_OPCODES_TARGETS = $(foreach op,$(TEST_OPCODES),cffopcodetest-$(op))

cffopcodetest : $(TEST_OPCODES_TARGETS)
$(TEST_OPCODES_TARGETS) : cffopcodetest-% : tests/payload/cffspecial/cff.%.otf
	@bin/release-x64/otfccdump '$<' | node tests/cffdump-opcode-check

TTF_ROUNDTRIP_PAYLOADS = NotoNastaliqUrdu-Regular iosevka-r BungeeColor-Regular_colr_Windows Reinebow-SVGinOT vtt Molengo-Regular
TTF_ROUNDTRIP_TARGETS = $(foreach f,$(TTF_ROUNDTRIP_PAYLOADS),ttfroundtriptest-$(f))

ttfroundtriptest: $(TTF_ROUNDTRIP_TARGETS)
$(TTF_ROUNDTRIP_TARGETS) : ttfroundtriptest-% : tests/payload/%.ttf
	@bin/release-x64/otfccdump $< -o build/$(basename $(notdir $<)).1.json --pretty
	@bin/release-x64/otfccbuild build/$(basename $(notdir $<)).1.json -o build/$(basename $(notdir $<)).2.ttf --keep-average-char-width --keep-modified-time
	@bin/release-x64/otfccdump build/$(basename $(notdir $<)).2.ttf -o build/$(basename $(notdir $<)).3.json --pretty
	@bin/release-x64/otfccbuild build/$(basename $(notdir $<)).3.json -o build/$(basename $(notdir $<)).4.ttf --keep-average-char-width --keep-modified-time
	@bin/release-x64/otfccdump build/$(basename $(notdir $<)).4.ttf -o build/$(basename $(notdir $<)).5.json --pretty
	@node tests/ttf-roundtrip-test.js build/$(basename $(notdir $<)).5.json build/$(basename $(notdir $<)).3.json
	-@rm build/$(basename $(notdir $<)).2.ttf build/$(basename $(notdir $<)).3.json build/$(basename $(notdir $<)).4.ttf build/$(basename $(notdir $<)).5.json
	@bin/release-x64/otfccbuild -O3 build/$(basename $(notdir $<)).1.json -o build/$(basename $(notdir $<)).2o3.ttf --keep-average-char-width --keep-modified-time
	@bin/release-x64/otfccdump build/$(basename $(notdir $<)).2o3.ttf -o build/$(basename $(notdir $<)).3o3.json --pretty
	@bin/release-x64/otfccbuild -O3 build/$(basename $(notdir $<)).3o3.json -o build/$(basename $(notdir $<)).4o3.ttf --keep-average-char-width --keep-modified-time
	@bin/release-x64/otfccdump build/$(basename $(notdir $<)).4o3.ttf -o build/$(basename $(notdir $<)).5o3.json --pretty
	@node tests/ttf-roundtrip-test.js build/$(basename $(notdir $<)).5o3.json build/$(basename $(notdir $<)).3o3.json
	-@rm build/$(basename $(notdir $<)).1.json build/$(basename $(notdir $<)).2o3.ttf build/$(basename $(notdir $<)).3o3.json build/$(basename $(notdir $<)).4o3.ttf build/$(basename $(notdir $<)).5o3.json


CFF_ROUNDTRIP_PAYLOADS = Cormorant-Medium WorkSans-Regular KRName-Regular
CFF_ROUNDTRIP_PAYLOADS_FJ = WorkSans-Regular kltf-bugfont1
CFF_ROUNDTRIP_TARGETS = $(foreach f,$(CFF_ROUNDTRIP_PAYLOADS),cffroundtriptest-$(f))
CFF_ROUNDTRIP_TARGETS_FJ = $(foreach f,$(CFF_ROUNDTRIP_PAYLOADS_FJ),cffroundtriptest-fj-$(f))
cffroundtriptest: $(CFF_ROUNDTRIP_TARGETS) $(CFF_ROUNDTRIP_TARGETS_FJ)

$(CFF_ROUNDTRIP_TARGETS) : cffroundtriptest-% : tests/payload/%.otf
	@bin/release-x64/otfccdump $< -o build/$(basename $(notdir $<)).1.json --pretty
	@bin/release-x64/otfccbuild build/$(basename $(notdir $<)).1.json -o build/$(basename $(notdir $<)).2.otf --keep-average-char-width --keep-modified-time
	@bin/release-x64/otfccdump build/$(basename $(notdir $<)).2.otf -o build/$(basename $(notdir $<)).3.json --pretty
	@bin/release-x64/otfccbuild build/$(basename $(notdir $<)).3.json -o build/$(basename $(notdir $<)).4.otf --keep-average-char-width --keep-modified-time
	@bin/release-x64/otfccdump build/$(basename $(notdir $<)).4.otf -o build/$(basename $(notdir $<)).5.json --pretty
	@node tests/ttf-roundtrip-test.js build/$(basename $(notdir $<)).5.json build/$(basename $(notdir $<)).3.json
	-@rm build/$(basename $(notdir $<)).2.otf build/$(basename $(notdir $<)).3.json build/$(basename $(notdir $<)).4.otf build/$(basename $(notdir $<)).5.json
	@bin/release-x64/otfccbuild -O3 build/$(basename $(notdir $<)).1.json -o build/$(basename $(notdir $<)).2o3.otf --keep-average-char-width --keep-modified-time
	@bin/release-x64/otfccdump build/$(basename $(notdir $<)).2o3.otf -o build/$(basename $(notdir $<)).3o3.json --pretty
	@bin/release-x64/otfccbuild -O3 build/$(basename $(notdir $<)).3o3.json -o build/$(basename $(notdir $<)).4o3.otf --keep-average-char-width --keep-modified-time
	@bin/release-x64/otfccdump build/$(basename $(notdir $<)).4o3.otf -o build/$(basename $(notdir $<)).5o3.json --pretty
	@node tests/ttf-roundtrip-test.js build/$(basename $(notdir $<)).5o3.json build/$(basename $(notdir $<)).3o3.json
	-@rm build/$(basename $(notdir $<)).1.json build/$(basename $(notdir $<)).2o3.otf build/$(basename $(notdir $<)).3o3.json build/$(basename $(notdir $<)).4o3.otf build/$(basename $(notdir $<)).5o3.json

$(CFF_ROUNDTRIP_TARGETS_FJ) : cffroundtriptest-fj-% : tests/payload/%.json
	@bin/release-x64/otfccbuild $< -o build/fj-$(basename $(notdir $<)).2.otf --keep-average-char-width --keep-modified-time
	@bin/release-x64/otfccdump build/fj-$(basename $(notdir $<)).2.otf -o build/fj-$(basename $(notdir $<)).3.json --pretty
	@bin/release-x64/otfccbuild build/fj-$(basename $(notdir $<)).3.json -o build/fj-$(basename $(notdir $<)).4.otf --keep-average-char-width --keep-modified-time
	@bin/release-x64/otfccdump build/fj-$(basename $(notdir $<)).4.otf -o build/fj-$(basename $(notdir $<)).5.json --pretty
	@node tests/ttf-roundtrip-test.js build/fj-$(basename $(notdir $<)).5.json build/fj-$(basename $(notdir $<)).3.json
	-@rm build/fj-$(basename $(notdir $<)).2.otf build/fj-$(basename $(notdir $<)).3.json build/fj-$(basename $(notdir $<)).4.otf build/fj-$(basename $(notdir $<)).5.json
	@bin/release-x64/otfccbuild -O3 $< -o build/fj-$(basename $(notdir $<)).2o3.otf --keep-average-char-width --keep-modified-time
	@bin/release-x64/otfccdump build/fj-$(basename $(notdir $<)).2o3.otf -o build/fj-$(basename $(notdir $<)).3o3.json  --pretty
	@bin/release-x64/otfccbuild -O3 build/fj-$(basename $(notdir $<)).3o3.json -o build/fj-$(basename $(notdir $<)).4o3.otf --keep-average-char-width --keep-modified-time
	@bin/release-x64/otfccdump build/fj-$(basename $(notdir $<)).4o3.otf -o build/fj-$(basename $(notdir $<)).5o3.json  --pretty
	@node tests/ttf-roundtrip-test.js build/fj-$(basename $(notdir $<)).5o3.json build/fj-$(basename $(notdir $<)).3o3.json
	-@rm build/fj-$(basename $(notdir $<)).2o3.otf build/fj-$(basename $(notdir $<)).3o3.json build/fj-$(basename $(notdir $<)).4o3.otf build/fj-$(basename $(notdir $<)).5o3.json

test: ttfroundtriptest cffroundtriptest cffopcodetest
