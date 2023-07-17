#ifndef CHUNK_H
#define CHUNK_H

#include "util/util.h"
#include "util/boundingBox.h"

#include <array>

class Chunk {
private:
  static constexpr int CHUNK_WIDTH = 16;
  static constexpr int CHUNK_HEIGHT = 16;
  static constexpr int CHUNK_DEPTH = 32;
  std::array<u8, CHUNK_WIDTH * CHUNK_HEIGHT * CHUNK_DEPTH> blocks;

public:
  Chunk();
  u8 getBlock(int x, int y, int z) const;
  void setBlock(int x, int y, int z, u8 blockID);
  BoundingBox getBlockBB(int x, int y, int z) const;

  bool intersects(BoundingBox& bb) const;
};

#endif // CHUNK_H
