#ifndef ANGLE_H
#define ANGLE_H

#include "util/util.h"

#include <num/num.h>

using libnum::num32;

constexpr num32 deg(num32 degrees) {
  return num32(64) * degrees / num32(45);
}

num32 wrapAngle(num32 in);

#endif // ANGLE_H
