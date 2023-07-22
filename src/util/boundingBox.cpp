#include "boundingBox.h"

BoundingBox::BoundingBox(const vec3& min, const vec3& max) : min(min), max(max) {}

void BoundingBox::setMin(const vec3& newMin) {
  min = newMin;
}

void BoundingBox::setMax(const vec3& newMax) {
  max = newMax;
}

vec3 BoundingBox::getMin() const {
  return min;
}

vec3 BoundingBox::getMax() const {
  return max;
}

bool BoundingBox::intersects(const BoundingBox &other) const {
  // Check for intersection along each axis
  if (max.x <= other.min.x || min.x >= other.max.x)
    return false;

  if (max.y <= other.min.y || min.y >= other.max.y)
    return false;

  if (max.z <= other.min.z || min.z >= other.max.z)
    return false;

  // If no separation along any axis, boxes intersect
  return true;
}


void BoundingBox::extend(num32 dist, Axis direction) {
  // Extend the bounding box in the direction
  switch (direction) {
  case Axis::AXIS_X:
    if (dist > 0) {
      setMax(max + vec3(dist, 0, 0));
    } else {
      setMin(min + vec3(dist, 0, 0));
    }
    break;
  case Axis::AXIS_Y:
    if (dist > 0) {
      setMax(max + vec3(0, dist, 0));
    } else {
      setMin(min + vec3(0, dist, 0));
    }
    break;
  case Axis::AXIS_Z:
    if (dist > 0) {
      setMax(max + vec3(0, 0, dist));
    } else {
      setMin(min + vec3(0, 0, dist));
    }
    break;
  }
}

num32 BoundingBox::distanceToIntersection(num32 maxDist, Axis direction, const BoundingBox& other) const {
  // Extend the bounding box in the direction
  BoundingBox extended = *this;
  extended.extend(maxDist, direction);

  // If the path wouldn't cross the other box then it can go all the way
  if (!extended.intersects(other)) {
    return maxDist;
  }

  switch (direction) {
  case Axis::AXIS_X:
    if (maxDist > 0) {
      return other.getMin().x - max.x;
    } else {
      return other.getMax().x - min.x;
    }
    break;
  case Axis::AXIS_Y:
    if (maxDist > 0) {
      return other.getMin().y - max.y;
    } else {
      return other.getMax().y - min.y;
    }
    break;
  case Axis::AXIS_Z:
    if (maxDist > 0) {
      return other.getMin().z - max.z;
    } else {
      return other.getMax().z - min.z;
    }
    break;
  }

  return 0;
}
