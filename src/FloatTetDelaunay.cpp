// This file is part of fTetWild, a software for generating tetrahedral meshes.
//
// Copyright (C) 2019 Yixin Hu <yixin.hu@nyu.edu>
// This Source Code Form is subject to the terms of the Mozilla Public License
// v. 2.0. If a copy of the MPL was not distributed with this file, You can
// obtain one at http://mozilla.org/MPL/2.0/.
//

#include <floattetwild/FloatTetDelaunay.h>

#include <floattetwild/Logger.hpp>

#include <igl/Timer.h>

#include <iterator>
#include <algorithm>
#include <bitset>

#include <floattetwild/Predicates.hpp>

#include <floattetwild/LocalOperations.h>
#include <floattetwild/MeshImprovement.h>
#include <floattetwild/MeshIO.hpp>

namespace floatTetWild {
    namespace {
        void
        get_bb_corners(const Parameters &params, const std::vector<Vector3> &vertices, Vector3 &min, Vector3 &max) {
            min = vertices.front();
            max = vertices.front();

            for (size_t j = 0; j < vertices.size(); j++) {
                for (int i = 0; i < 3; i++) {
                    min(i) = std::min(min(i), vertices[j](i));
                    max(i) = std::max(max(i), vertices[j](i));
                }
            }

//            const Scalar dis = std::max((max - min).minCoeff() * params.box_scale, params.eps_input * 2);
            const Scalar dis = std::max(params.ideal_edge_length, params.eps_input * 2);
            for (int j = 0; j < 3; j++) {
                min[j] -= dis;
                max[j] += dis;
            }

            logger().debug("min = {} {} {}", min[0], min[1], min[2]);
            logger().debug("max = {} {} {}", max[0], max[1], max[2]);
        }

        bool comp(const std::array<int, 4> &a, const std::array<int, 4> &b) {
            return std::tuple<int, int, int>(a[0], a[1], a[2]) < std::tuple<int, int, int>(b[0], b[1], b[2]);
        }

        void match_surface_fs(Mesh &mesh, const std::vector<Vector3> &input_vertices,
                              const std::vector<Vector3i> &input_faces, std::vector<bool> &is_face_inserted) {
            std::vector<std::array<int, 4>> input_fs(input_faces.size());
            for (int i = 0; i < input_faces.size(); i++) {
                input_fs[i] = {{input_faces[i][0], input_faces[i][1], input_faces[i][2], i}};
                std::sort(input_fs[i].begin(), input_fs[i].begin() + 3);
            }
            std::sort(input_fs.begin(), input_fs.end(), comp);

//            for(auto& f: input_fs){
//                cout<<f[0]<<" "<<f[1]<<" "<<f[2]<<" "<<f[3]<<endl;
//            }
//            cout<<"/////"<<endl;

            for (auto &t: mesh.tets) {
                for (int j = 0; j < 4; j++) {
                    std::array<int, 3> f = {{t[(j + 1) % 4], t[(j + 2) % 4], t[(j + 3) % 4]}};
                    std::sort(f.begin(), f.end());
//                    cout<<f[0]<<" "<<f[1]<<" "<<f[2]<<endl;
                    auto bounds = std::equal_range(input_fs.begin(), input_fs.end(),
                                                   std::array<int, 4>({{f[0], f[1], f[2], -1}}),
                                                   comp);
//                    bool is_matched = false;
//                    int total_ori = 0;
                    for (auto it = bounds.first; it != bounds.second; ++it) {
//                        is_matched = true;
                        int f_id = (*it)[3];
                        is_face_inserted[f_id] = true;
//                        int ori = Predicates::orient_3d(mesh.tet_vertices[t[j]].pos,
//                                                        input_vertices[input_faces[f_id][0]],
//                                                        input_vertices[input_faces[f_id][1]],
//                                                        input_vertices[input_faces[f_id][2]]);
//                        if (ori == Predicates::ORI_POSITIVE)
//                            total_ori++;
//                        else if (ori == Predicates::ORI_NEGATIVE)
//                            total_ori--;
                    }
//                    if (is_matched)
//                        t.is_surface_fs[j] = total_ori;
//                    else
//                        t.is_surface_fs[j] = NOT_SURFACE;

//                    if(is_matched)
//                        cout<<"matched: "<<total_ori<<endl;
                }
            }
        }

