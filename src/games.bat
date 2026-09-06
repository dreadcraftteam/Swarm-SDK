@echo off
cls
pushd %~dp0
	devtools\bin\vpc.exe /EPISODIC /HL2 /SWARM +game /mksln games.sln /2010
popd
@pause