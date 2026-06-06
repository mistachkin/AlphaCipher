@ECHO OFF

::
:: release.bat --
::
:: Binary Release Tool
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

SET PLATFORM=%1

IF DEFINED PLATFORM (
  CALL :fn_UnquoteVariable PLATFORM
) ELSE (
  %_AECHO% No platform specified, using default...
  SET PLATFORM=Win32
)

SET CONFIGURATION=%2

IF DEFINED CONFIGURATION (
  CALL :fn_UnquoteVariable CONFIGURATION
) ELSE (
  %_AECHO% No configuration specified, using default...
  SET CONFIGURATION=ReleaseDll
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

IF NOT DEFINED SfxBinary (
  SET SfxBinary=%TOOLS%\AlphaCipherSDK.sfx
)

SET SfxBinary=%SfxBinary:\\=\%

%_VECHO% SfxBinary = '%SfxBinary%'

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

IF NOT EXIST "Releases\%PLATFORM%_%CONFIGURATION%" (
  %__ECHO% MKDIR "Releases\%PLATFORM%_%CONFIGURATION%"

  IF ERRORLEVEL 1 (
    ECHO Could not create directory "Releases\%PLATFORM%_%CONFIGURATION%".
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

IF NOT DEFINED TEMP (
  ECHO Temporary directory must be defined.
  GOTO errors
)

%_VECHO% Temp = '%TEMP%'

IF EXIST "%TEMP%\AlphaCipherSDK_%PLATFORM%_%CONFIGURATION%" RMDIR /S /Q "%TEMP%\AlphaCipherSDK_%PLATFORM%_%CONFIGURATION%"

IF ERRORLEVEL 1 (
  ECHO Could not remove release staging directory.
  GOTO errors
)

%__ECHO% CALL "%TOOLS%\update.bat" "%TEMP%\AlphaCipherSDK_%PLATFORM%_%CONFIGURATION%\AlphaCipherSDK" "%PLATFORM%" "%CONFIGURATION%"

IF ERRORLEVEL 1 (
  ECHO Could not update binary files.
  GOTO errors
)

%__ECHO% PUSHD "%TEMP%\AlphaCipherSDK_%PLATFORM%_%CONFIGURATION%"

IF ERRORLEVEL 1 (
  ECHO Could not change directory to "%TEMP%\AlphaCipherSDK_%PLATFORM%_%CONFIGURATION%".
  GOTO errors
)

%_VECHO% Suffix = '%SUFFIX%'
%_VECHO% PatchLevel = '%PATCHLEVEL%'

%_CECHO% RAR.exe a -r -k -ed -av -rr "-z%ROOT%\Navajo\4.0\src\README" "-x@%TOOLS%\exclude_bin.txt" %RarNameFormat% -- "%ROOT%\Releases\%PLATFORM%_%CONFIGURATION%\AlphaCipherSdkBinary%SUFFIX%%PATCHLEVEL%.rar" "AlphaCipherSDK\*"
%__ECHO% RAR.exe a -r -k -ed -av -rr "-z%ROOT%\Navajo\4.0\src\README" "-x@%TOOLS%\exclude_bin.txt" %RarNameFormat% -- "%ROOT%\Releases\%PLATFORM%_%CONFIGURATION%\AlphaCipherSdkBinary%SUFFIX%%PATCHLEVEL%.rar" "AlphaCipherSDK\*"

IF ERRORLEVEL 1 (
  ECHO Failed to archive binary files.
  GOTO errors
)

IF DEFINED NOSFX (
  %_AECHO% Skipping creation of SFX...
  GOTO skip_sfx
)

IF EXIST "%SfxBinary%" (
  %__ECHO% ECHO F %PIPE% XCOPY "%SfxBinary%" "%ProgramFiles%\WinRAR\Default.SFX" %FFLAGS% %DFLAGS%

  IF ERRORLEVEL 1 (
    ECHO Failed to copy "%SfxBinary%" to "%ProgramFiles%\WinRAR\Default.SFX".
    GOTO errors
  )
)

%__ECHO% FOR /F %%F IN ('DIR /B /OD "%ROOT%\Releases\%PLATFORM%_%CONFIGURATION%\AlphaCipherSdkBinary*.rar"') DO (SET BinRarFile=%%~nF)

IF DEFINED BinRarFile (
  SET BinRarFile=%ROOT%\Releases\%PLATFORM%_%CONFIGURATION%\%BinRarFile%
)

IF DEFINED BinRarFile (
  %_VECHO% BinRarFile = '%BinRarFile%'

  IF EXIST "%BinRarFile%.exe" (
    %__ECHO% DEL "%BinRarFile%.exe"

    IF ERRORLEVEL 1 (
      ECHO Could not delete file "%BinRarFile%.exe".
      GOTO errors
    )
  )

  %_CECHO% WinRAR.exe a -r -k -ed -ibck %SfxAvOption% -rr "-z%ROOT%\Navajo\4.0\src\README" "-x@%TOOLS%\exclude_bin.txt" -sfx -- "%BinRarFile%.exe" "AlphaCipherSDK\*"
  %__ECHO% WinRAR.exe a -r -k -ed -ibck %SfxAvOption% -rr "-z%ROOT%\Navajo\4.0\src\README" "-x@%TOOLS%\exclude_bin.txt" -sfx -- "%BinRarFile%.exe" "AlphaCipherSDK\*"

  IF ERRORLEVEL 1 (
    ECHO Failed to create binary SFX.
    GOTO errors
  )

  IF NOT DEFINED NOSIGN (
    %__ECHO3% CALL "%TOOLS%\signFile.bat" "%BinRarFile%.exe" "AlphaCipher SDK Distribution"

    IF ERRORLEVEL 1 (
      ECHO Failed to sign file "%BinRarFile%.exe".
      GOTO errors
    )
  )
) ELSE (
  ECHO Failed to find archived binary files.
  GOTO errors
)

:skip_sfx

%__ECHO% POPD

IF ERRORLEVEL 1 (
  ECHO Could not restore directory.
  GOTO errors
)

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
  ECHO Usage: %~nx0 [platform] [configuration]
  GOTO errors

:errors
  CALL :fn_SetErrorLevel
  ENDLOCAL
  ECHO.
  ECHO Release failure, errors were encountered.
  GOTO end_of_file

:no_errors
  CALL :fn_ResetErrorLevel
  ENDLOCAL
  ECHO.
  ECHO Release success, no errors were encountered.
  GOTO end_of_file

:end_of_file
%__ECHO% EXIT /B %ERRORLEVEL%
