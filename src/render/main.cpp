#include "../util/util.h"

#include "azur/gint/render.h"
#include "game/entity/player.h"
#include "game/world/world.h"
#include "gint/display.h"
#include "gint/drivers/r61524.h"
#include <gint/clock.h>
#include "libprof.h"
#include "util/boundingBox.h"
#include <stdio.h>
#include <time.h>
#include <gint/kmalloc.h>

prof_t perf_s3l_sort;
prof_t perf_s3l_project;
prof_t perf_transform;
prof_t perf_checkvis;
prof_t perf_addtri;

#define S3L_PIXEL_FUNCTION drawPixel

#include "render_config.h"
#include "libs/small3dlib.h"

#include "../game/world/chunk.h"
#include "gint/display-cg.h"
#include "gint/keycodes.h"
#include "meshing.h"
#include <gint/keyboard.h>
#include <gint/gint.h>
#include <gint/rtc.h>

#include <stdlib.h>
#include <string.h>

#include <gint/fs.h>
#include "/home/heath/.local/share/fxsdk/sysroot/sh3eb-elf/include/unistd.h"

#include "util/libnumToS3L.h"
#include "game/world/raycast.h"
#include "util/angle.h"
#include "util/colour.h"

#define STDIN_FILENO  0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

volatile char* debugOut = (char*) 0xfffff000;

// Prints to the console on the emulator
ssize_t stdout_write(void *data, void const *buf, size_t size) {
  for (int i = 0; i < size; i++) {
    char c = ((const char*) buf)[i];
    *debugOut = c;
  }
  return size;
}

fs_descriptor_type_t stdouterr_type = {
  .read = NULL,
  .write = stdout_write,
  .lseek = NULL,
  .close = NULL,
};

// #include "levelModel.h"

S3L_Scene scene;

uint32_t frame = 0;

S3L_Transform3D modelTransform;
S3L_DrawConfig conf;

void drawPixel(S3L_PixelInfo *p) {}

int fps = 0;

extern font_t gint_font5x7;

// Print in a small font with a translucent background
void azrp_print2(int x, int y, int fg, char const *fmt, ...) {
  va_list args;
  va_start(args, fmt);

  char str[128];
  vsnprintf(str, sizeof str, fmt, args);
  va_end(args);

  int w, h;
  dsize(str, &gint_font5x7, &w, &h);
  azrp_rect(x - 1, y - 1, w + 2, h + 2, AZRP_RECT_DARKEN);
  azrp_text_opt(x, y, &gint_font5x7, fg, DTEXT_LEFT, DTEXT_TOP, str, -1);
}

bool debugHUD = true;

// TODO: Change the number
S3L_Vec4 projectedVertices[6750];

#define TRI_SORT

typedef struct {
  // uint8_t modelIndex;
  S3L_Index triangleIndex;
  uint16_t sortValue;
} sortableTri;

#ifdef TRI_SORT
sortableTri triSortArray[S3L_MAX_TRIANGES_DRAWN];
int triSortArraySize = 0;

void quicksort(sortableTri* arr, int16_t low, int16_t high) {
    if(low >= high) return;

    sortableTri pivot = arr[(low + high) >> 1];

    int i = low - 1;
    int j = high + 1;

    while (true) {
        do i++;
        while (arr[i].sortValue > pivot.sortValue);

        do j--;
        while (arr[j].sortValue < pivot.sortValue);

        if(i >= j) break;

        sortableTri tmp = arr[i];
        arr[i] = arr[j];
        arr[j] = tmp;
    }

    quicksort(arr, low, j);
    quicksort(arr, j+1, high);
}
#endif

