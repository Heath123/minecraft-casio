#ifndef MESHING_H
#define MESHING_H

#include "../game/world/chunk.h"
#include "libs/small3dlib-defs.h"

typedef struct {
  bool isColour;
  union {
    struct {
      u8 palIndex;
      u16 texIndex;
    } __attribute__((packed));
    u16 col;
  } __attribute__((packed));
} __attribute__((packed)) texture;

void generateMesh(const Chunk& chunk, S3L_Model3D &levelModel);
void makeSelectionBox(S3L_Model3D &boxModel);
void translateModel(S3L_Model3D &model, S3L_Model3D &newModel, S3L_Unit* vertexStorage, S3L_Unit x, S3L_Unit y, S3L_Unit z);
void colourTris(int x, int y, int z, S3L_Model3D &levelModel, texture* rollback_state, texture (*effect)(texture col, int index));

#define makeCol(x) (texture) { .isColour = true, .col = x }

#endif // MESHING_H
