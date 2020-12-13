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
#include <math.h>

std::vector<std::tuple<floatTetWild::Vector3,floatTetWild::Vector3,int>> oct_list;

//NEW!
int floatTetWild::get_cube(floatTetWild::Mesh &mesh, floatTetWild::MeshVertex &vert){

    if(vert.local == -2){
        int local=-1;

        //This is painful...
        for(int i = 0; i < oct_list.size();i++){
            floatTetWild::Vector3 lower = std::get<0>(oct_list[i]);
            floatTetWild::Vector3 upper = std::get<1>(oct_list[i]);
            int index = std::get<2>(oct_list[i]);
            bool between_x = (lower[0]<vert[0] && vert[0] < upper[0]) || (upper[0] < vert[0] && vert[0] < lower[0]);
            bool between_y = (lower[1]<vert[1] && vert[1] < upper[1]) || (upper[1] < vert[1] && vert[1] < lower[1]);
            bool between_z = (lower[2]<vert[2] && vert[2] < upper[2]) || (upper[2] < vert[2] && vert[2] < lower[2]);
            if(between_x && between_y && between_z){
                local = index;
                break;
            }
        }

        vert.local = local;
        return vert.local;
    }
    else{
        return vert.local;
    }
}

//NEW!
//Returns index of localization or -1 for none
int floatTetWild::localize_triangle(floatTetWild::Mesh &mesh, floatTetWild::Vector3i triangle){
    int c1 = get_cube(mesh,mesh.tet_vertices[triangle[0]]);
    int c2 = get_cube(mesh,mesh.tet_vertices[triangle[1]]);
    int c3 = get_cube(mesh,mesh.tet_vertices[triangle[2]]);

    if(c1 == c2 && c2 == c3 && c1 != -1){
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

        if(c1 == c2 && c3 == c4 && c1 == c3 && c1 != -1){
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
int floatTetWild::compute_octant(Vector3 point, Vector3 base){
//Bottom left is 0th orthant, top right is 7th orthant
    int above_x = 0;
    int above_y = 0;
    int above_z = 0;

    if(point[0] >= base[0])
        above_x = 1;
    if(point[1] >= base[1])
        above_y = 1;
    if(point[2] >= base[2])
        above_z = 1;

    return above_x+2*above_y+2*2*above_z;
}

//Max depth 9
//Threshold computed based on number of vertices
void floatTetWild::compute_oct_tree(std::vector<floatTetWild::MeshVertex> &verts,std::vector<int> &indices, Vector3 bottom_left, Vector3 top_right,int threshold,int depth,int index){

    if(indices.size() < threshold || depth >= MAX_DEPTH){
        for(int i = 0; i < indices.size(); i++){
            verts[indices[i]].local = index;
        }
        oct_list.push_back(std::make_tuple(bottom_left,top_right,index));
        return;
    }

    Vector3 base;
    base[0] = (bottom_left[0]+top_right[0])/2;
    base[1] = (bottom_left[1]+top_right[1])/2;
    base[2] = (bottom_left[2]+top_right[2])/2;

    std::vector<std::vector<int>> octants(8,std::vector<int>());

    for(int i = 0; i < indices.size(); i++){
        MeshVertex vert = verts[indices[i]];
        int octant = compute_octant(vert.pos,base);
        octants[octant].push_back(indices[i]);
    }

    depth++;

    Vector3 vect;
    vect[0] = base[0] - bottom_left[0];
    vect[1] = base[1] - bottom_left[1];
    vect[2] = base[2] - bottom_left[2];

    for(int i = 0; i < 8; ++i){
        int above_x = i % 2;
        int above_y = (i >> 1) % 2;
        int above_z = (i >> 2) % 2;
        above_x = (2*above_x)-1;
        above_y = (2*above_y)-1;
        above_z = (2*above_z)-1;

        vect[0] *= above_x;
        vect[1] *= above_y;
        vect[2] *= above_z;

        Vector3 pseudo_top_right;
        pseudo_top_right[0] = base[0]+vect[0];
        pseudo_top_right[1] = base[1] + vect[1];
        pseudo_top_right[2] = base[2] + vect[2];

        compute_oct_tree(verts,octants[i],base,vect,threshold,depth,index+(i+1)*(pow(10,MAX_DEPTH-depth)));
    }
}

int floatTetWild::cube_index_to_progression(int index){
    for(int i = 0; i < oct_list.size(); i++){
        if(index == std::get<2>(oct_list[i])){
            return i;
        }
    }
    return -1;
}

int floatTetWild::get_partition_size(){
    return oct_list.size();
}

int floatTetWild::progression_to_cube_index(int index){
    return std::get<2>(oct_list[index]);
}