Assignment #3: Ray tracing

FULL NAME: Baihua Yang
USC ID: 3281934045

Platform: Windows 11, Visual Studio 2022.
Language: ISO C++17 Standard.

Usage: 
	Run the program and follow the prompts. By default, the ray reflects at most 3 times, soft shadows and multithreading are always enabled.

MANDATORY FEATURES
------------------
Feature:                                 Status: finish? (yes/no)
-------------------------------------    -------------------------
1) Ray tracing triangles                  yes

2) Ray tracing sphere                     yes

3) Triangle Phong Shading                 yes

4) Sphere Phong Shading                   yes

5) Shadows rays                           yes

6) Still images                           yes
   
7) Extra Credit (up to 30 points)
	Recursive reflection
	Good antialiasing
	Soft shadows
	Multithreading
	Monte-Carlo sampling

Result:
	Phong:
		phong/SIGGRAPH.scene   -> 000.jpg
		phong/snow.scene       -> 001.jpg
		phong/spheres.scene    -> 002.jpg
		phong/table.scene      -> 003.jpg
		phong/test2.scene      -> 004.jpg
		phong/toy.scene        -> 005.jpg

	Monte-carlo:
		optical/test2.scene    -> 006.jpg
		optical/SIGGRAPH.scene -> 007.jpg
		optical/snow.scene     -> 008.jpg
		optical/spheres.scene  -> 009.jpg
