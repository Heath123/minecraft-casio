#ifndef MESHING_H
#define MESHING_H

#include "../game/world/chunk.h"
#include "libs/small3dlib-defs.h"

void generateMesh(const Chunk& chunk, S3L_Model3D &levelModel);
void makeSelectionBox(S3L_Model3D &boxModel);
void translateModel(S3L_Model3D &model, S3L_Model3D &newModel, S3L_Unit* vertexStorage, S3L_Unit x, S3L_Unit y, S3L_Unit z);
void colourTris(int x, int y, int z, S3L_Model3D &levelModel, u16* rollback_state, u16 (*effect)(u16 col, int index));

#endif // MESHING_H
