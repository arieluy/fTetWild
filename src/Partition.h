#ifndef FLOATTETWILD_PARTITION_H
#define FLOATTETWILD_PARTITION_H

#include <floattetwild/Mesh.hpp>
#include <floattetwild/AABBWrapper.h>
#include <floattetwild/CutMesh.h>

#ifdef FLOAT_TETWILD_USE_TBB
#include <tbb/concurrent_vector.h>
#endif

#include <floattetwild/Rational.h>

namespace floatTetWild {

    int get_cube(Mesh &mesh, MeshVertex &vert);

    int localize_triangle(Mesh &mesh, const std::vector<Vector3> &input_vertices, Vector3i triangle);

    bool check_tets(std::vector<MeshVertex> &points, std::vector<MeshTet> &new_tets, Mesh &mesh);

    void offset_new_tets(std::vector<MeshTet> &new_tets, int old_size, int new_size);

    void compute_oct_tree(std::vector<floatTetWild::MeshVertex> &verts, Vector3 bottom_left, Vector3 top_right);

    int compute_octant(Vector3 point, Vector3 base);
}


#endif // FLOATTETWILD_PARTITION_H