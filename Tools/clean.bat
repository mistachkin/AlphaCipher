@ECHO OFF

::
:: clean.bat --
::
:: Build Cleaning Tool
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
IF NOT DEFINED _AECHO (SET _AECHO=REM)
IF NOT DEFINED _CECHO (SET _CECHO=REM)
IF NOT DEFINED _VECHO (SET _VECHO=REM)

%_AECHO% Running %0 %*

SET DUMMY2=%1

IF DEFINED DUMMY2 (
  GOTO usage
)

SET SOURCE=%~dp0\..
SET SOURCE=%SOURCE:\\=\%

%_VECHO% Source = '%SOURCE%'

IF DEFINED CLEANDIRS GOTO skip_cleanDirs

SET CLEANDIRS=Win32 x64 "Pocket PC 2003 (ARMV4)"
SET CLEANDIRS=%CLEANDIRS% "Windows Mobile 5.0 Pocket PC SDK (ARMV4I)"
SET CLEANDIRS=%CLEANDIRS% "Windows Mobile 5.0 Smartphone SDK (ARMV4I)"

:skip_cleanDirs

%_VECHO% CleanDirs = '%CLEANDIRS%'

CALL :fn_ResetErrorLevel

%_AECHO%.

FOR %%C IN (%CLEANDIRS%) DO (
  FOR /F "delims=" %%D IN ('DIR /B /S /AD "%SOURCE%\%%~C" 2^> NUL') DO (
    %__ECHO% RMDIR /S /Q "%%D"

    IF ERRORLEVEL 1 (
      ECHO Could not remove directory "%%D".
      ECHO.
      GOTO errors
    ) ELSE (
      %_AECHO% Removed directory "%%D".
      %_AECHO%.
    )
  )
)

GOTO no_errors

:fn_ResetErrorLevel
  VERIFY > NUL
  GOTO :EOF

:fn_SetErrorLevel
  VERIFY MAYBE 2> NUL
  GOTO :EOF

:usage
  ECHO.
  ECHO Usage: %~nx0
  ECHO.
  GOTO errors

:errors
  CALL :fn_SetErrorLevel
  ENDLOCAL
  ECHO.
  ECHO Clean failure, errors were encountered.
  GOTO end_of_file

:no_errors
  CALL :fn_ResetErrorLevel
  ENDLOCAL
  ECHO.
  ECHO Clean success, no errors were encountered.
  GOTO end_of_file

:end_of_file
%__ECHO% EXIT /B %ERRORLEVEL%
