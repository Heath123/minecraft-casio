#ifndef RAYCAST_H
#define RAYCAST_H

#include "util/util.h"

#include "num/vec.h"
#include <math.h>
#include <stdio.h>

using libnum::vec;
using libnum::vec3;
using libnum::num32;

bool traceRay(bool (*getVoxel)(int x, int y, int z), vec3 origin, vec3 direction, num32 max_d, vec3& hit_pos, vec<int, 3>& hit_pos_int, vec<int, 3>& hit_norm);

#endif // RAYCAST_H
