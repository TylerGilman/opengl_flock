#define _POSIX_C_SOURCE 199309L
#include <GL/gl.h>
#include <GL/glu.h>
#include <GLFW/glfw3.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "particle.h"
#include "vector3d.h"
#include "spatial_grid.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define WIDTH 800
#define HEIGHT 600
#define DEPTH 600.0f
#define NUM_PARTICLES 500
#define PERCEPTION_RADIUS 50.0f
#define ORBIT_RADIUS 250.0f
#define LEADER_SPEED 0.015f
#define LEADER_INTERPOLATION 0.05f
#define SEPARATION_OSCILLATION_SPEED 0.01f
#define SORT_EVERY_N_FRAMES 0
#define UPDATE_GRID_EVERY_N_FRAMES 1
#define CACHE_NEIGHBORS_FRAMES 2
#define STAGGER_CACHE_UPDATES 1
#define MAX_FRAME_TIME_MS 10.0f
#define ENABLE_PROFILING 1
#define ENABLE_VSYNC 1

typedef struct {
    bool follow_cursor;
    Vector3D mouse_pos;
} AppState;

AppState app_state;
Particle* particles;
SpatialGrid* spatial_grid;
Vector3D leader1, leader2;
Vector3D target_leader1, target_leader2;
float leader_angle = 0.0f;
float separation_time = 0.0f;
int frame_counter = 0;
int grid_update_counter = 0;

typedef struct {
    int neighbors[MAX_NEIGHBORS];
    int count;
    int frame_cached;
} NeighborCache;
NeighborCache* neighbor_cache;
int current_frame = 0;

#if ENABLE_PROFILING
typedef struct {
    double grid_update_time;
    double flocking_time;
    double sorting_time;
    double total_time;
    int frame_count;
} ProfilingData;

ProfilingData profiling = {0};

double get_time_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1000000.0;
}
#endif

int compare_particles_by_z(const void* a, const void* b) {
    const Particle* pa = (const Particle*)a;
    const Particle* pb = (const Particle*)b;
    return (pb->position.z > pa->position.z) - (pb->position.z < pa->position.z);
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    (void)window;
    app_state.mouse_pos.x = (float)xpos;
    app_state.mouse_pos.y = (float)ypos;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action,
                  int mods) {
    (void)scancode;
    (void)mods;
    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
        app_state.follow_cursor = !app_state.follow_cursor;
        printf("Follow cursor: %s\n", app_state.follow_cursor ? "ON" : "OFF");
    }
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

void init_simulation() {
    srand(time(NULL));

    particles = malloc(sizeof(Particle) * NUM_PARTICLES);
    for (int i = 0; i < NUM_PARTICLES; i++) {
        particles[i] = particle_create(
            ((float)rand() / RAND_MAX) * WIDTH,
            ((float)rand() / RAND_MAX) * HEIGHT,
            ((float)rand() / RAND_MAX) * DEPTH);
    }

    spatial_grid = spatial_grid_create(WIDTH, HEIGHT, DEPTH,
                                       PERCEPTION_RADIUS * 2.0f, NUM_PARTICLES);

    neighbor_cache = calloc(NUM_PARTICLES, sizeof(NeighborCache));

    leader1 = vec3_create(WIDTH / 2.0f, HEIGHT / 2.0f, DEPTH / 2.0f);
    leader2 = vec3_create(WIDTH / 2.0f, HEIGHT / 2.0f, DEPTH / 2.0f);
    target_leader1 = leader1;
    target_leader2 = leader2;

    app_state.follow_cursor = false;
    app_state.mouse_pos = vec3_create(WIDTH / 2.0f, HEIGHT / 2.0f, DEPTH / 2.0f);
}

