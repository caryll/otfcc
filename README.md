<p align="center"><img src="https://raw.githubusercontent.com/caryll/design/master/caryll-logo-libs-githubreadme.png" width=200></p><h1 align="center">otfcc</h1>

The `otfcc` is a C library and utility used for parsing and writing OpenType font files.

## Key features

* Read an OpenType font; (TrueType is supported as well)
* Write an OpenType font;

## `otfcc` command line tool

### `otfccdump` : Dump an OpenType font file into JSON
```
otfccdump [OPTIONS] input.[otf|ttf|ttc]

 -h, --help              : Display this help message and exit.
 -v, --version           : Display version information and exit.
 -o <file>               : Set output file path to <file>.
 -n <n>, --ttc-index <n> : Use the <n>th subfont within the input font.
 --pretty                : Prettify the output JSON.
 --ugly                  : Force uglify the output JSON.
 --time                  : Time each substep.
 --ignore-glyph-order    : Do not export glyph order information.
 --ignore-hints          : Do not export hingint information.
 --add-bom               : Add BOM mark in the output. (This is default
                           on Windows when redirecting to another program.
                           Use --no-bom to turn it off.)
```

### `otfccbuild` : Build an OpenType font file from JSON
```
otfccbuild [OPTIONS] input.json -o output.[ttf|otf]

 -h, --help                : Display this help message and exit.
 -v, --version             : Display version information and exit.
 -o <file>                 : Set output file path to <file>.
 --time                    : Time each substep.
 --ignore-glyph-order      : Ignore the glyph order information in the input.
 --ignore-hints            : Ignore the hinting information in the input.
 --keep-average-char-width : Keep the OS/2.xAvgCharWidth value from the input
                             instead of stating the average width of glyphs.
                             Useful when creating a monospaced font.
 --short-post              : Don't export glyph names in the result font. It
                             will reduce file size.
 --dummy-DSIG              : Include an empty DSIG table in the font. For
                             some Microsoft applications, a DSIG is required
                             to enable OpenType features.
```

## Building from source

`otfcc` can be built on a number of platforms. It uses the�[premake](http://premake.github.io/)�build system.

It was developed and optimized for Clang/LLVM, therefore it is *strongly* recommended to compile with Clang/LLVM, but if that's not possible GCC is also supported, GCC version 5.1 or later being the preferred choice for performance.

### Windows

On Windows building `otfcc` is tested under the toolchains listed below. The default `premake5 vs2015` will produce a Visual Studio solution using Clang-CL as its compiler.

* GCC 5.1 included in `TDM-GCC`. Run the following from the command line:

  ```bash
  premake5 gmake
  cd build
  make
  ```

* [Visual C++ Building Tools (Mar 2016)](https://blogs.msdn.microsoft.com/vcblog/2016/03/31/announcing-the-official-release-of-the-visual-c-build-tools-2015/) with [Clang/LLVM 3.8](http://clang.llvm.org/). Run the following from the Visual C++ Command Prompt:

  ```bat
  premake5 vs2015
  msbuild build\otfcc.sln
  ```

### Linux

On Linux, Either Clang/LLVM or GCC can be used to build `otfcc`.

1. Install the latest Clang/LLVM or GCC if you do not have it already.
2. Download and install�[premake5](http://premake.github.io/)�for Linux and make it available in your path.
3. Run the following from the command line:

```bash
premake5 gmake
cd build
make
```

