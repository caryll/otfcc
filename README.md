<p align="center"><img src="https://raw.githubusercontent.com/caryll/design/master/caryll-logo-libs-githubreadme.png" width=200></p><h1 align="center">otfcc</h1><p align="center"><a target="_blank" href="https://travis-ci.org/caryll/otfcc"><img src="https://travis-ci.org/caryll/otfcc.svg?branch=master" alt=""></a> <a target="_blank" href="https://ci.appveyor.com/project/be5invis/otfcc"><img src="https://ci.appveyor.com/api/projects/status/github/caryll/otfcc?branch=master&amp;svg=true" alt=""></a> <a href="https://github.com/caryll/otfcc/releases"><img src="https://img.shields.io/github/release/caryll/otfcc.svg" alt="Version"></a> <a target="_blank" href="https://gitter.im/caryll/otfcc"><img src="https://img.shields.io/gitter/room/caryll/otfcc.svg" alt=""></a></p>

The `otfcc` is a C library and utility used for parsing and writing OpenType font files.

## Key features

* JSON serialization of TrueType and CFF OpenType fonts.
* Building OpenType fonts from JSON.
* Full support for OpenType features (`GSUB`, `GPOS` and `GDEF`), CID-keyed CFF, vertical metrics, and more.
* **4× faster than `ttx` on CFF OTF, and 40× on TTF.**

## Usage

### `otfccdump` : Dump an OpenType font file into JSON
```
otfccdump [OPTIONS] input.[otf|ttf|ttc]

 -h, --help              : Display this help message and exit.
 -v, --version           : Display version information and exit.
 -o <file>               : Set output file path to <file>. When absent,
                           the dump will be written to STDOUT.
 -n <n>, --ttc-index <n> : Use the <n>th subfont within the input font.
 --pretty                : Prettify the output JSON.
 --ugly                  : Force uglify the output JSON.
 --time                  : Time each sub-step.
 --glyph-name-prefix pfx : Add a prefix to the glyph names.
 --ignore-glyph-order    : Do not export glyph order information.
 --ignore-hints          : Do not export hinting information.
 --add-bom               : Add BOM mark in the output. (This is default
                           on Windows when redirecting to another program.
                           Use --no-bom to turn it off.)
```

### `otfccbuild` : Build an OpenType font file from JSON
```
otfccbuild [OPTIONS] [input.json] -o output.[ttf|otf]

 input.json                : Path to input file. When absent the input will
                             be read from the STDIN.
 -h, --help                : Display this help message and exit.
 -v, --version             : Display version information and exit.
 -o <file>                 : Set output file path to <file>.
 --time                    : Time each substep.
 --ignore-glyph-order      : Ignore the glyph order information in the input.
 --ignore-hints            : Ignore the hinting information in the input.
 --keep-average-char-width : Keep the OS/2.xAvgCharWidth value from the input
                             instead of stating the average width of glyphs.
                             Useful when creating a monospaced font.
 --keep-modified-time      : Keep the head.modified time in the json, instead
                             of using current time.
 --short-post              : Don't export glyph names in the result font. It
                             will reduce file size.
 --dummy-DSIG              : Include an empty DSIG table in the font. For
                             some Microsoft applications, a DSIG is required
                             to enable OpenType features.
 --ship                    : Equivalent to the combination of these options:
                              *  --ignore-glyph-order
                              *  --short-post
                              *  --dummy-dsig
```

## Building

`otfcc` can be built on a number of platforms. It uses the [premake](http://premake.github.io/) build system.

It was developed and optimized for Clang/LLVM, therefore it is *strongly* recommended to compile with Clang/LLVM, but if that's not possible GCC is also supported, GCC version 5.1 or later being the preferred choice for performance.

### Linux (or other Unix-like)

On Linux, Either Clang/LLVM or GCC can be used to build `otfcc`.

1. Install the latest Clang/LLVM or GCC if you do not have it already.
2. Download and install [premake5](http://premake.github.io/) for Linux and make it available in your path.
3. Run the following from the command line:

```bash
premake5 gmake
cd build/gmake
make
```

### Windows

On Windows, building `otfcc` is tested under the toolchains listed below. The default `premake5 vs2015` will produce a Visual Studio solution using Clang-CL as its compiler.

* GCC 5.1 included in `TDM-GCC`. Run the following from the command line:

  ```bash
  premake5 gmake
  cd build/gmake
  make
  ```
* [Visual C++ Building Tools (Mar 2016)](https://blogs.msdn.microsoft.com/vcblog/2016/03/31/announcing-the-official-release-of-the-visual-c-build-tools-2015/) with [Clang/LLVM 3.8](http://clang.llvm.org/). Only Release build is tested. Run the following from the Visual C++ Command Prompt:

  ```bat
  premake5 vs2015
  msbuild build\vs\otfcc.sln /property:Configuration=Release
  ```


### Mac OS

premake5 provides ability to produce XCode projects. Run

```bash
premake5 xcode4
```

And then you can open `build/xcode/otfcc.xcworkspace` and build with XCode. You can find built binaries in `bin/`.

Please ensure that Xcode’s Developer Mode is enabled.

To build binaries in your terminal, run

```bash
xcodebuild -workspace build/xcode/otfcc.xcworkspace -scheme otfccbuild -configuration Release
xcodebuild -workspace build/xcode/otfcc.xcworkspace -scheme otfccdump -configuration Release
```

