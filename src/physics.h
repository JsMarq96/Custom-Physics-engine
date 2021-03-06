#ifndef PHYSICS_H_
#define PHYSICS_H_

#include "imgui/imgui.h"
#include "math.h"
#include "collision_detection.h"
#include "math.h"
#include "collider_mesh.h"
#include "mesh_renderer.h"
#include "phys_parameters.h"
#include "quaternion.h"
#include "sat.h"
#include "transform.h"
#include "types.h"
#include "vector.h"
#include "contact_manager.h"

#include <cstdint>

#define PHYS_INSTANCE_COUNT 100


// TODO:
//  Instead on reinitializing the mesh if its different,
//  just calculate the difference transform and applied it

struct sSpeed {
    sVector3 linear = {0.0f, 0.0f, 0.0f};
    sVector3 angular = {0.0f, 0.0f, 0.0f};
};

enum eParentType : uint8_t {
    NO_PARENT = 0,
    GEOMETRY_PARENT,
    COLLIDER_PARENT,
    PARENT_TYPE_COUNT
};

struct sParenting {
    eParentType  parent_type = NO_PARENT;
    uint32_t parent_index_index = 0;
};


struct sPhysWorld {
    bool               initialized         [PHYS_INSTANCE_COUNT] = {};

    // Collider pareting & state
    sParenting         node_parenting      [PHYS_INSTANCE_COUNT] = {};
    bool               enabled             [PHYS_INSTANCE_COUNT] = {};
    bool               is_static           [PHYS_INSTANCE_COUNT] = {};
    eColiderTypes      shape               [PHYS_INSTANCE_COUNT] = {};

    // Collider data
    sColliderMesh      collider_meshes     [PHYS_INSTANCE_COUNT] = {};
    sTransform         transforms          [PHYS_INSTANCE_COUNT] = {};
    sTransform         old_transforms      [PHYS_INSTANCE_COUNT] = {};
    sSpeed             obj_speeds          [PHYS_INSTANCE_COUNT];

    // Physics properties
    float              mass                [PHYS_INSTANCE_COUNT] = {};
    float              inv_mass            [PHYS_INSTANCE_COUNT] = {};
    float              restitution         [PHYS_INSTANCE_COUNT] = {};
    float              friction            [PHYS_INSTANCE_COUNT] = {};
    sMat33             inv_inertia_tensors [PHYS_INSTANCE_COUNT] = {};

    // Collision & contact data
    sCollisionManager  coll_manager = {};
    int                curr_frame_col_count                      = 0;

    // Collider's Custom information
    // PLANE
    sVector3           plane_collider_normal [PHYS_INSTANCE_COUNT] = {};
    //

    // DEBUG ==============
    sMeshRenderer      debug_capsules_renderers[COLLIDER_COUNT] = {};

    // Helper functions
    inline float get_radius_of_collider(const int id) const {
        return MAX(transforms[id].scale.x, MAX(transforms[id].scale.y, transforms[id].scale.z));
    };

    // Lifecicle functions
    void init() {
        memset(enabled, false, sizeof(sPhysWorld::enabled));
        memset(initialized, false, sizeof(sPhysWorld::initialized));

        coll_manager.init();
        set_default_values();
    }

    void clean() {
        uint32_t index = 0;
        for(; index < PHYS_INSTANCE_COUNT; index++ ) {
            if (initialized[index] && shape[index] == CUBE_COLLIDER) {
                collider_meshes[index].clean();
            }
        }
    }

    void set_default_values() {
        // Set default values
        memset(enabled, false, sizeof(enabled));
        memset(is_static, false, sizeof(is_static));
        memset(obj_speeds, 0.0f, sizeof(obj_speeds));

        memset(plane_collider_normal, 0.0f, sizeof(plane_collider_normal));

        memset(initialized, false, sizeof(initialized));
    }

