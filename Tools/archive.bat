@ECHO OFF

::
:: archive.bat --
::
:: Source Archiving Tool
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

SET PIPE=^|
IF DEFINED __ECHO SET PIPE=^^^|

%_AECHO% Running %0 %*

REM SET DFLAGS=/L

%_VECHO% DFlags = '%DFLAGS%'

SET FFLAGS=/V /F /G /H /R /Y /Z

%_VECHO% FFlags = '%FFLAGS%'

SET CONFIGURATION=%1

IF DEFINED CONFIGURATION (
  CALL :fn_UnquoteVariable CONFIGURATION
) ELSE (
  %_AECHO% No configuration specified, using default...
  SET CONFIGURATION=Release
)

%_VECHO% Configuration = '%CONFIGURATION%'

SET ROOT=%~dp0\..
SET ROOT=%ROOT:\\=\%

SET TOOLS=%~dp0
SET TOOLS=%TOOLS:~0,-1%

%_VECHO% Root = '%ROOT%'
%_VECHO% Tools = '%TOOLS%'
%_VECHO% NoSign = '%NOSIGN%'

REM
REM HACK: Authenticode signing SFX executables breaks the WinRAR
REM       "authenticity verification" algorithm; therefore, disable it if
REM       necessary.
REM
IF DEFINED NOSIGN (
  SET SfxAvOption=-av
)

%_VECHO% SfxAvOption = '%SfxAvOption%'

IF NOT DEFINED SfxSource (
  SET SfxSource=%TOOLS%\AlphaCipherSDK.sfx
)

SET SfxSource=%SfxSource:\\=\%

%_VECHO% SfxSource = '%SfxSource%'

IF NOT DEFINED PATCHLEVEL (
  SET RarNameFormat=-agYYYY-MM-DD-NN
)

%_VECHO% RarNameFormat = '%RarNameFormat%'

CALL :fn_ResetErrorLevel

%__ECHO% PUSHD "%ROOT%"

IF ERRORLEVEL 1 (
  ECHO Could not change directory to "%ROOT%".
  GOTO errors
)

IF NOT EXIST Releases (
  %__ECHO% MKDIR Releases

  IF ERRORLEVEL 1 (
    ECHO Could not create directory "Releases".
    GOTO errors
  )
)

SET PATH=%TOOLS%;%PATH%

IF "%PROCESSOR_ARCHITECTURE%" == "x86" GOTO set_path_x86

SET PATH=%ProgramFiles(x86)%\WinRAR;%PATH%
GOTO set_path_done

:set_path_x86

SET PATH=%ProgramFiles%\WinRAR;%PATH%

:set_path_done

%_VECHO% Path = '%PATH%'
%_VECHO% Suffix = '%SUFFIX%'
%_VECHO% PatchLevel = '%PATCHLEVEL%'

%_CECHO% RAR.exe a -r -k -ed -av -rr "-z%ROOT%\Navajo\4.0\src\README" "-x@%TOOLS%\exclude_sdk.txt" %RarNameFormat% -- "%ROOT%\Releases\AlphaCipherSdkSource%SUFFIX%%PATCHLEVEL%.rar" "Common\*" "Navajo\*" "Tools\*"
%__ECHO% RAR.exe a -r -k -ed -av -rr "-z%ROOT%\Navajo\4.0\src\README" "-x@%TOOLS%\exclude_sdk.txt" %RarNameFormat% -- "%ROOT%\Releases\AlphaCipherSdkSource%SUFFIX%%PATCHLEVEL%.rar" "Common\*" "Navajo\*" "Tools\*"

IF ERRORLEVEL 1 (
  ECHO Failed to archive source files.
  GOTO errors
)

IF DEFINED NOSFX (
  %_AECHO% Skipping creation of SFX...
  GOTO skip_sfx
)

%__ECHO% FOR /F %%F IN ('DIR /B /OD "%ROOT%\Releases\AlphaCipherSdkSource*.rar"') DO (SET SrcRarFile=%%~nF)

IF DEFINED SrcRarFile (
  SET SrcRarFile=%ROOT%\Releases\%SrcRarFile%
)

IF DEFINED SrcRarFile (
  %_VECHO% SrcRarFile = '%SrcRarFile%'

  IF EXIST "%SrcRarFile%.exe" (
    %__ECHO% DEL "%SrcRarFile%.exe"

    IF ERRORLEVEL 1 (
      ECHO Could not delete file "%SrcRarFile%.exe".
      GOTO errors
    )
  )

  IF EXIST "%SfxSource%" (
    %__ECHO% ECHO F %PIPE% XCOPY "%SfxSource%" "%ProgramFiles%\WinRAR\Default.SFX" %FFLAGS% %DFLAGS%

    IF ERRORLEVEL 1 (
      ECHO Failed to copy "%SfxSource%" to "%ProgramFiles%\WinRAR\Default.SFX".
      GOTO errors
    )
  )

  %_CECHO% WinRAR.exe a -r -k -ed -ibck %SfxAvOption% -rr "-z%ROOT%\Navajo\4.0\src\README" "-x@%TOOLS%\exclude_sdk.txt" -sfx -- "%SrcRarFile%.exe" "Common\*" "Navajo\*" "Tools\*"
  %__ECHO% WinRAR.exe a -r -k -ed -ibck %SfxAvOption% -rr "-z%ROOT%\Navajo\4.0\src\README" "-x@%TOOLS%\exclude_sdk.txt" -sfx -- "%SrcRarFile%.exe" "Common\*" "Navajo\*" "Tools\*"

  IF ERRORLEVEL 1 (
    ECHO Failed to create source SFX.
    GOTO errors
  )

  IF NOT DEFINED NOSIGN (
    %__ECHO3% CALL "%TOOLS%\signFile.bat" "%SrcRarFile%.exe" "AlphaCipher SDK Source-Code Distribution"

    IF ERRORLEVEL 1 (
      ECHO Failed to sign file "%SrcRarFile%.exe".
      GOTO errors
    )
  )
) ELSE (
  ECHO Failed to find archived source files.
  GOTO errors
)

:skip_sfx

%__ECHO% POPD

IF ERRORLEVEL 1 (
  ECHO Could not restore directory.
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
  ECHO Usage: %~nx0 [configuration]
  GOTO errors

:errors
  CALL :fn_SetErrorLevel
  ENDLOCAL
  ECHO.
  ECHO Archive failure, errors were encountered.
  GOTO end_of_file

:no_errors
  CALL :fn_ResetErrorLevel
  ENDLOCAL
  ECHO.
  ECHO Archive success, no errors were encountered.
  GOTO end_of_file

:end_of_file
%__ECHO% EXIT /B %ERRORLEVEL%
