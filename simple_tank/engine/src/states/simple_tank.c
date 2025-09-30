#pragma bank 255

#include "data/states_defines.h"
#include "states/simple_tank.h"

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

#ifndef TANK_CAMERA_DEADZONE
#define TANK_CAMERA_DEADZONE 8
#endif

UBYTE tank_angle;
WORD tank_turn_speed;

void simple_tank_init(void) BANKED {
	// Set camera to follow player
	camera_offset_x = 0;
	camera_offset_y = 0;
	camera_deadzone_x = TANK_CAMERA_DEADZONE;
	camera_deadzone_y = TANK_CAMERA_DEADZONE;
	
	if (PLAYER.dir == DIR_UP) {
		tank_angle = 0;
	} else if (PLAYER.dir == DIR_RIGHT) {
		tank_angle = ANGLE_90DEG;
	} else if (PLAYER.dir == DIR_DOWN) {
		tank_angle = ANGLE_180DEG;
	} else if (PLAYER.dir == DIR_LEFT) {
		tank_angle = ANGLE_270DEG;
	}
	actor_set_frames(&PLAYER, PLAYER.animations[0].start, PLAYER.animations[0].start + 1);
}

void simple_tank_update(void) BANKED {
	actor_t *hit_actor;
	UBYTE tile_start, tile_end;
	direction_e new_dir = DIR_NONE;
	
	WORD forward_vel = 0;

	if (INPUT_UP) {
		forward_vel = PLAYER.move_speed;
		player_moving = TRUE;
	} else if (INPUT_DOWN) {
		forward_vel = -PLAYER.move_speed;
		player_moving = TRUE;
	}
	
	if (INPUT_LEFT) {
		tank_angle -= tank_turn_speed;
	} else if (INPUT_RIGHT) {
		tank_angle += tank_turn_speed;
	}
	
	if (tank_angle >= ANGLE_315DEG || tank_angle < ANGLE_45DEG) {
		PLAYER.dir = DIR_UP;
	} else if (tank_angle < ANGLE_135DEG) {
		PLAYER.dir = DIR_RIGHT;
	} else if (tank_angle < ANGLE_225DEG) {
		PLAYER.dir = DIR_DOWN;
	} else if (tank_angle < ANGLE_315DEG) {
		PLAYER.dir = DIR_LEFT;
	}
	
	if (player_moving) {
		point16_t new_pos;
		new_pos.x = PLAYER.pos.x;
		new_pos.y = PLAYER.pos.y;
		
		UBYTE move_angle = tank_angle;
		
		if (forward_vel < 0) {
			move_angle += 128;
			point_translate_angle(&new_pos, move_angle, -forward_vel);
		} else {
			point_translate_angle(&new_pos, move_angle, forward_vel);
		}
		
		// Step X
		tile_start = (((PLAYER.pos.y >> 4) + PLAYER.bounds.top)    >> 3);
		tile_end   = (((PLAYER.pos.y >> 4) + PLAYER.bounds.bottom) >> 3) + 1;
		if (move_angle < ANGLE_180DEG) {
			UBYTE tile_x = ((new_pos.x >> 4) + PLAYER.bounds.right) >> 3;
			while (tile_start != tile_end) {
				
				if (tile_at(tile_x, tile_start) & COLLISION_LEFT) {
					new_pos.x = (((tile_x << 3) - PLAYER.bounds.right) << 4) - 1;
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
					break;
				}
				tile_start++;
			}
			PLAYER.pos.x = MAX(0, (WORD)new_pos.x);
		}
		
		// Step Y
		tile_start = (((PLAYER.pos.x >> 4) + PLAYER.bounds.left)  >> 3);
		tile_end   = (((PLAYER.pos.x >> 4) + PLAYER.bounds.right) >> 3) + 1;
		if (move_angle > ANGLE_90DEG && move_angle < ANGLE_270DEG) {
			UBYTE tile_y = ((new_pos.y >> 4) + PLAYER.bounds.bottom) >> 3;
			while (tile_start != tile_end) {
				if (tile_at(tile_start, tile_y) & COLLISION_TOP) {
					new_pos.y = ((((tile_y) << 3) - PLAYER.bounds.bottom) << 4) - 1;
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
					break;
				}
				tile_start++;
			}
			PLAYER.pos.y = new_pos.y;
		}
	}
	
	UBYTE frameCount = PLAYER.animations[0].end + 1 - PLAYER.animations[0].start;
	UBYTE frame = (UBYTE)(tank_angle + ((256 / frameCount) >> 1)) / frameCount;
	
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
