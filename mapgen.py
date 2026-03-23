#!/usr/bin/env python3
"""
Map generator for assault-htw.
Produces a 1500x1500 map.txt with coherent terrain.

Biome logic:
  - Start with ocean baseline
  - Generate continents using overlapping ellipses + simplex-like noise
  - Assign biomes based on elevation + latitude (distance from equator)
  - Smooth transitions with a cellular automaton pass
  - Natural adjacency rules enforced (no lava next to forest, etc.)

Sector values (from config.h):
  0  = NULL (avoid on surface)
  1  = ROCK
  2  = SAND
  3  = HILLS
  4  = MOUNTAIN
  5  = WATER (inland)
  6  = SNOW
  7  = FIELD
  8  = FOREST
  9  = LAVA
  10 = BURNED
  11 = SNOW_BLIZZARD
  12 = ASH
  13 = AIR (unused on ground)
  14 = UNDERGROUND (unused on ground)
  15 = ICE
  16 = MAGMA
  17 = OCEAN
"""

import random
import math
import sys

SECT_NULL          = 0
SECT_ROCK          = 1
SECT_SAND          = 2
SECT_HILLS         = 3
SECT_MOUNTAIN      = 4
SECT_WATER         = 5
SECT_SNOW          = 6
SECT_FIELD         = 7
SECT_FOREST        = 8
SECT_LAVA          = 9
SECT_BURNED        = 10
SECT_SNOW_BLIZZARD = 11
SECT_ASH           = 12
SECT_ICE           = 15
SECT_MAGMA         = 16
SECT_OCEAN         = 17

SIZE = 1500
random.seed(42)

# ---------------------------------------------------------------------------
# Permutation table for value noise
# ---------------------------------------------------------------------------
_perm = list(range(256))
random.shuffle(_perm)
_perm = _perm * 2

def fade(t):
    return t * t * t * (t * (t * 6 - 15) + 10)

def lerp(a, b, t):
    return a + t * (b - a)

def grad(h, x, y):
    h &= 3
    u = x if h < 2 else y
    v = y if h < 2 else x
    return (u if h & 1 == 0 else -u) + (v if h & 2 == 0 else -v)

def perlin(x, y):
    xi, yi = int(math.floor(x)) & 255, int(math.floor(y)) & 255
    xf, yf = x - math.floor(x), y - math.floor(y)
    u, v = fade(xf), fade(yf)
    aa = _perm[_perm[xi]     + yi]
    ab = _perm[_perm[xi]     + yi + 1]
    ba = _perm[_perm[xi + 1] + yi]
    bb = _perm[_perm[xi + 1] + yi + 1]
    return lerp(
        lerp(grad(aa, xf,     yf    ), grad(ba, xf - 1, yf    ), u),
        lerp(grad(ab, xf,     yf - 1), grad(bb, xf - 1, yf - 1), u),
        v
    )

def octave_noise(x, y, octaves=6, persistence=0.5, lacunarity=2.0, scale=1.0):
    val = 0.0
    amp = 1.0
    freq = scale
    max_val = 0.0
    for _ in range(octaves):
        val += perlin(x * freq, y * freq) * amp
        max_val += amp
        amp *= persistence
        freq *= lacunarity
    return val / max_val

# ---------------------------------------------------------------------------
# Build elevation map
# ---------------------------------------------------------------------------
print("Generating elevation...", flush=True)

elev = [[0.0] * SIZE for _ in range(SIZE)]

# Large-scale continent shape + medium-scale detail for smaller land features
for x in range(SIZE):
    for y in range(SIZE):
        nx = x / SIZE
        ny = y / SIZE
        e = (
            1.00 * octave_noise(nx,       ny,       octaves=6, scale=2.0)  +  # continents
            0.50 * octave_noise(nx + 5.3, ny + 1.7, octaves=4, scale=6.0)  +  # regions
            0.25 * octave_noise(nx + 9.1, ny + 3.4, octaves=3, scale=14.0)    # local hills
        )
        e /= 1.75
        elev[x][y] = e

# Normalise to [0, 1]
flat = [elev[x][y] for x in range(SIZE) for y in range(SIZE)]
lo, hi = min(flat), max(flat)
span = hi - lo
for x in range(SIZE):
    for y in range(SIZE):
        elev[x][y] = (elev[x][y] - lo) / span

# ---------------------------------------------------------------------------
# Build moisture/temperature maps (for biome assignment)
# ---------------------------------------------------------------------------
print("Generating moisture/temperature...", flush=True)

