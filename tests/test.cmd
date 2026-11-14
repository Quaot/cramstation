@echo off
rem Builds and runs the checks. Nothing appears on screen.
setlocal
cd /d "%~dp0\.."
set PATH=C:\msys64\ucrt64\bin;%PATH%
g++ -std=c++17 -O1 -municode -DUNICODE -D_UNICODE -DNOMINMAX -D_WIN32_WINNT=0x0A00 -Wall ^
    tests\*.cpp src\canvas.cpp src\files.cpp src\profiles.cpp src\text.cpp src\tiling.cpp src\notes_tabs.cpp src\ui.cpp ^
    src\launch.cpp src\desktops.cpp src\timetable.cpp src\settings.cpp src\subjects.cpp src\panel_model.cpp src\background.cpp src\panel.cpp src\main_window.cpp src\dialogs.cpp src\editor_window.cpp ^
    -o tests\crammy_test.exe -lcomctl32 -lcomdlg32 -ldwmapi -lole32 -luuid -lshell32 -lgdi32 || exit /b 1
tests\crammy_test.exe %*
