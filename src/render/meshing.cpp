#include "../util/util.h"

#include "meshing.h"
#include "gint/display-cg.h"
#include "libs/small3dlib-defs.h"
#include <cstdint>
#include <cstdio>

S3L_Model3D chunkModel;


#define MAX_VERTICES 6750
#define MAX_TRIANGLES 13500

#define DIRT_BROWN  C_RGB(17, 12, 8)
// #define DIRT_BROWN2 C_RGB(15, 10, 6)
// #define DIRT_BROWN3 C_RGB(13, 8, 4)
#define GRASS_GREEN C_RGB(10, 15, 6)

// An approximate darken that just subtracts from each channel
constexpr color_t darken(color_t colour, unsigned int amount) {
    u16 r = colour >> 11;
    u16 g = (colour >> 5) & 0x3f;
    u16 b = colour & 0x1f;

    r -= amount;
    g -= amount * 2;
    b -= amount;

    return (r << 11) | (g << 5) | b;
}

#define U S3L_F

#define topped(top, col) top, darken(col, 2), darken(col, 1), darken(col, 1), col, col
#define all(col) makeCol(darken(col, 2)), makeCol(darken(col, 2)), makeCol(darken(col, 1)), makeCol(darken(col, 1)), makeCol(col), makeCol(col)
// #define uniform(col) col, col, col, col, col, col

#define texPal(tex, pal) (texture) { /* .isColour = */ false, { /* .palIndex = */ pal, /* .texIndex = */ tex } }

const texture blockColours[][6] = {
  // Grass
  {
    texPal(4, 0), texPal(2, 3), texPal(2, 2), texPal(2, 2), texPal(2, 1), texPal(2, 1)
  },
  // Wood
  {
    texPal(0, 0), texPal(0, 3), texPal(0, 2), texPal(0, 2), texPal(0, 1), texPal(0, 1)
  },
  // Leaves
  {
    all(C_RGB(4, 10, 3))
  },
  // Log
  {
    all(C_RGB(12, 9, 6))
  },
  // Sand
  {
    all(C_RGB(27, 26, 20))
  },
  // Stone
  {
    all(C_RGB(16, 16, 16))
  },
  // Water
  {
    all(C_RGB(4, 11, 31))
  },
};

texture vary(int blockID, texture col, int x, int y, int z) {
  if (!col.isColour) return col;
  if (blockID == 7) return col;

  return (x + y + z) % 2 == 0 ? makeCol(darken(col.col, 1)) : col;
}

S3L_Index addVertex(S3L_Unit* vertices, S3L_Index &vertIndex, S3L_Unit x, S3L_Unit y, S3L_Unit z) {
  // Search for an existing matching vertex
  // TODO: This takes it from O(1) to O(n)
  for (int i = 0; i < vertIndex; i++) {
    if (vertices[(i * 3)] != x) continue;
    if (vertices[(i * 3) + 1] != y) continue;
    if (vertices[(i * 3) + 2] != z) continue;

    return i;
  }
  
  vertices[(vertIndex * 3)] = x;
  vertices[(vertIndex * 3) + 1] = y;
  vertices[(vertIndex * 3) + 2] = z;
  S3L_Index index = vertIndex;
  vertIndex += 1;
  return index;
}

// TODO: Make these heap allocated like the other two
texture colours[MAX_TRIANGLES];
// This can be binary-searched in order to find triangles corresponding to a block coordinate
// TODO: Do it per quad instead of per triangle?
u16 blockCoords[MAX_TRIANGLES] = {UINT16_MAX};

S3L_Index addTriangle(S3L_Index* triangles, S3L_Index &triIndex, S3L_Index index1, S3L_Index index2, S3L_Index index3, texture col, u16 coord) {
  triangles[(triIndex * 3)] = index1;
  triangles[(triIndex * 3) + 1] = index2;
  triangles[(triIndex * 3) + 2] = index3;
  colours[triIndex] = col;
  blockCoords[triIndex] = coord;
  S3L_Index index = triIndex;
  triIndex += 1;
  return index;
}

#define flip(tex) (texture) { /*.isColour = */ false, { /* .palIndex = */ tex.palIndex, /* .texIndex = */ (u16) (tex.texIndex + 1) } }