// Much of this is a reimplementation of what small3dlib does anyway
// However some of it is more specialized and therefore a little faster
void drawScene_custom(S3L_Scene scene) {
  const S3L_Model3D* model = &(scene.models[0]);
  
  S3L_Index triangleCount = scene.models[0].triangleCount;
  S3L_Index vertexCount = scene.models[0].vertexCount;

  S3L_Index vertexIndex = 0;

  prof_enter(perf_transform);

  // TODO: Should we use a 3x3 matrix for this?
  // Is it faster or slower than two 2D transforms?
  S3L_Unit sinYaw = S3L_sin(-scene.camera.transform.rotation.y);
  S3L_Unit cosYaw = S3L_cos(-scene.camera.transform.rotation.y);

  S3L_Unit sinPitch = S3L_sin(-scene.camera.transform.rotation.x);
  S3L_Unit cosPitch = S3L_cos(-scene.camera.transform.rotation.x);

  while (vertexIndex < vertexCount) {
    i32 x = model->vertices[vertexIndex * 3];
    i32 y = model->vertices[vertexIndex * 3 + 1];
    i32 z = model->vertices[vertexIndex * 3 + 2];

    x -= scene.camera.transform.translation.x;
    y -= scene.camera.transform.translation.y;
    z -= scene.camera.transform.translation.z;

    const int renderDistance = 11;

    if (x > (S3L_F * renderDistance) || x < -(S3L_F * renderDistance)) {
      projectedVertices[vertexIndex] = { 0, 0, -1, -1 };
      vertexIndex++;
      continue;
    }

    if (z > (S3L_F * renderDistance) || z < -(S3L_F * renderDistance)) {
      projectedVertices[vertexIndex] = { 0, 0, -1, -1 };
      vertexIndex++;
      continue;
    }

    i32 newX = (x * cosYaw - z * sinYaw) / S3L_F;
    i32 newZ = (x * sinYaw + z * cosYaw) / S3L_F;

    x = newX;
    z = newZ;

    i32 newZ2 = (z * cosPitch - y * sinPitch) / S3L_F;
    i32 newY2 = (z * sinPitch + y * cosPitch) / S3L_F;

    z = newZ2;
    y = newY2;

    // Keeps the unprojected z in w sort sorting like small3dlib does
    S3L_Vec4 v = { x, y, z, z };
    _S3L_mapProjectedVertexToScreen(&v, scene.camera.focalLength);

    projectedVertices[vertexIndex] = v;

    vertexIndex++;
  }
  prof_leave(perf_transform);

  S3L_Index triangleIndex = 0;

  #ifdef TRI_SORT
  triSortArraySize = 0;
  #endif

  while (triangleIndex < triangleCount) {
    u32 vertexIndex0 = model->triangles[triangleIndex * 3];
    u32 vertexIndex1 = model->triangles[triangleIndex * 3 + 1];
    u32 vertexIndex2 = model->triangles[triangleIndex * 3 + 2];

    S3L_Vec4 v0, v1, v2;
    v0 = projectedVertices[vertexIndex0];
    v1 = projectedVertices[vertexIndex1];
    v2 = projectedVertices[vertexIndex2];

    if (v0.w == -1 || v1.w == -1 || v2.w == -1) {
      triangleIndex++;
      continue;
    }

    prof_enter(perf_checkvis);
    bool visible = S3L_triangleIsVisible(v0, v1, v2, model->config.backfaceCulling);
    prof_leave(perf_checkvis);
    if (visible) {
      #ifdef TRI_SORT
      // TODO
      if (triSortArraySize >= S3L_MAX_TRIANGES_DRAWN) break;
      triSortArray[triSortArraySize] = {
        .triangleIndex = triangleIndex,
        // Adding S3L_F helps prevent underflow of negative values
        // TODO: Consider using a signed value instead?
        .sortValue = (uint16_t) (((v0.w + v1.w + v2.w) >> 2) + S3L_F)
      };
      triSortArraySize++;
      #else
      prof_enter(perf_addtri);
      azrp_triangle(
        v0.x, v0.y,
        v1.x, v1.y,
        v2.x, v2.y,
        colours[triangleIndex]
      );
      prof_leave(perf_addtri);
      #endif
    }

    triangleIndex++;
  }

  #ifdef TRI_SORT
  prof_enter(perf_s3l_sort);
  quicksort(triSortArray, 0, triSortArraySize - 1);
  prof_leave(perf_s3l_sort);

  S3L_Index sortedIndex = 0;
  while (sortedIndex < triSortArraySize) {
    S3L_Index triangleIndex = triSortArray[sortedIndex].triangleIndex;

    u32 vertexIndex0 = model->triangles[triangleIndex * 3];
    u32 vertexIndex1 = model->triangles[triangleIndex * 3 + 1];
    u32 vertexIndex2 = model->triangles[triangleIndex * 3 + 2];

    S3L_Vec4 v0, v1, v2;
    v0 = projectedVertices[vertexIndex0];
    v1 = projectedVertices[vertexIndex1];
    v2 = projectedVertices[vertexIndex2];

    azrp_triangle(
      v0.x, v0.y,
      v1.x, v1.y,
      v2.x, v2.y,
      // v0.w > (S3L_F * 11) ? 0x86df : v0.w > (S3L_F * 8) ? alphaBlendRGB565(colours[triangleIndex], 0x86df, 31 - (v0.w - (S3L_F * 8)) / (S3L_F * 3 / 32)) : colours[triangleIndex]
      colours[triangleIndex]
    );

    sortedIndex++;
  }
  #endif
}

