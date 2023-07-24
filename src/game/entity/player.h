#ifndef PLAYER_H
#define PLAYER_H

#include "util/util.h"

#include <num/vec.h>
#include <num/num.h>

#include "util/boundingBox.h"
#include "game/world/chunk.h"

using libnum::vec2;
using libnum::vec3;
using libnum::num32;

class Player {
private:
  // TODO: World instead
  Chunk& chunk;
  bool sprinting;

  // 512 units = 360 degrees (or 2*pi radians)
  // This matches small3dlib and allows faster wrapping
  vec2 rotation; // x = pitch, y = yaw

public:
  Player(Chunk &chunk);
  vec3 vel; // TODO: Private?
  BoundingBox bounds;
  vec3 pos;
  void updateBounds();

  void setPos(const vec3& newPos);
  void addPos(const vec3& toAdd);
  void subPos(const vec3& toSub);

  void addVel(const vec3& toAdd);

  vec2 getRotation() const;
  void rotate(const vec2& rotateBy);

  vec3 getPos() const;
  vec3 getCameraPos() const;

  bool isOnGround() const;

  void tick(num32 lrMove, num32 fbMove);
};

#endif // PLAYER_H
