@echo off
cls
pushd %~dp0
	devtools\bin\vpc.exe /SWARM +shaders /mksln shaders.sln /2010
popd
@pause