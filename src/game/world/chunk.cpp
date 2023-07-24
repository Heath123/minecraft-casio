#include "chunk.h"
#include "util/boundingBox.h"
#include "util/numConsts.h"
#include <stdio.h>

Chunk::Chunk() {
  // std::fill(blocks.begin(), blocks.end(), 0);
}

u8 Chunk::getBlock(int x, int y, int z) const {
  // if (x >= CHUNK_WIDTH) x-= CHUNK_WIDTH;
  
  if (x < 0 || x >= CHUNK_WIDTH || y < 0 || y >= CHUNK_DEPTH || z < 0 ||
      z >= CHUNK_HEIGHT)
    return 1;
  return blocks[x + (z * CHUNK_WIDTH) + (y * CHUNK_WIDTH * CHUNK_HEIGHT)];
}

void Chunk::setBlock(int x, int y, int z, u8 blockID) {
  if (x < 0 || x >= CHUNK_WIDTH || y < 0 || y >= CHUNK_DEPTH || z < 0 ||
      z >= CHUNK_HEIGHT)
    return; // Ignore out-of-bounds coordinates
  blocks[x + (z * CHUNK_WIDTH) + (y * CHUNK_WIDTH * CHUNK_HEIGHT)] = blockID;
}

void Chunk::setBlock(vec<int, 3> pos, u8 blockID) {
  setBlock(pos.x, pos.y, pos.z, blockID);
}

BoundingBox Chunk::getBlockBB(int x, int y, int z) const {
  u8 blockID = getBlock(x, y, z);

  if (blockID != 0) {
    return BoundingBox(vec3(x, y, z), vec3(x + 1, y + 1, z + 1));
  }

  // TODO: A proper solution for this
  // return BoundingBox(vec3(0, -9999, 0), vec3(0, -9999, 0));
}

bool Chunk::intersects(BoundingBox& bb) const {
  vec3 min = bb.getMin();
  vec3 max = bb.getMax();
  // printf("%f %f %f to %f %f %f\n", ((float) min.x.v) / 65536.0f, ((float) min.y.v) / 65536.0f, ((float) min.z.v) / 65536.0f, ((float) max.x.v) / 65536.0f, ((float) max.y.v) / 65536.0f, ((float) max.z.v) / 65536.0f);
  // printf("%d %d %d to %d %d %d\n", min.x.ifloor(), min.y.ifloor(), min.z.ifloor(), max.x.ifloor(), max.y.ifloor(), max.z.ifloor());

  for (int x = min.x.ifloor(); x < max.x.iceil(); x++) {
    for (int y = min.y.ifloor(); y < max.y.iceil(); y++) {
      for (int z = min.z.ifloor(); z < max.z.iceil(); z++) {
        // printf("%d %d %d\n", x, y, z);
        if (getBlock(x, y, z) != 0 && getBlockBB(x, y, z).intersects(bb)) {
          // printf("%d %d %d\n", x, y, z);
          // printf("test\n");
          return true;
        }
      }
    }
  }

  return false;
}

num32 Chunk::distanceToIntersection(num32 maxDist, Axis direction, const BoundingBox& other) const {
  int sign = (maxDist > 0) ? 1 : -1;
  num32 lowest = sign * maxDist;

  // TODO: Would reusing this in the check for each block help performance?
  // Or just skip the intersection check there completely because we do it here
  BoundingBox extended = other;
  extended.extend(maxDist, direction);

  vec3 min = extended.getMin();
  vec3 max = extended.getMax();
  
  for (int x = min.x.ifloor(); x <= max.x.iceil() + 1; x++) {
    for (int y = min.y.ifloor(); y <= max.y.iceil() + 1; y++) {
      for (int z = min.z.ifloor(); z <= max.z.iceil() + 1; z++) {
        if (getBlock(x, y, z) == 0) continue;

        BoundingBox bb = getBlockBB(x, y, z);
        if (bb.intersects(extended)) {
          num32 dist = other.distanceToIntersection(lowest * sign, direction, bb);
          dist *= sign;
          // printf("intermed: %d\n", (dist * 100).ifloor());
          if (dist < lowest) lowest = dist;
        }
      }
    }
  }

  return lowest * sign;
}
