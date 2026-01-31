@echo off
setlocal

set BUILD_SHADER=call buildshaders.bat

set SOURCE_DIR="..\..\"

set GAME_DIR="..\..\..\build\mutation"

%BUILD_SHADER% stdshader_dx9_20b -game %GAME_DIR% -source %SOURCE_DIR%
%BUILD_SHADER% stdshader_dx9_30  -game %GAME_DIR% -source %SOURCE_DIR% -force30