void update_simulation() {
#if ENABLE_PROFILING
    double frame_start = get_time_ms();
    double t1, t2;
#endif

    separation_time += SEPARATION_OSCILLATION_SPEED;
    float separation_weight = 1.1f + sinf(separation_time) * 0.3f;

    if (app_state.follow_cursor) {
        target_leader1 = vec3_copy(&app_state.mouse_pos);
        target_leader2 = vec3_copy(&app_state.mouse_pos);
    } else {
        leader_angle += LEADER_SPEED;
        target_leader1 = vec3_create(
            WIDTH / 2.0f + cosf(leader_angle) * ORBIT_RADIUS,
            HEIGHT / 2.0f + sinf(leader_angle) * ORBIT_RADIUS * 0.7f,
            DEPTH / 2.0f + sinf(leader_angle * 1.5f) * 100.0f);
        target_leader2 = vec3_create(
            WIDTH / 2.0f + cosf(leader_angle + M_PI) * ORBIT_RADIUS,
            HEIGHT / 2.0f + sinf(leader_angle + M_PI) * ORBIT_RADIUS * 0.7f,
            DEPTH / 2.0f + cosf(leader_angle * 1.5f) * 100.0f);
    }

    leader1.x += (target_leader1.x - leader1.x) * LEADER_INTERPOLATION;
    leader1.y += (target_leader1.y - leader1.y) * LEADER_INTERPOLATION;
    leader1.z += (target_leader1.z - leader1.z) * LEADER_INTERPOLATION;
    leader2.x += (target_leader2.x - leader2.x) * LEADER_INTERPOLATION;
    leader2.y += (target_leader2.y - leader2.y) * LEADER_INTERPOLATION;
    leader2.z += (target_leader2.z - leader2.z) * LEADER_INTERPOLATION;

#if ENABLE_PROFILING
    t1 = get_time_ms();
#endif

    if (current_frame % UPDATE_GRID_EVERY_N_FRAMES == 0) {
        spatial_grid_update(spatial_grid, particles, NUM_PARTICLES);
    }

#if ENABLE_PROFILING
    t2 = get_time_ms();
    profiling.grid_update_time += (t2 - t1);
    t1 = t2;
#endif

    int particles_updated = 0;
    for (int i = 0; i < NUM_PARTICLES; i++) {
#if ENABLE_PROFILING
        double elapsed = get_time_ms() - frame_start;
        if (elapsed > MAX_FRAME_TIME_MS && particles_updated > NUM_PARTICLES / 2) {
            break;
        }
#endif

        int* neighbors;
        int neighbor_count;

#if STAGGER_CACHE_UPDATES
        int stagger_group = i % CACHE_NEIGHBORS_FRAMES;
        int should_update = (current_frame % CACHE_NEIGHBORS_FRAMES) == stagger_group;
        int needs_init = (current_frame < CACHE_NEIGHBORS_FRAMES && neighbor_cache[i].count == 0);

        if (should_update || needs_init) {
#else
        if (current_frame - neighbor_cache[i].frame_cached >= CACHE_NEIGHBORS_FRAMES) {
#endif
            spatial_grid_query_neighbors(spatial_grid, particles,
                                          &particles[i].position, PERCEPTION_RADIUS,
                                          &neighbors, &neighbor_count);

            neighbor_cache[i].count = neighbor_count;
            for (int j = 0; j < neighbor_count && j < MAX_NEIGHBORS; j++) {
                neighbor_cache[i].neighbors[j] = neighbors[j];
            }
            neighbor_cache[i].frame_cached = current_frame;
        }

        neighbors = neighbor_cache[i].neighbors;
        neighbor_count = neighbor_cache[i].count;

        particle_flock_optimized(&particles[i], i, particles, neighbors, neighbor_count,
                                &leader1, &leader2, PERCEPTION_RADIUS, separation_weight);
        particle_update(&particles[i]);
        particle_wrap(&particles[i], WIDTH, HEIGHT, DEPTH);
        particles_updated++;
    }

    current_frame++;

#if ENABLE_PROFILING
    t2 = get_time_ms();
    profiling.flocking_time += (t2 - t1);
    t1 = t2;
#endif

#if SORT_EVERY_N_FRAMES > 0
    frame_counter++;
    if (frame_counter >= SORT_EVERY_N_FRAMES) {
        qsort(particles, NUM_PARTICLES, sizeof(Particle), compare_particles_by_z);
        frame_counter = 0;
    }
#endif

#if ENABLE_PROFILING
    t2 = get_time_ms();
    profiling.sorting_time += (t2 - t1);

    profiling.total_time += (t2 - frame_start);
    profiling.frame_count++;

    if (profiling.frame_count >= 120) {
        printf("\n=== Performance Profile (120 frames) ===\n");
        printf("Grid Update:  %.2f ms/frame (%.1f%%)\n",
               profiling.grid_update_time / profiling.frame_count,
               100.0 * profiling.grid_update_time / profiling.total_time);
        printf("Flocking:     %.2f ms/frame (%.1f%%)\n",
               profiling.flocking_time / profiling.frame_count,
               100.0 * profiling.flocking_time / profiling.total_time);
        printf("Sorting:      %.2f ms/frame (%.1f%%)\n",
               profiling.sorting_time / profiling.frame_count,
               100.0 * profiling.sorting_time / profiling.total_time);
        printf("Total:        %.2f ms/frame (%.1f FPS)\n",
               profiling.total_time / profiling.frame_count,
               1000.0 / (profiling.total_time / profiling.frame_count));
        printf("======================================\n\n");

        profiling.grid_update_time = 0;
        profiling.flocking_time = 0;
        profiling.sorting_time = 0;
        profiling.total_time = 0;
        profiling.frame_count = 0;
    }
#endif
}

void render_particle(const Particle* p) {
    float gray = (p->position.z / DEPTH) * 0.863f;
    glColor3f(gray, gray, gray);

    float size = 1.0f + ((DEPTH - p->position.z) / DEPTH) * 2.0f;

    glPointSize(size);
    glBegin(GL_POINTS);
    glVertex2f(p->position.x, p->position.y);
    glEnd();
}

void render() {
    glClear(GL_COLOR_BUFFER_BIT);

    for (int i = 0; i < NUM_PARTICLES; i++) {
        render_particle(&particles[i]);
    }
}

int main() {
    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return -1;
    }

    GLFWwindow* window =
        glfwCreateWindow(WIDTH, HEIGHT, "Flocking Simulation", NULL, NULL);
    if (!window) {
        fprintf(stderr, "Failed to create window\n");
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

#if ENABLE_VSYNC
    glfwSwapInterval(1);
#else
    glfwSwapInterval(0);
#endif

    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetKeyCallback(window, key_callback);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, WIDTH, HEIGHT, 0);
    glMatrixMode(GL_MODELVIEW);

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glEnable(GL_POINT_SMOOTH);
    glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    init_simulation();

    printf("Flocking Simulation\n");
    printf("Press SPACE to toggle cursor following\n");
    printf("Press ESC to exit\n");

    while (!glfwWindowShouldClose(window)) {
        update_simulation();
        render();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    spatial_grid_destroy(spatial_grid);
    free(neighbor_cache);
    free(particles);
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
