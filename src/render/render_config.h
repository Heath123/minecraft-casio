#define TEXTURES 0 // whether to use textures for the level
#define FOG 0

#define RES_SCALE 2

#define S3L_RESOLUTION_X (396 / RES_SCALE)
#define S3L_RESOLUTION_Y (224 / RES_SCALE)

#define S3L_NEAR_CROSS_STRATEGY 1

#if TEXTURES
  #define S3L_PERSPECTIVE_CORRECTION 0
#else
  #define S3L_PERSPECTIVE_CORRECTION 0
#endif

#define S3L_NEAR (S3L_F / 5)

#define S3L_USE_WIDER_TYPES 0
#define S3L_FLAT 1
#define S3L_SORT 1
#define S3L_Z_BUFFER 0
#define S3L_COMPUTE_DEPTH 0
#define S3L_MAX_TRIANGES_DRAWN 2048
