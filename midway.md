## Update
It turns out the parallel meshing problem is fairly similar to the renderer. We have triangles we seek to insert into a backgroud mesh and so we partition the 
space to parallelize over triangles in a cube. However the problem is harder because the insertion of triangles changes the mesh locally(and globally in some ways)
so we cannot parallelize over triangles too close to each other, as we could with points in the renderer. 

We've implementd a basic paritionining scheme chunking the background mesh in cubes as a function of the number of processors being used. We've also implemented a parallel approach
using the C++ TBB library parallelizing over the cubes. This involves computing whether each tet can be localized to a cube and then committing a computation in parallel iff the involved 
tets are localized to the same cube. However we've run into some unexpected challenges due to the datastructure being used to represent vertcies and tets. Even if an insertion 
can be localized to a cube, it mutates the datastructures in a global way, requiring us to alternative "shallow" copies of tets and vertices for parallel threads. We're still implementing
this. 

The localization method allows us to make several optimizations not included in the original algorithm. For example, we only need to check if a triangle intersects with 
tets in its localized cube, instead of looping over all tets as done originally.

## New Goals
1. Finish implenting parallelization over cubes by addressing global datastructure issues.
2. Test better space partitioning schemes leading to better workload balance. In particular oct trees.
3. Finish implementing optimizations afforded by localization
4. Extend parallel benefits to other operations besides triangle insertion via precomputed localization.

## Revised Scheduling
12/3: Fix remaining issues with cubing partioning and start implementing oct tree.

12/7: Finish oct tree. Start final optimizations afforded by localizing triangles. Try to apply to other operations beside triangle insertion

12/10: Finish other operations and locality optimizations. Start final timings and image generations. Start poster and writeup.

12/14: Finish all timing and image generation. Have poster and writeup finished for poster session.

## Deliverables
- Images of meshes produced by our implementation
- Graphs of runtime and speedup compared to the FTetMesh code and other meshing algorithms, both the overall program and the sections we targeted for parallelism
- Graphs demonstrating the quality of the meshes we produce

## Issues
The global datastructure has been annoying to work with but we think is coming under control.
