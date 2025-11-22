#include "spatial_grid.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

static inline int get_cell_index(SpatialGrid* grid, int x, int y, int z) {
    if (x < 0 || x >= grid->grid_width ||
        y < 0 || y >= grid->grid_height ||
        z < 0 || z >= grid->grid_depth) {
        return -1;
    }
    return x + y * grid->grid_width + z * grid->grid_width * grid->grid_height;
}

static inline void get_grid_coords(SpatialGrid* grid, const Vector3D* pos,
                                   int* x_out, int* y_out, int* z_out) {
    *x_out = (int)(pos->x / grid->cell_size);
    *y_out = (int)(pos->y / grid->cell_size);
    *z_out = (int)(pos->z / grid->cell_size);
}

SpatialGrid* spatial_grid_create(float world_width, float world_height, float world_depth,
                                  float cell_size, int max_particles) {
    SpatialGrid* grid = malloc(sizeof(SpatialGrid));

    grid->cell_size = cell_size;
    grid->grid_width = (int)ceilf(world_width / cell_size);
    grid->grid_height = (int)ceilf(world_height / cell_size);
    grid->grid_depth = (int)ceilf(world_depth / cell_size);
    grid->total_cells = grid->grid_width * grid->grid_height * grid->grid_depth;
    grid->num_particles = max_particles;

    grid->particle_indices = malloc(sizeof(int) * max_particles);
    grid->cell_starts = malloc(sizeof(int) * grid->total_cells);
    grid->cell_counts = malloc(sizeof(int) * grid->total_cells);

    return grid;
}

void spatial_grid_destroy(SpatialGrid* grid) {
    free(grid->particle_indices);
    free(grid->cell_starts);
    free(grid->cell_counts);
    free(grid);
}

void spatial_grid_update(SpatialGrid* grid, Particle* particles, int count) {
    memset(grid->cell_counts, 0, sizeof(int) * grid->total_cells);
    memset(grid->cell_starts, -1, sizeof(int) * grid->total_cells);

    for (int i = 0; i < count; i++) {
        int gx, gy, gz;
        get_grid_coords(grid, &particles[i].position, &gx, &gy, &gz);
        int cell_idx = get_cell_index(grid, gx, gy, gz);

        if (cell_idx >= 0) {
            grid->cell_counts[cell_idx]++;
        }
    }

    int offset = 0;
    for (int i = 0; i < grid->total_cells; i++) {
        if (grid->cell_counts[i] > 0) {
            grid->cell_starts[i] = offset;
            offset += grid->cell_counts[i];
        }
    }

    memset(grid->cell_counts, 0, sizeof(int) * grid->total_cells);

    for (int i = 0; i < count; i++) {
        int gx, gy, gz;
        get_grid_coords(grid, &particles[i].position, &gx, &gy, &gz);
        int cell_idx = get_cell_index(grid, gx, gy, gz);

        if (cell_idx >= 0) {
            int idx = grid->cell_starts[cell_idx] + grid->cell_counts[cell_idx];
            grid->particle_indices[idx] = i;
            grid->cell_counts[cell_idx]++;
        }
    }
}

void spatial_grid_query_neighbors(SpatialGrid* grid, Particle* particles,
                                   const Vector3D* position, float radius,
                                   int** neighbors_out, int* neighbor_count_out) {
    static int neighbor_buffer[MAX_NEIGHBORS];
    int neighbor_count = 0;

    int cx, cy, cz;
    get_grid_coords(grid, position, &cx, &cy, &cz);

    int cell_radius = 1;
    float radius_sq = radius * radius;

    for (int dz = -cell_radius; dz <= cell_radius && neighbor_count < MAX_NEIGHBORS; dz++) {
        for (int dy = -cell_radius; dy <= cell_radius && neighbor_count < MAX_NEIGHBORS; dy++) {
            for (int dx = -cell_radius; dx <= cell_radius && neighbor_count < MAX_NEIGHBORS; dx++) {
                int gx = cx + dx;
                int gy = cy + dy;
                int gz = cz + dz;

                int cell_idx = get_cell_index(grid, gx, gy, gz);
                if (cell_idx < 0) continue;

                int start = grid->cell_starts[cell_idx];
                int count = grid->cell_counts[cell_idx];

                if (start < 0) continue;

                for (int i = 0; i < count && neighbor_count < MAX_NEIGHBORS; i++) {
                    int particle_idx = grid->particle_indices[start + i];
                    float dist_sq = vec3_dist_sq_inline(position, &particles[particle_idx].position);

                    if (dist_sq < radius_sq) {
                        neighbor_buffer[neighbor_count++] = particle_idx;
                    }
                }
            }
        }
    }

    *neighbors_out = neighbor_buffer;
    *neighbor_count_out = neighbor_count;
}
