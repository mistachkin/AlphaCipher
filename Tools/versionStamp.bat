@ECHO OFF

::
:: versionStamp.bat --
::
:: Extensible Adaptable Generalized Logic Engine (Eagle)
:: Version Stamping Tool
::
:: Copyright (c) 2007-2026 by Joe Mistachkin.  All rights reserved.
::
:: See the file "license.terms" for information on usage and redistribution of
:: this file, and for a DISCLAIMER OF ALL WARRANTIES.
::
:: RCS: @(#) $Id: $
::

SETLOCAL

REM SET __ECHO=ECHO
IF NOT DEFINED _AECHO (SET _AECHO=ECHO)
IF NOT DEFINED _CECHO (SET _CECHO=ECHO)
IF NOT DEFINED _VECHO (SET _VECHO=REM)

%_AECHO% Running %0 %*

SET MAJORMINOR=%1

IF NOT DEFINED MAJORMINOR (
  GOTO usage
)

%_VECHO% MajorMinor = '%MAJORMINOR%'

SET FILE=%2

IF NOT DEFINED FILE (
  GOTO usage
)

%_VECHO% File = '%FILE%'

SET CONFIGURATION=%3

IF DEFINED CONFIGURATION (
  CALL :fn_UnquoteVariable CONFIGURATION
) ELSE (
  %_AECHO% No configuration specified, using default...
  SET CONFIGURATION=Release
)

SET CONFIGURATION=%CONFIGURATION:All=%
SET CONFIGURATION=%CONFIGURATION:Dll=%

%_VECHO% Configuration = '%CONFIGURATION%'

SET DUMMY2=%4

IF DEFINED DUMMY2 (
  GOTO usage
)

IF NOT DEFINED EAGLE (
  SET EAGLE=%~dp0\..\..\bin\%CONFIGURATION%
)

IF NOT EXIST "%EAGLE%\bin\EagleShell.exe" (
  SET EAGLE=%~dp0\..\..\bin\%CONFIGURATION%
)

IF DEFINED NOLKG GOTO skip_lastKnownGood

IF DEFINED LKG (
  IF NOT EXIST "%EAGLE%\bin\EagleShell.exe" (
    SET EAGLE=%LKG%\Eagle
  )
)

:skip_lastKnownGood

SET EAGLE=%EAGLE:\\=\%

%_VECHO% Eagle = '%EAGLE%'

SET EAGLEBINDIR=%EAGLE%\bin
SET EAGLEBINDIR=%EAGLEBINDIR:\\=\%

%_VECHO% EagleBinDir = '%EAGLEBINDIR%'

IF NOT EXIST "%EAGLEBINDIR%\EagleShell.exe" (
  ECHO Skipping version stamping build step...
  ECHO Missing tool file "%EAGLEBINDIR%\EagleShell.exe".

  REM
  REM NOTE: This build step is optional.  Just skip it.
  REM
  GOTO no_errors
)

SET PATH=%EAGLEBINDIR%;%PATH%

%_VECHO% Path = '%PATH%'

SET TOOLS=%~dp0
SET TOOLS=%TOOLS:~0,-1%

%_VECHO% Tools = '%TOOLS%'

CALL :fn_ResetErrorLevel

%__ECHO% EagleShell.exe -file "%TOOLS%\versionStamp.eagle" "%MAJORMINOR%" "%FILE%"

IF ERRORLEVEL 1 (
  ECHO Version stamping failed.
  GOTO errors
)

GOTO no_errors

:fn_UnquoteVariable
  IF NOT DEFINED %1 GOTO :EOF
  SETLOCAL
  SET __ECHO_CMD=ECHO %%%1%%
  FOR /F "delims=" %%V IN ('%__ECHO_CMD%') DO (
    SET VALUE=%%V
  )
  SET VALUE=%VALUE:"=%
  REM "
  ENDLOCAL && SET %1=%VALUE%
  GOTO :EOF

:fn_ResetErrorLevel
  VERIFY > NUL
  GOTO :EOF

:fn_SetErrorLevel
  VERIFY MAYBE 2> NUL
  GOTO :EOF

:usage
  ECHO.
  ECHO Usage: %~nx0 ^<majorMinorVersion^> ^<fileName^> [configuration]
  GOTO errors

:errors
  CALL :fn_SetErrorLevel
  ENDLOCAL
  ECHO.
  ECHO Failure, errors were encountered.
  GOTO end_of_file

:no_errors
  CALL :fn_ResetErrorLevel
  ENDLOCAL
  ECHO.
  ECHO Success, no errors were encountered.
  GOTO end_of_file

:end_of_file
%__ECHO% EXIT /B %ERRORLEVEL%