    inline uint32_t add_cube_collider(const sVector3& obj_position,
                                      const sVector3& obj_scale,
                                      const float obj_mass,
                                      const float restitut,
                                      const bool obj_is_static) {
        uint32_t index = 0;
        for(; index < PHYS_INSTANCE_COUNT; index++ ) {
            if (!initialized[index]) {
                break;
            }
        }

        is_static[index] = obj_is_static;
        enabled[index] = true;

        initialized[index] = true;
        shape[index] = CUBE_COLLIDER;
        restitution[index] = restitut;

        if (obj_is_static) {
            mass[index] = 0.0f;
            inv_mass[index] = 0.0f;
            inv_inertia_tensors[index].set_identity();
        } else {
            mass[index] = obj_mass;
            inv_mass[index] = 1.0f / obj_mass;

            sMat33 inertia_tensor;
            inertia_tensor.set_identity();
            inertia_tensor.mat_values[0][0] = 1.0f/12.0f * obj_mass * (obj_scale.z * obj_scale.z + obj_scale.y * obj_scale.y);
            inertia_tensor.mat_values[1][1] = 1.0f/12.0f * obj_mass * (obj_scale.z * obj_scale.z + obj_scale.x * obj_scale.x);
            inertia_tensor.mat_values[2][2] = 1.0f/12.0f * obj_mass * (obj_scale.x * obj_scale.x + obj_scale.y * obj_scale.y);

            inertia_tensor.invert(&inv_inertia_tensors[index]);
        }

        transforms[index].position = obj_position;
        transforms[index].scale = obj_scale;
        transforms[index].rotation = sQuaternion4{1.0f, 0.0f, 0.0f, 0.f};

        collider_meshes[index].init_cuboid(transforms[index]);
        memcpy(&old_transforms[index], &transforms[index], sizeof(sTransform));

        return index;
    }

    inline uint32_t add_sphere_collider(const sVector3& obj_position,
                                        const float radius,
                                        const float obj_mass,
                                        const float restitut,
                                        const bool obj_is_static) {
        uint32_t index = 0;
        for(; index < PHYS_INSTANCE_COUNT; index++ ) {
            if (!initialized[index]) {
                break;
            }
        }
        initialized[index] = true;
        is_static[index] = obj_is_static;
        enabled[index] = true;
        shape[index] = SPHERE_COLLIDER;
        restitution[index] = restitut;

        if (obj_is_static) {
            mass[index] = 0.0f;
            inv_mass[index] = 0.0f;
            inv_inertia_tensors[index].set_identity();
        } else {
            mass[index] = obj_mass;
            inv_mass[index] = 1.0f / obj_mass;

            sMat33 inertia_tensor;
            inertia_tensor.set_identity();
            inertia_tensor.mat_values[0][0] = 2.0f/5.0f * obj_mass * radius * radius;
            inertia_tensor.mat_values[1][1] = 2.0f/5.0f * obj_mass * radius * radius;
            inertia_tensor.mat_values[2][2] = 2.0f/5.0f * obj_mass * radius * radius;

            inertia_tensor.invert(&inv_inertia_tensors[index]);
        }

        transforms[index].position = obj_position;
        transforms[index].scale = sVector3{radius, radius, radius};

        return index;
    }


