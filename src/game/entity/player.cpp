#include "player.h"
#include "game/world/chunk.h"
#include "util/boundingBox.h"
#include <cstdio>

const num32 PLAYER_HEIGHT = 1.8;
const num32 PLAYER_WIDTH = 0.6;

const num32 CAMERA_HEIGHT = 1.625;

Player::Player(Chunk &chunk) : bounds(vec3(0), vec3(0)), chunk(chunk) {
  pos.x = 0;
  pos.y = 50;
  pos.z = 0;

  updateBounds();
}

void Player::tick(num32 lrMove, num32 fbMove) {
  // TODO: Remove this?
  if (vel.y < -1) {
    vel.y = -1;
  }

  // printf("Before: x: %d, y: %d, z: %d", pos.x.ifloor(), pos.y.ifloor(), pos.z.ifloor());
  pos.y.v += 1;
  updateBounds();

  bool wasColliding = chunk.intersects(bounds);
  // printf("Colliding: %d\n", wasColliding);

  // BoundingBox blockBB = chunk.getBlockBB(8, 3, 3);
  // printf("Colliding2: %d\n", blockBB.intersects(bounds));
  // if (wasColliding) {
  //   pos += vel;
  // } else {
  
  // Do this properly with the swept AABB thing
  pos.x += vel.x;
  updateBounds();
  if (chunk.intersects(bounds)) {
    pos.x -= vel.x;
    vel.x = 0;
  }

  pos.z += vel.z;
  updateBounds();
  if (chunk.intersects(bounds)) {
    pos.z -= vel.z;
    vel.z = 0;
  }

  pos.y.v -= 1;

  pos.y += vel.y;
  updateBounds();
  if (chunk.intersects(bounds)) {
    if (vel.y < 0) {
      pos.y = pos.y.ceil();
    } else {
      pos.y -= vel.y;
    }
    vel.y = 0;
  }

  vel.y -= 0.08;
  vel.y *= num32(0.98);
  // printf("After: x: %d, y: %d, z: %d", pos.x.ifloor(), pos.y.ifloor(), pos.z.ifloor());

  num32 impulse = fbMove;
  num32 slipperiness = 0.6;
  vel.z *= slipperiness * num32(0.91);
  vel.z += num32(0.1) * impulse * 1 * /* (num32(0.6) / slipperiness) * (num32(0.6) / slipperiness) * (num32(0.6) / slipperiness) */ 1;

  // int zVel = vel.z.ifloor();
  // int zVelFrac = (vel.z * 100).ifloor() % 100;
  // printf("zVel: %d.%02d\n", zVel, zVelFrac);

  impulse = lrMove;
  vel.x *= slipperiness * num32(0.91);
  vel.x += num32(0.1) * impulse * 1 * /* (num32(0.6) / slipperiness) * (num32(0.6) / slipperiness) * (num32(0.6) / slipperiness) */ 1;
}

void Player::updateBounds() {
  vec3 newMin = pos - vec3(PLAYER_WIDTH / 2, 0, PLAYER_WIDTH / 2);
  bounds.setMin(newMin);

  vec3 newMax = pos + vec3(PLAYER_WIDTH / 2, PLAYER_HEIGHT, PLAYER_WIDTH / 2);
  bounds.setMax(newMax);
}

void Player::setPos(const vec3 &newPos) {
  pos = newPos;
  updateBounds();
}

void Player::addPos(const vec3& toAdd) {
  pos += toAdd;
  updateBounds();
}

void Player::subPos(const vec3& toSub) {
  pos -= toSub;
  updateBounds();
}

void Player::addVel(const vec3& toAdd) {
  vel += toAdd;
}

vec3 Player::getPos() const { return pos; }

vec3 Player::getCameraPos() const {
  return pos + vec3(0, CAMERA_HEIGHT, 0);
}
