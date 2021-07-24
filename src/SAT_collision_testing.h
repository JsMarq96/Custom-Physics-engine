//
// Created by jsmar on 16/06/2021.
//

#ifndef QUEST_DEMO_SAT_COLLISION_TESTING_H
#define QUEST_DEMO_SAT_COLLISION_TESTING_H

#include <float.h>

#include "imgui/imgui.h"

#include "math.h"
#include "geometry.h"

#include "collision_types.h"

// NOTE: In order to cheaply detect if the overlap has been done on,
// say, axis (1,0,0) or axis (-1, 0, 0), the comparison will be done
// relative to collider1
inline bool test_overlap_on_axis(const sVector3  *shape1_vertices, 
                                 const int       shape1_size,
                                 const sVector3  *shape2_vertices,
                                 const int       shape2_size,
                                 const sVector3  &axis,
                                       float     *diff,
                                       bool      *overlap_on_inv_axis) {
  float min_shape1 = FLT_MAX;
  float max_shape1 = -FLT_MAX;
  float min_shape2 = FLT_MAX;
  float max_shape2 = -FLT_MAX;

  // Get the maximun and minimun on the projections
  for(int i = 0; i < shape1_size; i++) {
    float proj = dot_prod(shape1_vertices[i], axis);

    min_shape1 = MIN(min_shape1, proj);
    max_shape1 = MAX(max_shape1, proj);
  }
  for(int i = 0; i < shape2_size; i++) {
    float proj = dot_prod(shape2_vertices[i], axis);

    min_shape2 = MIN(min_shape2, proj);
    max_shape2 = MAX(max_shape2, proj);
  }

  float shape1_len = max_shape1 - min_shape1;
  float shape2_len = max_shape2 - min_shape2;
  float total_shapes_len = MAX(max_shape1, max_shape2) - MIN(min_shape1, min_shape2);

  *diff = (shape1_len + shape2_len) - total_shapes_len;
  
  // Test the direction of the overlap
  if (min_shape1 > min_shape2) {
    *overlap_on_inv_axis = false;
  } else {
    *overlap_on_inv_axis = true;
  }

  return (shape1_len + shape2_len) > total_shapes_len;
}

inline bool SAT_test_OBB_v_OBB(const sBoxCollider &collider1,
                               const sBoxCollider &collider2,
                               sCollisionManifold *manifold) {

  //ImGui::Text("Obj 1 %f %f %f", collider1.vertices[0].x, collider1.vertices[0].y, collider1.vertices[0].z);
  //ImGui::Text("Obj 2 %f %f %f", collider2.vertices[0].x, collider2.vertices[0].y, collider2.vertices[0].z);

  // TODO: only test on 3 axis...? Check box2d or qu3b
  // Get the biggest difference of the overlap
  float collider1_max_diff = -FLT_MAX;
  int collider1_axis = -1;
  bool test_overlap_dir = false;
  for(int i = 0; i < 3; i++) {
    float diff;
    if (!test_overlap_on_axis(collider1.vertices, 8, 
                              collider2.vertices, 8, 
                              collider1.axis[i],
                              &diff,
                              &test_overlap_dir)) {
      return false;
    }
     
    if (diff > collider1_max_diff) {
      collider1_max_diff = diff;
      collider1_axis = i;
      if (!test_overlap_dir) {
        collider1_axis += 3; // Select the inverse axis
      }
    }
  }

  float collider2_max_diff = -FLT_MAX;
  int collider2_axis = -1;
  for(int i = 0; i < 3; i++) {
    float diff;
    if (!test_overlap_on_axis(collider1.vertices, 8, 
                              collider2.vertices, 8, 
                              collider2.axis[i],
                              &diff,
                              &test_overlap_dir)) {
      return false;
    }
     
    if (diff > collider2_max_diff) {
      collider2_max_diff = diff;
      collider2_axis = i;
      if (!test_overlap_dir) {
        collider2_axis += 3;// Select the inverse axis
      }
    }
  }

  // Edge cases
 
  ImGui::Text("Colider 1 diff %f", collider1_max_diff);
  ImGui::Text("Collider 2 diff %f", collider2_max_diff);
  ImGui::Separator();

  // Generate manifold
  // Kinda janky
  sPlane reference_plane = collider1.planes[collider1_axis];

  // Get the indent face of the colider 2
  float most_facing = -FLT_MAX;
  int indent_index = -1;
  for (int i = 0; i < 6; i++) {
    float facing = dot_prod(reference_plane.normal.invert(), collider2.axis[i]);

    if (facing > most_facing) {
      most_facing = facing;
      indent_index = i;
    }
  }

  // Sutherland-Hodgman Clipping
  sVector3 segments_of_face[4][2];

  collider2.get_lines_of_face(indent_index, segments_of_face);

  for(int i = 0; i < 4; i++) {
    sVector3 p1 = segments_of_face[i][0];
    sVector3 p2 = segments_of_face[i][1];
    for(int j = 0; j < 6; j++) {
      collider1.planes[j].clip_segment(&p1, &p2);
    }

    manifold->add_collision_point(p1, 0.0f);
    manifold->add_collision_point(p2, 0.0f);
  }

  manifold->collision_normal = reference_plane.normal;
  manifold->face_obj1 = collider1_axis;
  manifold->face_obj2 = indent_index;

  ImGui::Text("Reference face: %d Indent Face: %d", collider1_axis, indent_index);
  ImGui::Text("Num collision points: %d", manifold->contact_point_count);

  for(int i = 0; i < manifold->contact_point_count; i++) {
    sVector3 *ve = &manifold->contact_points[i];
    manifold->points_depth[i] = reference_plane.distance(*ve);
    ImGui::Text("Point %f %f %f dist %f", ve->x, ve->y, ve->z, manifold->points_depth[i]);
  }

  return true;
}

#endif //QUEST_DEMO_SAT_COLLISION_TESTING_H