    // Apply collisions & speeds, check for collisions, and resolve them
    void step(const double elapsed_time) {
        // 0 - Clean manifolds via the manager
        coll_manager.clean_frame();

        // 1 - Rotate inertia tensors
        for(int i = 0; i < PHYS_INSTANCE_COUNT; i++) {
            sMat33 r_mat = {}, r_mat_t = {}, inv_inertia = {};
            r_mat.convert_quaternion_to_matrix(transforms[i].rotation);
            r_mat.transponse_to(&r_mat_t);

            // Rotate the inertia tensor: I^-1 = r * I^-1 * r^t
            inv_inertia_tensors[i].multiply_to(&r_mat_t, &inv_inertia);
            r_mat.multiply_to(&inv_inertia, &inv_inertia_tensors[i]);
        }

        // 2 - Apply gravity
        apply_gravity(elapsed_time);

        // 3 - Collision Detection
        uint16_t tmp_contanct_point_count = 0;
        sVector3 tmp_contact_points[MAX_CONTACT_COUNT] = {};
        float    tmp_contact_depth[MAX_CONTACT_COUNT] = {};
        sVector3 tmp_contact_normal = {};

        for(uint8_t i = 0; i < PHYS_INSTANCE_COUNT; i++) {
            for(uint8_t j = i+1; j < PHYS_INSTANCE_COUNT; j++) {
                if (!enabled[i] || !enabled[j]) {
                    continue;
                }

                // Skip the test if its between two static bodies
                if (is_static[i] && is_static[j]) {
                    continue;
                }

                uint8_t obj1 = 0;
                uint8_t obj2 = 0;
                bool collided = false;

                // If there is a collision, store it in the manifold array
                // Test the different colliders
                if (shape[i] == SPHERE_COLLIDER && shape[j] == SPHERE_COLLIDER) {
                    if (test_sphere_sphere_collision(transforms[i].position,
                                                     get_radius_of_collider(i),
                                                     transforms[j].position,
                                                     get_radius_of_collider(j),
                                                     &tmp_contact_normal,
                                                     tmp_contact_points,
                                                     tmp_contact_depth,
                                                     &tmp_contanct_point_count)) {
                        obj1 = i;
                        obj2 = j;
                        collided = true;
                    }
                } else if (shape[i] == SPHERE_COLLIDER && shape[j] == CUBE_COLLIDER) {
                    if (!transforms[j].is_equal(old_transforms[j])) {
                        collider_meshes[j].clean();
                        collider_meshes[j].init_cuboid(transforms[j]);
                        memcpy(&old_transforms[j], &transforms[j], sizeof(sTransform));
                    }

                    if (SAT::SAT_sphere_cube_collision(transforms[i].position,
                                                       get_radius_of_collider(i),
                                                       transforms[j],
                                                       collider_meshes[j],
                                                       &tmp_contact_normal,
                                                       tmp_contact_points,
                                                       tmp_contact_depth,
                                                       &tmp_contanct_point_count)) {
                        obj1 = i;
                        obj2 = j;
                        collided = true;
                    }

                } else if (shape[i] == CUBE_COLLIDER && shape[j] == SPHERE_COLLIDER) {
                    if (!transforms[i].is_equal(old_transforms[i])) {
                        collider_meshes[i].clean();
                        collider_meshes[i].init_cuboid(transforms[i]);
                        memcpy(&old_transforms[i], &transforms[i], sizeof(sTransform));
                    }

                    if (SAT::SAT_sphere_cube_collision(transforms[j].position,
                                                       get_radius_of_collider(j),
                                                       transforms[i],
                                                       collider_meshes[i],
                                                       &tmp_contact_normal,
                                                       tmp_contact_points,
                                                       tmp_contact_depth,
                                                       &tmp_contanct_point_count)) {
                        obj1 = i;
                        obj2 = j;
                        collided = true;
                    }

                } else if (shape[i] == CUBE_COLLIDER && shape[j] == CUBE_COLLIDER) {
                    if (!transforms[i].is_equal(old_transforms[i])) {
                        collider_meshes[i].clean();
                        collider_meshes[i].init_cuboid(transforms[i]);
                        memcpy(&old_transforms[i], &transforms[i], sizeof(sTransform));
                    }

                    if (!transforms[j].is_equal(old_transforms[j])) {
                        collider_meshes[j].clean();
                        collider_meshes[j].init_cuboid(transforms[j]);
                        memcpy(&old_transforms[j], &transforms[j], sizeof(sTransform));
                    }
                    //std::cout << i << " " << j << std::endl;

                    if (SAT::SAT_collision_test(collider_meshes[i],
                                                collider_meshes[j],
                                                &tmp_contact_normal,
                                                tmp_contact_points,
                                                tmp_contact_depth,
                                                &tmp_contanct_point_count)) {
                        obj1 = i;
                        obj2 = j;
                        collided = true;
                    }
                }

                if (collided) {
                    std::cout << (uint16_t) shape[i] << " " << (uint16_t) shape[j] << std::endl;
                    coll_manager.renew_contacts_to_collision(obj1,
                                                             obj2,
                                                             tmp_contact_points,
                                                             tmp_contact_depth,
                                                             tmp_contanct_point_count);
                }
            }
        }

        // 4 - Collision Resolution
        // 4.1 - Collision presolving
        int indices[MAX_COLLISION_COUNT] = {};
        int collision_count = 0;
        for(int i = 0; i < MAX_COLLISION_COUNT; i++) {
            if (!coll_manager.has_collided_on_frame[i])
                continue;

            impulse_presolver(coll_manager.manifold[i], elapsed_time);
            indices[collision_count++] = i;
        }

        // 4.2 = Collision Solving via iterations
        for(int iter = 0; iter < PHYS_SOLVER_ITERATIONS; iter++) {
            for(int i = 0; i < collision_count; i++) {
                impulse_response(coll_manager.manifold[indices[i]], elapsed_time);
            }
        }

        // 5 - Integrate solutions
        integrate(elapsed_time);

        curr_frame_col_count = collision_count;
    }

