<p align="center"><img src="https://raw.githubusercontent.com/caryll/design/master/caryll-logo-libs-githubreadme.png" width=200></p><h1 align="center">otfcc</h1>

The `otfcc` is a C library and utility used for parsing and writing OpenType font files.

## Key features

* Read a OpenType font; (TrueType is supported as well)
* Write a OpenType font;

## `otfcc` command line tool

### `otfccdump` : Dumps a OpenType font file into JSON
``` bash
otfccdump [OPTIONS] input.[otf|ttf|ttc]

 -h, --help              : Display this help message and exit.
 -v, --version           : Display version information and exit.
 -o <file>               : Set output file path to <file>.
 -n <n>, --ttc-index <n> : Use the <n>th subfont within the input font file.
 --pretty                : Prettify the output JSON.
 --ugly                  : Force uglify the output JSON.
 --time                  : Time each substep.
 --ignore-glyph-order    : Do not export glyph order information.
 --ignore-hints          : Do not export hingint information.
```

### `otfccbuild` : Builds an OpenType font file form JSON
```bash
otfccbuild [OPTIONS] input.json -o output.[ttf|otf]

 -h, --help           : Display this help message and exit.
 -v, --version        : Display version information and exit.
 -o <file>            : Set output file path to <file>.
 --time               : Time each substep.
 --ignore-glyph-order : Ignore the glyph order information in the input, except for gid0.
 --ignore-hints       : Ignore the hinting information in the input.
```

