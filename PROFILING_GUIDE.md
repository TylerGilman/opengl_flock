# Profiling Guide - Finding Performance Bottlenecks

## Current Build Changes

I've added **detailed performance profiling** and **disabled sorting** to isolate the stuttering issue.

### What Changed
```c
#define SORT_EVERY_N_FRAMES 0              // Sorting DISABLED for testing
#define UPDATE_GRID_EVERY_N_FRAMES 1       // Update grid every frame
#define ENABLE_PROFILING 1                 // Print performance stats
```

Also applied from your linter:
```c
#define MAX_NEIGHBORS 32                   // Reduced from 64
```

---

## How to Profile

### Run the simulation:
```bash
./flock
```

### What to watch:
Every **120 frames** (~2 seconds), you'll see output like this in the terminal:

```
=== Performance Profile (120 frames) ===
Grid Update:  2.15 ms/frame (45.2%)
Flocking:     2.42 ms/frame (50.8%)
Sorting:      0.00 ms/frame (0.0%)
Total:        4.76 ms/frame (210.1 FPS)
======================================
```

---

## Interpreting Results

### 1. If Grid Update is highest (>40%)
**Bottleneck:** `spatial_grid_update()` is rebuilding the entire grid every frame

**Solutions:**
- Increase `UPDATE_GRID_EVERY_N_FRAMES` to 2 or 3
- Implement incremental grid updates (only move particles that changed cells)
- Use sparse grid representation

**Test:** Change main.c line 28:
```c
#define UPDATE_GRID_EVERY_N_FRAMES 2  // Update every 2 frames
```

---

### 2. If Flocking is highest (>50%)
**Bottleneck:** Particle behavior calculations (neighbor queries + vector math)

**Solutions:**
- Reduce `MAX_NEIGHBORS` to 24 or 16
- Reduce `NUM_PARTICLES` to 300-400
- Optimize vector operations with SIMD

**Test:** Change spatial_grid.h line 7:
```c
#define MAX_NEIGHBORS 24  // Reduce from 32
```

---

### 3. If Sorting is highest (>20%) - ONLY IF RE-ENABLED
**Bottleneck:** qsort() on 500 particles

**Solutions:**
- Keep `SORT_EVERY_N_FRAMES 0` (disabled) or set to 10+
- Use radix sort (O(n) instead of O(n log n))
- Implement z-binning instead of full sort

**Test:** Keep sorting disabled or use very high interval

---

### 4. If Total frame time has spikes
**Look for patterns:**
- Does it spike when particles cluster?
  → Reduce MAX_NEIGHBORS or increase cell size

- Does it spike periodically?
  → Grid rebuild or sorting causing stutters

- Does it spike randomly?
  → Could be OS/driver, VSync, or garbage collection

---

## Quick Fixes Based on Profiling

### Scenario A: Grid Update is 40-60% of frame time
```c
// main.c line 28
#define UPDATE_GRID_EVERY_N_FRAMES 2  // Update every 2 frames
```
**Expected:** 50% reduction in grid rebuild cost

---

### Scenario B: Flocking is 60-80% of frame time
```c
// spatial_grid.h line 7
#define MAX_NEIGHBORS 16              // Reduce to 16 neighbors
```
**Expected:** 50% reduction in flocking cost

---

### Scenario C: Both Grid + Flocking are expensive
```c
// main.c
#define UPDATE_GRID_EVERY_N_FRAMES 2
#define NUM_PARTICLES 400             // Reduce from 500

// spatial_grid.h
#define MAX_NEIGHBORS 24
```
**Expected:** Smoother performance, slight quality reduction

---

## Advanced Profiling

### Add per-particle timing
If you want to see which particles are slowest, add to update_simulation():

```c
int max_neighbors_found = 0;
for (int i = 0; i < NUM_PARTICLES; i++) {
    // ... existing code ...
    if (neighbor_count > max_neighbors_found) {
        max_neighbors_found = neighbor_count;
    }
}
printf("Max neighbors in frame: %d\n", max_neighbors_found);
```

If this prints 32 often, particles are hitting the MAX_NEIGHBORS cap frequently (good for performance, but might affect quality).

---

### Monitor grid cell distribution
Add after spatial_grid_update():

```c
int max_per_cell = 0;
for (int i = 0; i < spatial_grid->total_cells; i++) {
    if (spatial_grid->cell_counts[i] > max_per_cell) {
        max_per_cell = spatial_grid->cell_counts[i];
    }
}
printf("Max particles per cell: %d\n", max_per_cell);
```

If this prints >50, cells are very dense → increase cell_size or reduce particles.

---

## What To Report Back

Run `./flock` for 10-15 seconds and share:

1. **Profiling output** (the printed stats)
2. **When stutters occur** (random? when clustering? periodically?)
3. **Which metric is highest** (Grid Update, Flocking, or Sorting)

Then I'll implement targeted optimizations based on the actual bottleneck!

---

## Disable Profiling Later

When done profiling, set in main.c:
```c
#define ENABLE_PROFILING 0
```

This will remove all timing overhead.
