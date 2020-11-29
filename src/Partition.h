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

    int get_cube1(Mesh &mesh, double x, double y, double z);

    int localize_triangle1(Mesh &mesh, const std::vector<Vector3> &input_vertices, Vector3i triangle);

    bool check_tets1(std::vector<Vector3> points, std::vector<MeshTet> new_tets, Mesh &mesh);
}


#endif // FLOATTETWILD_PARTITION_H