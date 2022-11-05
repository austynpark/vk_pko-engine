# PKO-ENGINE
VULKAN RENDERER

## REQUIREMENTS
- Currently only targets Windows x64 devices with modern vulkan drivers.
- Vulkan SDK version at least 1.2 (SDK must be added to system PATH).

### User Interface Description 

- Translate & Rotate Camera with Arrow Keys
	* ← , → (NUMPAD 4, 6): Modify Yaw 
	* ↑ , ↓ (NUMPAD 2, 8): Modify Pitch
	* W,A,S,D to move camera position

	* R: Reset camera
	* +, -: Zoom In/Out

### Implemented

vector / quaternion / VQS / interpolation function(lerp, slerp, elerp)
bone hierarchy / animation using VQS / path-following / bezier curve

path: src\math
path: src\core\renderer\animation (\skeleton_node / VQS)
path: src\core\renderer\vulkan_renderer\vulkan_skinned_mesh
path: src\core\renderer\animation\path_builder (path, curve building functions)

### Algorithm

Project2: 
	1. add control points & velocity-time stamps to path
	2. insert extra control points between existing ones
	3. pre-calculate arc-length between each curve
	4. during the rendering loop, based on 'u' (accumulated delta-time) which is [0: 1], calculate another 'u' that fits into each curves starting u as 0 and end control point of curve as 1.
	5. get position with the 'u', and calculate 'center of interest' to get rotation matrix
	6. based on the position and rotation matrix, build the transform matrix which would eventually move the object along the path


### Machine Tested
- OS : Windows 10 x64
- GPU Vendor : ATI Technologies Inc
- Vulkan Renderer : AMD Ryzen 7 4800HS with Radeon Graphics
- Triple buffering

### How To Use
- From GUI, select the skeleton model and animation

