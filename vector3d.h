#ifndef VECTOR3D_H
#define VECTOR3D_H

#include <math.h>

typedef struct {
  float x;
  float y;
  float z;
} Vector3D;

Vector3D vec3_create(float x, float y, float z);
void vec3_add(Vector3D *v1, const Vector3D *v2);
void vec3_sub(Vector3D *v1, const Vector3D *v2);
void vec3_mult(Vector3D *v, float n);
void vec3_div(Vector3D *v, float n);
float vec3_mag(const Vector3D *v);
float vec3_mag_sq(const Vector3D *v);
void vec3_normalize(Vector3D *v);
void vec3_limit(Vector3D *v, float max);
void vec3_set_mag(Vector3D *v, float mag);
float vec3_dist(const Vector3D *v1, const Vector3D *v2);
float vec3_dist_sq(const Vector3D *v1, const Vector3D *v2);
Vector3D vec3_copy(const Vector3D *v);
Vector3D vec3_sub_new(const Vector3D *v1, const Vector3D *v2);
Vector3D vec3_random3d(void);

static inline float vec3_mag_sq_inline(const Vector3D *v) {
    return v->x * v->x + v->y * v->y + v->z * v->z;
}

static inline float vec3_dist_sq_inline(const Vector3D *v1, const Vector3D *v2) {
    float dx = v1->x - v2->x;
    float dy = v1->y - v2->y;
    float dz = v1->z - v2->z;
    return dx * dx + dy * dy + dz * dz;
}

#endif
