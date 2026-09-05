#ifndef COLLISION_H
#define COLLISION_H

#include "structs.h"
#include "texture.h"
#include "math/vec3.h"

#include <stdint.h>

int gjk(shape_t* shape1, shape_t* shape2);
vec3_t epa(shape_t* shape1, shape_t* shape2);

#endif // COLLISION_H
