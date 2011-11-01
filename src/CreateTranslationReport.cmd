@rem 
@echo off

::
:: This script is used to create a translation report in the format:
::
:: tr ... translated strings
:: to ... total strings
::
:: Language ....................... tr/to ... percentage
:: =====================================================
:: English (US) ................... 65/65 ... 100%
::
:: Copyright (c) 2011 by Stefan Pendl (stefanpe@users.sourceforge.net).
::
:: This program is free software; you can redistribute it and/or modify
:: it under the terms of the GNU General Public License as published by
:: the Free Software Foundation; either version 2 of the License, or
:: (at your option) any later version.
::
:: This program is distributed in the hope that it will be useful,
:: but WITHOUT ANY WARRANTY; without even the implied warranty of
:: MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
:: GNU General Public License for more details.
::
:: You should have received a copy of the GNU General Public License
:: along with this program; if not, write to the Free Software
:: Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
::

set dots=........................................

echo.
echo Extracting strings, please wait ...
echo.
for %%F in ( "%~dp0\gui\i18n\translation.template" "%~dp0\gui\i18n\*.lng" ) do echo Processing "%%~nF" ... & call :ExtractStrings "%%~F" >"%TMP%\%%~nF.tmp"

echo.
echo Counting total tramslation strings ...
set gCounter=0
for /F %%L in ( %TMP%\translation.tmp ) do call :IncreaseCounter
set TotalStrings=%gCounter%

echo.
echo %TotalStrings% translation strings total ...

echo.
echo Comparing files ...
for %%F in ( "%~dp0\gui\i18n\*.lng" ) do fc "%TMP%\translation.tmp" "%TMP%\%%~nF.tmp" >"%TMP%\%%~nF.cmp"

echo.
echo Creating report ...
echo.
(
    echo.
    echo tr ... translated strings
    echo to ... total strings
    echo.
    echo Language ...............................  tr/ to ... percentage
    echo ===============================================================
    echo.
) >"%~dp0\gui\i18n\TranslationReport.txt"

for %%F in ( "%~dp0\gui\i18n\*.lng" ) do call :CycleLines "%TMP%\%%~nF.cmp"

cls
type "%~dp0\gui\i18n\TranslationReport.txt"

:quit
echo.
pause

goto :EOF

::
:: === procedures ===
::

::
:: This procedure processes one file
::
:CycleLines
    set gCounter=0
    set FileName="%~n1 %dots%"
    set StartCounting=0
    set suffix=
    
    echo Processing "%~n1" ...
    for /F "tokens=1*" %%L in ( 'type "%~1"' ) do if not "%%~L" == "" call :ParseResult "%~n1" "%%~L" "%%~nM"
    
    set /A Percentage="%gCounter% * 100 / %TotalStrings%"
    
    if %Percentage% GTR 100 set suffix=???
    
    set TranslatedFormat=   %gCounter%
    set TotalFormat=   %TotalStrings%
    set PercentFormat=      %Percentage%
    
    echo %FileName:~1,40% %TranslatedFormat:~-3%/%TotalFormat:~-3% ... %PercentFormat:~-6% %% %suffix% >>"%~dp0\gui\i18n\TranslationReport.txt"
goto :EOF

::
:: This procedure parses the compare results
::
:: usage: call :ParseResult {file name} {first token} {remaining tokens as file name}
::
:ParseResult
    if "%~2" == "*****" (
        if /i "%~1" == "%~3" (
            set StartCounting=1
        ) else (
            set StartCounting=0
        )
    ) else (
        if %StartCounting% EQU 1 call :IncreaseCounter
    )
goto :EOF

::
:: This procedure increases the global counter
::
:IncreaseCounter
    set /A gCounter+=1
goto :EOF

::
:: This procedure filters comments and empty lines,
:: so only translation strings remain
::
:ExtractStrings
    type "%~1" | findstr /v /r "^;" | findstr /v /r "^$" >"%TMP%\%~n1.tmp1"
    
    setlocal EnableDelayedExpansion
    
    for /f "tokens=*" %%L in ('type "%TMP%\%~n1.tmp1"') do (
        set "line=%%~L"

        echo !line:"=!
        echo.
    )
    
    endlocal
goto :EOF
