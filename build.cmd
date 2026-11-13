@echo off
rem Builds cramstation.exe with g++ from MSYS2 UCRT64.
setlocal
cd /d "%~dp0"
set PATH=C:\msys64\ucrt64\bin;%PATH%
windres cramstation.rc -O coff -o cramstation.res || exit /b 1
g++ -std=c++17 -O2 -s -static -municode -mwindows -DUNICODE -D_UNICODE -DNOMINMAX -D_WIN32_WINNT=0x0A00 -Wall ^
    src\main.cpp src\launcher_window.cpp src\editor_window.cpp src\ui.cpp ^
    src\launch.cpp src\desktops.cpp src\tiling.cpp src\files.cpp src\profiles.cpp src\text.cpp ^
    cramstation.res -o cramstation.exe ^
    -lcomctl32 -lcomdlg32 -ldwmapi -lole32 -lshell32 || exit /b 1
del cramstation.res
echo Built cramstation.exe
