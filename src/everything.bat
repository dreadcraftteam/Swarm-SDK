@echo off
cls
pushd %~dp0
	devtools\bin\vpc.exe /EPISODIC /HL2 /SWARM +everything /mksln everything.sln /2010
popd
@pause