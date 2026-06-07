#pragma bank 255

#include "data/states_defines.h"
#include "states/vintage_adventure.h"

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

#ifndef ADVENTURE_CAMERA_DEADZONE
#define ADVENTURE_CAMERA_DEADZONE 8
#endif

#define ALIGNMENT_SPEED PLAYER.move_speed

UBYTE movement_locked = 0;
UBYTE direction_locked = 0;

void vintage_adventure_init(void) BANKED {
	// Set camera to follow player
	camera_offset_x = 0;
	camera_offset_y = 0;
	camera_deadzone_x = ADVENTURE_CAMERA_DEADZONE;
	camera_deadzone_y = ADVENTURE_CAMERA_DEADZONE;
}

void vintage_adventure_update(void) BANKED {
	actor_t *hit_actor;
	UBYTE tile_start, tile_end;
	direction_e new_dir = DIR_NONE;

	player_moving = FALSE;

	if (INPUT_RECENT_LEFT) {
		new_dir = DIR_LEFT;
	} else if (INPUT_RECENT_RIGHT) {
		new_dir = DIR_RIGHT;
	} else if (INPUT_RECENT_UP) {
		new_dir = DIR_UP;
	} else if (INPUT_RECENT_DOWN) {
		new_dir = DIR_DOWN;
	}
	
	player_moving = !movement_locked && (INPUT_LEFT || INPUT_RIGHT || INPUT_UP || INPUT_DOWN);

	if (player_moving) {
		upoint16_t new_pos;
		new_pos.x = PLAYER.pos.x + PLAYER.bounds.left;
		new_pos.y = PLAYER.pos.y + PLAYER.bounds.top;
		
		upoint16_t aligned_pos;
		aligned_pos.x = TILE_TO_SUBPX(SUBPX_TO_TILE(PLAYER.pos.x + PLAYER.bounds.left + PX_TO_SUBPX(4)));
		aligned_pos.y = TILE_TO_SUBPX(SUBPX_TO_TILE(PLAYER.pos.y + PLAYER.bounds.top + PX_TO_SUBPX(4)));
		
		switch (new_dir) {
			
			#if VINTAGE_MOVEMENT_STYLE
			
			case DIR_RIGHT:
				new_pos.x += PLAYER.move_speed;
				new_pos.y += CLAMP((WORD)aligned_pos.y - (WORD)new_pos.y, -ALIGNMENT_SPEED, ALIGNMENT_SPEED);
				break;
			
			case DIR_LEFT:
				new_pos.x -= PLAYER.move_speed;
				new_pos.y += CLAMP((WORD)aligned_pos.y - (WORD)new_pos.y, -ALIGNMENT_SPEED, ALIGNMENT_SPEED);
				break;
			
			case DIR_DOWN:
				new_pos.x += CLAMP((WORD)aligned_pos.x - (WORD)new_pos.x, -ALIGNMENT_SPEED, ALIGNMENT_SPEED);
				new_pos.y += PLAYER.move_speed;
				break;
			
			case DIR_UP:
				new_pos.x += CLAMP((WORD)aligned_pos.x - (WORD)new_pos.x, -ALIGNMENT_SPEED, ALIGNMENT_SPEED);
				new_pos.y -= PLAYER.move_speed;
				break;
			
			#else
			
			case DIR_RIGHT:
				if (new_pos.y == aligned_pos.y) {
					new_pos.x += PLAYER.move_speed;
				} else {
					new_pos.y += CLAMP((WORD)aligned_pos.y - (WORD)new_pos.y, -ALIGNMENT_SPEED, ALIGNMENT_SPEED);
					new_dir = aligned_pos.y > new_pos.y ? DIR_DOWN : DIR_UP;
				}
				break;
			
			case DIR_LEFT:
				if (new_pos.y == aligned_pos.y) {
					new_pos.x -= PLAYER.move_speed;
				} else {
					new_pos.y += CLAMP((WORD)aligned_pos.y - (WORD)new_pos.y, -ALIGNMENT_SPEED, ALIGNMENT_SPEED);
					new_dir = aligned_pos.y > new_pos.y ? DIR_DOWN : DIR_UP;
				}
				break;
			
			case DIR_DOWN:
				if (new_pos.x == aligned_pos.x) {
					new_pos.y += PLAYER.move_speed;
				} else {
					new_pos.x += CLAMP((WORD)aligned_pos.x - (WORD)new_pos.x, -ALIGNMENT_SPEED, ALIGNMENT_SPEED);
					new_dir = aligned_pos.x > new_pos.x ? DIR_RIGHT : DIR_LEFT;
				}
				break;
			
			case DIR_UP:
				if (new_pos.x == aligned_pos.x) {
					new_pos.y -= PLAYER.move_speed;
				} else {
					new_pos.x += CLAMP((WORD)aligned_pos.x - (WORD)new_pos.x, -ALIGNMENT_SPEED, ALIGNMENT_SPEED);
					new_dir = aligned_pos.x > new_pos.x ? DIR_RIGHT : DIR_LEFT;
				}
				break;
			
			#endif
		}
		
		new_pos.x -= PLAYER.bounds.left;
		new_pos.y -= PLAYER.bounds.top;

		// Step X
		tile_start = SUBPX_TO_TILE(PLAYER.pos.y + PLAYER.bounds.top);
		tile_end   = SUBPX_TO_TILE(PLAYER.pos.y + PLAYER.bounds.bottom) + 1;
		if (new_dir == DIR_RIGHT) {
			UBYTE tile_x = SUBPX_TO_TILE(new_pos.x + PLAYER.bounds.right);
			while (tile_start != tile_end) {
				
				if (tile_at(tile_x, tile_start) & COLLISION_LEFT) {
					new_pos.x = TILE_TO_SUBPX(tile_x) - PLAYER.bounds.right - 1;
					break;
				}
				tile_start++;
			}
			PLAYER.pos.x = MIN((image_width - PLAYER.bounds.right - 1) << 5, new_pos.x);
		} else {
			UBYTE tile_x = SUBPX_TO_TILE(new_pos.x + PLAYER.bounds.left);
			while (tile_start != tile_end) {
				if (tile_at(tile_x, tile_start) & COLLISION_RIGHT) {
					new_pos.x = TILE_TO_SUBPX(tile_x + 1) - PLAYER.bounds.left + 1;
					break;
				}
				tile_start++;
			}
			PLAYER.pos.x = MAX(0, (WORD)new_pos.x);
		}

		// Step Y
		tile_start = SUBPX_TO_TILE(PLAYER.pos.x + PLAYER.bounds.left);
		tile_end   = SUBPX_TO_TILE(PLAYER.pos.x + PLAYER.bounds.right) + 1;
		if (new_dir == DIR_DOWN) {
			UBYTE tile_y = SUBPX_TO_TILE(new_pos.y + PLAYER.bounds.bottom);
			while (tile_start != tile_end) {
				if (tile_at(tile_start, tile_y) & COLLISION_TOP) {
					new_pos.y = TILE_TO_SUBPX(tile_y) - PLAYER.bounds.bottom - 1;
					break;
				}
				tile_start++;
			}
			PLAYER.pos.y = new_pos.y;
		} else {
			UBYTE tile_y = SUBPX_TO_TILE(new_pos.y + PLAYER.bounds.top);
			while (tile_start != tile_end) {
				if (tile_at(tile_start, tile_y) & COLLISION_BOTTOM) {
					new_pos.y = TILE_TO_SUBPX(tile_y + 1) - PLAYER.bounds.top + 1;
					break;
				}
				tile_start++;
			}
			PLAYER.pos.y = new_pos.y;
		}
	}

	if (new_dir != DIR_NONE && !direction_locked) {
		
		uint8_t oldAnim = PLAYER.animation - PLAYER.dir;
		UBYTE oldFrame = actor_get_frame_offset(&PLAYER);
		bool dirChanged = new_dir != PLAYER.dir;
		
		actor_set_dir(&PLAYER, new_dir, player_moving);
		
		if (dirChanged && (PLAYER.animation - PLAYER.dir) == oldAnim) {
			
			actor_set_frame_offset(&PLAYER, oldFrame);
			
		}
		
	} else {
		actor_set_anim_idle(&PLAYER);
	}

	hit_actor = NULL;
	if (IS_FRAME_ODD) {
		// Check for trigger collisions
		if (trigger_activate_at_intersection(&PLAYER.bounds, &PLAYER.pos, FALSE)) {
			// Landed on a trigger
			return;
		}

		// Check for actor collisions
		hit_actor = actor_overlapping_player();
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
