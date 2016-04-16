@CALL "%VS140COMNTOOLS%\..\..\VC\vcvarsall.bat" x86
@msbuild build\vs\otfcc.sln /m:%NUMBER_OF_PROCESSORS% /nr:false /nologo /verbosity:minimal %*
