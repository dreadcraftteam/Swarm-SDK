@echo off
cls
pushd %~dp0
	devtools\bin\vpc.exe /SCRATCH /SWARM +shaders /mksln shaders.sln /2010
popd
@pause