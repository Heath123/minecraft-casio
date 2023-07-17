#include "libnumToS3L.h"

S3L_Vec4 toS3L_Vec4(const vec3& in) {
  return (S3L_Vec4) {
    .x = in.x.v / (65536 / S3L_F),
    .y = in.y.v / (65536 / S3L_F),
    .z = in.z.v / (65536 / S3L_F),
  };
}

vec3 toVec3(const S3L_Vec4& in) {
  num32 x, y, z;
  x.v = in.x * (65536 / S3L_F);
  y.v = in.y * (65536 / S3L_F);
  z.v = in.z * (65536 / S3L_F);
  return vec3(x, y, z);
}
