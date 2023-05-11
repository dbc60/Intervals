@ECHO OFF
SETLOCAL ENABLEDELAYEDEXPANSION ENABLEEXTENSIONS

:: See LICENSE.txt for copyright and licensing information about this file.

SET PROJECT_NAME="Intervals"
CALL %~dp0setup.cmd %*

:: Clean up
if %clean% EQU 1 set DEL_LOCAL_EXE=1
if %cleanall% EQU 1 set DEL_LOCAL_EXE=1
if defined DEL_LOCAL_EXE (
    del cppglob.exe
)

:: Build the project
IF %build% EQU 1 (
    IF %verbose% EQU 1 (
        ECHO.
        ECHO Build Intervals
    )
    cl %CommonCompilerFlagsFinal% ^
    /I%DIR_INCLUDE% ^
    !DIR_REPO!\src\main.cpp  /Fo:%DIR_OUT_OBJ%\ ^
    /Fd:%DIR_OUT_BIN%\intervals.pdb /Fe:%DIR_OUT_BIN%\intervals.exe /link ^
    %CommonLinkerFlagsFinal% /ENTRY:mainCRTStartup
    copy %DIR_OUT_BIN%\Intervals.exe !DIR_REPO!
)
ENDLOCAL
