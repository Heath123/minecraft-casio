#include "chunk.h"
#include "util/boundingBox.h"
#include <cstdio>

Chunk::Chunk() {
  // std::fill(blocks.begin(), blocks.end(), 0);
}

u8 Chunk::getBlock(int x, int y, int z) const {
  if (x >= CHUNK_WIDTH) x-= CHUNK_WIDTH;
  
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
  vec3 max = bb.getMin();
  // printf("%f %f %f to %f %f %f\n", ((float) min.x.v) / 65536.0f, ((float) min.y.v) / 65536.0f, ((float) min.z.v) / 65536.0f, ((float) max.x.v) / 65536.0f, ((float) max.y.v) / 65536.0f, ((float) max.z.v) / 65536.0f);
  // printf("%d %d %d to %d %d %d\n", min.x.ifloor(), min.y.ifloor(), min.z.ifloor(), max.x.ifloor(), max.y.ifloor(), max.z.ifloor());

  for (int x = min.x.ifloor(); x <= max.x.ifloor() + 1; x++) {
    for (int y = min.y.ifloor(); y <= max.y.ifloor() + 1; y++) {
      for (int z = min.z.ifloor(); z <= max.z.ifloor() + 1; z++) {
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
