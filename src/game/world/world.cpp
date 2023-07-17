#include "world.h"

World::World(Chunk &newChunk) : chunk(newChunk) {
}

u8 World::getBlock(int x, int y, int z) const {
  return chunk.getBlock(x, y, z);
}

void World::setBlock(int x, int y, int z, u8 blockID) {
  chunk.setBlock(x, y, z, blockID);
}
