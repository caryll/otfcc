@CALL "%VS140COMNTOOLS%\..\..\VC\vcvarsall.bat" amd64
@set PATH=C:\Program Files\LLVM\msbuild-bin;%PATH%
@cd build/ninja
@ninja -f build.ninja %*
@cd ../