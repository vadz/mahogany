@echo off
rem Usage: m4.bat input_dir output_dir
rem
rem This batch file is used to produce MInterface.{h|cpp} from MInterface.mid
rem both on the systems with m4 installed and on the systems without it.
if .%1==. goto :usage
if not .%3==. goto :usage

rem you may edit the line below if m4 is not in your PATH
set M4=m4

%M4% -DM4FILE=%1\mid2cpp.m4 %1\MInterface.mid > %2\MInterface.cpp
if errorlevel 1 goto no_m4
%M4% -DM4FILE=%1\mid2h.m4 %1\MInterface.mid > %2\MInterface.h
goto end

:no_m4
copy %2\MInterface.cpp.m4 %1\MInterface.cpp
copy %2\MInterface.h.m4 %1\MInterface.h
goto end

:usage
echo Usage: m4.bat input_dir output_dir
rem echo was called as %0 %1 %2 %3 %4 %5 %6 %7 %8 %9
exit /b 1

:end
exit /b 0
