#ifndef BOUNDINGBOX_H
#define BOUNDINGBOX_H

#include "util/util.h"

#include <num/vec.h>
using libnum::vec3;

class BoundingBox {
private:
  vec3 min, max;

public:
  BoundingBox(const vec3& min, const vec3& max);

  // TODO: Do I need the whole getter/setter thing here?
  // Maybe for validation?
  void setMin(const vec3& newMin);
  void setMax(const vec3& newMax);
  vec3 getMin() const;
  vec3 getMax() const;

  bool intersects(const BoundingBox& other) const;
};

#endif // BOUNDINGBOX_H
