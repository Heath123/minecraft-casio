#include "player.h"
#include "game/world/chunk.h"
#include "util/boundingBox.h"
#include "util/numConsts.h"
#include "util/angle.h"
#include <stdio.h>

// TODO: Wrap
#include "libs/small3dlib-defs.h"

const num32 PLAYER_HEIGHT = 1.8;
const num32 PLAYER_WIDTH = 0.6;

const num32 CAMERA_HEIGHT = 1.625;

Player::Player(Chunk &chunk) : bounds(vec3(0), vec3(0)), chunk(chunk) {
  pos.x = 0;
  pos.y = 50;
  pos.z = 0;

  rotation = vec2(0, 0);

  updateBounds();
}

void Player::tick(num32 lrMove, num32 fbMove) {
  // BoundingBox bb = chunk.getBlockBB(5, 4, 9);
  // num32 dist = bounds.distanceToIntersection(100, Axis::AXIS_Z, bb);

  // TODO: This is the other way around?
  // pos.z += dist;

  // num32 dist = chunk.distanceToIntersection(100, Axis::AXIS_Z, bounds);
  // printf("dist %d\n", (dist * 100).ifloor());

  // bool forwardPressed = fb

  bool wasColliding = chunk.intersects(bounds);
  // printf("Colliding: %d\n", wasColliding);

  if (wasColliding) {
    pos += vel;
  } else {
    // TODO: Do this properly with the swept AABB thing
    if (vel.x != 0) {
      num32 dist = chunk.distanceToIntersection(vel.x, Axis::AXIS_X, bounds);
      if (dist != vel.x) {
        vel.x = 0;
      }
      pos.x += dist;
      if (dist != 0) {
        updateBounds();
      }
    }

    if (vel.z != 0) {
      num32 dist = chunk.distanceToIntersection(vel.z, Axis::AXIS_Z, bounds);
      if (dist != vel.z) {
        vel.z = 0;
      }
      pos.z += dist;
      updateBounds();
    }

    if (vel.y != 0) {
      num32 dist = chunk.distanceToIntersection(vel.y, Axis::AXIS_Y, bounds);
      if (dist != vel.y) {
        vel.y = 0;
      }
      pos.y += dist;
      updateBounds();
    }
  }

  // for (int i = 0; i < 64; i++) {
  //   pos.y += (vel.y / 64);
  //   updateBounds();
  //   if (chunk.intersects(bounds)) {
  //     if (vel.y < 0) {
  //       pos.y = pos.y.ceil();
  //     } else {
  //       pos.y -= (vel.y / 64);
  //     }
  //     vel.y = 0;
  //   }
  // }

  vel.y -= 0.08;
  vel.y *= num32(0.98);
  // printf("After: x: %d, y: %d, z: %d", pos.x.ifloor(), pos.y.ifloor(), pos.z.ifloor());

  num32 impulse = fbMove * S3L_cos(rotation.y.ifloor()) + lrMove * S3L_sin(rotation.y.ifloor());
  impulse /= 512;

  bool onGround = isOnGround();
  num32 slipperiness;
  num32 multiplier;
  if (onGround) {
    slipperiness = 0.6;
    multiplier = num32(0.6) / slipperiness;
    multiplier = multiplier * multiplier * multiplier * /*effectMultiplier*/ 1;
  } else {
    slipperiness = 1.0;
    multiplier = 0.2;
  }

  vel.z *= slipperiness * num32(0.91);
  vel.z += num32(0.1) * impulse * multiplier;

  // int zVel = vel.z.ifloor();
  // int zVelFrac = (vel.z * 100).ifloor() % 100;
  // printf("zVel: %d.%02d\n", zVel, zVelFrac);

  impulse = fbMove * S3L_sin(rotation.y.ifloor()) - lrMove * S3L_cos(rotation.y.ifloor());
  impulse = -impulse;
  impulse /= 512;

  vel.x *= slipperiness * num32(0.91);
  vel.x += num32(0.1) * impulse * multiplier;
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

// vec2 wrapRotation(vec2 in) {
//   in.x %= 512;
//   in.y %= 512;
//   return in;
// }

vec2 Player::getRotation() const {
  return rotation;
}

void Player::rotate(const vec2& rotateBy) {
  rotation += rotateBy;
  // rotation = wrapRotation(rotation);
  // Clamp the pitch but wrap the yaw
  if (rotation.x > deg(90)) rotation.x = deg(90);
  if (rotation.x < deg(-90)) rotation.x = deg(-90);
  rotation.y = wrapAngle(rotation.y);
}

vec3 Player::getPos() const { return pos; }

vec3 Player::getCameraPos() const {
  return pos + vec3(0, CAMERA_HEIGHT, 0);
}

bool Player::isOnGround() const {
  BoundingBox tempBounds = bounds;

  vec3 min = tempBounds.getMin();
  min.y -= num32consts::epsilon;
  tempBounds.setMin(min);

  return chunk.intersects(tempBounds);
}
