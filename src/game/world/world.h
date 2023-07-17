#ifndef WORLD_H
#define WORLD_H

#include "util/util.h"
#include "game/world/chunk.h"

class World {
private:
  // TODO: Multiple chunks per world
  Chunk& chunk;

public:
  World(Chunk &chunk);
  u8 getBlock(int x, int y, int z) const;
  void setBlock(int x, int y, int z, u8 blockID);
};

#endif // WORLD_H
