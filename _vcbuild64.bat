CALL "%VS140COMNTOOLS%\..\..\VC\vcvarsall.bat" amd64
msbuild build/otfcc.sln /m %*
