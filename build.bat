@echo off
rem build.bat - Build ojgen.exe tren Windows bang MSYS2 MinGW-w64 (g++).
rem   build.bat          build ban phat hanh
rem   build.bat run      build xong mo menu
setlocal EnableDelayedExpansion
cd /d "%~dp0"

set "CXX=g++"
where %CXX% >nul 2>nul
if errorlevel 1 (
    set "FOUND="
    for %%D in ("C:\msys64\ucrt64\bin" "C:\msys64\mingw64\bin" "D:\msys64\ucrt64\bin" "D:\msys64\mingw64\bin") do (
        if not defined FOUND if exist "%%~D\g++.exe" set "FOUND=%%~D"
    )
    if not defined FOUND (
        echo [LOI] Khong tim thay g++.
        echo   1. Cai MSYS2 tu https://www.msys2.org
        echo   2. Mo "MSYS2 UCRT64" va chay: pacman -S --needed mingw-w64-ucrt-x86_64-gcc
        echo   3. Them C:\msys64\ucrt64\bin vao bien moi truong PATH, mo lai cua so lenh.
        exit /b 1
    )
    echo [i] g++ chua co trong PATH, tam dung: !FOUND!
    set "PATH=!FOUND!;!PATH!"
)

rem Duong dan tuong doi (tranh loi voi thu muc co dau tieng Viet).
rem Neu them thu muc nguon moi, bo sung vao danh sach duoi day.
set "SRCS="
for %%F in (src\*.cpp src\core\*.cpp src\problems\*.cpp) do set SRCS=!SRCS! "%%F"

echo Dang bien dich ojgen.exe ...
%CXX% -std=c++17 -O2 -Wall -Wextra -static -Isrc/include !SRCS! -o ojgen.exe -lcomdlg32 -lshell32
if errorlevel 1 (
    echo [LOI] Build that bai. Xem thong bao loi o tren.
    exit /b 1
)
echo [OK] Da tao ojgen.exe
echo   Mo menu     : ojgen.exe
echo   Tro giup    : ojgen.exe --help
echo   Tu kiem tra : ojgen.exe --selftest

if /i "%~1"=="run" ojgen.exe
exit /b 0
