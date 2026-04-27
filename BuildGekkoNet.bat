@echo off
setlocal

set SCRIPT_DIR=%~dp0
set SUBMODULE_DIR=%SCRIPT_DIR%Source\ThirdParty\GekkoNet\GekkoNet\GekkoLib
set BUILD_DIR=%SUBMODULE_DIR%\build_ue
set OUT_DIR=%SCRIPT_DIR%Source\ThirdParty\GekkoNet\Binaries\Win64

echo [GekkoNetUE] Configuring GekkoNet (static, no docs)...
cmake -S "%SUBMODULE_DIR%" -B "%BUILD_DIR%" ^
    -DBUILD_SHARED_LIBS=OFF ^
    -DNO_ASIO_BUILD=OFF ^
    -DBUILD_DOCS=OFF ^
    -DCMAKE_BUILD_TYPE=Release

echo [GekkoNetUE] Building GekkoNet...
cmake --build "%BUILD_DIR%" --config Release

echo [GekkoNetUE] Copying output to Binaries/Win64...
if not exist "%OUT_DIR%" mkdir "%OUT_DIR%"
copy /Y "%SUBMODULE_DIR%\out\GekkoNet_STATIC.lib" "%OUT_DIR%\"

echo [GekkoNetUE] Done.
endlocal