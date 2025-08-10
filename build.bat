@echo off
setlocal

:: Set project directories
set PROJECT_ROOT=%~dp0
set SRC_DIR=%PROJECT_ROOT%src
set BUILD_DIR=%PROJECT_ROOT%build
set THIRDPARTY_DIR=%PROJECT_ROOT%thirdparty

:: Set SDL3 paths
set SDL3_DIR=%THIRDPARTY_DIR%\SDL3
set SDL3_BUILD_DIR=%SDL3_DIR%\build
set SDL3_INCLUDE_DIR=%SDL3_DIR%\include
set SDL3_LIB_DIR=%SDL3_BUILD_DIR%

:: Set GLAD paths
set GLAD_DIR=%THIRDPARTY_DIR%\GLAD
set GLAD_INCLUDE_DIR=%GLAD_DIR%\include
set GLAD_SRC_DIR=%GLAD_DIR%\src

:: Create build directory if it doesn't exist
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

:: Check if SDL3 is built
if not exist "%SDL3_LIB_DIR%\RelWithDebInfo\SDL3.lib" (
    echo SDL3 library not found. Building SDL3...
    pushd "%SDL3_DIR%"
    
    :: Create build directory for SDL3 if it doesn't exist
    if not exist "build" mkdir build
    pushd build
    
    :: Configure and build SDL3 with CMake
    cmake .. -G "Visual Studio 17 2022" -A x64 ^
        -DCMAKE_BUILD_TYPE=RelWithDebInfo ^
        -DSDL_STATIC=OFF ^
        -DSDL_SHARED=ON
    
    if errorlevel 1 (
        echo Failed to configure SDL3
        popd
        popd
        exit /b 1
    )
    
    cmake --build . --config RelWithDebInfo
    
    if errorlevel 1 (
        echo Failed to build SDL3
        popd
        popd
        exit /b 1
    )
    
    popd
    popd
    echo SDL3 built successfully
)

:: Compile GLAD as C first
if not exist "%BUILD_DIR%\glad.o" (
    echo Compiling GLAD...
    clang -c -I"%GLAD_INCLUDE_DIR%" ^
        -o "%BUILD_DIR%\glad.o" ^
        "%GLAD_SRC_DIR%\glad.c" ^
        -MD

    if errorlevel 1 (
        echo GLAD compilation failed
        exit /b 1
    )
)

:: Compile the application
echo Compiling application...

clang++ -std=c++23 ^
    -g ^
    -Wall ^
    -Wextra ^
    -Wpedantic ^
    -Wno-c23-extensions ^
    -Wno-language-extension-token ^
    -I"%GLAD_INCLUDE_DIR%" ^
    -I"%SDL3_INCLUDE_DIR%" ^
    -L"%SDL3_LIB_DIR%\RelWithDebInfo" ^
    -o "%BUILD_DIR%\main.exe" ^
    "%SRC_DIR%\main.cpp" ^
    "%BUILD_DIR%\glad.o" ^
    -Wl,/SUBSYSTEM:WINDOWS ^
    -lSDL3 ^
    -lopengl32 ^
    -lkernel32 ^
    -luser32 ^
    -lgdi32 ^
    -MD

if errorlevel 1 (
    echo Compilation failed
    exit /b 1
)

:: Copy SDL3.dll to build directory
copy "%SDL3_LIB_DIR%\RelWithDebInfo\SDL3.dll" "%BUILD_DIR%\"

echo Build completed successfully!
echo Executable: %BUILD_DIR%\main.exe

endlocal
