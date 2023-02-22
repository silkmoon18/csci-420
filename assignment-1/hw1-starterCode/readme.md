Author: Baihua Yang


Description: Create a heightfield based on given input image.

Platform: Windows 11
c++ version: ISO C++17 Standard

To run the program, specify the path of image directory. For instance: hw1.exe C:\heightmap. If run without specifying, the program will ask for one in terminal.



Features:
1. Render with lighting.

2. Draw with element arrays.

3. Support colors. 

4. Support wireframe on triangles.

5. Switch images in runtime.

6. Use another approach to handle smoothened color:

   Before:<img src="E:\code\csci-420\assignment-1\hw1-starterCode\images\before.png" style="zoom: 25%;" />

   After: <img src="E:\code\csci-420\assignment-1\hw1-starterCode\images\after.png" style="zoom:25%;" />

   This is done by passing the height scalar value into fragment shader and computing the final color.

7. A rough framework.


Controls:
1. Press '1' to render with Points.
2. Press '2' to render with Lines. 
3. Press '3' to render with Triangles.
4. Press '4' to render with Triangles and smoothened height.
5. Press 'o' to switch to previous image.
6. Press 'p' to switch to next image.
7. Press 'l' to toggle wirefram display when render mode is Triangles.
8. Press 'v' to render with defualt lighting. 
9. Press'b' to render with ambient only.
10. Press 'n' to render with diffuse only.
11. Press 'm' to render with specular only.
12. Rotate with mouse drag.
13. Move with Ctrl + mouse drag.
14. Scale with Shift + mouse drag.
15. Press 'x' to toggle screenshots recording (15 screenshots per second under 120 fps). 