## :alien: Alien Swarm SDK
This project was created to simplify the creation of your modifications and games on the Alien Swarm engine (Source Engine: Alien Swarm engine branch). Access to the Alien Swarm source code is provided here. Please note that some of the projects available here are ports from leaked CSGO sources (VideoCFG, Interfaces and MeshUtils), therefore, if you are planning to release a game on Steam based on this SDK, then you need to delete these projects.

Pull requests are welcome!

## :chart_with_upwards_trend: Features
- Support HL2 and Episodic (except Alien Swarm)
- Ability to compile shader binaries (game_shader_dx9.dll with hlsl shaders).
- Shared ASW gameui for all client projects

## :pushpin: TODO
- Restore blobulator surface render code
- Add Portal, TF2, CSS and DOD projects
- Make video backgrounds are shared 
- Add SDK template (or no?)

## :hammer: How to build
1. Download Visual Studio 2010 (or VS2010 build tools with VS2022)
2. Open everything.bat (or games.bat) and wait for .sln file creation
3. Open .sln file and compile solution
4. Wait for compile and run mod