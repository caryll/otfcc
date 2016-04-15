@CALL "%VS140COMNTOOLS%\..\..\VC\vcvarsall.bat" amd64
@msbuild build\vs\otfcc.sln /m:%NUMBER_OF_PROCESSORS% /nr:false /nologo /verbosity:minimal %*
