#ifndef STATE_VINTAGE_PLATFORM_H
#define STATE_VINTAGE_PLATFORM_H

#include <gbdk/platform.h>

void vintage_platform_init(void) BANKED;
void vintage_platform_update(void) BANKED;

extern WORD pl_vel_x;
extern WORD pl_vel_y;
extern WORD plat_climb_vel;
extern WORD plat_jump_vel;
extern WORD plat_grav;
extern WORD plat_max_fall_vel;

#endif
