#pragma bank 255

#include "data/states_defines.h"
#include "states/simple_car.h"

#include "actor.h"
#include "camera.h"
#include "collision.h"
#include "game_time.h"
#include "input.h"
#include "scroll.h"
#include "trigger.h"
#include "data_manager.h"
#include "rand.h"
#include "vm.h"
#include "math.h"

#ifndef INPUT_CAR_ACCELERATE
#define INPUT_CAR_ACCELERATE	INPUT_A
#endif
#ifndef INPUT_CAR_BRAKE
#define INPUT_CAR_BRAKE			INPUT_B
#endif

#ifndef CAR_CAMERA_DEADZONE
#define CAR_CAMERA_DEADZONE 8
#endif

UBYTE car_angle;
WORD car_forward_vel;
WORD car_acc;
WORD car_dec;
WORD car_brake;
WORD car_turn_radius;

void simple_car_init(void) BANKED {
	// Set camera to follow player
	camera_offset_x = 0;
	camera_offset_y = 0;
	camera_deadzone_x = CAR_CAMERA_DEADZONE;
	camera_deadzone_y = CAR_CAMERA_DEADZONE;
	
	if (PLAYER.dir == DIR_UP) {
		car_angle = 0;
	} else if (PLAYER.dir == DIR_RIGHT) {
		car_angle = ANGLE_90DEG;
	} else if (PLAYER.dir == DIR_DOWN) {
		car_angle = ANGLE_180DEG;
	} else if (PLAYER.dir == DIR_LEFT) {
		car_angle = ANGLE_270DEG;
	}
	actor_set_frames(&PLAYER, PLAYER.animations[0].start, PLAYER.animations[0].start + 1);
}

