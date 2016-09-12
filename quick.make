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

cffopcodetest :
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

ttfroundtriptest: ttfroundtriptest1 ttfroundtriptest2
ttfroundtriptest1 :
	@bin/Release-x64/otfccdump tests/payload/iosevka-r.ttf -o build/roundtrip-iosevka-r-1.json
	@bin/Release-x64/otfccbuild build/roundtrip-iosevka-r-1.json -o build/roundtrip-iosevka-r-2.ttf --keep-average-char-width --keep-modified-time
	@bin/Release-x64/otfccdump build/roundtrip-iosevka-r-2.ttf -o build/roundtrip-iosevka-r-3.json
	@bin/Release-x64/otfccbuild build/roundtrip-iosevka-r-3.json -o build/roundtrip-iosevka-r-4.ttf --keep-average-char-width --keep-modified-time
	@bin/Release-x64/otfccdump build/roundtrip-iosevka-r-4.ttf -o build/roundtrip-iosevka-r-5.json
	@node tests/ttf-roundtrip-test.js build/roundtrip-iosevka-r-3.json build/roundtrip-iosevka-r-5.json
ttfroundtriptest2 :
	@bin/Release-x64/otfccdump tests/payload/NotoNastaliqUrdu-Regular.ttf -o build/roundtrip-NotoNastaliqUrdu-Regular-1.json --pretty
	@bin/Release-x64/otfccbuild build/roundtrip-NotoNastaliqUrdu-Regular-1.json -o build/roundtrip-NotoNastaliqUrdu-Regular-2.ttf --keep-average-char-width --keep-modified-time
	@bin/Release-x64/otfccdump build/roundtrip-NotoNastaliqUrdu-Regular-2.ttf -o build/roundtrip-NotoNastaliqUrdu-Regular-3.json --pretty
	@bin/Release-x64/otfccbuild build/roundtrip-NotoNastaliqUrdu-Regular-3.json -o build/roundtrip-NotoNastaliqUrdu-Regular-4.ttf --keep-average-char-width --keep-modified-time
	@bin/Release-x64/otfccdump build/roundtrip-NotoNastaliqUrdu-Regular-4.ttf -o build/roundtrip-NotoNastaliqUrdu-Regular-5.json --pretty
	@node tests/ttf-roundtrip-test.js build/roundtrip-NotoNastaliqUrdu-Regular-3.json build/roundtrip-NotoNastaliqUrdu-Regular-5.json

cffroundtriptest: cffroundtriptest1 cffroundtriptest1O3 cffroundtriptest2 cffroundtriptest3 cffroundtriptest3O3
cffroundtriptest1 :
	@bin/Release-x64/otfccdump tests/payload/WorkSans-Regular.otf -o build/roundtrip-WorkSans-Regular-1.json --pretty
	@bin/Release-x64/otfccbuild -O1 build/roundtrip-WorkSans-Regular-1.json -o build/roundtrip-WorkSans-Regular-2.otf --keep-average-char-width --keep-modified-time
	@bin/Release-x64/otfccdump build/roundtrip-WorkSans-Regular-2.otf -o build/roundtrip-WorkSans-Regular-3.json --pretty
	@bin/Release-x64/otfccbuild -O1 build/roundtrip-WorkSans-Regular-3.json -o build/roundtrip-WorkSans-Regular-4.otf --keep-average-char-width --keep-modified-time
	@bin/Release-x64/otfccdump build/roundtrip-WorkSans-Regular-4.otf -o build/roundtrip-WorkSans-Regular-5.json --pretty
	@node tests/ttf-roundtrip-test.js build/roundtrip-WorkSans-Regular-3.json build/roundtrip-WorkSans-Regular-5.json