    void debug_speeds() const {
        // Debug speeds
        for(int i = 0; i < PHYS_INSTANCE_COUNT; i++) {
            if (!enabled[i]) {
                continue;
            }
            const sSpeed *speed = &obj_speeds[i];

            char instance_name []= "Obj 00";
            instance_name[5] = 48 + i;

            if(ImGui::TreeNode(instance_name)) {
                ImGui::Text("Position: %f %f %f", transforms[i].position.x, transforms[i].position.y, transforms[i].position.z);
                ImGui::Text("Scale: %f %f %f", transforms[i].scale.x, transforms[i].scale.y, transforms[i].scale.z);
                ImGui::Text("Linear speed: %f %f %f", speed->linear.x, speed->linear.y, speed->linear.z);
                ImGui::Text("Angular speed: %f %f %f", speed->angular.x, speed->angular.y, speed->angular.z);
                ImGui::Text("Angular magnitude %f", speed->angular.magnitude());
                ImGui::TreePop();
            }

        }
        ImGui::Text("Collision num: %i", curr_frame_col_count);
    }

    void render_colliders() const {
        sMat44 collider_mat[COLLIDER_COUNT][10] = {};
        sVector4 colors[10] = {};

        int indexes[COLLIDER_COUNT] = {0,0,0};

        for(int i = 0; i <  PHYS_INSTANCE_COUNT; i++) {
            if (!enabled[i]) {
                continue;
            }
            int collider_shape = shape[i];

            sMat44 *mat = &collider_mat[collider_shape][indexes[collider_shape]];
            mat->set_identity();

            // Add
        }
    };

    // Apply the speeds to the position
    void integrate(const double elapsed_time) {
        for(int i = 0; i < PHYS_INSTANCE_COUNT; i++) {
            if (is_static[i] || !enabled[i]) {
                continue;
            }

            sTransform *transf = &transforms[i];

            // Integrate linear speed: pos += speed * elapsed_time
            transf->position = transf->position.sum(obj_speeds[i].linear.mult(elapsed_time));

            // Integrate angular speed
            sQuaternion4 rotation_incr = obj_speeds[i].angular.get_pure_quaternion();
            rotation_incr = rotation_incr.multiply(0.5).multiply(elapsed_time);

            sQuaternion4 rotation = transf->rotation;
            rotation = rotation.sum(rotation_incr.multiply(rotation));
            rotation = rotation.normalize();

            transf->set_rotation(rotation);

            // Add some energy loss to the system
            obj_speeds[i].linear = obj_speeds[i].linear.mult(0.999f);
            obj_speeds[i].angular = obj_speeds[i].angular.mult(0.999f);
        }
    }

    // Apply the gravity based
    // TODO: Gravioty constant cleanup
    void apply_gravity(const double elapsed_time) {
        for(int i = 0; i < PHYS_INSTANCE_COUNT; i++) {
            if (is_static[i] || !enabled[i]) {
                continue;
            }

            obj_speeds[i].linear.y += -0.98f * elapsed_time;
        }
    }

