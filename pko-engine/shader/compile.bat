glslc.exe test.vert -o test.vert.spv
glslc.exe test.frag -o test.frag.spv

rem basic mesh shaders
glslc.exe mesh_basic.vert -o mesh_basic.vert.spv
glslc.exe mesh_basic.frag -o mesh_basic.frag.spv
pause