void addQuad(S3L_Index* triangles, S3L_Index &triIndex, S3L_Index index1, S3L_Index index2, S3L_Index index3, S3L_Index index4, texture col, u16 coord) {
  if (col.isColour) {
    addTriangle(triangles, triIndex, index4, index1, index2, col, coord);
    addTriangle(triangles, triIndex, index2, index3, index4, col, coord);
  } else {
    addTriangle(triangles, triIndex, index4, index1, index2, flip(col), coord);
    addTriangle(triangles, triIndex, index2, index3, index4, col, coord);
  }
}

void addQuadWithVertices(S3L_Index* triangles, S3L_Index &triIndex, S3L_Unit* vertices, S3L_Index &vertIndex, S3L_Vec4 pos1, S3L_Vec4 pos2, S3L_Vec4 pos3, S3L_Vec4 pos4, texture col, u16 coord) {
  S3L_Index a = addVertex(vertices, vertIndex, pos1.x, pos1.y, pos1.z);
  S3L_Index b = addVertex(vertices, vertIndex, pos2.x, pos2.y, pos2.z);
  S3L_Index c = addVertex(vertices, vertIndex, pos3.x, pos3.y, pos3.z);
  S3L_Index d = addVertex(vertices, vertIndex, pos4.x, pos4.y, pos4.z);

  addQuad(triangles, triIndex, a, b, c, d, col, coord);
}

// Looking towards Z+
typedef struct {
  bool top;
  bool bottom;

  bool right; // X+
  bool left; // X-

  bool back; // Z+
  bool front; // Z-
} faceVisibility;

typedef struct {
  bool a;
  bool b;
  bool c;
  bool d;
  bool e;
  bool f;
  bool g;
  bool h;
} vertexVisibility;

faceVisibility invisibleFaces = {
  false, false, false, false, false, false
};

vertexVisibility invisibleVertices = {
  false, false, false, false, false, false, false, false
};

// #define iter_1 x
// #define iter_2 y
// #define iter_3 z

// #define n_1 16
// #define n_2 32
// #define n_3 16

#define iter_1 x
#define iter_2 y
#define iter_3 z

#define n_1 16
#define n_2 32
#define n_3 16

