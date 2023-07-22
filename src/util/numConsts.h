#ifndef NUMCONSTS_H
#define NUMCONSTS_H

#include "util/util.h"
#include <num/num.h>

using libnum::num32;

constexpr num32 withV(u32 v) {
  num32 num;
  num.v = v;
  return num;
}

namespace num32consts {
  // TODO: Is that the right name?
  constexpr num32 epsilon = withV(1);

  constexpr num32 max = withV(INT32_MAX);
}

#endif // NUMCONSTS_H
