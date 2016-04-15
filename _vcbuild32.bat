@CALL "%VS140COMNTOOLS%\..\..\VC\vcvarsall.bat" x86
@msbuild build/otfcc.sln /m /nologo /verbosity:minimal %*
