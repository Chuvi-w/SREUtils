@echo off
setlocal EnableDelayedExpansion enableextensions
set CUR=%~dp0
set CUR_NO_SLASH=%CUR%
IF %CUR_NO_SLASH:~-1%==\ SET CUR_NO_SLASH=%CUR_NO_SLASH:~0,-1%

FOR /F %%i IN ("%CUR_NO_SLASH%") DO set SrcDir=%%~ni
FOR /F %%i IN ("%CUR_NO_SLASH%") DO set SrcExt=%%~xi
FOR /F %%i IN ("%CUR%") DO set BuildRoot=%%~fi
if "%SrcExt%" NEQ "" (set SrcDir=%SrcDir%%SrcExt%)


echo SrcDir=%BuildRoot%

set /a HaveErrors=0
set /a CleanDir=0
set /a Build=0
set /a XP_Tool=0


goto Start

:CheckCMakeExist
	WHERE /q cmake  >nul 2>nul
	if %errorlevel% neq 0 ( 
		echo CMake not found. 
		exit /b 1
	) 
	exit /b 0
:ParseArgs
	set /a SkipArg=0
	for %%x in (%*) do (
		if /i "%%x"=="-c" (set /a CleanDir=1)
		if /i "%%x"=="-b" (set /a Build=1)
		if /i "%%x"=="-x" (set /a XP_Tool=1)	
	)
	exit /b 0

:DetectVSVersion
		
		set VS_Platform=win32
		if "%VisualStudioVersion%"=="" (
		echo VisualStudioVersion not set
		echo Run vsvarsall.bat from VS Directory and try again		
		exit /b 1
	)
	if "%VisualStudioVersion%" == "10.0" ( 
		set GenVer=Visual Studio 10 2010
		set Toolset=v100
		)

	if "%VisualStudioVersion%" == "12.0" ( 
		set GenVer=Visual Studio 12 2013
		set Toolset=v120
		)
		
	if "%VisualStudioVersion%" == "14.0" ( 
		set GenVer=Visual Studio 14 2015
		set Toolset=v140
		)
		
	if "%VisualStudioVersion%" == "15.0" ( 
		set GenVer=Visual Studio 15 2017
		set Toolset=v141
		)	
	if "%Toolset%"=="" (
		echo Unknown VisualStudioVersion: %VisualStudioVersion%
		exit /b 1
		)
	if %XP_Tool%==1 (set Toolset=%Toolset%_xp)
	if /i "%Platform%" == "x64" ( 
		set VS_Platform=x64  
		set VSPlat= Win64
	)
	exit /b 0

:InitBuildPathVars
	set BuildDir="%BuildRoot%Build\%Toolset%_%VS_Platform%"
	set InstallDir="%BuildRoot%Install\%Toolset%_%VS_Platform%"
	set SourceDir="%BuildRoot%"
	
	echo SourceDir==%SourceDir%
	set InstallDir=!InstallDir:\=/!
	
	
	IF %CleanDir%==1 (
		if exist %BuildDir% (rmdir /S /Q %BuildDir%)
		rem if exist %InstallDir% (rmdir /S /Q %InstallDir%)
		)
		if not exist %BuildDir% (mkdir %BuildDir%)
		rem if not exist %InstallDir% (mkdir %InstallDir%)
exit /b 0

:GenerateProject
	pushd %BuildDir%	
	if %errorlevel% neq 0 ( exit /b 1)
	
	set CMAKE_ARGS=!CMAKE_ARGS! -DCMAKE_INSTALL_PREFIX:PATH=%InstallDir%
	cmake "%CUR_NO_SLASH%" -G "%GenVer%%VSPlat%"  !CMAKE_ARGS_SFML!
	if %Build%==1 (
		msbuild ALL_BUILD.vcxproj /p:Configuration="Debug"
		msbuild ALL_BUILD.vcxproj /p:Configuration="Release"
		msbuild ALL_BUILD.vcxproj /p:Configuration="MinSizeRel"
		msbuild ALL_BUILD.vcxproj /p:Configuration="RelWithDebInfo"
	)
	popd 
	exit /b 0

	
:Start
call :CheckCMakeExist
	if %errorlevel% neq 0 goto BuildError
call :ParseArgs %*
call :DetectVSVersion
	if %errorlevel% neq 0 goto BuildError
call :InitBuildPathVars
call :GenerateProject
	if %errorlevel% neq 0 goto BuildError
	
goto :eof
:BuildError
echo Build failed.