    // TODO: arbiter & warmstarting
    void impulse_presolver(sCollisionManifold &manifold, const float elapsed_time) {
        uint8_t id_1 = manifold.obj1;
        uint8_t id_2 = manifold.obj2;

        sTransform *transf_1 = &transforms[id_1];
        sTransform *transf_2 = &transforms[id_2];

        sSpeed *speed_1 = &obj_speeds[id_1];
        sSpeed *speed_2 = &obj_speeds[id_2];

        // Calculate impulse response for each contact point
        for(uint8_t i = 0; i < manifold.contact_count ; i++) {
            sContactData *contact_data = &manifold.precompute_data[i];
            // NORMAL IMPULSE ========
            // Vector from the center to the collision point
            contact_data->r1 = manifold.contact_point[i].subs(transf_1->position);
            contact_data->r2 = manifold.contact_point[i].subs(transf_2->position);

            sVector3 r1_cross_n = cross_prod(contact_data->r1, manifold.normal);
            sVector3 r2_cross_n = cross_prod(contact_data->r2, manifold.normal);

            // Calculate the collision momenton, aka the contact speed
            float collision_momentun = dot_prod(speed_1->linear.subs(speed_2->linear), manifold.normal) +
                                       dot_prod(r1_cross_n, speed_1->angular) -
                                       dot_prod(r2_cross_n, speed_2->angular);

            contact_data->linear_mass = inv_mass[id_1] + inv_mass[id_2];
            contact_data->angular_mass = dot_prod(r1_cross_n, inv_inertia_tensors[id_1].multiply(r1_cross_n)) +
                dot_prod(r2_cross_n, inv_inertia_tensors[id_2].multiply(r2_cross_n));

            std::cout <<(uint32_t) id_1 << " " << (uint32_t)id_2 << std::endl;

            // Baumgarte correction for the impulse
            contact_data->bias = -BAUMGARTE_TERM / elapsed_time * MIN(0.0f, manifold.contact_depth[i] + PENETRATION_SLOP);

            // Restitution constant
            contact_data->restitution = MIN(restitution[id_1], restitution[id_2]);

            // Warmstarting
            float impulse_magnitude = (1 + contact_data->restitution) * (collision_momentun + contact_data->bias) / (contact_data->linear_mass + contact_data->angular_mass);

            sVector3 impulse = manifold.normal.mult(impulse_magnitude);

            if (!is_static[id_2]) {
                speed_2->linear = speed_2->linear.sum(impulse.mult(inv_mass[id_2]));
                speed_2->angular = speed_2->angular.sum(inv_inertia_tensors[id_2].multiply(cross_prod(contact_data->r2, impulse)));
            }

            // Invert the impulse for the other body
            impulse = impulse.mult(-1.0f);

            if (!is_static[id_1]) {
                speed_1->linear = speed_1->linear.sum(impulse.mult(inv_mass[id_1]));
                speed_1->angular = speed_1->angular.sum(inv_inertia_tensors[id_1].multiply(cross_prod(contact_data->r1, impulse)));
            }

            // FRICTION IMPULSES =====

            // Calculate the tangent wrenches
            plane_space(manifold.normal, manifold.tangents[0], manifold.tangents[1]);

            for(int tang = 0; tang < 2; tang++) {
                sVector3 r1_cross_t = cross_prod(contact_data->r1, manifold.tangents[tang]);
                sVector3 r2_cross_t = cross_prod(contact_data->r2, manifold.tangents[tang]);

                contact_data->tangental_angular_mass[tang] = dot_prod(r1_cross_t, inv_inertia_tensors[id_1].multiply(r1_cross_t)) +
                    dot_prod(r2_cross_t, inv_inertia_tensors[id_2].multiply(r2_cross_t));
            }
        }
    }

