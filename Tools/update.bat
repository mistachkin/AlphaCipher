@ECHO OFF

::
:: update.bat --
::
:: Update Tool
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

REM SET DFLAGS=/L

%_VECHO% DFlags = '%DFLAGS%'

SET FLAGS=/V /F /G /H /I /R /S /Y /Z

%_VECHO% Flags = '%FLAGS%'

SET FFLAGS=/V /F /G /H /I /R /Y /Z

%_VECHO% FFlags = '%FFLAGS%'

SET PLATFORM=%2

IF DEFINED PLATFORM (
  CALL :fn_UnquoteVariable PLATFORM
) ELSE (
  %_AECHO% No platform specified, using default...
  SET PLATFORM=Win32
)

%_VECHO% Platform = '%PLATFORM%'

SET CONFIGURATION=%3

IF DEFINED CONFIGURATION (
  CALL :fn_UnquoteVariable CONFIGURATION
) ELSE (
  %_AECHO% No configuration specified, using default...
  SET CONFIGURATION=ReleaseDll
)

%_VECHO% Configuration = '%CONFIGURATION%'

SET TOOLS=%~dp0
SET TOOLS=%TOOLS:~0,-1%

%_VECHO% Tools = '%TOOLS%'

SET SOURCE=%~dp0\..
SET SOURCE=%SOURCE:\\=\%

%_VECHO% Source = '%SOURCE%'

SET TARGET=%1

IF NOT DEFINED TARGET (
  GOTO usage
)

CALL :fn_UnquoteVariable TARGET

%_VECHO% Target = '%TARGET%'

CALL :fn_ResetErrorLevel

FOR %%D IN (Common\Externals\sqlite3 Navajo\4.0\src Navajo\4.0\test) DO (
  FOR %%E IN (.dll .exe .pdb) DO (
    %__ECHO% XCOPY "%SOURCE%\%%D\%PLATFORM%\%CONFIGURATION%\*%%E" "%TARGET%\%PLATFORM%_%CONFIGURATION%\bin" %FLAGS% %DFLAGS%

    IF ERRORLEVEL 1 (
      ECHO Failed to copy "%SOURCE%\%%D\%PLATFORM%\%CONFIGURATION%\*%%E" to "%TARGET%\%PLATFORM%_%CONFIGURATION%\bin".
      GOTO errors
    )
  )

  FOR %%E IN (.lib) DO (
    %__ECHO% XCOPY "%SOURCE%\%%D\%PLATFORM%\%CONFIGURATION%\*%%E" "%TARGET%\%PLATFORM%_%CONFIGURATION%\lib" %FLAGS% %DFLAGS%

    IF ERRORLEVEL 1 (
      ECHO Failed to copy "%SOURCE%\%%D\%PLATFORM%\%CONFIGURATION%\*%%E" to "%TARGET%\%PLATFORM%_%CONFIGURATION%\lib".
      GOTO errors
    )
  )
)

IF NOT EXIST "%TARGET%\%PLATFORM%_%CONFIGURATION%\include" (
  %__ECHO% MKDIR "%TARGET%\%PLATFORM%_%CONFIGURATION%\include"

  IF ERRORLEVEL 1 (
    ECHO Could not create directory "%TARGET%\%PLATFORM%_%CONFIGURATION%\include".
    GOTO errors
  )
)

FOR /F "delims=" %%F IN (%TOOLS%\include_bin.txt) DO (
  %__ECHO% XCOPY "%SOURCE%\%%F" "%TARGET%\%PLATFORM%_%CONFIGURATION%\include" %FLAGS% %DFLAGS%

  IF ERRORLEVEL 1 (
    ECHO Failed to copy "%SOURCE%\%%F" to "%TARGET%\%PLATFORM%_%CONFIGURATION%\include".
    GOTO errors
  )
)

IF NOT EXIST "%TARGET%\%PLATFORM%_%CONFIGURATION%\test" (
  %__ECHO% MKDIR "%TARGET%\%PLATFORM%_%CONFIGURATION%\test"

  IF ERRORLEVEL 1 (
    ECHO Could not create directory "%TARGET%\%PLATFORM%_%CONFIGURATION%\test".
    GOTO errors
  )
)

FOR /F "delims=" %%F IN (%TOOLS%\include_test.txt) DO (
  %__ECHO% XCOPY "%SOURCE%\%%F" "%TARGET%\%PLATFORM%_%CONFIGURATION%\test" %FLAGS% %DFLAGS%

  IF ERRORLEVEL 1 (
    ECHO Failed to copy "%SOURCE%\%%F" to "%TARGET%\%PLATFORM%_%CONFIGURATION%\test".
    GOTO errors
  )
)

IF NOT EXIST "%TARGET%\%PLATFORM%_%CONFIGURATION%\keys" (
  %__ECHO% MKDIR "%TARGET%\%PLATFORM%_%CONFIGURATION%\keys"

  IF ERRORLEVEL 1 (
    ECHO Could not create directory "%TARGET%\%PLATFORM%_%CONFIGURATION%\keys".
    GOTO errors
  )
)

%__ECHO% XCOPY "%SOURCE%\Navajo\4.0\keys" "%TARGET%\%PLATFORM%_%CONFIGURATION%\keys" %FLAGS% %DFLAGS%

IF ERRORLEVEL 1 (
  ECHO Failed to copy "%SOURCE%\Navajo\4.0\keys" to "%TARGET%\%PLATFORM%_%CONFIGURATION%\keys".
  GOTO errors
)

%__ECHO% XCOPY "%SOURCE%\Navajo\4.0\src\README" "%TARGET%\%PLATFORM%_%CONFIGURATION%" %FLAGS% %DFLAGS%

IF ERRORLEVEL 1 (
  ECHO Failed to copy "%SOURCE%\Navajo\4.0\src\README" to "%TARGET%\%PLATFORM%_%CONFIGURATION%".
  GOTO errors
)

%__ECHO% XCOPY "%SOURCE%\Navajo\4.0\src\license.terms" "%TARGET%\%PLATFORM%_%CONFIGURATION%" %FLAGS% %DFLAGS%

IF ERRORLEVEL 1 (
  ECHO Failed to copy "%SOURCE%\Navajo\4.0\src\license.terms" to "%TARGET%\%PLATFORM%_%CONFIGURATION%".
  GOTO errors
)

GOTO no_errors

:fn_UnquoteVariable
  SETLOCAL
  IF NOT DEFINED %1 GOTO :EOF
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
  ECHO Usage: %~nx0 ^<target^> [platform] [configuration]
  ECHO.
  GOTO errors

:errors
  CALL :fn_SetErrorLevel
  ENDLOCAL
  ECHO.
  ECHO Update failure, errors were encountered.
  GOTO end_of_file

:no_errors
  CALL :fn_ResetErrorLevel
  ENDLOCAL
  ECHO.
  ECHO Update success, no errors were encountered.
  GOTO end_of_file

:end_of_file
%__ECHO% EXIT /B %ERRORLEVEL%