moist = [[0.0] * SIZE for _ in range(SIZE)]
for x in range(SIZE):
    for y in range(SIZE):
        nx = (x + 100) / SIZE
        ny = (y + 200) / SIZE
        m = (
            1.0 * octave_noise(nx,       ny,       octaves=5, scale=6.0)  +
            0.5 * octave_noise(nx + 3.7, ny + 8.2, octaves=3, scale=14.0)
        )
        moist[x][y] = m

mflat = [moist[x][y] for x in range(SIZE) for y in range(SIZE)]
mlo, mhi = min(mflat), max(mflat)
mspan = mhi - mlo
for x in range(SIZE):
    for y in range(SIZE):
        moist[x][y] = (moist[x][y] - mlo) / mspan

# Temperature: latitude baseline + local noise so cold/warm isn't just horizontal bands
_temp_noise = [[0.0] * SIZE for _ in range(SIZE)]
for x in range(SIZE):
    for y in range(SIZE):
        nx = (x + 50) / SIZE
        ny = (y + 300) / SIZE
        _temp_noise[x][y] = octave_noise(nx, ny, octaves=4, scale=8.0)
_tnflat = [_temp_noise[x][y] for x in range(SIZE) for y in range(SIZE)]
_tnlo, _tnhi = min(_tnflat), max(_tnflat)
_tnspan = _tnhi - _tnlo
for x in range(SIZE):
    for y in range(SIZE):
        _temp_noise[x][y] = (_temp_noise[x][y] - _tnlo) / _tnspan

