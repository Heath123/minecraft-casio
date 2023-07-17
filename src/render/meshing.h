#ifndef MESHING_H
#define MESHING_H

#include "../game/world/chunk.h"
#include "libs/small3dlib-defs.h"

void generateMesh(const Chunk& chunk, S3L_Model3D &levelModel);
void makeSelectionBox(S3L_Model3D &boxModel);
void translateModel(S3L_Model3D &model, S3L_Model3D &newModel, S3L_Unit* vertexStorage, S3L_Unit x, S3L_Unit y, S3L_Unit z);

#endif // MESHING_H
