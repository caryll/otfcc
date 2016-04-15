@CALL "%VS140COMNTOOLS%\..\..\VC\vcvarsall.bat" amd64
@msbuild build\vs\otfcc.sln /m /nologo /verbosity:minimal %*
