#include "particle.h"
#include <stdlib.h>
#include <math.h>

Particle particle_create(float x, float y, float z) {
    Particle p;
    p.position = vec3_create(x, y, z);
    p.velocity = vec3_random3d();
    vec3_mult(&p.velocity, 2.0f);
    p.acceleration = vec3_create(0, 0, 0);
    p.max_speed = 4.0f;
    p.max_force = 0.1f;
    return p;
}

Vector3D particle_separate(Particle* p, Particle* particles, int count, float perception_radius) {
    Vector3D steering = vec3_create(0, 0, 0);
    int total = 0;

    for (int i = 0; i < count; i++) {
        Particle* other = &particles[i];
        float d = vec3_dist(&p->position, &other->position);

        if (other != p && d < perception_radius) {
            Vector3D diff = vec3_sub_new(&p->position, &other->position);
            vec3_div(&diff, d * d);
            vec3_add(&steering, &diff);
            total++;
        }
    }

    if (total > 0) {
        vec3_div(&steering, (float)total);
        vec3_set_mag(&steering, p->max_speed);
        vec3_sub(&steering, &p->velocity);
        vec3_limit(&steering, p->max_force);
    }

    return steering;
}

Vector3D particle_align(Particle* p, Particle* particles, int count, float perception_radius) {
    Vector3D steering = vec3_create(0, 0, 0);
    int total = 0;

    for (int i = 0; i < count; i++) {
        Particle* other = &particles[i];
        float d = vec3_dist(&p->position, &other->position);

        if (other != p && d < perception_radius) {
            vec3_add(&steering, &other->velocity);
            total++;
        }
    }

    if (total > 0) {
        vec3_div(&steering, (float)total);
        vec3_set_mag(&steering, p->max_speed);
        vec3_sub(&steering, &p->velocity);
        vec3_limit(&steering, p->max_force);
    }

    return steering;
}

Vector3D particle_cohesion(Particle* p, Particle* particles, int count, float perception_radius) {
    Vector3D steering = vec3_create(0, 0, 0);
    int total = 0;

    for (int i = 0; i < count; i++) {
        Particle* other = &particles[i];
        float d = vec3_dist(&p->position, &other->position);

        if (other != p && d < perception_radius) {
            vec3_add(&steering, &other->position);
            total++;
        }
    }

    if (total > 0) {
        vec3_div(&steering, (float)total);
        vec3_sub(&steering, &p->position);
        vec3_set_mag(&steering, p->max_speed);
        vec3_sub(&steering, &p->velocity);
        vec3_limit(&steering, p->max_force);
    }

    return steering;
}

Vector3D particle_seek(Particle* p, const Vector3D* target) {
    Vector3D desired = vec3_sub_new(target, &p->position);
    vec3_set_mag(&desired, p->max_speed);
    Vector3D steer = vec3_sub_new(&desired, &p->velocity);
    vec3_limit(&steer, p->max_force);
    return steer;
}

void particle_flock(Particle* p, Particle* particles, int count,
                   const Vector3D* leader1, const Vector3D* leader2,
                   float perception_radius, float separation_weight) {
    Vector3D separation = particle_separate(p, particles, count, perception_radius);
    Vector3D alignment = particle_align(p, particles, count, perception_radius);
    Vector3D cohesion = particle_cohesion(p, particles, count, perception_radius);

    float dist1 = vec3_dist(&p->position, leader1);
    float dist2 = vec3_dist(&p->position, leader2);
    const Vector3D* closest_leader = (dist1 < dist2) ? leader1 : leader2;
    Vector3D leader_attraction = particle_seek(p, closest_leader);

    vec3_mult(&separation, separation_weight);
    vec3_mult(&alignment, 1.0f);
    vec3_mult(&cohesion, 1.0f);
    vec3_mult(&leader_attraction, 0.5f);

    vec3_add(&p->acceleration, &separation);
    vec3_add(&p->acceleration, &alignment);
    vec3_add(&p->acceleration, &cohesion);
    vec3_add(&p->acceleration, &leader_attraction);
}

void particle_update(Particle* p) {
    vec3_add(&p->velocity, &p->acceleration);
    vec3_limit(&p->velocity, p->max_speed);
    vec3_add(&p->position, &p->velocity);
    vec3_mult(&p->acceleration, 0.0f);
}

void particle_flock_optimized(Particle* p, int particle_idx, Particle* particles,
                              int* neighbors, int neighbor_count,
                              const Vector3D* leader1, const Vector3D* leader2,
                              float perception_radius, float separation_weight) {
    Vector3D separation = vec3_create(0, 0, 0);
    Vector3D alignment = vec3_create(0, 0, 0);
    Vector3D cohesion = vec3_create(0, 0, 0);
    int sep_count = 0, align_count = 0, coh_count = 0;

    float perception_radius_sq = perception_radius * perception_radius;

    for (int i = 0; i < neighbor_count; i++) {
        int neighbor_idx = neighbors[i];
        if (neighbor_idx == particle_idx) continue;

        Particle* other = &particles[neighbor_idx];
        float dist_sq = vec3_dist_sq_inline(&p->position, &other->position);

        if (dist_sq < perception_radius_sq && dist_sq > 0.001f) {
            Vector3D diff = vec3_sub_new(&p->position, &other->position);
            vec3_div(&diff, dist_sq);
            vec3_add(&separation, &diff);
            sep_count++;

            vec3_add(&alignment, &other->velocity);
            align_count++;

            vec3_add(&cohesion, &other->position);
            coh_count++;
        }
    }

    if (sep_count > 0) {
        vec3_div(&separation, (float)sep_count);
        vec3_set_mag(&separation, p->max_speed);
        vec3_sub(&separation, &p->velocity);
        vec3_limit(&separation, p->max_force);
        vec3_mult(&separation, separation_weight);
    }

    if (align_count > 0) {
        vec3_div(&alignment, (float)align_count);
        vec3_set_mag(&alignment, p->max_speed);
        vec3_sub(&alignment, &p->velocity);
        vec3_limit(&alignment, p->max_force);
    }

    if (coh_count > 0) {
        vec3_div(&cohesion, (float)coh_count);
        vec3_sub(&cohesion, &p->position);
        vec3_set_mag(&cohesion, p->max_speed);
        vec3_sub(&cohesion, &p->velocity);
        vec3_limit(&cohesion, p->max_force);
    }

    float dist1_sq = vec3_dist_sq_inline(&p->position, leader1);
    float dist2_sq = vec3_dist_sq_inline(&p->position, leader2);
    const Vector3D* closest_leader = (dist1_sq < dist2_sq) ? leader1 : leader2;
    Vector3D leader_attraction = particle_seek(p, closest_leader);
    vec3_mult(&leader_attraction, 0.5f);

    vec3_add(&p->acceleration, &separation);
    vec3_add(&p->acceleration, &alignment);
    vec3_add(&p->acceleration, &cohesion);
    vec3_add(&p->acceleration, &leader_attraction);
}

void particle_wrap(Particle* p, float width, float height, float depth) {
    if (p->position.x > width) p->position.x = 0.0f;
    if (p->position.x < 0.0f) p->position.x = width;
    if (p->position.y > height) p->position.y = 0.0f;
    if (p->position.y < 0.0f) p->position.y = height;
    if (p->position.z > depth) p->position.z = 0.0f;
    if (p->position.z < 0.0f) p->position.z = depth;
}