    void impulse_response(const sCollisionManifold &manifold, const float elapsed_time) {
        uint8_t id_1 = manifold.obj1;
        uint8_t id_2 = manifold.obj2;

        sTransform *transf_1 = &transforms[id_1];
        sTransform *transf_2 = &transforms[id_2];

        sSpeed *speed_1 = &obj_speeds[id_1];
        sSpeed *speed_2 = &obj_speeds[id_2];

        // Calculate impulse response for each contact point
        for(int i = 0; i < manifold.contact_count; i++) {
            const sContactData *contact_data = &manifold.precompute_data[i];
            // NORMAL IMPULSE ========
            // Vector from the center to the collision point

            sVector3 r1_cross_n = cross_prod(contact_data->r1, manifold.normal);
            sVector3 r2_cross_n = cross_prod(contact_data->r2, manifold.normal);

            // Calculate the collision momenton, aka the contact speed
            float collision_momentun = dot_prod(speed_1->linear.subs(speed_2->linear), manifold.normal) +
                                       dot_prod(r1_cross_n, speed_1->angular) -
                                       dot_prod(r2_cross_n, speed_2->angular);



            float impulse_magnitude = (1 + contact_data->restitution) * (collision_momentun + contact_data->bias) / (contact_data->linear_mass + contact_data->angular_mass);

            // The impulse cannot be negative
            //std::cout << contact_data->linear_mass + contact_data->angular_mass << std::endl;
            impulse_magnitude = MAX(impulse_magnitude, 0.0f);

            sVector3 impulse = manifold.normal.mult(impulse_magnitude);

            if (!is_static[id_2]) {
                speed_2->linear = speed_2->linear.sum(impulse.mult(inv_mass[id_2]));
                speed_2->angular = speed_2->angular.sum(inv_inertia_tensors[id_2].multiply(cross_prod(contact_data->r2, impulse)));
            }

            // Invert the impulse for the other body
            impulse = impulse.mult(-1.0f);

            if (!is_static[id_1]) {
                speed_1->linear = speed_1->linear.sum(impulse.mult(inv_mass[id_1]));
                speed_1->angular = speed_1->angular.sum(inv_inertia_tensors[id_1].multiply(cross_prod(contact_data->r1, impulse)));
            }

            // FRICTION IMPULSES =====

            float friction_constant = sqrt(friction[id_1] * friction[id_2]);
            float max_friction = friction_constant * impulse_magnitude;

            for(int tang = 0; tang < 2; tang++) {
                sVector3 r1_cross_t = cross_prod(contact_data->r1, manifold.tangents[tang]);
                sVector3 r2_cross_t = cross_prod(contact_data->r2, manifold.tangents[tang]);

                // Calculate the momentun & impulse, but with the tangent wrench instead of the normal
                collision_momentun = dot_prod(speed_1->linear.subs(speed_2->linear), manifold.tangents[tang]) +
                                     dot_prod(r1_cross_t, speed_1->angular) -
                                     dot_prod(r2_cross_t, speed_2->angular);



                float friction_impulse_magnitude = collision_momentun / (contact_data->linear_mass + contact_data->tangental_angular_mass[tang]);

                // Clamp friction
                friction_impulse_magnitude = (friction_impulse_magnitude < -max_friction) ? -max_friction : ((friction_impulse_magnitude > max_friction) ? max_friction : friction_impulse_magnitude);

                sVector3 friction_impulse = manifold.tangents[tang].mult(friction_impulse_magnitude);

                if (!is_static[id_2]) {
                    speed_2->linear = speed_2->linear.sum(friction_impulse.mult(inv_mass[id_2]));
                    speed_2->angular = speed_2->angular.sum(inv_inertia_tensors[id_2].multiply(cross_prod(contact_data->r2, friction_impulse)));
                }

                friction_impulse = friction_impulse.mult(-1.0f);

                if (!is_static[id_1]) {
                    speed_1->linear = speed_1->linear.sum(friction_impulse.mult(inv_mass[id_1]));
                    speed_1->angular = speed_1->angular.sum(inv_inertia_tensors[id_1].multiply(cross_prod(contact_data->r1, friction_impulse)));
                }

            }
        }
    }

    inline void add_collider(const uint32_t transform_id,
                             const eColiderTypes col_shape,
                             const float col_mass,
                             const bool col_is_static) {
        enabled[transform_id] = true;
        shape[transform_id] = col_shape;

    }
};

#endif // PHYSICS_H_
