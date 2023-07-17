#include "raycast.h"

// Modified version of https://github.com/fenomas/fast-voxel-raycast/ using fixed point
// Has some slight precision issues but should be mostly fine

num32 abs(num32 in) {
  if (in < 0) return -in;
  return in;
}

bool traceRay_impl( bool (*getVoxel)(int x, int y, int z),
  num32 px, num32 py, num32 pz,
  num32 dx, num32 dy, num32 dz,
  num32 max_d, vec3& hit_pos, vec<int, 3>& hit_pos_int, vec<int, 3>& hit_norm) {

  // printf("dx: %f, dy: %f, dz: %f\n", dx.v / 65536.0, dy.v / 65536.0, dz.v / 65536.0);
  
  // consider raycast vector to be parametrized by t
  //   vec = [px,py,pz] + t * [dx,dy,dz]
  
  // algo below is as described by this paper:
  // http://www.cse.chalmers.se/edu/year/2010/course/TDA361/grid.pdf

  num32 inf;
  inf.v = 0x7fffffff;
  
  num32 t = 0.0;
  num32 ix = px.floor();
  num32 iy = py.floor();
  num32 iz = pz.floor();
   
  num32 stepx = (dx > 0) ? 1 : -1;
  num32 stepy = (dy > 0) ? 1 : -1;
  num32 stepz = (dz > 0) ? 1 : -1;
    
  // dx,dy,dz are already normalized
  num32 txDelta = dx == 0 ? 
    inf :
    abs(num32(1) / dx); // TODO: Is this precise enough?
  num32 tyDelta = dy == 0 ?
    inf :
    abs(num32(1) / dy);
  num32 tzDelta = dz == 0 ?
    inf :
    abs(num32(1) / dz);

  num32 xdist = (stepx > 0) ? (ix + num32(1) - px) : (px - ix);
  num32 ydist = (stepy > 0) ? (iy + num32(1) - py) : (py - iy);
  num32 zdist = (stepz > 0) ? (iz + num32(1) - pz) : (pz - iz);
    
  // location of nearest voxel boundary, in units of t 
  num32 txMax = txDelta == inf ? inf : txDelta * xdist;
  num32 tyMax = tyDelta == inf ? inf : tyDelta * ydist;
  num32 tzMax = tzDelta == inf ? inf : tzDelta * zdist;

  int steppedIndex = -1;
  
  // main loop along raycast vector
  while (t <= max_d) {
    
    // exit check
    bool b = getVoxel(ix.ifloor(), iy.ifloor(), iz.ifloor());
    if (b) {
			hit_pos.x = px + t * dx;
			hit_pos.y = py + t * dy;
			hit_pos.z = pz + t * dz;

      hit_pos_int.x = ix.ifloor();
      hit_pos_int.y = iy.ifloor();
      hit_pos_int.z = iz.ifloor();

			hit_norm.x = hit_norm.y = hit_norm.z = 0;
			if (steppedIndex == 0) hit_norm[0] = -stepx.ifloor();
			if (steppedIndex == 1) hit_norm[1] = -stepy.ifloor();
			if (steppedIndex == 2) hit_norm[2] = -stepz.ifloor();
      return b;
    }
    
    // advance t to next nearest voxel boundary
    if (txMax < tyMax) {
      if (txMax < tzMax) {
        ix += stepx;
        t = txMax;
        txMax += txDelta;
        steppedIndex = 0;
      } else {
        iz += stepz;
        t = tzMax;
        tzMax += tzDelta;
        steppedIndex = 2;
      }
    } else {
      if (tyMax < tzMax) {
        iy += stepy;
        t = tyMax;
        tyMax += tyDelta;
        steppedIndex = 1;
      } else {
        iz += stepz;
        t = tzMax;
        tzMax += tzDelta;
        steppedIndex = 2;
      }
    }

  }
  
  // no voxel hit found
	hit_pos.x = px + t * dx;
	hit_pos.y = py + t * dy;
	hit_pos.z = pz + t * dz;

  hit_pos_int.x = ix.ifloor();
  hit_pos_int.y = iy.ifloor();
  hit_pos_int.z = iz.ifloor();

	hit_norm.x = hit_norm.y = hit_norm.z = 0;

  return false;
}

// conform inputs

bool traceRay(bool (*getVoxel)(int x, int y, int z), vec3 origin, vec3 direction, num32 max_d, vec3& hit_pos, vec<int, 3>& hit_pos_int, vec<int, 3>& hit_norm) {
  num32 px = origin[0];
  num32 py = origin[1];
  num32 pz = origin[2];
  num32 dx = direction[0];
  num32 dy = direction[1];
  num32 dz = direction[2];
  num32 ds = (dx * dx + dy * dy + dz * dz).sqrt();

  if (ds == 0) {
    printf("ERR: Can't raycast along a zero vector\n");
		return false;
  }

  dx /= ds;
  dy /= ds;
  dz /= ds;
  return traceRay_impl(getVoxel, px, py, pz, dx, dy, dz, max_d, hit_pos, hit_pos_int, hit_norm);
}