void simple_car_update(void) BANKED {
	actor_t *hit_actor;
	UBYTE tile_start, tile_end;
	
	if (INPUT_CAR_BRAKE) {
		if (INPUT_CAR_ACCELERATE) {
			car_forward_vel = car_forward_vel - CLAMP(car_forward_vel, -car_brake, car_brake);
		} else if (car_forward_vel > 0) {
			car_forward_vel = MAX(car_forward_vel - car_brake, 0);
		} else {
			car_forward_vel = MAX(car_forward_vel - car_acc, -PLAYER.move_speed << 4);
		}
	} else if (INPUT_CAR_ACCELERATE) {
		if (car_forward_vel >= 0) {
			car_forward_vel = MIN(car_forward_vel + car_acc, PLAYER.move_speed << 4);
		} else {
			car_forward_vel = MIN(car_forward_vel + car_brake, 0);
		}
	} else {
		car_forward_vel = car_forward_vel - CLAMP(car_forward_vel, -car_dec, car_dec);
	}
	
	player_moving = car_forward_vel != 0;
		
	if (player_moving) {
		point16_t new_pos;
		new_pos.x = PLAYER.pos.x;
		new_pos.y = PLAYER.pos.y;
		
		UBYTE turn_speed = car_forward_vel >> car_turn_radius;
		
		if (INPUT_LEFT) {
			car_angle -= turn_speed;
		} else if (INPUT_RIGHT) {
			car_angle += turn_speed;
		}
		
		if (car_angle >= ANGLE_315DEG || car_angle < ANGLE_45DEG) {
			PLAYER.dir = DIR_UP;
		} else if (car_angle < ANGLE_135DEG) {
			PLAYER.dir = DIR_RIGHT;
		} else if (car_angle < ANGLE_225DEG) {
			PLAYER.dir = DIR_DOWN;
		} else if (car_angle < ANGLE_315DEG) {
			PLAYER.dir = DIR_LEFT;
		}
		
		UBYTE vel_angle = car_angle;
		
		if (car_forward_vel < 0) {
			vel_angle += 128;
			point_translate_angle(&new_pos, vel_angle, -car_forward_vel >> 4);
		} else {
			point_translate_angle(&new_pos, vel_angle, car_forward_vel >> 4);
		}
		
		// Step X
		tile_start = (((PLAYER.pos.y >> 4) + PLAYER.bounds.top)    >> 3);
		tile_end   = (((PLAYER.pos.y >> 4) + PLAYER.bounds.bottom) >> 3) + 1;
		if (vel_angle < ANGLE_180DEG) {
			UBYTE tile_x = ((new_pos.x >> 4) + PLAYER.bounds.right) >> 3;
			while (tile_start != tile_end) {
				
				if (tile_at(tile_x, tile_start) & COLLISION_LEFT) {
					new_pos.x = (((tile_x << 3) - PLAYER.bounds.right) << 4) - 1;
					if (vel_angle > ANGLE_45DEG && vel_angle < ANGLE_135DEG) car_forward_vel = 0;
					break;
				}
				tile_start++;
			}
			PLAYER.pos.x = MIN((image_width - PLAYER.bounds.right - 1) << 4, new_pos.x);
		} else {
			UBYTE tile_x = ((new_pos.x >> 4) + PLAYER.bounds.left) >> 3;
			while (tile_start != tile_end) {
				if (tile_at(tile_x, tile_start) & COLLISION_RIGHT) {
					new_pos.x = ((((tile_x + 1) << 3) - PLAYER.bounds.left) << 4) + 1;
					if (vel_angle > ANGLE_225DEG && vel_angle < ANGLE_315DEG) car_forward_vel = 0;
					break;
				}
				tile_start++;
			}
			PLAYER.pos.x = MAX(0, (WORD)new_pos.x);
		}
		
		// Step Y
		tile_start = (((PLAYER.pos.x >> 4) + PLAYER.bounds.left)  >> 3);
		tile_end   = (((PLAYER.pos.x >> 4) + PLAYER.bounds.right) >> 3) + 1;
		if (vel_angle > ANGLE_90DEG && vel_angle < ANGLE_270DEG) {
			UBYTE tile_y = ((new_pos.y >> 4) + PLAYER.bounds.bottom) >> 3;
			while (tile_start != tile_end) {
				if (tile_at(tile_start, tile_y) & COLLISION_TOP) {
					new_pos.y = ((((tile_y) << 3) - PLAYER.bounds.bottom) << 4) - 1;
					if (vel_angle > ANGLE_135DEG && vel_angle < ANGLE_225DEG) car_forward_vel = 0;
					break;
				}
				tile_start++;
			}
			PLAYER.pos.y = new_pos.y;
		} else {
			UBYTE tile_y = (((new_pos.y >> 4) + PLAYER.bounds.top) >> 3);
			while (tile_start != tile_end) {
				if (tile_at(tile_start, tile_y) & COLLISION_BOTTOM) {
					new_pos.y = ((((UBYTE)(tile_y + 1) << 3) - PLAYER.bounds.top) << 4) + 1;
					if (vel_angle > ANGLE_315DEG || vel_angle < ANGLE_45DEG) car_forward_vel = 0;
					break;
				}
				tile_start++;
			}
			PLAYER.pos.y = new_pos.y;
		}
	}
	
	UBYTE frameCount = PLAYER.animations[0].end + 1 - PLAYER.animations[0].start;
	UBYTE frame = (UBYTE)(car_angle + ((256 / frameCount) >> 1)) / frameCount;
	
	actor_set_frames(&PLAYER, PLAYER.animations[0].start + frame, PLAYER.animations[0].start + frame + 1);
	
	hit_actor = NULL;
	if (IS_FRAME_ODD) {
		// Check for trigger collisions
		if (trigger_activate_at_intersection(&PLAYER.bounds, &PLAYER.pos, FALSE)) {
			// Landed on a trigger
			return;
		}
		
		// Check for actor collisions
		hit_actor = actor_overlapping_player(FALSE);
		if (hit_actor != NULL && hit_actor->collision_group) {
			player_register_collision_with(hit_actor);
		}
	}
	
	if (INPUT_A_PRESSED) {
		if (!hit_actor) {
			hit_actor = actor_in_front_of_player(8, TRUE);
		}
		if (hit_actor && !hit_actor->collision_group && hit_actor->script.bank) {
			script_execute(hit_actor->script.bank, hit_actor->script.ptr, 0, 1, 0);
		}
	}
}
