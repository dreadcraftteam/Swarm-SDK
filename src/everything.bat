@echo off
cls
pushd %~dp0
	devtools\bin\vpc.exe /SWARM +everything /mksln everything.sln /2010
popd
@pause