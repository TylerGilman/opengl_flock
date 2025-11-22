#include "vector3d.h"
#include <math.h>
#include <stdlib.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Vector3D vec3_create(float x, float y, float z) {
    Vector3D v = {x, y, z};
    return v;
}

void vec3_add(Vector3D* v1, const Vector3D* v2) {
    v1->x += v2->x;
    v1->y += v2->y;
    v1->z += v2->z;
}

void vec3_sub(Vector3D* v1, const Vector3D* v2) {
    v1->x -= v2->x;
    v1->y -= v2->y;
    v1->z -= v2->z;
}

void vec3_mult(Vector3D* v, float n) {
    v->x *= n;
    v->y *= n;
    v->z *= n;
}

void vec3_div(Vector3D* v, float n) {
    if (n != 0.0f) {
        v->x /= n;
        v->y /= n;
        v->z /= n;
    }
}

float vec3_mag(const Vector3D* v) {
    return sqrtf(v->x * v->x + v->y * v->y + v->z * v->z);
}

float vec3_mag_sq(const Vector3D* v) {
    return v->x * v->x + v->y * v->y + v->z * v->z;
}

void vec3_normalize(Vector3D* v) {
    float m = vec3_mag(v);
    if (m > 0.0f) {
        vec3_div(v, m);
    }
}

void vec3_limit(Vector3D* v, float max) {
    float m = vec3_mag(v);
    if (m > max) {
        vec3_normalize(v);
        vec3_mult(v, max);
    }
}

void vec3_set_mag(Vector3D* v, float mag) {
    vec3_normalize(v);
    vec3_mult(v, mag);
}

float vec3_dist(const Vector3D* v1, const Vector3D* v2) {
    float dx = v1->x - v2->x;
    float dy = v1->y - v2->y;
    float dz = v1->z - v2->z;
    return sqrtf(dx * dx + dy * dy + dz * dz);
}

float vec3_dist_sq(const Vector3D* v1, const Vector3D* v2) {
    float dx = v1->x - v2->x;
    float dy = v1->y - v2->y;
    float dz = v1->z - v2->z;
    return dx * dx + dy * dy + dz * dz;
}

Vector3D vec3_copy(const Vector3D* v) {
    return vec3_create(v->x, v->y, v->z);
}

Vector3D vec3_sub_new(const Vector3D* v1, const Vector3D* v2) {
    return vec3_create(v1->x - v2->x, v1->y - v2->y, v1->z - v2->z);
}

Vector3D vec3_random3d(void) {
    float theta = ((float)rand() / RAND_MAX) * M_PI * 2.0f;
    float phi = ((float)rand() / RAND_MAX) * M_PI;
    return vec3_create(
        sinf(phi) * cosf(theta),
        sinf(phi) * sinf(theta),
        cosf(phi)
    );
}