def latitude_temp(x, y):
    dist_from_equator = abs(x - SIZE // 2) / (SIZE // 2)  # 0=equator, 1=pole
    base = 1.0 - dist_from_equator
    # Blend in 30% local noise so temperature varies regionally, not just in bands
    return base * 0.70 + _temp_noise[x][y] * 0.30

# ---------------------------------------------------------------------------
# Assign terrain
# OCEAN_LEVEL: below this elevation = ocean/water
# ---------------------------------------------------------------------------
OCEAN_LEVEL    = 0.42   # ~45% of map is ocean
SHALLOW_LEVEL  = 0.46   # shallow water / beaches
LOWLAND_LEVEL  = 0.58
HIGHLAND_LEVEL = 0.72
MOUNTAIN_LEVEL = 0.82
PEAK_LEVEL     = 0.90

print("Assigning terrain...", flush=True)

grid = [[SECT_OCEAN] * SIZE for _ in range(SIZE)]

for x in range(SIZE):
    for y in range(SIZE):
        temp = latitude_temp(x, y)
        e = elev[x][y]
        m = moist[x][y]

        if e < OCEAN_LEVEL:
            grid[x][y] = SECT_OCEAN
            continue

        if e < SHALLOW_LEVEL:
            # Beach / coastal: sand or water
            if temp < 0.25:
                grid[x][y] = SECT_ICE   # frozen coast
            else:
                grid[x][y] = SECT_SAND
            continue

        # Land — pick biome
        if e >= PEAK_LEVEL:
            # Volcanic peaks near hot equator, snowy otherwise
            if temp > 0.7 and m < 0.4:
                grid[x][y] = SECT_LAVA
            elif temp < 0.35:
                grid[x][y] = SECT_SNOW_BLIZZARD
            else:
                grid[x][y] = SECT_MOUNTAIN
            continue

        if e >= MOUNTAIN_LEVEL:
            if temp < 0.30:
                grid[x][y] = SECT_SNOW
            elif temp < 0.45:
                grid[x][y] = SECT_MOUNTAIN
            else:
                grid[x][y] = SECT_MOUNTAIN
            continue

        if e >= HIGHLAND_LEVEL:
            if temp < 0.25:
                grid[x][y] = SECT_SNOW
            elif temp < 0.40:
                grid[x][y] = SECT_HILLS
            elif m > 0.65:
                grid[x][y] = SECT_FOREST
            else:
                grid[x][y] = SECT_HILLS
            continue

        if e >= LOWLAND_LEVEL:
            if temp < 0.20:
                grid[x][y] = SECT_SNOW
            elif temp < 0.30:
                grid[x][y] = SECT_FIELD   # tundra
            elif m > 0.70:
                grid[x][y] = SECT_FOREST
            elif m > 0.40:
                grid[x][y] = SECT_FIELD
            else:
                # dry lowland: rocky desert or sand
                if e > 0.52:
                    grid[x][y] = SECT_ROCK
                else:
                    grid[x][y] = SECT_SAND
            continue

        # Low land (just above sea level)
        if temp < 0.20:
            grid[x][y] = SECT_ICE
        elif temp < 0.28:
            grid[x][y] = SECT_SNOW
        elif m > 0.72:
            grid[x][y] = SECT_FOREST
        elif m > 0.45:
            grid[x][y] = SECT_FIELD
        elif m < 0.28:
            grid[x][y] = SECT_SAND
        else:
            grid[x][y] = SECT_FIELD

# Inland water: low spots on land surrounded by land become lakes/rivers
print("Adding inland water...", flush=True)
for x in range(1, SIZE - 1):
    for y in range(1, SIZE - 1):
        if grid[x][y] == SECT_OCEAN:
            continue
        e = elev[x][y]
        # Small lakes in low moist areas
        if e < SHALLOW_LEVEL + 0.04 and moist[x][y] > 0.78:
            if (grid[x-1][y] != SECT_OCEAN and grid[x+1][y] != SECT_OCEAN and
                    grid[x][y-1] != SECT_OCEAN and grid[x][y+1] != SECT_OCEAN):
                grid[x][y] = SECT_WATER

# ---------------------------------------------------------------------------
# Add a few volcanic features (isolated, near equator)
# ---------------------------------------------------------------------------
print("Adding volcanic features...", flush=True)
num_volcanoes = 8
for _ in range(num_volcanoes):
    # Volcanoes mostly near the equator (x around SIZE//2)
    cx = random.randint(SIZE // 2 - 250, SIZE // 2 + 250)
    cy = random.randint(50, SIZE - 50)
    # Only place on land
    if grid[cx][cy] == SECT_OCEAN:
        continue
    # Lava core
    for dx in range(-4, 5):
        for dy in range(-4, 5):
            if dx*dx + dy*dy <= 16:
                nx, ny = cx + dx, cy + dy
                if 0 <= nx < SIZE and 0 <= ny < SIZE and grid[nx][ny] != SECT_OCEAN:
                    grid[nx][ny] = SECT_LAVA
    # Ash ring around it
    for dx in range(-12, 13):
        for dy in range(-12, 13):
            r2 = dx*dx + dy*dy
            if 17 <= r2 <= 144:
                nx, ny = cx + dx, cy + dy
                if 0 <= nx < SIZE and 0 <= ny < SIZE and grid[nx][ny] not in (SECT_OCEAN, SECT_LAVA):
                    grid[nx][ny] = SECT_ASH
    # Burned zone further out
    for dx in range(-22, 23):
        for dy in range(-22, 23):
            r2 = dx*dx + dy*dy
            if 145 <= r2 <= 484:
                nx, ny = cx + dx, cy + dy
                if 0 <= nx < SIZE and 0 <= ny < SIZE and grid[nx][ny] not in (
                        SECT_OCEAN, SECT_LAVA, SECT_ASH):
                    grid[nx][ny] = SECT_BURNED

# ---------------------------------------------------------------------------
# Smooth pass: fix jarring adjacencies
# Rule: a cell takes the majority value of its 8 neighbours if it's a minority
# Also enforce: no LAVA adjacent to FOREST/SNOW/ICE (buffer via BURNED/ROCK)
# ---------------------------------------------------------------------------
print("Smoothing...", flush=True)

# Transition compatibility: what biomes can border each other
# (cells that can't border anything are forced to transition via these)
VOLCANIC = {SECT_LAVA, SECT_MAGMA, SECT_ASH, SECT_BURNED}
COLD     = {SECT_SNOW, SECT_SNOW_BLIZZARD, SECT_ICE}

def smooth_once(g):
    ng = [row[:] for row in g]
    for x in range(1, SIZE - 1):
        for y in range(1, SIZE - 1):
            cell = g[x][y]
            if cell == SECT_OCEAN:
                continue
            # Fix lava next to cold/forest/field/hills
            if cell in VOLCANIC:
                for dx in (-1, 0, 1):
                    for dy in (-1, 0, 1):
                        if dx == 0 and dy == 0:
                            continue
                        nb = g[x+dx][y+dy]
                        if nb in COLD or nb in (SECT_FOREST, SECT_FIELD, SECT_WATER):
                            # Insert burned as buffer
                            ng[x+dx][y+dy] = SECT_BURNED
    return ng

for _ in range(2):
    grid = smooth_once(grid)

# Majority smoothing: reduces salt-and-pepper noise
def majority_smooth(g, passes=3):
    for _ in range(passes):
        ng = [row[:] for row in g]
        for x in range(1, SIZE - 1):
            for y in range(1, SIZE - 1):
                if g[x][y] == SECT_OCEAN:
                    continue
                counts = {}
                for dx in (-1, 0, 1):
                    for dy in (-1, 0, 1):
                        v = g[x+dx][y+dy]
                        counts[v] = counts.get(v, 0) + 1
                best = max(counts, key=counts.__getitem__)
                if counts[best] >= 6:
                    ng[x][y] = best
        g = ng
    return g

print("Majority smoothing...", flush=True)
grid = majority_smooth(grid, passes=1)

# ---------------------------------------------------------------------------
# Resource scatter: seed small patches of key terrain throughout the map so
# players never have to travel far to find what they need.
# Every ~80 tiles in a grid, place a small patch of a rotating terrain type.
# Only placed on land (not ocean).  Patches are 3-8 tiles radius.
# ---------------------------------------------------------------------------
print("Scattering resource patches...", flush=True)

SCATTER_TYPES = [
    SECT_ROCK, SECT_FOREST, SECT_HILLS, SECT_SAND, SECT_FIELD,
    SECT_MOUNTAIN, SECT_SNOW, SECT_WATER,
]
SCATTER_SPACING = 60   # one patch every ~60 tiles
SCATTER_RADIUS  = 5    # base patch radius

patch_idx = 0
for gx in range(SCATTER_SPACING // 2, SIZE, SCATTER_SPACING):
    for gy in range(SCATTER_SPACING // 2, SIZE, SCATTER_SPACING):
        # Jitter the centre so patches aren't perfectly grid-aligned
        cx = gx + random.randint(-20, 20)
        cy = gy + random.randint(-20, 20)
        cx = max(SCATTER_RADIUS + 1, min(SIZE - SCATTER_RADIUS - 2, cx))
        cy = max(SCATTER_RADIUS + 1, min(SIZE - SCATTER_RADIUS - 2, cy))
        if grid[cx][cy] == SECT_OCEAN:
            patch_idx += 1
            continue
        terrain = SCATTER_TYPES[patch_idx % len(SCATTER_TYPES)]
        patch_idx += 1
        r = SCATTER_RADIUS + random.randint(-2, 3)
        for dx in range(-r, r + 1):
            for dy in range(-r, r + 1):
                if dx * dx + dy * dy <= r * r:
                    nx, ny = cx + dx, cy + dy
                    if 0 < nx < SIZE - 1 and 0 < ny < SIZE - 1:
                        if grid[nx][ny] != SECT_OCEAN:
                            grid[nx][ny] = terrain

# ---------------------------------------------------------------------------
# Final adjacency fix: sweep once more for lava/cold borders
# ---------------------------------------------------------------------------
for x in range(1, SIZE - 1):
    for y in range(1, SIZE - 1):
        if grid[x][y] in COLD:
            for dx in (-1, 0, 1):
                for dy in (-1, 0, 1):
                    if dx == 0 and dy == 0:
                        continue
                    nb = grid[x+dx][y+dy]
                    if nb == SECT_LAVA:
                        grid[x][y] = SECT_ROCK  # rock as border between cold and lava

# ---------------------------------------------------------------------------
# Write map.txt  (1500 rows, 1500 space-separated integers per row)
# ---------------------------------------------------------------------------
out_path = "/home/matt/assault-htw/data/map.txt"
print(f"Writing {out_path}...", flush=True)
with open(out_path, "w") as f:
    for x in range(SIZE):
        f.write(" ".join(str(grid[x][y]) for y in range(SIZE)))
        f.write("\n")
        if x % 100 == 0:
            print(f"  row {x}/{SIZE}", flush=True)

# ---------------------------------------------------------------------------
# Print terrain stats
# ---------------------------------------------------------------------------
from collections import Counter
flat_grid = [grid[x][y] for x in range(SIZE) for y in range(SIZE)]
counts = Counter(flat_grid)
names = {
    SECT_NULL: "null", SECT_ROCK: "rock", SECT_SAND: "sand", SECT_HILLS: "hills",
    SECT_MOUNTAIN: "mountain", SECT_WATER: "water(inland)", SECT_SNOW: "snow",
    SECT_FIELD: "field", SECT_FOREST: "forest", SECT_LAVA: "lava",
    SECT_BURNED: "burned", SECT_SNOW_BLIZZARD: "blizzard", SECT_ASH: "ash",
    SECT_ICE: "ice", SECT_MAGMA: "magma", SECT_OCEAN: "ocean",
}
total = SIZE * SIZE
print("\nTerrain distribution:")
for sect, name in sorted(names.items()):
    if counts[sect]:
        print(f"  {name:15s}: {counts[sect]:8d}  ({100*counts[sect]/total:.1f}%)")
print("\nDone.")
