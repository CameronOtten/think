This project is meant to be a learning experience for myself and others and serves as an example of the difference between making a game and making a game engine. There is quite a bit of custom stuff that does really just the minimal thing to work and is meant to be that way. This is not architected to be an engine to make any arbitrary game but in the end will just be a game. There is stuff in here that can be used in an engine like the glTF import code or the rendering as well. The style that this code is written in may not be for everyone but is a good way to structure your code to make it easy to refactor as you are working out the details of how something works.

To build (only in debug atm)
	1)install visual studio community 2019
	2)run build.bat (just double click it or run from command line)
	3)find the build in builds/Windows(Debug) or just use run.bat for a shortcut
 
	
There is still more I would like to fix and like to refactor or restructure entirely so hopefully I can find the time to do so! If, for whatever reason, you would like to use this code, feel free, just be aware of the 3rd party libraries used.


Game Hotkeys:
	-J toggles on the collider view
	-K toggles on the spherecast debugging
	-O toggles free flying camera
	-P toggles the shadowmap debug viewer
	-[] speeds up or slows down the timestep
	-R resets the player position

