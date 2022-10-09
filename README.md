# PKO-ENGINE
VULKAN RENDERER

## REQUIREMENTS
- Currently only targets Windows x64 devices with modern vulkan drivers.
- Vulkan SDK version at least 1.2 (SDK must be added to system PATH).

### User Interface Description 

- Rotate Camera with Arrow Keys
	* ← , → (NUMPAD 4, 6): Modify Yaw 
	* ↑ , ↓ (NUMPAD 2, 8): Modify Pitch

	* R: Reset camera
	* +, -: Zoom In/Out

### Implemented

vector / quaternion / VQS / interpolation function(lerp, slerp, elerp)
bone hierarchy / animation using VQS 

path: src\math
path: src\core\renderer\animation (\skeleton_node / VQS)
path: src\core\renderer\vulkan_renderer\vulkan_skinned_mesh

### Machine Tested
- OS : Windows 10 x64
- GPU Vendor : ATI Technologies Inc
- Vulkan Renderer : AMD Ryzen 7 4800HS with Radeon Graphics
- Triple buffering

### How To Use
- From GUI, select the skeleton model and animation

