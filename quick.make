ifndef PREMAKE5
PREMAKE5=premake5
endif

mf-vs2015 :
	@$(PREMAKE5) vs2015
mf-gmake :
	@$(PREMAKE5) gmake

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

TEST_OPCODES = abs add div drop dup eq.(mul) exch ifelse index.(roll,drop) mul neg not or.(mul) put.get roll.(drop) sqrt.(mul) sub
TEST_OPCODES_TARGETS = $(foreach op,$(TEST_OPCODES),cffopcodetest-$(op))

cffopcodetest : $(TEST_OPCODES_TARGETS)
$(TEST_OPCODES_TARGETS) : cffopcodetest-% : tests/payload/cffspecial/cff.%.otf
	@bin/Release-x64/otfccdump '$<' | node tests/cffdump-opcode-check

TTF_ROUNDTRIP_PAYLOADS = NotoNastaliqUrdu-Regular iosevka-r
TTF_ROUNDTRIP_TARGETS = $(foreach f,$(TTF_ROUNDTRIP_PAYLOADS),ttfroundtriptest-$(f))
TTF_ROUNDTRIP_TARGETS_O3 = $(foreach f,$(TTF_ROUNDTRIP_PAYLOADS),ttfroundtriptest-$(f)-o3)

ttfroundtriptest: $(TTF_ROUNDTRIP_TARGETS) $(TTF_ROUNDTRIP_TARGETS_O3)
$(TTF_ROUNDTRIP_TARGETS) : ttfroundtriptest-% : tests/payload/%.ttf
	@bin/Release-x64/otfccdump $< -o build/$(basename $(notdir $<)).1.json
	@bin/Release-x64/otfccbuild build/$(basename $(notdir $<)).1.json -o build/$(basename $(notdir $<)).2.ttf --keep-average-char-width --keep-modified-time
	@bin/Release-x64/otfccdump build/$(basename $(notdir $<)).2.ttf -o build/$(basename $(notdir $<)).3.json
	@bin/Release-x64/otfccbuild build/$(basename $(notdir $<)).3.json -o build/$(basename $(notdir $<)).4.ttf --keep-average-char-width --keep-modified-time
	@bin/Release-x64/otfccdump build/$(basename $(notdir $<)).4.ttf -o build/$(basename $(notdir $<)).5.json
	@node tests/ttf-roundtrip-test.js build/$(basename $(notdir $<)).5.json build/$(basename $(notdir $<)).3.json
$(TTF_ROUNDTRIP_TARGETS_O3) : ttfroundtriptest-%-o3 : tests/payload/%.ttf ttfroundtriptest-%
	@bin/Release-x64/otfccbuild build/$(basename $(notdir $<)).1.json -o build/$(basename $(notdir $<)).2o3.ttf --keep-average-char-width --keep-modified-time
	@bin/Release-x64/otfccdump build/$(basename $(notdir $<)).2o3.ttf -o build/$(basename $(notdir $<)).3o3.json
	@bin/Release-x64/otfccbuild build/$(basename $(notdir $<)).3o3.json -o build/$(basename $(notdir $<)).4o3.ttf --keep-average-char-width --keep-modified-time
	@bin/Release-x64/otfccdump build/$(basename $(notdir $<)).4o3.ttf -o build/$(basename $(notdir $<)).5o3.json
	@node tests/ttf-roundtrip-test.js build/$(basename $(notdir $<)).5o3.json build/$(basename $(notdir $<)).3o3.json


CFF_ROUNDTRIP_PAYLOADS = Cormorant-Medium WorkSans-Regular
CFF_ROUNDTRIP_PAYLOADS_FJ = WorkSans-Regular
CFF_ROUNDTRIP_TARGETS = $(foreach f,$(CFF_ROUNDTRIP_PAYLOADS),cffroundtriptest-$(f))
CFF_ROUNDTRIP_TARGETS_O3 = $(foreach f,$(CFF_ROUNDTRIP_PAYLOADS),cffroundtriptest-$(f)-o3)
CFF_ROUNDTRIP_TARGETS_FJ = $(foreach f,$(CFF_ROUNDTRIP_PAYLOADS_FJ),cffroundtriptest-fj-$(f))
CFF_ROUNDTRIP_TARGETS_FJ_O3 = $(foreach f,$(CFF_ROUNDTRIP_PAYLOADS_FJ),cffroundtriptest-fj-$(f)-o3)
cffroundtriptest: $(CFF_ROUNDTRIP_TARGETS) $(CFF_ROUNDTRIP_TARGETS_O3) $(CFF_ROUNDTRIP_TARGETS_FJ) $(CFF_ROUNDTRIP_TARGETS_FJ_O3)