void genBlock(S3L_Unit* vertices, S3L_Index* triangles, S3L_Index &vertIndex, S3L_Index &triIndex, const Chunk& chunk, int x, int y, int z) {
  // printf("Cube added at %d, %d, %d\n", x, y, z);
  u8 blockID = chunk.getBlock(x, y, z);
  if (!blockID) return;

  u16 coord = (iter_1 * n_2 * n_3) + (iter_2 * n_3) + iter_3;

  // // If it's surrounded in solid blocks then it's invisible
  if (chunk.getBlock(x + 1, y, z) != 0 &&
      chunk.getBlock(x - 1, y, z) != 0 &&
      chunk.getBlock(x, y + 1, z) != 0 &&
      chunk.getBlock(x, y - 1, z) != 0 &&
      chunk.getBlock(x, y, z + 1) != 0 &&
      chunk.getBlock(x, y, z - 1) != 0) {
        return;
  }

  faceVisibility faceVisible = invisibleFaces;
  vertexVisibility vertexVisible = invisibleVertices;
  
  if (chunk.getBlock(x, y + 1, z) == 0) {
    faceVisible.top = true;
    // d h
    // c g
    vertexVisible.d = true; vertexVisible.h = true;
    vertexVisible.c = true; vertexVisible.g = true;
  }
  if (chunk.getBlock(x, y - 1, z) == 0) {
    // a e
    // b f
    faceVisible.bottom = true;
    vertexVisible.a = true; vertexVisible.e = true;
    vertexVisible.b = true; vertexVisible.f = true;
  }

  if (chunk.getBlock(x + 1, y, z) == 0) {
    faceVisible.right = true;
    // g h
    // e f
    vertexVisible.g = true; vertexVisible.h = true;
    vertexVisible.e = true; vertexVisible.f = true;
  }

  if (chunk.getBlock(x - 1, y, z) == 0) {
    faceVisible.left = true;
    // d c
    // b a
    vertexVisible.d = true; vertexVisible.c = true;
    vertexVisible.b = true; vertexVisible.a = true;
  }

  if (chunk.getBlock(x, y, z + 1) == 0) {
    faceVisible.back = true;
    // h d
    // f b
    vertexVisible.h = true; vertexVisible.d = true;
    vertexVisible.f = true; vertexVisible.b = true;
  }
  if (chunk.getBlock(x, y, z - 1) == 0) {
    faceVisible.front = true;
    // c g
    // a e
    vertexVisible.c = true; vertexVisible.g = true;
    vertexVisible.a = true; vertexVisible.e = true;
  }

  // int a = addVertex(vertices, vertIndex, x, y, z);
  // int b = addVertex(vertices, vertIndex, x + U, y, z);
  // int c = addVertex(vertices, vertIndex, x + U, y + U, z);
  // int d = addVertex(vertices, vertIndex, x, y + U, z);

  // 4 3
  // 1 2

  // addTriangle(triangles, triIndex, a, b, c);
  // addQuad(triangles, triIndex, a, b, c, d, C_RED);

  S3L_Unit xPos = x * S3L_F;
  S3L_Unit yPos = y * S3L_F;
  S3L_Unit zPos = z * S3L_F;

  S3L_Index a = vertexVisible.a ? addVertex(vertices, vertIndex, xPos, yPos, zPos) : 0;
  S3L_Index b = vertexVisible.b ? addVertex(vertices, vertIndex, xPos, yPos, zPos + U) : 0;
  S3L_Index c = vertexVisible.c ? addVertex(vertices, vertIndex, xPos, yPos + U, zPos) : 0;
  S3L_Index d = vertexVisible.d ? addVertex(vertices, vertIndex, xPos, yPos + U, zPos + U) : 0;
  S3L_Index e = vertexVisible.e ? addVertex(vertices, vertIndex, xPos + U, yPos, zPos) : 0;
  S3L_Index f = vertexVisible.f ? addVertex(vertices, vertIndex, xPos + U, yPos, zPos + U) : 0;
  S3L_Index g = vertexVisible.g ? addVertex(vertices, vertIndex, xPos + U, yPos + U, zPos) : 0;
  S3L_Index h = vertexVisible.h ? addVertex(vertices, vertIndex, xPos + U, yPos + U, zPos + U) : 0;

  // addQuad follows these anticlockwise

  if (faceVisible.top) {
    // d h
    // c g
    addQuad(triangles, triIndex, c, g, h, d, vary(blockID, blockColours[blockID - 1][0], x, y, z), coord);
  }

  if (faceVisible.bottom) {
    // a e
    // b f
    addQuad(triangles, triIndex, b, f, e, a, vary(blockID, blockColours[blockID - 1][1], x, y, z), coord);
  }

  if (faceVisible.left) {
    // d c
    // b a
    addQuad(triangles, triIndex, b, a, c, d, vary(blockID, blockColours[blockID - 1][2], x, y, z), coord);
  }

  if (faceVisible.right) {
    // g h
    // e f
    addQuad(triangles, triIndex, e, f, h, g, vary(blockID, blockColours[blockID - 1][3], x, y, z), coord);
  }

  if (faceVisible.front) {
    // c g
    // a e
    addQuad(triangles, triIndex, a, e, g, c, vary(blockID, blockColours[blockID - 1][4], x, y, z), coord);
  }

  if (faceVisible.back) {
    // h d
    // f b
    addQuad(triangles, triIndex, f, b, d, h, vary(blockID, blockColours[blockID - 1][5], x, y, z), coord);
  }

  // vertices += vertIndex * 3;
  // triangles += triIndex * 3;

  // for (int i = 0; i < 36; i++) {
  //   vertices[(i * 3)] = cubeVertices[(i * 3)] + x;
  //   vertices[(i * 3) + 1] = cubeVertices[(i * 3) + 1] + y;
  //   vertices[(i * 3) + 2] = cubeVertices[(i * 3) + 2] + z;
  // }

  // for (int i = 0; i < 12; i++) {
  //   triangles[(i * 3)] = cubeTriangles[(i * 3)] + vertIndex;
  //   triangles[(i * 3) + 1] = cubeTriangles[(i * 3) + 1] + vertIndex;
  //   triangles[(i * 3) + 2] = cubeTriangles[(i * 3) + 2] + vertIndex;
  // }
  
  // vertIndex += 36;
  // triIndex += 12;
}

int findIndex(u16 coord, S3L_Model3D &levelModel) {
  int start = 0;
  int end = levelModel.triangleCount - 1;

  while (start <= end) {
    int mid = start + (end - start) / 2;

    if (blockCoords[mid] == coord) {
      return mid; // Found the element, return its index
    } else if (blockCoords[mid] < coord) {
      start = mid + 1; // Search the right half
    } else {
      end = mid - 1; // Search the left half
    }
  }

  return -1;
}

