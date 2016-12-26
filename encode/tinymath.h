#ifndef _TINYMATH_H_
#define _TINYMATH_H_

#include <xdc/std.h>

#define TAN_MAP_RES         0.003921569
#define TAN_MAP_SIZE        256
#define PIII                3.14159
#define TWOPI               6.2831853
#define PIII_HALF           1.57079


Float chasin(Float x0);
Float chacos(Float x0);
Float chatan(Float x0);
Float my_abs(Float f);
Float fast_atan2(Float y, Float x);
Float fast_atan(Float at);


#endif