void draw(void) {
  S3L_newFrame();
  azrp_clear(0x86df);

  // printf("Start frame\n");
  // S3L_drawScene(scene);
  drawScene_custom(scene);
  // printf("End frame\n");

  // azrp_triangle(407, 322, 407, 217, 302, 217, 0x8B08);

  fps++;

  static int lastTime = 0;
  int currentTime = rtc_ticks();

  static int realFPS = 0;
  // If 1 second has passed
  if (currentTime - lastTime >= 128) {
    lastTime = currentTime;
    realFPS = fps;
    fps = 0;
  }
  if (debugHUD) {
    azrp_print2(1, 10, C_WHITE, "FPS: %d (max 20)", realFPS);
  }
  // azrp_print_opt(1, 16, &gint_font5x7, C_BLACK, DTEXT_LEFT, DTEXT_TOP, "FPS: %d", realFPS);
}

#define MAX_VERTICES 30000
#define MAX_TRIANGLES 10000

S3L_Model3D models[2];

#define levelModel models[0]
#define selectionBoxModel models[1]

S3L_Model3D baseSelectionBoxModel;

Player* mainPlayer;
Chunk* chunk;

bool getVoxel(int x, int y, int z) {
  return chunk->getBlock(x, y, z) != 0;
}

void mainLoop(void) {
  cleareventflips();
  clearevents();
  if (keydown(KEY_MENU)) {
    gint_osmenu();
  }

  // if (state[SDL_SCANCODE_ESCAPE])
  if (keydown(KEY_EXIT))
    exit(0);

  #define speed_mult 15
  // TODO: Acceleration
  // if (state[SDL_SCANCODE_A])

  // if (keydown(KEY_LEFT))
  //   scene.camera.transform.rotation.y += 1 * speed_mult;
  // // else if (state[SDL_SCANCODE_D])
  // if (keydown(KEY_RIGHT))
  //   scene.camera.transform.rotation.y -= 1 * speed_mult;
  // // else if (state[SDL_SCANCODE_W])
  // if (keydown(KEY_UP))
  //   scene.camera.transform.rotation.x += 1 * speed_mult;
  // // else if (state[SDL_SCANCODE_S])
  // if (keydown(KEY_DOWN))
  //   scene.camera.transform.rotation.x -= 1 * speed_mult;

  // static int leftPressedTime = 0;
  // if (keydown(KEY_LEFT)) {
  //   leftPressedTime++;
  //   if (leftPressedTime == 1) {
  //     mainPlayer->rotate(vec2(0, 1));
  //   } else if (leftPressedTime > 3 && leftPressedTime < 7) {
  //     mainPlayer->rotate(vec2(0, 1));
  //   } else if (leftPressedTime >= 7 && leftPressedTime < 15) {
  //     mainPlayer->rotate(vec2(0, 2));
  //   } else if (leftPressedTime >= 15) {
  //     mainPlayer->rotate(vec2(0, 3));
  //   }
  // } else {
  //   leftPressedTime = 0;
  // }

  // static int rightPressedTime = 0;
  // if (keydown(KEY_RIGHT)) {
  //   rightPressedTime++;
  //   if (rightPressedTime == 1) {
  //     mainPlayer->rotate(vec2(0, -1));
  //   } else if (rightPressedTime > 3 && rightPressedTime < 7) {
  //     mainPlayer->rotate(vec2(0, -1));
  //   } else if (rightPressedTime >= 7 && rightPressedTime < 15) {
  //     mainPlayer->rotate(vec2(0, -2));
  //   } else if (rightPressedTime >= 15) {
  //     mainPlayer->rotate(vec2(0, -3));
  //   }
  // } else {
  //   rightPressedTime = 0;
  // }

  constexpr num32 increment = deg(5);

  if (keydown(KEY_LEFT))
    mainPlayer->rotate(vec2(0, increment));
  if (keydown(KEY_RIGHT))
    mainPlayer->rotate(vec2(0, -increment));
  if (keydown(KEY_UP))
    mainPlayer->rotate(vec2(increment, 0));
  if (keydown(KEY_DOWN))
    mainPlayer->rotate(vec2(-increment, 0));

  vec2 rotation = mainPlayer->getRotation();
  scene.camera.transform.rotation.y = rotation.y.ifloor();
  scene.camera.transform.rotation.x = rotation.x.ifloor();

  // S3L_Vec4 camF, camR;
  // rayDir = camF;
  // camF.y = 0;
  // camR.y = 0;
  // S3L_vec3Normalize(&camF);
  // S3L_vec3Normalize(&camR);
  // camF.x /= 4; camF.z /= 4;
  // camR.x /= 4; camR.z /= 4;

  static bool shiftWasPressed = keydown(KEY_SHIFT);

  // S3L_Vec4 toMove = { 0, 0, 0 };

  num32 lr = 0;
  num32 fb = 0;

  if (keydown(KEY_OPTN))
    fb += 1;
  if (keydown(KEY_LOG))
    fb -= 1;
  if (keydown(KEY_ALPHA))
    lr -= 1;
  if (keydown(KEY_POWER))
    lr += 1;
  if (keydown(KEY_SHIFT) /*&& !shiftWasPressed*/ && mainPlayer->isOnGround())
    // S3L_vec3Add(&scene.camera.transform.translation, { 0, 80, 0 });
    // mainPlayer->addVel(vec3(0, 0.42, 0));
    mainPlayer->vel.y = 0.42;
  // else if (keydown(KEY_ALPHA))
  //   S3L_vec3Add(&scene.camera.transform.translation, { 0, -80, 0 });

  shiftWasPressed = keydown(KEY_SHIFT);

  // vec3 toMove2 = toVec3(toMove);
  mainPlayer->tick(lr, fb);
  scene.camera.transform.translation = toS3L_Vec4(mainPlayer->getCameraPos());

  static num32 fovMult = 1;

  // num32 targetFovMult = keydown(KEY_F4) ? 1.15 : 1.0; // mainPlayer->getFOVModifier();
  num32 targetFovMult = 1.0;
  fovMult += (targetFovMult - fovMult) / 2;
  if (fovMult > num32(1.5)) fovMult = 1.5;
  if (fovMult < num32(0.1)) fovMult = 0.1;

  #define M_PI 3.14159265358979323846
  float fovf = ((float) fovMult * 70.0) * (M_PI / 180);
  float mult = 1.0 / (2.0 * tanf(fovf / 2.0));
  // printf("fov: %d\n", (int) (mult * 1000));
  scene.camera.focalLength = S3L_F * mult;

  libnum::vec3 hit_position = { 0, 0, 0 };
  libnum::vec<int, 3> hit_position_int = { 0, 0, 0 };
  libnum::vec<int, 3> hit_normal = { 0, 0, 0 };


    // printf("%d %d %d\n", hit_position_int.x, hit_position_int.y, hit_position_int.z);

    // selectionBoxModel.transform.translation.x = hit_position_int.x * S3L_F;
    // selectionBoxModel.transform.translation.y = hit_position_int.y * S3L_F;
    // selectionBoxModel.transform.translation.z = hit_position_int.z * S3L_F;

    // Doing a normal transform causes some kind of precision issue, I think?
    // It doesn't match up perfectly
    // Instead, change the positions of the vertices themselves
    
    // S3L_Model3D newModel;
    // static S3L_Unit storage[300 * 3];
    // translateModel(baseSelectionBoxModel, selectionBoxModel, storage, hit_position_int.x * S3L_F, hit_position_int.y * S3L_F, hit_position_int.z * S3L_F - 128);

  bool changed = false;

  static bool lastF1 = false;
  static bool lastF2 = false;

  bool F1 = keydown(KEY_F1);
  bool F2 = keydown(KEY_F2);

  // if ((F1 && !lastF1) || (F2 && !lastF2)) {
  S3L_Vec4 rayDir;
  S3L_rotationToDirections(scene.camera.transform.rotation, S3L_F, &rayDir, 0, 0);
  bool result = traceRay(getVoxel, mainPlayer->getCameraPos(), toVec3(rayDir), 3, hit_position, hit_position_int, hit_normal);
  if (!result) {
    hit_position_int = { -1, -1, -1 };
  }

  static vec<int, 3> lastPos = { -1, -1, -1 };
  static u16 last_state[12] = { 0 };

  if (lastPos != hit_position_int) {
    // Rollback the last one if it exists
    if (lastPos != vec<int, 3>{ -1, -1, -1 }) {
      colourTris(lastPos.x, lastPos.y, lastPos.z, levelModel, 0, [](u16 col, int index) -> u16 { return last_state[index]; });
    }

    if (result) {
      // printf("%d, %d, %d\n", hit_position_int.x, hit_position_int.y, hit_position_int.z);

      colourTris(hit_position_int.x, hit_position_int.y, hit_position_int.z, levelModel, last_state, [](u16 col, int index) -> u16 {
        // Lighten slightly
        return mix(lighten(col), col);
      });
    }
    lastPos = hit_position_int;

  }

  if (result) {
    bool shouldChange = false;
    vec<int, 3> pos;
    u8 block;
    if (F1 && !lastF1) {
      pos = hit_position_int;
      block = 0;
      shouldChange = true;
    } else if (F2 && !lastF2) {
      pos = hit_position_int + hit_normal;
      block = 2;
      shouldChange = true;
    }

    if (shouldChange) {
      vec3 fracPos = vec3(pos.x, pos.y, pos.z);
      BoundingBox toPlaceBB = BoundingBox(fracPos, fracPos + vec3(1));
      // mainPlayer->pos.y.v += 1;
      // mainPlayer->updateBounds();
      if (!mainPlayer->bounds.intersects(toPlaceBB)) {
        chunk->setBlock(pos, block);

        delete levelModel.triangles;
        delete levelModel.vertices;
        generateMesh(*chunk, levelModel);
      }
    }
  }

  lastF1 = keydown(KEY_F1);
  lastF2 = keydown(KEY_F2);

  static bool lastF3 = false;

  if (keydown(KEY_F3) && !lastF3) {
    debugHUD = !debugHUD;
  }

  lastF3 = keydown(KEY_F3);

  u32 total = prof_exec({
    azrp_perf_clear();
    perf_s3l_sort = prof_make();
    perf_s3l_project = prof_make();
    perf_transform = prof_make();
    perf_checkvis = prof_make();
    perf_addtri = prof_make();
    draw();
    // azrp_print2(1, 32, C_BLACK, "Time %dus", time);

    // Draw the crosshair
    azrp_rect((azrp_width / 2) - 1, (azrp_height / 2) - 5, 2, 10, AZRP_RECT_INVERT);
    azrp_rect((azrp_width / 2) - 5, (azrp_height / 2) - 1, 4, 2, AZRP_RECT_INVERT);
    azrp_rect((azrp_width / 2) + 1, (azrp_height / 2) - 1, 4, 2, AZRP_RECT_INVERT);

    // mainPlayer->setPos(vec3(0, 50, 0));
    vec3 pos = mainPlayer->getPos();

    int x = pos.x.ifloor();
    int xFrac = (pos.x * 100).ifloor() % 100;

    int y = pos.y.ifloor();
    int yFrac = (pos.y * 100).ifloor() % 100;

    int z = pos.z.ifloor();
    int zFrac = (pos.z * 100).ifloor() % 100;

    if (debugHUD) {
      azrp_print2(1, 1, C_WHITE, "x: %d.%02d, y: %d.%02d, z: %d.%02d", x, xFrac, y, yFrac, z, zFrac);
    }
    azrp_update();
  });
  // azrp_print(0, 16, C_BLACK, "cmdgen: %d", prof_time(azrp_perf_cmdgen));
  // azrp_print(0, 16 * 2, C_BLACK, "sort: %d", prof_time(azrp_perf_sort));
  // azrp_print(0, 16 * 3, C_BLACK, "shader: %d", prof_time(azrp_perf_shaders));
  // prof_enter(azrp_perf_cmdgen);
  // sleep_ms(1000);
  // prof_leave(azrp_perf_cmdgen);

  u32 s3l_sort = prof_time(perf_s3l_sort);
  u32 sort = prof_time(azrp_perf_sort);
  u32 render = prof_time(azrp_perf_render);
  u32 project = prof_time(perf_s3l_project);
  u32 transform = prof_time(perf_transform);
  u32 checkvis = prof_time(perf_checkvis);
  u32 addtri = prof_time(perf_addtri);
  u32 other = total - (s3l_sort + sort + render + project + transform + checkvis + addtri);

  // drect(0, DHEIGHT-40, DWIDTH-1, DHEIGHT-1, C_WHITE);
  // dprint(4, 189, C_BLACK, "s1: %d, s2: %d, rend: %d, proj: %d", s3l_sort, sort, render, project);
  // dprint(4, 209, C_BLACK, "t: %d, vis: %d, atri: %d, other: %d", transform, checkvis, addtri, other);
  // r61524_display(gint_vram, DHEIGHT-40, 40, R61524_DMA_WAIT);

  // azrp_render_fragments();
  // azrp_clear_commands();

  // while (SDL_PollEvent(&event))
  //   if (event.type == SDL_QUIT)
  //     running = 0;

  frame++;
}

