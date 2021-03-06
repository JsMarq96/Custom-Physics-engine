#ifndef COLLISION_DETECTION_H_
#define COLLISION_DETECTION_H_

#include "math.h"
#include "geometry.h"
#include "transform.h"
#include "raw_geometry.h"
#include "collider_mesh.h"
#include "vector.h"
#include <cstdint>



// ===========================
// COLLISION METHODS
// ===========================

inline bool test_sphere_sphere_collision(const sVector3  &center1,
                                         const float radius1,
                                         const sVector3 &center2,
                                         const float radius2,
                                         sVector3 *normal,
                                         sVector3 *contact_points,
                                         float *contact_depth,
                                         uint16_t *contanct_points_count) {

    sVector3 center1_to_2 = center1.subs(center2);
    float center_distance = center1_to_2.magnitude();
    float total_radius = radius1 + radius2;

    if (center_distance < total_radius) {
        // The spheres are colliding
        *normal = center1_to_2.normalize().mult(-1.0f);

        contact_points[0] = center1.sum(normal->mult(radius1));
        contact_depth[0] = center_distance - total_radius;
        
        *contanct_points_count = 1;

        return true;
    }
    return false;
}

inline bool test_plane_sphere_collision(const sVector3 &sphere_center,
                                        const float radius,
                                        const sVector3 &plane_origin,
                                        const sVector3 &plane_normal,
                                        sVector3 *normal,
                                        sVector3 *contact_points,
                                        float *contact_depth,
                                        uint16_t *contanct_points_count) {
    // Based arround the signed distance of the plane
    sPlane plane;
    plane.origin_point = plane_origin;
    plane.normal = plane_normal;

    float distance = plane.distance(sphere_center) - radius;

    if (distance < 0.0f) {
        *normal = plane_normal.normalize();
        contact_depth[0] = distance;
        contact_points[0] = plane.project_point(sphere_center).sum(plane_origin.mult(distance));

        *contanct_points_count = 1;

        //std::cout << "====================" << std::endl;
        return true;
    }

    return false;
}

inline bool test_cube_cube_collision(const sTransform &cube1_trasform,
                                     const sRawGeometry &cube1_geometry,
                                     const sTransform &cube2_trasform,
                                     const sRawGeometry &cube2_geometry,
                                     sVector3 *normal,
                                     sVector3 *contact_points,
                                     float *contact_depth,
                                     uint16_t *contanct_points_count);

// NOTE: this works for avery convex shape
inline bool test_cube_sphere_collision(const sTransform &cube_transform,
                                       const sRawGeometry &cube_geometry,
                                       const sVector3 &sphere_center,
                                       const float radius,
                                       sVector3 *normal,
                                       sVector3 *contact_points,
                                       float *contact_depth,
                                       uint16_t *contanct_points_count) {
    int plane = -1;
    float facing = -1000;

    // Select the most facing plane of all of them
    for(int i = 0; i < cube_geometry.planes_size; i++) {
        // the most facing point to the plane
        sVector3 sphere_to_plane_origin = sphere_center.subs(cube_geometry.planes[i].origin_point).normalize();
        sphere_to_plane_origin = sphere_center.sum(sphere_to_plane_origin.mult(radius));

        float curr_facing = dot_prod(sphere_to_plane_origin,
                                     cube_geometry.planes[i].origin_point.sum(cube_geometry.planes[i].normal.mult(radius)));

        // Since it is a cube,
        if (curr_facing > facing) {
            plane = i;
            facing = curr_facing;
        }
    }

    return test_plane_sphere_collision(sphere_center,
                                       radius,
                                       cube_geometry.planes[plane].origin_point,
                                       cube_geometry.planes[plane].normal,
                                       normal,
                                       contact_points,
                                       contact_depth,
                                       contanct_points_count);
}

// NOTE: this works for avery convex shape
inline bool test_cube_sphere_collision(const sTransform &cube_transform,
                                       const sColliderMesh &cube_geometry,
                                       const sVector3 &sphere_center,
                                       const float radius,
                                       sVector3 *normal,
                                       sVector3 *contact_points,
                                       float *contact_depth,
                                       uint16_t *contanct_points_count) {
    return false;
}


// TODO: sphere plane function

#endif // COLLISION_DETECTION_H_
