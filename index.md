## Summary
We are going to further parallelize the fast Tetrahedral Meshing Algorithm on the Bridges supercomputer, using both shared-memory and message-passing parallelism. We aim to maximize speedup and scalability, allowing for much faster mesh computations.

Project Website: <https://auy86.github.io/fTetWild/> <br>
Project Repo: <https://github.com/auy86/fTetWild/>

## Background
The meshing problem is an old one, having deep roots in geometry, computer graphics, and numerical simulation. Given a represenation for a domain, how can we transform this into a well structured mesh which well approximates and discretizes the domain, leading to accurate and stable computation? The Fast Tet Meshing Algorithm (FTMA) presented in 2017 at SGP gives major speedup to the classic Tet Meshing Algorithm: Given a surface in 3D space represented as a triangle soup, i.e. an arbitrary collection of triangles, how can we best mesh the interior volume with tetrahedra? At a high level, the approach can be broken into four parts:
1. Simplify the input triangle soup within an epsilon bound of the surface (existing parallelism)
2. Generate a background mesh and insert triangles iteratively 
3. Improve the quality of the mesh using local operations
4. Filter mesh elements to remove outside surface (existing parallelism)

Steps 1 and 4 already have naive forms of task level parallelism implemented. In both cases indepedent components of a mesh are identified via a graph coloring procedure and then operated on in parallel. We believe the independence detection schemes here can be improved, leading to better speedup. For example in 1. vertices which are close in the triangle soup are compressed into one node. However edges are only contracted if first no other neighboring triangles have vertices being compressed. In some cases this leads to under parallelization where could operate on more nodes in parallel if we had a scheme to resolve contraction conflicts. We seek to implement this.

Our main goal is to parallelize the iterative addition of triangles to the background mesh, as this is a bottleneck of the computation. As will be addressed below, this task is nontrivial as the addition of a triangle locally affects the strucutre of the mesh, hence leading to dependencies.

## Challenges
Adding a triangle to the background mesh changes the local tetrahedral connectivity. Furthermore, there is no clear locality, since the triangles in the triangle soup are not ordered in any way. In order to implement a good parallelization, we aim to map partitions of the polygon soup to neighborhoods of the tetrahedral mesh, parallelizing over each partition. This potentially addresses the dependency issue and may also improve locality if we ensure partitioned triangles are close in memory.

As always, things are not quite this simple, as there may not exist a perfect partitioning of the triangle soup to neighborhoods of the tetrahedral mesh. I.e. in any partition, there will be overlaps across the corresponding borders of the tetrahedral mesh neighborhoods. Thus, we also need to identify these dependencies and correct them iteratively.

## Resources 
We will be using the algorithm found in this paper: <https://cs.nyu.edu/~yixinhu/ftetwild.pdf> <br>
We will additionally be using code from their research as a starting point: <https://github.com/wildmeshing/fTetWild> <br> 
The repo contains open source code for the algorithm as described in the paper, leaving us to focus on its parallelization, which is ideal. The paper also references the Thingi10k mesh dataset which we will be using for testing and comparison.

## Goals
#### Main Goals
- Parallelize the iterative addition of triangles to the background mesh, as this is a bottleneck of the computation
- Achieve good speedup on high processor counts (exact numbers can be added once we have a sense of feasibility)
- Significantly improve runtime of algorithm compared to the non-optimized version presented in the paper
- Maintain robustness/correctness of the computation
- Analyze the quality of meshes produced

#### Stretch Goal
- Demonstrate the applicability of our project to mesh repair or boolean mesh operations.

#### Deliverables
- Images of meshes produced by our implementation
- Graphs of runtime and speedup compared to the FTetMesh code and other meshing algorithms, both the overall program and the sections we targeted for parallelism
- Graphs demonstrating the quality of the meshes we produce

## Platform Choice
We will be using C++ since this is the language of our starter code. C++ is also a good choice as it is fast and has many libraries available. For platform, we will start with the GHC machines with the end goal of using Bridges with OpenMPI to communicate between nodes.

## Schedule
11/8: Finish reading the reference paper and other relevant background reading. Get the starter code running and familiarize ourselves with it. 

11/15: Design 2-3 approaches for partitioning the polygon soup. Implement basic versions of each of them.

11/22: Improve on last week's code, trying out various scheduling approaches and timing code to determine the best approach.

11/29: Identify other places we could implement parallelism and make further improvements. Begin making measurements of execution time on Bridges.

12/6: Create our final writeup and poster, including generating graphs and images.


## References
<https://cs.nyu.edu/~yixinhu/ftetwild.pdf> <br>
<https://github.com/wildmeshing/fTetWild>
