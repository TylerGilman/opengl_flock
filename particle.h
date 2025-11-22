#ifndef PARTICLE_H
#define PARTICLE_H

#include "vector3d.h"

typedef struct {
    Vector3D position;
    Vector3D velocity;
    Vector3D acceleration;
    float max_speed;
    float max_force;
} Particle;

Particle particle_create(float x, float y, float z);
Vector3D particle_separate(Particle* p, Particle* particles, int count, float perception_radius);
Vector3D particle_align(Particle* p, Particle* particles, int count, float perception_radius);
Vector3D particle_cohesion(Particle* p, Particle* particles, int count, float perception_radius);
Vector3D particle_seek(Particle* p, const Vector3D* target);
void particle_flock(Particle* p, Particle* particles, int count,
                   const Vector3D* leader1, const Vector3D* leader2,
                   float perception_radius, float separation_weight);
void particle_flock_optimized(Particle* p, int particle_idx, Particle* particles,
                              int* neighbors, int neighbor_count,
                              const Vector3D* leader1, const Vector3D* leader2,
                              float perception_radius, float separation_weight);
void particle_update(Particle* p);
void particle_wrap(Particle* p, float width, float height, float depth);

#endif
