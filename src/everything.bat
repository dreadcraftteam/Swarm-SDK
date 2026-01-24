@echo off
cls
pushd %~dp0
	devtools\bin\vpc.exe /sdk +everything /mksln everything.sln /2013
popd
@pause