#ifndef _PHYSICS_H_
#define _PHYSICS_H_

#include "math.h"
#include "types.h"
#include "collision_types.h"
#include "collision_testing.h"

#include "phys_arbiter.h"
#include <cstdint>

#include "imgui/imgui.h"

#define INSTANCE_SIZE 4

struct sPhysicsWorld {
  // Shared transforms
  sTransform  *transforms;

  sPhysArbiter arbiter = {};

  float     mass                  [INSTANCE_SIZE] = {0.0f};
  float     friction              [INSTANCE_SIZE] = {0.0f};
  float     restitution           [INSTANCE_SIZE] = {0.0f};
  bool      is_static             [INSTANCE_SIZE] = {false};
  sVector3  mass_center           [INSTANCE_SIZE] = {{0.0f}};
  sVector3  speed                 [INSTANCE_SIZE] = {{}};
  sVector3  angular_speed         [INSTANCE_SIZE] = {{}};
  sMat33    inv_inertia_tensors   [INSTANCE_SIZE] = {};

  void config_simulation();


  void step(const double elapsed_time);


  void generate_inertia_tensors();

  void apply_gravity(const double elapsed_time);

  // Integrate the speeds to the position & rotation
  void integrate (const double elapsed_time);

  void pre_step(const double elapsed_time) {
    for(int i = 0; i < MAX_ARBITERS_SIZE; i++) {
      if (!arbiter.used_in_frame[i]) {
        continue;
      }

      uint16_t ref_id = arbiter.reference_ids[i];
      uint16_t inc_id = arbiter.incident_ids[i];

      // Get the mass centers in worldspace
      sVector3 mass_center_ref = {}, mass_center_inc = {};

      mass_center_ref = transforms[ref_id].apply(mass_center[ref_id]);
      mass_center_inc = transforms[inc_id].apply(mass_center[inc_id]);


      sPhysContactData *contact = arbiter.contact_data[i];
      for(int j = 0; j < arbiter.contact_size[i]; j++) {
        sVector3 ref_contact_cross_normal = cross_prod(contact[j].contanct_point.subs(mass_center_ref),
                                                       arbiter.separating_axis[i]);
        sVector3 inc_contact_cross_normal = cross_prod(contact[j].contanct_point.subs(mass_center_inc),
                                                       arbiter.separating_axis[i]);

        // t1 = (inv_inertia * (contact x normal)) x (contact - mass_Center)
        sVector3 t1 = cross_prod(inv_inertia_tensors[ref_id].multiply(ref_contact_cross_normal), contact[j].contanct_point.subs(mass_center_ref));
        sVector3 t2 = cross_prod(inv_inertia_tensors[inc_id].multiply(inc_contact_cross_normal), contact[j].contanct_point.subs(mass_center_inc));

        // Calculate the normal impulse mass
        float normal_mass = (mass[ref_id] != 0.0f) ? 1.0f / mass[ref_id] : 0.0f;
        normal_mass += (mass[inc_id] != 0.0f) ? 1.0f / mass[inc_id] : 0.0f;
        normal_mass += dot_prod(t1.sum(t2), arbiter.separating_axis[i]);

        contact[j].normal_mass = 1.0f / normal_mass;

        // Calcilate de bias impulse
        // 0.2 is teh bias factor and 0.01 is the penetration tollerance
        contact[j].impulse_bias = -0.2f * (1.0f / elapsed_time) * MIN(0.0f, contact[j].distance + 0.01f);

        // TODO: accolulate impulses..?
      }
    }
  }

  void apply_impulses(double elapsed_time) {
    for(int i = 0; i < MAX_ARBITERS_SIZE; i++) {
      if (!arbiter.used_in_frame[i]) {
        continue;
      }
      uint16_t ref_id = arbiter.reference_ids[i];
      uint16_t inc_id = arbiter.incident_ids[i];

      // Get the mass centers in worldspace
      sVector3 mass_center_ref = {}, mass_center_inc = {};

      mass_center_ref = transforms[ref_id].apply(mass_center[ref_id]);
      mass_center_inc = transforms[inc_id].apply(mass_center[inc_id]);

      sPhysContactData *contact = arbiter.contact_data[i];
      for(int j = 0; j < arbiter.contact_size[i]; j++) {
        // Compute relative velocity
        sVector3 ref_contact_center = contact[j].contanct_point.subs(mass_center_ref);
        sVector3 inc_contact_center = contact[j].contanct_point.subs(mass_center_inc);

        sVector3 ref_contactd_speed = cross_prod(angular_speed[ref_id], ref_contact_center).sum(speed[ref_id]);
        sVector3 inc_contactd_speed = cross_prod(angular_speed[inc_id], inc_contact_center).sum(speed[inc_id]);

        // separating axis is the collision normal
        float relative_normal_speed = dot_prod(arbiter.separating_axis[i], ref_contactd_speed.subs(inc_contactd_speed));

        float force_normal_impulse = contact[j].normal_mass * (-relative_normal_speed + contact[j].impulse_bias);

        force_normal_impulse = MAX(force_normal_impulse, 0.0f);

        sVector3 normal_impulse = arbiter.separating_axis[i].mult(force_normal_impulse);

        add_impulse(ref_id, ref_contact_center, normal_impulse);
        add_impulse(inc_id, inc_contact_center, normal_impulse.invert());
      }
    }
  }

