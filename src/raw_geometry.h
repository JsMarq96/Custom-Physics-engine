#ifndef RAW_GEOMETRY_H_
#define RAW_GEOMETRY_H_


#include <cstring>
#include <stdlib.h>

#include "math.h"
#include "geometry.h"
#include "transform.h"


struct sRawGeometry {
  sVector3  *raw_points = NULL;
  sVector3  *normals = NULL;
  sPlane    *planes = NULL;

  int face_indexes_size = 0;
  int *face_indexes = NULL;
  int *neighbor_indexes = NULL;
  // TODO: generalize the face indexes and add w,h if needed

  bool is_cube = false;

  unsigned int normal_count = 0; // TODO: Fill normal count
  unsigned int vertices_size = 0;
  unsigned int planes_size = 0;
  unsigned int points_per_plane = 0;
  unsigned int neighbor_faces_per_face = 0;

  void init_cuboid(const sVector3 &scale) {
    // Indexes of each face
    int box_LUT_vertices[6 * 4] = {
      4, 5, 7, 6, // 0
      6, 7, 3, 2, // 1
      1, 3, 7, 5, // 5
      0, 1, 3, 2, // 3
      0, 1, 5, 4, // 4
      0, 2, 6, 4 // 2
    };
    int neighboor_faces_LUT[4 * 6] = {
      4, 5, 2, 1, // Face 0
      0, 3, 2, 5, // 1
      3, 4, 1, 0, // 2
      1, 2, 4, 5, // 3
      3, 2, 5, 0, // 4
      0, 1, 3, 4  // 5
   };

    face_indexes = (int*) malloc(sizeof(int) * 6 * 4);
    memcpy(face_indexes, box_LUT_vertices, sizeof(box_LUT_vertices));
    face_indexes_size = 6 * 4;

    neighbor_indexes = (int*) malloc(sizeof(int) * 6 * 4);
    memcpy(neighbor_indexes, neighboor_faces_LUT, sizeof(int) * 6 * 4);
    neighbor_faces_per_face = 4;

    // Raw pointers
    // TODO: optimize
    sVector3 half_scale_vect = scale.mult(0.5f);
    raw_points = (sVector3*) malloc(sizeof(sVector3) * 8);
    raw_points[0] = sVector3{0.0f, 0.0f, 0.0f}.subs(half_scale_vect);
    raw_points[1] = sVector3{scale.x, 0.0f, 0.0f}.subs(half_scale_vect);
    raw_points[2] = sVector3{0.0f, scale.y, 0.0f}.subs(half_scale_vect);
    raw_points[3] = sVector3{scale.x, scale.y, 0.0f}.subs(half_scale_vect);
    raw_points[4] = sVector3{0.0f, 0.0f, scale.z}.subs(half_scale_vect);
    raw_points[5] = sVector3{scale.x, 0.0f, scale.z}.subs(half_scale_vect);
    raw_points[6] = sVector3{0.0f, scale.y, scale.z}.subs(half_scale_vect);
    raw_points[7] = sVector3{scale.x, scale.y, scale.z}.subs(half_scale_vect);

    // Generate planes
    planes = (sPlane*) malloc(sizeof(sPlane) * 6);
    for(int i = 0; i < 6; i++) {
      sVector3 center = {};

      for(int j = 0; j < 4; j++) {
        sVector3 tmp = raw_points[ box_LUT_vertices[(i * 4) + j] ];
        center.x += tmp.x;
        center.y += tmp.y;
        center.z += tmp.z;
      }

      center.x /= 4.0f;
      center.y /= 4.0f;
      center.z /= 4.0f;

      planes[i].origin_point = center;
    }
    // Plane orientations
    planes[0].normal = sVector3{0.0f, 0.0f, 1.0f};
    planes[1].normal = sVector3{0.0f, 1.0f, 0.0f};
    planes[2].normal = sVector3{1.0f, 0.0f, 0.0f};
    planes[3].normal = sVector3{0.0f, 0.0f, -1.0f};
    planes[4].normal = sVector3{0.0f, -1.0f, 0.0f};
    planes[5].normal = sVector3{-1.0f, 0.0f, 0.0f};

    vertices_size = 8;
    planes_size = 6;
    points_per_plane = 4;
    is_cube = true; // This signalizes to only test 3 axis, instead of all the surface
                    // normals
  };

