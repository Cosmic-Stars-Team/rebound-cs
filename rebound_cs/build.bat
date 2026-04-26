@echo off
REM build.bat — Build CS shared library for Python on Windows (MSVC)
REM Run from the pycs/ directory.

cd /d "%~dp0"

call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" > nul

set ROOT=..\
set BLDDIR=build_tmp

echo Compiling CS shared library...

if not exist "%BLDDIR%" mkdir "%BLDDIR%"

cl /nologo /LD /I. /I%ROOT%\src /D_GNU_SOURCE /DINLINE=__inline /Ox /fp:precise ^
    %ROOT%\src\rebound.c ^
    %ROOT%\src\tree.c ^
    %ROOT%\src\particle.c ^
    %ROOT%\src\gravity.c ^
    %ROOT%\src\boundary.c ^
    %ROOT%\src\collision.c ^
    %ROOT%\src\communication_mpi.c ^
    %ROOT%\src\display.c ^
    %ROOT%\src\tools.c ^
    %ROOT%\src\rotations.c ^
    %ROOT%\src\derivatives.c ^
    %ROOT%\src\simulationarchive.c ^
    %ROOT%\src\input.c ^
    %ROOT%\src\output.c ^
    %ROOT%\src\binarydiff.c ^
    %ROOT%\src\transformations.c ^
    %ROOT%\src\fmemopen.c ^
    %ROOT%\src\frequency_analysis.c ^
    %ROOT%\src\glad.c ^
    %ROOT%\src\server.c ^
    %ROOT%\src\integrator.c ^
    %ROOT%\src\integrator_bs.c ^
    %ROOT%\src\integrator_eos.c ^
    %ROOT%\src\integrator_ias15.c ^
    %ROOT%\src\integrator_janus.c ^
    %ROOT%\src\integrator_leapfrog.c ^
    %ROOT%\src\integrator_mercurius.c ^
    %ROOT%\src\integrator_saba.c ^
    %ROOT%\src\integrator_sei.c ^
    %ROOT%\src\integrator_trace.c ^
    %ROOT%\src\integrator_whfast.c ^
    %ROOT%\src\integrator_whfast512.c ^
    %ROOT%\cs\cs_simulation.c ^
    %ROOT%\cs\cs_gr.c ^
    %ROOT%\cs\cs_radiation.c ^
    %ROOT%\cs\cs_harmonics.c ^
    %ROOT%\cs\cs_tides.c ^
    %ROOT%\cs\cs_solarmass.c ^
    /Fe:%BLDDIR%\_cs.dll

if errorlevel 1 goto err

copy /y %BLDDIR%\_cs.dll _cs.dll > nul
rmdir /s /q %BLDDIR% 2>nul
echo SUCCESS: _cs.dll built
echo Run: pip install -e .. && python ../demo_cs.py
goto end

:err
echo FAILED: see errors above.
exit /b 1

:end
