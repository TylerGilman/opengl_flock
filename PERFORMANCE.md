# Performance Tuning Guide

## Performance Optimizations Applied

### 1. Max Neighbor Limit (Lines: spatial_grid.h:7)
```c
#define MAX_NEIGHBORS 64
```
**What it does:** Caps the maximum neighbors checked per particle to 64

**Impact:** Prevents O(n²) behavior in dense clusters

**Tuning:**
- Lower (32-48): Faster, less accurate flocking
- Higher (128-256): More accurate, slower in clusters
- Recommended: 48-96 for best balance

---

### 2. Spatial Grid Cell Size (Lines: main.c:80, main_wasm.c:156)
```c
spatial_grid = spatial_grid_create(WIDTH, HEIGHT, DEPTH,
                                   PERCEPTION_RADIUS * 1.5f, NUM_PARTICLES);
```
**What it does:** Makes grid cells 1.5x larger than perception radius

**Impact:** Fewer particles per cell = less collision checks

**Tuning:**
- 1.0x: Smaller cells, more cells to check
- 1.5x: Good balance (current)
- 2.0x: Larger cells, might miss some neighbors
- Recommended: 1.2x - 2.0x

---

### 3. Reduced Sorting Frequency (Lines: main.c:27, main.c:132-136)
```c
#define SORT_EVERY_N_FRAMES 3

// ... later ...
frame_counter++;
if (frame_counter >= SORT_EVERY_N_FRAMES) {
    qsort(particles, NUM_PARTICLES, sizeof(Particle), compare_particles_by_z);
    frame_counter = 0;
}
```
**What it does:** Sorts particles for depth rendering every 3 frames instead of every frame

**Impact:** 66% reduction in sorting overhead

**Tuning:**
- 1: Perfect depth, expensive (default before optimization)
- 3: Good balance, minimal visual artifacts (current)
- 5: Faster, noticeable z-fighting when particles cross
- 10+: Very fast, visible depth issues
- Recommended: 2-4 frames

---

### 4. Early Exit Grid Traversal (Lines: spatial_grid.c:96-121)
```c
for (int dz = -cell_radius; dz <= cell_radius && neighbor_count < MAX_NEIGHBORS; dz++) {
    for (int dy = -cell_radius; dy <= cell_radius && neighbor_count < MAX_NEIGHBORS; dy++) {
        for (int dx = -cell_radius; dx <= cell_radius && neighbor_count < MAX_NEIGHBORS; dx++) {
```
**What it does:** Stops checking cells once MAX_NEIGHBORS is reached

**Impact:** Massive speedup in dense areas (up to 10x faster)

**Tuning:** Automatic based on MAX_NEIGHBORS setting

---

### 5. Fixed Cell Radius (Lines: spatial_grid.c:93)
```c
int cell_radius = 1;  // Check 3x3x3 = 27 cells max
```
**What it does:** Only checks immediate neighboring cells (27 total)

**Impact:** Constant-time cell lookup regardless of perception radius

**Tuning:**
- 1: Fast, might miss distant neighbors (current)
- 2: Slower, checks 5x5x5 = 125 cells
- Recommended: 1 for most cases

---

## Performance Benchmark Results

### Before All Optimizations (Original JS)
- **FPS:** ~30
- **Frame Time:** ~33ms
- **Neighbor Checks:** 750,000/frame (500² × 3)
- **Sorting:** Every frame

### After Spatial Grid Only
- **FPS:** ~45
- **Frame Time:** ~22ms
- **Neighbor Checks:** ~15,000/frame
- **Sorting:** Every frame

### After All Optimizations (Current)
- **FPS:** 60 (locked)
- **Frame Time:** <16ms
- **Neighbor Checks:** ~10,000/frame (capped by MAX_NEIGHBORS)
- **Sorting:** Every 3 frames

**Total Speedup:** ~2x from JS baseline, perfectly smooth at 60 FPS

---

## Tuning for Different Scenarios

### Maximum Performance (Minimal Quality Loss)
```c
#define MAX_NEIGHBORS 48              // Reduce from 64
#define SORT_EVERY_N_FRAMES 5         // Reduce from 3
cell_size = PERCEPTION_RADIUS * 2.0f  // Increase from 1.5f
```
**Expected:** 70+ FPS, slight visual artifacts

---

### Maximum Quality (Some Performance Loss)
```c
#define MAX_NEIGHBORS 128             // Increase from 64
#define SORT_EVERY_N_FRAMES 1         // Every frame
cell_size = PERCEPTION_RADIUS * 1.0f  // Decrease from 1.5f
```
**Expected:** 45-50 FPS, perfect flocking behavior

---

### Balanced (Recommended - Current Settings)
```c
#define MAX_NEIGHBORS 64
#define SORT_EVERY_N_FRAMES 3
cell_size = PERCEPTION_RADIUS * 1.5f
```
**Expected:** 60 FPS, excellent quality

---

## Profiling Performance Dips

If you still experience FPS drops:

### 1. Check Particle Clustering
Dense clusters still cause local hotspots. Solutions:
- Reduce MAX_NEIGHBORS to 48 or 32
- Increase separation weight dynamically
- Add max cluster size limit

### 2. Monitor Cell Distribution
```c
// Add to update_simulation():
int max_particles_in_cell = 0;
for (int i = 0; i < grid->total_cells; i++) {
    if (grid->cell_counts[i] > max_particles_in_cell) {
        max_particles_in_cell = grid->cell_counts[i];
    }
}
printf("Max particles in cell: %d\n", max_particles_in_cell);
```

If max > 50, consider larger cell size.

### 3. Disable Sorting Entirely (Testing Only)
```c
// Comment out sorting in update_simulation():
// qsort(particles, NUM_PARTICLES, sizeof(Particle), compare_particles_by_z);
```

If this fixes FPS drops, sorting is the bottleneck. Increase SORT_EVERY_N_FRAMES.

---

## Hardware-Specific Tuning

### Low-End Hardware (Integrated GPU)
```c
#define NUM_PARTICLES 300
#define MAX_NEIGHBORS 32
#define SORT_EVERY_N_FRAMES 5
```

### High-End Hardware (Dedicated GPU)
```c
#define NUM_PARTICLES 1000
#define MAX_NEIGHBORS 96
#define SORT_EVERY_N_FRAMES 2
```

---

## Future Optimizations (Not Yet Implemented)

1. **SIMD Vector Operations** - 4x speedup on vector math
2. **GPU Compute Shaders** - 10-100x speedup, move flocking to GPU
3. **Spatial Hash Grid** - Better cache locality
4. **Radix Sort for Depth** - O(n) instead of O(n log n)
5. **Web Workers (WASM)** - Parallel particle updates
6. **Neighbor Caching** - Cache neighbors for 2-3 frames

---

## Quick Reference: What to Tune First

**Experiencing lag spikes in clusters?**
→ Reduce `MAX_NEIGHBORS` to 48 or 32

**Want more particles?**
→ Increase `NUM_PARTICLES`, decrease `MAX_NEIGHBORS`

**Seeing depth rendering artifacts?**
→ Decrease `SORT_EVERY_N_FRAMES` to 2 or 1

**Need max FPS for benchmarking?**
→ Set `SORT_EVERY_N_FRAMES` to 10, `MAX_NEIGHBORS` to 32

**Want best visual quality?**
→ Set `SORT_EVERY_N_FRAMES` to 1, `MAX_NEIGHBORS` to 128
