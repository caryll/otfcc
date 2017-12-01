@echo off

set PROJDIR=%~dp0

for /f "usebackq tokens=*" %%i in (`"C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere" -latest -products * -requires Microsoft.Component.MSBuild -property installationPath`) do (
  set InstallDir=%%i
)

if exist "%InstallDir%\MSBuild\15.0\Bin\MSBuild.exe" (
	call "%InstallDir%\Common7\Tools\VsDevCmd.bat"
	pushd %PROJDIR%
	"%InstallDir%\MSBuild\15.0\Bin\MSBuild.exe" /m:%NUMBER_OF_PROCESSORS% /nr:false /nologo /verbosity:minimal %*
	popd
)
