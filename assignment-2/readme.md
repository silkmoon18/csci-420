Author: Baihua Yang

Description: A scene with a roller-coaster.

![image-20230227090924244](C:\Users\ybhba\AppData\Roaming\Typora\typora-user-images\image-20230227090924244.png)

Platform: Windows 11, Visual Studio 2022.
c++ version: ISO C++17 Standard.



Features:
1. Implemented Level 0 to Level 5 requirements.
2. Double rail.
3. Support closed rail path.
4. Additional scene elements: a animated planet model, street lamps, road.
5. Skybox. (Rotates with time)
6. Mimic real world roller-coaster Magic Mountain.
7. Support generating a track from multiple splines. (Not used in the scene)
8. Generate spline using recursive subdivision. (Line length = 0.1 by default)
9. Physically realistic motion.
10. Multiple light sources. (1 directional light, 6 street lamps)
11. Controllable player and a world camera.
12. .sp contains only point positions, no need to include the number of points. track.txt contains only .sp file paths, no need to include the number of .sp files.
13. A rough framework "Utility.h". Details in "Utility.cpp". 

![image-20230227091431313](C:\Users\ybhba\AppData\Roaming\Typora\typora-user-images\image-20230227091431313.png)




Controls:
1. Press w, a, s, d to move player / world camera.
2. Press Spacebar to jump(player) / move upward(world camera).
3. Press c to move downward(world camera).
4. Press e to start a roller-coaster. (Need to get close enough. Distance = 5 by default)
5. Press r to lock / unlock player's view when riding a roller-coaster.
6. Press p to switch between player and world camera.
7. Rotate first-person view with mouse drag.
8. Press 'x' to toggle screenshots recording. 