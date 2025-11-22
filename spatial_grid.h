#ifndef SPATIAL_GRID_H
#define SPATIAL_GRID_H

#include "particle.h"

#define MAX_PARTICLES_PER_CELL 64
#define MAX_NEIGHBORS 10

typedef struct {
  int *particle_indices;
  int *cell_starts;
  int *cell_counts;
  int grid_width;
  int grid_height;
  int grid_depth;
  float cell_size;
  int total_cells;
  int num_particles;
} SpatialGrid;

SpatialGrid *spatial_grid_create(float world_width, float world_height,
                                 float world_depth, float cell_size,
                                 int max_particles);
void spatial_grid_destroy(SpatialGrid *grid);
void spatial_grid_update(SpatialGrid *grid, Particle *particles, int count);
void spatial_grid_query_neighbors(SpatialGrid *grid, Particle *particles,
                                  const Vector3D *position, float radius,
                                  int **neighbors_out, int *neighbor_count_out);

#endif
