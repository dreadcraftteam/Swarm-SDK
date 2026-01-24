@echo off
cls
pushd %~dp0
	devtools\bin\vpc.exe /sdk +shaders /mksln shaders.sln /2013
popd
@pause