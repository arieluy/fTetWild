#include <floattetwild/Partition.h>

#include <floattetwild/LocalOperations.h>
#include <floattetwild/Predicates.hpp>
#include <floattetwild/MeshIO.hpp>
#include <floattetwild/auto_table.hpp>
#include <floattetwild/Logger.hpp>
#include <floattetwild/intersections.h>
#include <floattetwild/Partition.h>

#include <floattetwild/MeshImprovement.h>//fortest

#include <igl/writeSTL.h>
#include <igl/writeOFF.h>
#include <igl/Timer.h>

#ifdef FLOAT_TETWILD_USE_TBB
#include <tbb/task_scheduler_init.h>
#include <tbb/parallel_for.h>
#include <tbb/atomic.h>
//#include <floattetwild/FloatTetCuttingParallel.h>
#include <tbb/concurrent_queue.h>
#endif

#include <bitset>
#include <numeric>
#include <unordered_map>

//NEW!
int floatTetWild::get_cube(floatTetWild::Mesh &mesh, double x,double y,double z){
    double min_x = mesh.params.bbox_min[0];
    double min_y = mesh.params.bbox_min[1];
    double min_z = mesh.params.bbox_min[2];

    double centered_x = x - min_x;
    double centered_y = y - min_y;
    double centered_z = z-min_z;

    int index_x = (int)(centered_x/mesh.params.part_width[0]);
    int index_y = (int)(centered_y/mesh.params.part_width[1]);
    int index_z = (int)(centered_z/mesh.params.part_width[2]);

    int index = index_x + index_y*mesh.params.blocks_dim[0] + index_z*mesh.params.blocks_dim[0]*mesh.params.blocks_dim[1]; 
    return index;
}

//NEW!
//Returns index of localization or -1 for none
int floatTetWild::localize_triangle(floatTetWild::Mesh &mesh,const std::vector<floatTetWild::Vector3> &input_vertices, floatTetWild::Vector3i triangle){
    int c1 = get_cube(mesh,input_vertices[triangle[0]][0],input_vertices[triangle[0]][1],input_vertices[triangle[0]][2]);
    int c2 = get_cube(mesh,input_vertices[triangle[1]][0],input_vertices[triangle[1]][1],input_vertices[triangle[1]][2]);
    int c3 = get_cube(mesh,input_vertices[triangle[2]][0],input_vertices[triangle[2]][1],input_vertices[triangle[2]][2]);

    if(c1 == c2 && c2 == c3){
        return c1;
    }
    return -1;
}

//NEW!
//Putting namespace before function name indexes into namespace, but needs to be declared in namespace
bool floatTetWild::check_tets(std::vector<floatTetWild::Vector3> points, std::vector<floatTetWild::MeshTet> new_tets, floatTetWild::Mesh &mesh){

    auto &tets = new_tets;
    auto &tet_vertices = mesh.tet_vertices;

    //Will just compute cubes dynamically
    for(int i = 0; i < new_tets.size();++i){

        if (tets[i].cube_index >= 0) {
          continue;
        }

        int c1 = get_cube(mesh,points[tets[i][0]][0],points[tets[i][0]][1],points[tets[i][0]][2]);
        int c2 = get_cube(mesh,points[tets[i][1]][0],points[tets[i][1]][1],points[tets[i][1]][2]);
        int c3 = get_cube(mesh,points[tets[i][2]][0],points[tets[i][2]][1],points[tets[i][2]][2]);
        int c4 = get_cube(mesh,points[tets[i][3]][0],points[tets[i][3]][1],points[tets[i][3]][2]);

        if(c1 == c2 && c3 == c4 && c1 == c3){
            tets[i].cube_index = c1;
            tets[i].is_in_cube = true;
        }
    }

    for(int i = 0; i < new_tets.size()-1;++i){
        auto tet = new_tets[i];
        if(!tet.is_in_cube || tet.cube_index != new_tets[i+1].cube_index)
            return false;
    }
    return true;
}