cffroundtriptest1O3 : cffroundtriptest1
	@bin/Release-x64/otfccbuild -O3 build/roundtrip-WorkSans-Regular-1.json -o build/roundtrip-WorkSans-Regular-2a.otf --keep-average-char-width --keep-modified-time
	@bin/Release-x64/otfccdump build/roundtrip-WorkSans-Regular-2a.otf -o build/roundtrip-WorkSans-Regular-3a.json --pretty
	@bin/Release-x64/otfccbuild -O3 build/roundtrip-WorkSans-Regular-3a.json -o build/roundtrip-WorkSans-Regular-4a.otf --keep-average-char-width --keep-modified-time
	@bin/Release-x64/otfccdump build/roundtrip-WorkSans-Regular-4a.otf -o build/roundtrip-WorkSans-Regular-5a.json --pretty
	@node tests/ttf-roundtrip-test.js build/roundtrip-WorkSans-Regular-3a.json build/roundtrip-WorkSans-Regular-5a.json
cffroundtriptest2:
	@bin/Release-x64/otfccbuild -O1 tests/payload/WorkSans-Regular.json -o build/roundtrip-WorkSans-Regular-6.otf --keep-average-char-width --keep-modified-time
	@bin/Release-x64/otfccdump build/roundtrip-WorkSans-Regular-6.otf -o build/roundtrip-WorkSans-Regular-7.json --pretty
	@bin/Release-x64/otfccbuild -O1 build/roundtrip-WorkSans-Regular-7.json -o build/roundtrip-WorkSans-Regular-8.otf --keep-average-char-width --keep-modified-time
	@bin/Release-x64/otfccdump build/roundtrip-WorkSans-Regular-8.otf -o build/roundtrip-WorkSans-Regular-9.json --pretty
	@node tests/ttf-roundtrip-test.js build/roundtrip-WorkSans-Regular-7.json build/roundtrip-WorkSans-Regular-9.json
cffroundtriptest3:
	@bin/Release-x64/otfccdump tests/payload/Cormorant-Medium.otf -o build/roundtrip-Cormorant-Medium-1.json --pretty
	@bin/Release-x64/otfccbuild -O1 build/roundtrip-Cormorant-Medium-1.json -o build/roundtrip-Cormorant-Medium-2.otf --keep-average-char-width --keep-modified-time
	@bin/Release-x64/otfccdump build/roundtrip-Cormorant-Medium-2.otf -o build/roundtrip-Cormorant-Medium-3.json --pretty
	@bin/Release-x64/otfccbuild -O1 build/roundtrip-Cormorant-Medium-3.json -o build/roundtrip-Cormorant-Medium-4.otf --keep-average-char-width --keep-modified-time
	@bin/Release-x64/otfccdump build/roundtrip-Cormorant-Medium-4.otf -o build/roundtrip-Cormorant-Medium-5.json --pretty
	@node tests/ttf-roundtrip-test.js build/roundtrip-Cormorant-Medium-3.json build/roundtrip-Cormorant-Medium-5.json
cffroundtriptest3O3: cffroundtriptest3
	@bin/Release-x64/otfccbuild -O3 build/roundtrip-Cormorant-Medium-1.json -o build/roundtrip-Cormorant-Medium-2a.otf --keep-average-char-width --keep-modified-time
	@bin/Release-x64/otfccdump build/roundtrip-Cormorant-Medium-2a.otf -o build/roundtrip-Cormorant-Medium-3a.json --pretty
	@bin/Release-x64/otfccbuild -O3 build/roundtrip-Cormorant-Medium-3a.json -o build/roundtrip-Cormorant-Medium-4a.otf --keep-average-char-width --keep-modified-time
	@bin/Release-x64/otfccdump build/roundtrip-Cormorant-Medium-4a.otf -o build/roundtrip-Cormorant-Medium-5a.json --pretty
	@node tests/ttf-roundtrip-test.js build/roundtrip-Cormorant-Medium-3a.json build/roundtrip-Cormorant-Medium-5a.json

test: cffroundtriptest cffopcodetest ttfroundtriptest
