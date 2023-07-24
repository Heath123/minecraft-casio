#include "angle.h"

num32 wrapAngle(num32 in) {
  num32 result;
  result.v = in.v & (num32(512).v - 1);
  return result;
}
