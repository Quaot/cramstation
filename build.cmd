@echo off
rem Builds crammy.exe with g++ from MSYS2 UCRT64.
setlocal
cd /d "%~dp0"
set PATH=C:\msys64\ucrt64\bin;%PATH%
windres crammy.rc -O coff -o crammy.res || exit /b 1
g++ -std=c++17 -O2 -s -static -municode -mwindows -DUNICODE -D_UNICODE -DNOMINMAX -D_WIN32_WINNT=0x0A00 -Wall ^
    src\main.cpp src\main_window.cpp src\canvas.cpp src\dialogs.cpp src\editor_window.cpp src\ui.cpp ^
    src\launch.cpp src\notes_tabs.cpp src\desktops.cpp src\tiling.cpp src\files.cpp src\profiles.cpp src\text.cpp ^
    src\timetable.cpp src\settings.cpp src\subjects.cpp src\panel_model.cpp src\background.cpp src\panel.cpp ^
    crammy.res -o crammy.exe ^
    -lcomctl32 -lcomdlg32 -ldwmapi -lole32 -luuid -lshell32 || exit /b 1
del crammy.res
echo Built crammy.exe
