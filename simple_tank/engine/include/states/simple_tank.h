#ifndef STATE_SIMPLE_TANK_H
#define STATE_SIMPLE_TANK_H

#include <gbdk/platform.h>

extern UBYTE tank_angle;
extern WORD tank_turn_speed;

void simple_tank_init(void) BANKED;
void simple_tank_update(void) BANKED;

#endif
