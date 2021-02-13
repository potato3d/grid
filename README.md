# grid
GPU-Accelerated Uniform Grid Construction for Ray Tracing

Source code for the article [GPU-Accelerated Uniform Grid Construction for Ray Tracing](http://www.dbd.puc-rio.br/depto_informatica/09_14_ivson.pdf)

# Description

The library includes two implementations of uniform grid construction
* CPU: src/CpuGrid.*
* GPU: src/GpuGrid.* + src/cuda + shaders

The GPU algorithm combines very fast scan and sorting procedures to classify scene primitives according to the spatial subdivision. Since the grid structure can be effciently rebuilt each rendering frame, we can maintain performance with fully animated scenes containing unstructured movements.

The basic idea of the GPU algorithm is to first start a thread per triangle and write to memory, in parallel, which grid cells each triangle overlaps. We write a pair (triangleID, cellID). Then, we rearrange this list so that all triangles belonging to the same cell are contiguous in memory, saving a pair of values per cell (offset to scell start, number of triangles). We use scan and sort procedures to achieve this. Now the structure is ready for GPU-based ray tracing. At rendering time, we trace one ray per GPU thread using a 3D-DDA algorithm. We have two implementations for the ray tracing: GLSL-based and CUDA-based. At the time, the GLSL implementation was about 30% faster.

Please check the paper reference above for more implementation details.

# Examples

These are some images generated with our algorithm for benchmark animated scenes:

![scenes](https://github.com/potato3d/grid/blob/main/imgs/scenes.png "Animated scenes")

These are the results in performance, compared to state of the art:

![speed](https://github.com/potato3d/grid/blob/main/imgs/speed.png "Performance results")

# Build

Visual Studio project files available in prj directory.