  inline void add_impulse(const int       index,
                          const sVector3 &position,
                          const sVector3 &impulse) {
    sVector3 tmp = speed[index];
    float inv_mass = (is_static[index]) ? 0.0f : 1.0f / mass[index];
    tmp.x += inv_mass * impulse.x;
    tmp.y += inv_mass * impulse.y;
    tmp.z += inv_mass * impulse.z;

    speed[index] = tmp;

    angular_speed[index] = angular_speed[index].sum(inv_inertia_tensors[index].multiply(cross_prod(position, impulse)));
  }

  void resolve_collision(const sCollisionManifold &manifold,
                         const double             elapsed_time) {
    // Impulse resolution ===
    int obj1 = manifold.obj1_index, obj2 = manifold. obj2_index; 

    float inv_mass1 = (is_static[obj1]) ? 0.0f : 1.0f / mass[obj1];
    float inv_mass2 = (is_static[obj2]) ? 0.0f : 1.0f / mass[obj2];

    // Get the mass centers in worldspace
    sVector3 mass_center_obj1 = {}, mass_center_obj2 = {};

    mass_center_obj1 = transforms[obj1].apply(mass_center[obj1]);
    mass_center_obj2 = transforms[obj2].apply(mass_center[obj2]);

    // Bounciness of the materials
    float col_restitution = MIN(restitution[obj1], restitution[obj2]); 

    // Compute an impulse for each contact point
    // using the formula https://www.euclideanspace.com/physics/dynamics/collision/index.htm
    float max_depth = FLT_MAX;
    // Added 3 iterations
    for(int p = 0; p < 3; p++) {
    for(int i = 0; i < manifold.contact_point_count; i++) {
      sVector3 point_center_1 = manifold.contact_points[i].subs(mass_center_obj1);
      sVector3 point_center_2 = manifold.contact_points[i].subs(mass_center_obj2);
      
      sVector3 contact_speed_1 = cross_prod(angular_speed[obj1], point_center_1).sum(speed[obj1]);
      sVector3 contact_speed_2 = cross_prod(angular_speed[obj2], point_center_2).sum(speed[obj2]);

      float relative_speed_among_normal = dot_prod(manifold.collision_normal,
                                                   { contact_speed_1.x - contact_speed_2.x,
                                                    contact_speed_1.y - contact_speed_2.y, 
                                                    contact_speed_1.z - contact_speed_2.z}); 



      sVector3 point_normal_cross1 = cross_prod(point_center_1, manifold.collision_normal);
      sVector3 point_normal_cross2 = cross_prod(point_center_2, manifold.collision_normal);
      
      // The impulse force is the relative speed divided by the sum of the inverse masses
      std::cout << -manifold.points_depth[i] << std::endl;
      float impulse_force_common = -(1.0f + col_restitution) * relative_speed_among_normal;//+ (0.3f/elapsed_time  * MAX(-manifold.points_depth[i] - 0.02f, 0.0f));
      float to_divide = inv_mass1 + inv_mass2;

      sVector3 t1 = cross_prod(inv_inertia_tensors[obj1].multiply(point_normal_cross1), point_center_1);
      sVector3 t2 = cross_prod(inv_inertia_tensors[obj2].multiply(point_normal_cross2), point_center_2);
      to_divide += dot_prod(t1.sum(t2), manifold.collision_normal);

      float impulse_force_complete = MAX((impulse_force_common) / to_divide, 0.0f);// + (-manifold.points_depth[i] * 0.7f);
      //impulse_force_common /= manifold.contact_point_count;

      sVector3 impulse = manifold.collision_normal.mult(impulse_force_complete);

      add_impulse(obj1, point_center_1, impulse);
      add_impulse(obj2, point_center_2, impulse.invert());

      angular_speed[obj1] = angular_speed[obj1].sum(inv_inertia_tensors[obj1].multiply(cross_prod(point_center_1, impulse)));
      angular_speed[obj2] = angular_speed[obj2].sum(inv_inertia_tensors[obj2].multiply(cross_prod(point_center_2, impulse.invert())));

      // Second impulse to solve the depth
      float depth_impulse_force = 0.001f * MAX(-manifold.points_depth[i] - 0.01f, 0.0f) / elapsed_time;// / (elapsed_time * to_divide);
      sVector3 depth_impulse = manifold.collision_normal.mult(depth_impulse_force);

      //add_impulse(obj1, depth_impulse);
      //add_impulse(obj2, depth_impulse.invert());

      // Penetration correction
      // TODO: More stable, via baumgarte or add a non penetration impulse
      float penetration = 0.2 * MAX(-manifold.points_depth[i] - 0.001f, 0.0f) / (inv_mass1 + inv_mass2);
      sVector3 correction = manifold.collision_normal.mult(penetration);

      //transforms[obj1].position = transforms[obj1].position.sum(correction.mult(inv_mass1));
      //transforms[obj2].position = transforms[obj2].position.subs(correction.mult(inv_mass2));
    }
    }
  }
};

#endif
