#ifndef STATE_SIMPLE_CAR_H
#define STATE_SIMPLE_CAR_H

#include <gbdk/platform.h>

extern UBYTE car_angle;
extern WORD car_forward_vel;
extern WORD car_acc;
extern WORD car_dec;
extern WORD car_brake;
extern WORD car_turn_radius;

void simple_car_init(void) BANKED;
void simple_car_update(void) BANKED;

#endif