void colourTris(int x, int y, int z, S3L_Model3D &levelModel, texture* rollback_state, texture (*effect)(texture col, int index)) {
  u16 coord = (iter_1 * n_2 * n_3) + (iter_2 * n_3) + iter_3;
  int index = findIndex(coord, levelModel);
  // printf("Index: %d\n", index);
  if (index != -1) {
    // The index could be any tri in the block, so step back to find all of them
    index -= 12;
    if (index < 0) index = 0;
    while (blockCoords[index] != coord) {
      index++;
    }
    int i = 0;
    while (blockCoords[index] == coord) {
      if (rollback_state != NULL) {
        rollback_state[i] = colours[index];
      }
      colours[index] = effect(colours[index], i);
      index++;
      i++;
    }
  }
}

void generateMesh(const Chunk& chunk, S3L_Model3D &levelModel) {
  // printf("1.1");
  S3L_Unit* vertices = new S3L_Unit[MAX_VERTICES * 3];
  // printf("1.2");
  S3L_Index* triangles = new S3L_Index[MAX_TRIANGLES * 3];
  // printf("1.3");
  S3L_Index vertIndex = 0;
  S3L_Index triIndex = 0;

  // for (int iter_1 = 0; iter_1 < n_1; iter_1++) {
  for (int iter_1 = 0; iter_1 < n_1; iter_1++) {
    for (int iter_2 = 0; iter_2 < n_2; iter_2++) {
      for (int iter_3 = 0; iter_3 < n_3; iter_3++) {
        genBlock(vertices, triangles, vertIndex, triIndex, chunk, x, y, z);
      }
    }
  }
  // genBlock(vertices, triangles, vertIndex, triIndex, chunk, 0, 0, 0);
  // genBlock(vertices, triangles, vertIndex, triIndex, chunk, 0,1, 0);
  // genBlock(vertices, triangles, vertIndex, triIndex, chunk, 0, 2, 0);

  printf("%d verts, %d tris\n", vertIndex, triIndex);

  S3L_model3DInit(
    vertices,
    vertIndex,
    triangles,
    triIndex,
    &levelModel);
  
  levelModel.config.backfaceCulling = 2;
  // levelModel.config.backfaceCulling = 0;
}

void makeSelectionBox(S3L_Model3D &boxModel) {
  // // printf("1.1");
  // S3L_Unit* vertices = new S3L_Unit[300 * 3];
  // // printf("1.2");
  // S3L_Index* triangles = new S3L_Index[100 * 3];
  // // printf("1.3");
  // S3L_Index vertIndex = 0;
  // S3L_Index triIndex = 0;

  // // Left
  // addQuadWithVertices(triangles, triIndex, vertices, vertIndex, {0, 0, 0}, {U / 16, 0, 0}, {U / 16, U, 0}, {0, U, 0}, 0xffff);
  // // Right
  // addQuadWithVertices(triangles, triIndex, vertices, vertIndex, {U - (U / 16), 0, 0}, {U, 0, 0}, {U, U, 0}, {U - (U / 16), U, 0}, 0xffff);

  // addQuadWithVertices(triangles, triIndex, vertices, vertIndex, {U / 8, 0, 0}, {U - (U / 8), 0, 0}, {U - (U / 8), U / 16, 0}, {U / 8, U / 16, 0}, 0xffff);

  // printf("%d verts, %d tris\n", vertIndex, triIndex);

  // S3L_model3DInit(
  //   vertices,
  //   vertIndex,
  //   triangles,
  //   triIndex,
  //   &boxModel);
  
  // boxModel.config.backfaceCulling = 2;
  // // boxModel.config.alwaysOnTop = 1;
}

// Apply the transform to the actual vertex positions
void translateModel(S3L_Model3D &model, S3L_Model3D &newModel, S3L_Unit* vertexStorage, S3L_Unit x, S3L_Unit y, S3L_Unit z) {
  newModel.vertexCount = model.vertexCount;
  for (int i = 0; i < model.vertexCount; i++) {
    vertexStorage[(i * 3) + 0] = model.vertices[(i * 3) + 0] + x;
    vertexStorage[(i * 3) + 1] = model.vertices[(i * 3) + 1] + y;
    vertexStorage[(i * 3) + 2] = model.vertices[(i * 3) + 2] + z;
  }
  newModel.vertices = vertexStorage;
  newModel.config = model.config;
  newModel.customTransformMatrix = model.customTransformMatrix;
  newModel.transform = model.transform;
  newModel.triangleCount = model.triangleCount;
  newModel.triangles = model.triangles;
}
