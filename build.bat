@echo off
REM Builds docsearch.exe and run_tests.exe with MinGW-w64 gcc (no make needed).
REM Install MinGW-w64 first (see README.md), then double-click this file
REM or run it from a Command Prompt inside the project folder.

where gcc >nul 2>nul
if errorlevel 1 (
    echo gcc was not found. Install MinGW-w64 and add its bin folder to PATH.
    pause
    exit /b 1
)

echo Building docsearch.exe ...
gcc -std=c11 -O2 -Wall -Wextra -pedantic -o docsearch.exe src\*.c -lws2_32 -lm
if errorlevel 1 goto failed

echo Building run_tests.exe ...
gcc -std=c11 -O2 -Wall -Wextra -Isrc -o run_tests.exe src\app.c src\analyzer.c src\bench.c src\config.c src\dict_hash.c src\dict_list.c src\dict_sorted.c src\dictionary.c src\document.c src\eval.c src\index.c src\menu.c src\persist.c src\platform.c src\postings.c src\query.c src\ranking.c src\report.c src\search.c src\snippet.c src\spell.c src\stemmer.c src\util.c src\webgui.c tests\*.c -lws2_32 -lm
if errorlevel 1 goto failed

echo.
echo Done. Run the program with:   docsearch.exe
echo Run the unit tests with:      run_tests.exe
pause
exit /b 0

:failed
echo.
echo BUILD FAILED - see the messages above.
pause
exit /b 1
