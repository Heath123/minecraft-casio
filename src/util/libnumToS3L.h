#ifndef LIBNUMTOS3L_H
#define LIBNUMTOS3L_H

#include "util/util.h"
#include "libs/small3dlib-defs.h"

#include "util/util.h"
#include "num/vec.h"
#include "util/boundingBox.h"

using libnum::vec3;
using libnum::num32;

S3L_Vec4 toS3L_Vec4(const vec3& in);
vec3 toVec3(const S3L_Vec4& in);

#endif // LIBNUMTOS3L_H
