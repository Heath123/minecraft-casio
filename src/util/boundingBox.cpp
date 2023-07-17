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
  // // Check for intersection along each axis
  // if (max.x < other.min.x || min.x > other.max.x)
  //   return false;

  // if (max.y < other.min.y || min.y > other.max.y)
  //   return false;

  // if (max.z < other.min.z || min.z > other.max.z)
  //   return false;

  // // If no separation along any axis, boxes intersect
  // return true;
  return min.x < other.max.x && max.x > other.min.x && min.y < other.max.y && max.y > other.min.y && min.z < other.max.z && max.z > other.min.z;
}
