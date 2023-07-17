#ifndef PLAYER_H
#define PLAYER_H

#include "util/util.h"
#include "num/vec.h"
#include "util/boundingBox.h"
#include "game/world/chunk.h"

using libnum::vec3;
using libnum::num32;

class Player {
private:
  vec3 pos;
  BoundingBox bounds;
  // TODO: World instead
  Chunk& chunk;

  void updateBounds();
public:
  Player(Chunk &chunk);
  vec3 vel; // TODO: Private?

  void setPos(const vec3& newPos);
  void addPos(const vec3& toAdd);
  void subPos(const vec3& toSub);
  void addVel(const vec3& toAdd);
  vec3 getPos() const;
  vec3 getCameraPos() const;

  void tick(num32 lrMove, num32 fbMove);
};

#endif // PLAYER_H
