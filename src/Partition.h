#ifndef FLOATTETWILD_PARTITION_H
#define FLOATTETWILD_PARTITION_H

#include <floattetwild/Mesh.hpp>
#include <floattetwild/AABBWrapper.h>
#include <floattetwild/CutMesh.h>

#ifdef FLOAT_TETWILD_USE_TBB
#include <tbb/concurrent_vector.h>
#endif

#define MAX_DEPTH 3

#include <floattetwild/Rational.h>

namespace floatTetWild {



    int get_cube(Mesh &mesh, MeshVertex &vert);

    int localize_triangle(Mesh &mesh, Vector3i triangle);

    bool check_tets(std::vector<MeshVertex> &points, std::vector<MeshTet> &new_tets, Mesh &mesh);

    bool check_tets_oct(std::vector<MeshVertex> &points, std::vector<MeshTet> &new_tets, Mesh &mesh);

    void offset_new_tets(std::vector<MeshTet> &new_tets, int old_size, int new_size);

    void compute_oct_tree(std::vector<floatTetWild::MeshVertex> &verts,std::vector<int> &indices, Vector3 bottom_left, Vector3 top_right,int threshold,int depth,int index);

    int compute_octant(Vector3 point, Vector3 base);

    int cube_index_to_progression(int index);

    int get_partition_size();

    int progression_to_cube_index(int index);
}


#endif // FLOATTETWILD_PARTITION_H