  void init_cuboid(const sTransform &transform) {
    // Indexes of each face
    int box_LUT_vertices[6 * 4] = {
      4, 5, 7, 6, // 0
      6, 7, 3, 2, // 1
      1, 3, 7, 5, // 5
      0, 1, 3, 2, // 3
      0, 1, 5, 4, // 4
      0, 2, 6, 4 // 2
    };

    int neighboor_faces_LUT[4 * 6] = {
      4, 5, 2, 1, // Face 0
      0, 3, 2, 5, // 1
      3, 4, 1, 0, // 2
      1, 2, 4, 5, // 3
      3, 2, 5, 0, // 4
      0, 1, 3, 4  // 5
   };

    face_indexes = (int*) malloc(sizeof(int) * 6 * 4);
    memcpy(face_indexes, box_LUT_vertices, sizeof(box_LUT_vertices));

    neighbor_indexes = (int*) malloc(sizeof(int) * 6 * 4);
    memcpy(neighbor_indexes, neighboor_faces_LUT, sizeof(int) * 6 * 4);
    neighbor_faces_per_face = 4;

    // Raw pointers
    // TODO: optimize
    raw_points = (sVector3*) malloc(sizeof(sVector3) * 8);
    raw_points[0] = transform.apply(sVector3{0.0f, 0.0f, 0.0f});
    raw_points[1] = transform.apply(sVector3{1.0f, 0.0f, 0.0f});
    raw_points[2] = transform.apply(sVector3{0.0f, 1.0f, 0.0f});
    raw_points[3] = transform.apply(sVector3{1.0f, 1.0f, 0.0f});
    raw_points[4] = transform.apply(sVector3{0.0f, 0.0f, 1.0f});
    raw_points[5] = transform.apply(sVector3{1.0f, 0.0f, 1.0f});
    raw_points[6] = transform.apply(sVector3{0.0f, 1.0f, 1.0f});
    raw_points[7] = transform.apply(sVector3{1.0f, 1.0f, 1.0f});

    // Generate planes
    planes = (sPlane*) malloc(sizeof(sPlane) * 6);
    for(int i = 0; i < 6; i++) {
      sVector3 center = {};

      for(int j = 0; j < 4; j++) {
        sVector3 tmp = raw_points[ box_LUT_vertices[(i * 4) + j] ];
        center.x += tmp.x;
        center.y += tmp.y;
        center.z += tmp.z;
      }

      center.x /= 4.0f;
      center.y /= 4.0f;
      center.z /= 4.0f;

      planes[i].origin_point = center;//transform.apply_without_scale(center);
    }
    // Plane orientations
    planes[0].normal = transform.apply_rotation(sVector3{0.0f, 0.0f, 1.0f});
    planes[1].normal = transform.apply_rotation(sVector3{0.0f, 1.0f, 0.0f});
    planes[2].normal = transform.apply_rotation(sVector3{1.0f, 0.0f, 0.0f});
    planes[3].normal = transform.apply_rotation(sVector3{0.0f, 0.0f, -1.0f});
    planes[4].normal = transform.apply_rotation(sVector3{0.0f, -1.0f, 0.0f});
    planes[5].normal = transform.apply_rotation(sVector3{-1.0f, 0.0f, 0.0f});

    vertices_size = 8;
    planes_size = 6;
    points_per_plane = 4;
    is_cube = true; // This signalizes to only test 3 axis, instead of all the surface
                    // normals
  };


  void duplicate(sRawGeometry *copy_to) const {
    copy_to->is_cube = is_cube;
    copy_to->vertices_size = vertices_size;
    copy_to->planes_size = planes_size;
    copy_to->points_per_plane = points_per_plane;

    copy_to->face_indexes = (int*) malloc(sizeof(int) * planes_size * points_per_plane);
    copy_to->raw_points = (sVector3*) malloc(sizeof(sVector3) * vertices_size);
    copy_to->planes = (sPlane*) malloc(sizeof(sPlane) * planes_size);

    memcpy(copy_to->face_indexes, face_indexes, sizeof(int) * planes_size * points_per_plane);
    memcpy(copy_to->raw_points, raw_points, sizeof(sVector3) * vertices_size);
    memcpy(copy_to->planes, planes, sizeof(sPlane) * planes_size);
  }

  /*
   * Calculates a support point in a given direction
   * A support point is the furcest point in that direction
   * */
  inline sVector3 get_support_point(const sVector3 &direction) const {
    sVector3 support = {};
    float support_proj = -FLT_MAX;

    for(int i = 0; i < vertices_size; i++) {
      float proj = dot_prod(raw_points[i], direction);

      if (proj > support_proj) {
        support = raw_points[i];
        support_proj = proj;
      }
    }

    return support;
  }

  inline sVector3 get_point_of_face(const int face,
                                    const int point) const {
    int index = face_indexes[face * points_per_plane + point];
    return raw_points[index];
  }

  inline sPlane* get_neighboring_plane(const int   current_face,
                                       const int   neighboor) const {
    return &planes[neighbor_indexes[neighboor + (current_face * neighbor_faces_per_face)]];
  }

  inline int     get_neighboring_index(const int   current_face,
                                       const int   neighboor) const {
    return neighbor_indexes[neighboor + (current_face * neighbor_faces_per_face)];
  }


  inline void apply_transform(const sTransform &transf) {
    for(int i = 0; i < vertices_size; i++) {
      raw_points[i] = transf.apply(raw_points[i]);
    }

    for(int i = 0; i < planes_size; i++) {
      planes[i].origin_point = transf.apply_without_scale(planes[i].origin_point);
      planes[i].normal = transf.apply_rotation(planes[i].normal);
    }
  }

  void clean() {
    free(face_indexes);
    free(neighbor_indexes);
    free(raw_points);
    free(planes);

    // TODO: finish, but laterrr
  };
};


#endif // RAW_GEOMETRY_H_
