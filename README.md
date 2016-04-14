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
 --ignore-hints          : Do not export hinting information.
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
