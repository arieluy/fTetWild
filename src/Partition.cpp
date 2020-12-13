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
int floatTetWild::get_cube(floatTetWild::Mesh &mesh, floatTetWild::MeshVertex &vert){

    if(vert.local == -2){

        double x = vert[0];
        double y = vert[1];
        double z = vert[2];

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
        vert.local = index;
        return index;
    }
    else{
        return vert.local;
    }
}

//NEW!
//Returns index of localization or -1 for none
int floatTetWild::localize_triangle(floatTetWild::Mesh &mesh,const std::vector<floatTetWild::Vector3> &input_vertices, floatTetWild::Vector3i triangle){
    int c1 = get_cube(mesh,mesh.tet_vertices[triangle[0]]);
    int c2 = get_cube(mesh,mesh.tet_vertices[triangle[1]]);
    int c3 = get_cube(mesh,mesh.tet_vertices[triangle[2]]);

    if(c1 == c2 && c2 == c3){
        return c1;
    }
    return -1;
}

//NEW!
//Putting namespace before function name indexes into namespace, but needs to be declared in namespace
bool floatTetWild::check_tets(std::vector<floatTetWild::MeshVertex> &points, std::vector<floatTetWild::MeshTet> &new_tets, floatTetWild::Mesh &mesh){

    auto &tets = new_tets;
    //-2 means not initialized, -1 means not localized

    //!!Can speed this up by memoizing cubes: subdivision wont change containment!!
    //Will just compute cubes dynamically
    for(int i = 0; i < new_tets.size();++i){

        //Optimization already done
        if (tets[i].cube_index >= 0) {
          continue;
        }

        int c1 = get_cube(mesh,points[tets[i][0]]);
        int c2 = get_cube(mesh,points[tets[i][1]]);
        int c3 = get_cube(mesh,points[tets[i][2]]);
        int c4 = get_cube(mesh,points[tets[i][3]]);

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

void floatTetWild::offset_new_tets(std::vector<floatTetWild::MeshTet> &new_tets, int old_size, int new_size) {
    int offset = new_size - old_size;
    //printf("Offset: %d\n",offset);
    for(int tet = 0; tet < new_tets.size(); ++tet){
        for(int i = 0; i < 4; ++i){
            if(new_tets[tet][i] >= old_size){
                new_tets[tet][i] += offset;
                //printf("old val: %d, new val: %d\n", new_tets[tet][i]-offset, new_tets[tet][i]);
            }
        }
    }
}

//Report octant of point w.r.t base(origin)
/*int floatTetWild::compute_octant(Vector3 point, Vector3 base){

    int above_x = 0;
    int above_y = 0;
    int above_z = 0;

    if(point.x >= base.x)
        above_x = 1;
    if(point.y >= base.y)
        above_y = 1;
    if(point.z >= base.z)
        above_z = 1;



    return 0;
}*/


void floatTetWild::compute_oct_tree(std::vector<floatTetWild::MeshVertex> &verts,Vector3 bottom_left, Vector3 top_right){



}