        void match_bbox_fs(Mesh &mesh, const Vector3 &min, const Vector3 &max) {
            auto get_bbox_fs = [&](const MeshTet &t, int j) {
                std::array<int, 6> cnts = {{0, 0, 0, 0, 0, 0}};
                for (int k = 0; k < 3; k++) {
                    Vector3 &pos = mesh.tet_vertices[t[(j + k + 1) % 4]].pos;
                    for (int n = 0; n < 3; n++) {
                        if (pos[n] == min[n])
                            cnts[n * 2]++;
                        else if (pos[n] == max[n])
                            cnts[n * 2 + 1]++;
                    }
                }
                for (int i = 0; i < cnts.size(); i++) {
                    if (cnts[i] == 3)
                        return i;
                }
                return NOT_BBOX;
            };

            for (auto &t: mesh.tets) {
                for (int j = 0; j < 4; j++) {
                    t.is_bbox_fs[j] = get_bbox_fs(t, j);
                }
            }
        }

        void
        compute_voxel_points(const Vector3 &min, const Vector3 &max, const Parameters &params, const AABBWrapper &tree,
                             std::vector<Vector3> &voxels) {
            const Vector3 diag = max - min;
            Vector3i n_voxels = (diag / (params.bbox_diag_length * params.box_scale)).cast<int>();

            for (int d = 0; d < 3; ++d)
                n_voxels(d) = std::max(n_voxels(d), 1);

            const Vector3 delta = diag.array() / n_voxels.array().cast<Scalar>();

            voxels.clear();
            voxels.reserve((n_voxels(0) + 1) * (n_voxels(1) + 1) * (n_voxels(2) + 1));

//            const double sq_distg = std::min(params.ideal_edge_length / 2, 10 * params.eps);
            const double sq_distg = 100 * params.eps_2;
            GEO::vec3 nearest_point;

            for (int i = 0; i <= n_voxels(0); ++i) {
                const Scalar px = (i == n_voxels(0)) ? max(0) : (min(0) + delta(0) * i);
                for (int j = 0; j <= n_voxels(1); ++j) {
                    const Scalar py = (j == n_voxels(1)) ? max(1) : (min(1) + delta(1) * j);
                    for (int k = 0; k <= n_voxels(2); ++k) {
                        const Scalar pz = (k == n_voxels(2)) ? max(2) : (min(2) + delta(2) * k);
                        if (tree.get_sq_dist_to_sf(Vector3(px, py, pz)) > sq_distg)
                            voxels.emplace_back(px, py, pz);
                    }
                }
            }
        }
    }

//#include <igl/unique_rows.h>
//#include <floattetwild/Predicates.hpp>
//    extern "C" floatTetWild::Scalar orient3d(const floatTetWild::Scalar *pa, const floatTetWild::Scalar *pb, const floatTetWild::Scalar *pc, const floatTetWild::Scalar *pd);

//NEW!
#ifdef FLOAT_TETWILD_USE_TBB
int get_cube(Mesh mesh, double x,double y,double z){
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
#endif


//Does this generate the background mesh?
    void FloatTetDelaunay::tetrahedralize(const std::vector<Vector3>& input_vertices, const std::vector<Vector3i>& input_faces, const AABBWrapper &tree,
            Mesh &mesh, std::vector<bool> &is_face_inserted) {
        const Parameters &params = mesh.params;
        auto &tet_vertices = mesh.tet_vertices;
        auto &tets = mesh.tets;

        is_face_inserted.resize(input_faces.size(), false);

        Vector3 min, max;
        //Gets bounding dimensions: sets two Vector3 points as corners of box
        get_bb_corners(params, input_vertices, min, max);
        mesh.params.bbox_min = min;
        mesh.params.bbox_max = max;

        //NEW!
        //Hopefully mesh params same as main params
#ifdef  FLOAT_TETWILD_USE_TBB
        printf("\nBREAKPOINT FloatTetDelaunay: 182\n\n");
        int procs = mesh.params.num_threads;
        //int procs_squared = procs*procs;
        double sq = 1.0*procs;
        Vector3 dims;
        mesh.params.part_width[0] = (max[0]-min[0])/sq;
        mesh.params.part_width[1] = (max[1]-min[1])/sq;
        mesh.params.part_width[2]  = (max[2]-min[2])/sq;
        int blocks_dim = procs < 8 ? procs : std::cbrt(procs) * 16;
        mesh.params.blocks_dim[0] = blocks_dim;
        mesh.params.blocks_dim[1] = blocks_dim;
        mesh.params.blocks_dim[2] = blocks_dim;
        //procs_squared blocks in every dimension
        printf("%f %f %f \n",mesh.params.part_width[0],mesh.params.part_width[1],mesh.params.part_width[2]);
#endif
        //END NEW!

        //No boxpoints?
        std::vector<Vector3> boxpoints; //(8);
        // for (int i = 0; i < 8; i++) {
        //     auto &p = boxpoints[i];
        //     std::bitset<sizeof(int) * 8> flag(i);
        //     for (int j = 0; j < 3; j++) {
        //         if (flag.test(j))
        //             p[j] = max[j];
        //         else
        //             p[j] = min[j];
        //     }
        // }


        //computing gridding of bounding box
        std::vector<Vector3> voxel_points;
        compute_voxel_points(min, max, params, tree, voxel_points);

        const int n_pts = input_vertices.size() + boxpoints.size() + voxel_points.size();
        tet_vertices.resize(n_pts);
//        std::vector<double> V_d;
//        V_d.resize(n_pts * 3);

        size_t index = 0;
        int offset = 0;
        for (int i = 0; i < input_vertices.size(); i++) {
            tet_vertices[offset + i].pos = input_vertices[i];
            // tet_vertices[offset + i].is_on_surface = true;
//            for (int j = 0; j < 3; j++)
//                V_d[index++] = input_vertices[i](j);
        }
        offset += input_vertices.size();
        for (int i = 0; i < boxpoints.size(); i++) {
            tet_vertices[i + offset].pos = boxpoints[i];
            // tet_vertices[i + offset].is_on_bbox = true;
//            for (int j = 0; j < 3; j++)
//                V_d[index++] = boxpoints[i](j);
        }
        offset += boxpoints.size();
        for (int i = 0; i < voxel_points.size(); i++) {
            tet_vertices[i + offset].pos = voxel_points[i];
            // tet_vertices[i + offset].is_on_bbox = false;
//            for (int j = 0; j < 3; j++)
//                V_d[index++] = voxel_points[i](j);
        }

        std::vector<double> V_d;
        V_d.resize(n_pts * 3);
        for (int i = 0; i < tet_vertices.size(); i++) {
            for (int j = 0; j < 3; j++)
                V_d[i * 3 + j] = tet_vertices[i].pos[j];
        }

        GEO::Delaunay::initialize();
        GEO::Delaunay_var T = GEO::Delaunay::create(3, "BDEL");
        T->set_vertices(n_pts, V_d.data());
        //
        //Create delaunay triangulation using tets
        tets.resize(T->nb_cells());
        const auto &tet2v = T->cell_to_v();
        //Iterating through tets
        int in_count = 0;
        for (int i = 0; i < T->nb_cells(); i++) {
            //setting vertices of each tet
            for (int j = 0; j < 4; ++j) {
                const int v_id = tet2v[i * 4 + j];

                tets[i][j] = v_id;
                tet_vertices[v_id].conn_tets.push_back(i);
            }
            std::swap(tets[i][1], tets[i][3]);

            //NEW!
            //Seems to be very slow?
// #ifdef FLOAT_TETWILD_USE_TBB
//             int c1 = get_cube(mesh,tet_vertices[tets[i][0]].pos[0],tet_vertices[tets[i][0]].pos[1],tet_vertices[tets[i][0]].pos[2]);
//             int c2 = get_cube(mesh,tet_vertices[tets[i][1]].pos[0],tet_vertices[tets[i][1]].pos[1],tet_vertices[tets[i][1]].pos[2]);
//             int c3 = get_cube(mesh,tet_vertices[tets[i][2]].pos[0],tet_vertices[tets[i][2]].pos[1],tet_vertices[tets[i][2]].pos[2]);
//             int c4 = get_cube(mesh,tet_vertices[tets[i][3]].pos[0],tet_vertices[tets[i][3]].pos[1],tet_vertices[tets[i][3]].pos[2]);

//             if(c1 == c2 && c3 == c4 && c1 == c3){
//                 tets[i].cube_index = c1;
//                 tets[i].is_in_cube = true;
//                 in_count = in_count+1;
//             }
//             //Surprisingly almost all tets seem clustered in a box
//             //printf("%d\n",c1);
// #endif
        }
        printf("Total Tets: %d \n",tets.size());
        printf("Cubed tets: %d \n\n",in_count);


        for (int i = 0; i < mesh.tets.size(); i++) {
            auto &t = mesh.tets[i];
            //Quit if got inverted Triangulation
            if (is_inverted(mesh.tet_vertices[t[0]].pos, mesh.tet_vertices[t[1]].pos,
                            mesh.tet_vertices[t[2]].pos, mesh.tet_vertices[t[3]].pos)) {
                cout << "EXIT_INV" << endl;
                exit(0);
            }
        }


        match_bbox_fs(mesh, min, max);

//        MeshIO::write_mesh("delaunay.msh", mesh);
    }
}