bool frameCapEnabled = true;

static int callback_tick(volatile int *newFrameNeeded) {
  *newFrameNeeded = 1;
  return TIMER_CONTINUE;
}

void runMainLoop(void (*loop)(), int fps) {
  static volatile int newFrameNeeded = 1;
  int t = timer_configure(TIMER_ANY, 1000000 / fps, GINT_CALL(callback_tick, &newFrameNeeded));
  if (t >= 0) timer_start(t);

  while (1) {
    if (frameCapEnabled) {
      while (!newFrameNeeded) sleep();
    }
    newFrameNeeded = 0;
    loop();
    // while (1) {}
  }
}

bool getVoxel(double x, double y, double z) {
    // return a truthy value here for voxels that block the raycast
    return (y<0) ? 1 : 0;
}

int main(void) {
  prof_init();
  close(STDOUT_FILENO);
  open_generic(&stdouterr_type, NULL, STDOUT_FILENO);

  // size_t extraMemorySize = 5 * 1024 * 1024;
  // static kmalloc_arena_t extra_ram = { 0 };
	// extra_ram.name = "_extra_ram";
	// extra_ram.is_default = true;
	// extra_ram.start = (void*) 0x8c200000;
	// extra_ram.end = (void*) ((uintptr_t) 0x8c200000 + extraMemorySize);
	// kmalloc_init_arena(&extra_ram, true);
	// kmalloc_add_arena(&extra_ram);

  printf("Hello world!\n");

  azrp_config_scale(RES_SCALE);

  // levelModelInit();

  // Allocate vertices
  // S3L_Unit* vertices = new S3L_Unit[MAX_VERTICES * 3];
  // S3L_Index* triangles = new S3L_Index[MAX_TRIANGLES * 3];
  // int vertIndex = 0;
  // int triIndex = 0;


  chunk = new Chunk();
  World* world = new World(*chunk);
  mainPlayer = new Player(*chunk);
  mainPlayer->setPos(vec3(19.3, 5.0, 5.7));
  // printf("First: x: %d, y: %d, z: %d\n", mainPlayer->getPos().x.ifloor(), mainPlayer->getPos().y.ifloor(), mainPlayer->getPos().z.ifloor());
  // chunk->setBlock(0, 0, 0, 0);
  // printf("2: %d, y: %d, z: %d\n", mainPlayer->getPos().x.ifloor(), mainPlayer->getPos().y.ifloor(), mainPlayer->getPos().z.ifloor());

  for (int x = 0; x < 32; x++) {
    for (int y = 0; y < 32; y++) {
      for (int z = 0; z < 32; z++) {
        chunk->setBlock(x, y, z, 0);
      }
    }
  }

  // printf("3: %d, y: %d, z: %d\n", mainPlayer->getPos().x.ifloor(), mainPlayer->getPos().y.ifloor(), mainPlayer->getPos().z.ifloor());

  #include "data/exampleChunk.h"

  // chunk->setBlock(8, 4, 3, 0);

  generateMesh(*chunk, levelModel);
  // makeSelectionBox(baseSelectionBoxModel);

  S3L_sceneInit(&models[0], 1, &scene);

  // scene.camera.focalLength = S3L_F * 0.714074; // 70 degrees

  runMainLoop(mainLoop, 20);
  return 0;
}