$(CFF_ROUNDTRIP_TARGETS) : cffroundtriptest-% : tests/payload/%.otf
	@bin/Release-x64/otfccdump $< -o build/$(basename $(notdir $<)).1.json
	@bin/Release-x64/otfccbuild build/$(basename $(notdir $<)).1.json -o build/$(basename $(notdir $<)).2.otf --keep-average-char-width --keep-modified-time
	@bin/Release-x64/otfccdump build/$(basename $(notdir $<)).2.otf -o build/$(basename $(notdir $<)).3.json
	@bin/Release-x64/otfccbuild build/$(basename $(notdir $<)).3.json -o build/$(basename $(notdir $<)).4.otf --keep-average-char-width --keep-modified-time
	@bin/Release-x64/otfccdump build/$(basename $(notdir $<)).4.otf -o build/$(basename $(notdir $<)).5.json
	@node tests/ttf-roundtrip-test.js build/$(basename $(notdir $<)).5.json build/$(basename $(notdir $<)).3.json
$(CFF_ROUNDTRIP_TARGETS_O3) : cffroundtriptest-%-o3 : tests/payload/%.otf cffroundtriptest-%
	@bin/Release-x64/otfccbuild build/$(basename $(notdir $<)).1.json -o build/$(basename $(notdir $<)).2o3.otf --keep-average-char-width --keep-modified-time
	@bin/Release-x64/otfccdump build/$(basename $(notdir $<)).2o3.otf -o build/$(basename $(notdir $<)).3o3.json
	@bin/Release-x64/otfccbuild build/$(basename $(notdir $<)).3o3.json -o build/$(basename $(notdir $<)).4o3.otf --keep-average-char-width --keep-modified-time
	@bin/Release-x64/otfccdump build/$(basename $(notdir $<)).4o3.otf -o build/$(basename $(notdir $<)).5o3.json
	@node tests/ttf-roundtrip-test.js build/$(basename $(notdir $<)).5o3.json build/$(basename $(notdir $<)).3o3.json

$(CFF_ROUNDTRIP_TARGETS_FJ) : cffroundtriptest-fj-% : tests/payload/%.json
	@bin/Release-x64/otfccbuild $< -o build/fj-$(basename $(notdir $<)).2.otf --keep-average-char-width --keep-modified-time
	@bin/Release-x64/otfccdump build/fj-$(basename $(notdir $<)).2.otf -o build/fj-$(basename $(notdir $<)).3.json
	@bin/Release-x64/otfccbuild build/fj-$(basename $(notdir $<)).3.json -o build/fj-$(basename $(notdir $<)).4.otf --keep-average-char-width --keep-modified-time
	@bin/Release-x64/otfccdump build/fj-$(basename $(notdir $<)).4.otf -o build/fj-$(basename $(notdir $<)).5.json
	@node tests/ttf-roundtrip-test.js build/fj-$(basename $(notdir $<)).5.json build/fj-$(basename $(notdir $<)).3.json
$(CFF_ROUNDTRIP_TARGETS_FJ_O3) : cffroundtriptest-fj-%-o3 : tests/payload/%.json cffroundtriptest-fj-%
	@bin/Release-x64/otfccbuild $< -o build/fj-$(basename $(notdir $<)).2o3.otf --keep-average-char-width --keep-modified-time
	@bin/Release-x64/otfccdump build/fj-$(basename $(notdir $<)).2o3.otf -o build/fj-$(basename $(notdir $<)).3o3.json
	@bin/Release-x64/otfccbuild build/fj-$(basename $(notdir $<)).3o3.json -o build/fj-$(basename $(notdir $<)).4o3.otf --keep-average-char-width --keep-modified-time
	@bin/Release-x64/otfccdump build/fj-$(basename $(notdir $<)).4o3.otf -o build/fj-$(basename $(notdir $<)).5o3.json
	@node tests/ttf-roundtrip-test.js build/fj-$(basename $(notdir $<)).5o3.json build/fj-$(basename $(notdir $<)).3o3.json

test: cffopcodetest ttfroundtriptest cffroundtriptest
