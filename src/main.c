#include <stdlib.h>
#include <string.h>

#include "pixler.h"
#include "common.h"

static void debug_message(const char* str){
	px_buffer_blit(NT_ADDR(0, 0, 25), str, strlen(str));
}

#define BG_COLOR 0x17
static const u8 PALETTE[] = {
	BG_COLOR, 0x1D, 0x30, 0x10,
	BG_COLOR, 0x1D, 0x31, 0x11,
	BG_COLOR, 0x1D, 0x3A, 0x1A,
	BG_COLOR, 0x1D, 0x1D, 0x1D,
	
	BG_COLOR, 0x16, 0x2A, 0x1D,
	BG_COLOR, 0x13, 0x28, 0x1D,
	BG_COLOR, 0x1D, 0x30, 0x10,
	BG_COLOR, 0x1D, 0x1D, 0x1D,
};

Gamepad pad1, pad2;

void read_gamepads(void){
	pad1.prev = pad1.value;
	pad1.value = joy_read(0);
	pad1.press = pad1.value & (pad1.value ^ pad1.prev);
	pad1.release = pad1.prev & (pad1.value ^ pad1.prev);
	
	pad2.prev = pad2.value;
	pad2.value = joy_read(1);
	pad2.press = pad2.value & (pad2.value ^ pad2.prev);
	pad2.release = pad2.prev & (pad2.value ^ pad2.prev);
}

void wait_noinput(void){
	while(joy_read(0) || joy_read(1)) px_wait_nmi();
}

static void darken(register const u8* palette, u8 shift){
	for(idx = 0; idx < 32; idx++){
		ix = palette[idx];
		ix -= shift << 4;
		if(ix > 0x40 || ix == 0x0D) ix = 0x1D;
		px_buffer_set_color(idx, ix);
	}
}

void fade_from_black(const u8* palette, u8 delay){
	darken(palette, 4);
	px_wait_frames(delay);
	darken(palette, 3);
	px_wait_frames(delay);
	darken(palette, 2);
	px_wait_frames(delay);
	darken(palette, 1);
	px_wait_frames(delay);
	darken(palette, 0);
}

static const u8 SYMBOL_ATTR[] = {0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01};
static const u8 SYMBOL_CHR[] = {0x8C, 0x8D, 0x8E, 0x8F, 0x8C, 0x8D, 0x8E, 0x8F, 0x9C, 0x9D, 0x9E, 0x9F, 0x9C, 0x9D, 0x9E, 0x9F};

static const u8 SYMBOL_GRID_POSX[] = {
	0,
	44 + 0*32,
	44 + 1*32,
	44 + 2*32,
	44 + 3*32,
	44 + 4*32,
	44 + 5*32,
	0,
};

static const u8 SYMBOL_GRID_POSY[] = {
	0,
	23 + 0*32,
	23 + 1*32,
	23 + 2*32,
	23 + 3*32,
	23 + 4*32,
	23 + 5*32,
	0,
};

static const u8 METATILES[] = {
	// Regular
	0x80, 0x81, 0x82, 0x83,
	0x90, 0x91, 0x92, 0x93,
	0xA0, 0xA1, 0xA2, 0xA3,
	0xB0, 0xB1, 0xB2, 0xB3,

	0x84, 0x85, 0x86, 0x87,
	0x94, 0x91, 0x96, 0x97,
	0xA4, 0xA5, 0xA6, 0xA7,
	0xB4, 0xB5, 0xB6, 0xB7,
	
	0x88, 0x89, 0x8A, 0x8B,
	0x98, 0x91, 0x9A, 0x9B,
	0xA8, 0xA9, 0xAA, 0xAB,
	0xB8, 0xB9, 0xBA, 0xBB,
	
	// Locked
	0xC0, 0xC1, 0xC2, 0x83,
	0xD0, 0x91, 0xD2, 0x93,
	0xE0, 0xE1, 0xE2, 0xA3,
	0xB0, 0xB1, 0xB2, 0xB3,
	
	0xC4, 0xC5, 0xC6, 0x87,
	0xD4, 0x91, 0xD6, 0x97,
	0xE4, 0xE5, 0xE6, 0xA7,
	0xB4, 0xB5, 0xB6, 0xB7,
	
	0xC8, 0xC9, 0xCA, 0x8B,
	0xD8, 0x91, 0xDA, 0x9B,
	0xE8, 0xE9, 0xEA, 0xAB,
	0xB8, 0xB9, 0xBA, 0xBB,
};

static const u8* METATILE_CHRS[] = {
	METATILES + 0x00, METATILES + 0x00, METATILES + 0x00, NULL, NULL, METATILES + 0x30, METATILES + 0x30, NULL,
	METATILES + 0x10, METATILES + 0x10, METATILES + 0x10, NULL, NULL, METATILES + 0x40, METATILES + 0x40, NULL,
	METATILES + 0x20, METATILES + 0x20, METATILES + 0x20, NULL, NULL, METATILES + 0x50, METATILES + 0x50, NULL,
};

#define PLAYER1_ATTR 0x55
#define PLAYER2_ATTR 0xAA

static const u8 METATILE_ATTRS[] = {
	0x00, PLAYER1_ATTR, PLAYER2_ATTR, NULL, NULL, PLAYER1_ATTR, PLAYER2_ATTR, NULL,
	0x00, PLAYER1_ATTR, PLAYER2_ATTR, NULL, NULL, PLAYER1_ATTR, PLAYER2_ATTR, NULL,
	0x00, PLAYER1_ATTR, PLAYER2_ATTR, NULL, NULL, PLAYER1_ATTR, PLAYER2_ATTR, NULL,
};

static const u16 TILE_NT_OFFS[] = {
	0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
	0x000, 0x000, 0x004, 0x008, 0x00C, 0x010, 0x014, 0x000,
	0x000, 0x080, 0x084, 0x088, 0x08C, 0x090, 0x094, 0x000,
	0x000, 0x100, 0x104, 0x108, 0x10C, 0x110, 0x114, 0x000,
	0x000, 0x180, 0x184, 0x188, 0x18C, 0x190, 0x194, 0x000,
	0x000, 0x200, 0x204, 0x208, 0x20C, 0x210, 0x214, 0x000,
	0x000, 0x280, 0x284, 0x288, 0x28C, 0x290, 0x294, 0x000,
	0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
};

static const u8 TILE_AT_OFFS[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x00,
	0x00, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x00,
	0x00, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x00,
	0x00, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x00,
	0x00, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x00,
	0x00, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static const u8 GRID_INDICES[] = {
	0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E,
	0x11, 0x12, 0x13, 0x14, 0x15, 0x16,
	0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E,
	0x21, 0x22, 0x23, 0x24, 0x25, 0x26,
	0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E,
	0x31, 0x32, 0x33, 0x34, 0x35, 0x36,
};

#define SYMBOL_TYPE_BITS 0x0F
#define SYMBOL_ANIM_BITS 0x30
#define SYMBOL_BORDER_BIT 0x80;
static u8 symbol_grid[64];

static u8 SYMBOL_GRID_INIT[] = {
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	0x0, 0x0, 0x8, 0x3, 0xB, 0x6, 0xE, 0x0,
	0x0, 0x1, 0x9, 0x4, 0xC, 0x7, 0xF, 0x0,
	0x0, 0x2, 0xA, 0x5, 0xD, 0x8, 0x0, 0x0,
	0x0, 0x3, 0xB, 0x6, 0xE, 0x9, 0x1, 0x0,
	0x0, 0x4, 0xC, 0x7, 0xF, 0xA, 0x2, 0x0,
	0x0, 0x5, 0xD, 0x8, 0x0, 0xB, 0x3, 0x0,
	0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
};

static u8 shuffle_cursor = 12;
static u8 shuffle_arr[16] = {0x6, 0xE, 0x9, 0x1, 0xC, 0x4, 0x7, 0xF, 0xA, 0x2, 0xD, 0x5};

#define TILE_PLAYER1_OWN_BIT 0x1
#define TILE_PLAYER2_OWN_BIT 0x2
#define TILE_OWNER_BITS (TILE_PLAYER1_OWN_BIT | TILE_PLAYER2_OWN_BIT)
#define TILE_LOCKED_BIT 0x4
#define TILE_ANIM_BITS 0x18
#define TILE_BORDER_BIT 0x80
#define TILE_PERIMETER_BITS (TILE_BORDER_BIT | TILE_OWNER_BITS)
static u8 tile_grid[64];

#define P TILE_PERIMETER_BITS
static const u8 TILE_GRID_INIT[64] = {
	P, P, P, P, P, P, P, P,
	P, 0, 0, 0, 0, 0, 0, P,
	P, 0, 0, 0, 0, 0, 0, P,
	P, 0, 0, 0, 0, 0, 0, P,
	P, 0, 0, 0, 0, 0, 0, P,
	P, 0, 0, 0, 0, 0, 0, P,
	P, 0, 0, 0, 0, 0, 0, P,
	P, P, P, P, P, P, P, P,
};

static u8 FOOBAR[512];

#define MOUSE_SPEED 0x280
static u16 mouse_x, mouse_y;
static u8 grid_idx, grid_val;
static u8 player_own_bit = TILE_PLAYER1_OWN_BIT;

static void blit_tile(){
	static u8 metatile;
	static u16 nt_offs;
	
	nt_offs = TILE_NT_OFFS[grid_idx];
	metatile = tile_grid[grid_idx];
	
	px_buffer_blit((NT_ADDR(0, 0, 0) + 0x00) + nt_offs, METATILE_CHRS[metatile] + 0x0, 4);
	px_buffer_blit((NT_ADDR(0, 0, 0) + 0x20) + nt_offs, METATILE_CHRS[metatile] + 0x4, 4);
	px_buffer_blit((NT_ADDR(0, 0, 0) + 0x40) + nt_offs, METATILE_CHRS[metatile] + 0x8, 4);
	px_buffer_blit((NT_ADDR(0, 0, 0) + 0x60) + nt_offs, METATILE_CHRS[metatile] + 0xC, 4);
	px_buffer_blit(AT_ADDR(0) + TILE_AT_OFFS[grid_idx], METATILE_ATTRS + metatile, 1);
}

static void debug_blit_grid(){
	for(ix = 1; ix <= 6; ix++){
		for(iy = 1; iy <= 6; iy++){
			grid_idx = ix + iy*8;
			blit_tile();
			px_coro_yield(0);
		}
	}
}

static u8 tile_at_mouse(void){
	u8 x = (mouse_x + 0x0000) >> 13;
	u8 y = (mouse_y + 0x1400) >> 13;
	
	if((u8)(x - 1) < 6 && (u8)(y - 1) < 6){
		return x + 8*y;
	} else {
		return 0;
	}
}

static void animate_up(){
	symbol_grid[grid_idx] -= 0x10;
	tile_grid[grid_idx] -= 0x8;
	blit_tile();
	px_coro_yield(0);
	px_coro_yield(0);
}

static void animate_down(){
	symbol_grid[grid_idx] += 0x10;
	tile_grid[grid_idx] += 0x8;
	blit_tile();
	px_coro_yield(0);
	px_coro_yield(0);
}

static u8 selected_tile_count;

static void click_tile(){
	if(tile_grid[grid_idx] & TILE_ANIM_BITS){
		animate_up();
		animate_up();
		selected_tile_count--;
	} else {
		animate_down();
		animate_down();
		selected_tile_count++;
	}
}

static const u8 CHECK_BITS[4] = {1, 2, 4, 8};
static const bool CHECK_ONE[16] = {0, 1, 1, 0, 1, 0, 0, 0, 1};
static bool check_match(void){
	ix = 0, iy = 0;
	for(grid_idx = 0; grid_idx < sizeof(symbol_grid); grid_idx++){
		grid_val = symbol_grid[grid_idx];
		if(grid_val & SYMBOL_ANIM_BITS){
			ix |= CHECK_BITS[(grid_val >> 0) & 0x3];
			iy |= CHECK_BITS[(grid_val >> 2) & 0x3];
		}
	}
	
	if(ix == 0xF && CHECK_ONE[iy]) return true;
	if(iy == 0xF && CHECK_ONE[ix]) return true;
	return false;
}

static void shuffle_tiles(){
	// Coppy symbols to shuffle array.
	for(grid_idx = 0; grid_idx < sizeof(tile_grid); grid_idx++){
		grid_val = symbol_grid[grid_idx];
		if(grid_val & SYMBOL_ANIM_BITS){
			shuffle_arr[shuffle_cursor & 0xF] = grid_val & SYMBOL_TYPE_BITS;
			shuffle_cursor++;
		}
	}
	
	// Swap elements. (Copy pasta... move on lol)
	ix = (shuffle_cursor + 0) & 0xF;
	iy = (ix + (rand8() & 0x7)) & 0xF;
	tmp = shuffle_arr[ix];
	shuffle_arr[ix] = shuffle_arr[iy];
	shuffle_arr[iy] = tmp;
	
	ix = (shuffle_cursor + 1) & 0xF;
	iy = (ix + (rand8() & 0x7)) & 0xF;
	tmp = shuffle_arr[ix];
	shuffle_arr[ix] = shuffle_arr[iy];
	shuffle_arr[iy] = tmp;
	
	ix = (shuffle_cursor + 2) & 0xF;
	iy = (ix + (rand8() & 0x7)) & 0xF;
	tmp = shuffle_arr[ix];
	shuffle_arr[ix] = shuffle_arr[iy];
	shuffle_arr[iy] = tmp;
	
	ix = (shuffle_cursor + 3) & 0xF;
	iy = (ix + (rand8() & 0x7)) & 0xF;
	tmp = shuffle_arr[ix];
	shuffle_arr[ix] = shuffle_arr[iy];
	shuffle_arr[iy] = tmp;
	
	// Copy symbols back to the grid.
	for(grid_idx = 0; grid_idx < sizeof(tile_grid); grid_idx++){
		if(symbol_grid[grid_idx] & SYMBOL_ANIM_BITS){
			symbol_grid[grid_idx] = shuffle_arr[shuffle_cursor & 0xF] + 0x20;
			shuffle_cursor++;
		}
	}
	
	// Move cursor back.
	shuffle_cursor -= 4;
}

static void assign_tiles(void){
	for(grid_idx = 0; grid_idx < sizeof(tile_grid); grid_idx++){
		grid_val = tile_grid[grid_idx];
		if((grid_val & TILE_ANIM_BITS) && !(grid_val & TILE_LOCKED_BIT)){
			tile_grid[grid_idx] = (grid_val & ~TILE_OWNER_BITS) | player_own_bit;
			blit_tile();
			px_coro_yield(0);
		}
	}
}

static void capture_tiles(void){
	for(grid_idx = 0; grid_idx < sizeof(tile_grid); grid_idx++){
		grid_val = tile_grid[grid_idx];
		
		// Skip the border tiles.
		if(grid_val & TILE_BORDER_BIT) continue;
		
		// Only process owned tiles.
		if(grid_val & TILE_OWNER_BITS){
			tmp  = (tile_grid - 1)[grid_idx]; // Left
			tmp &= (tile_grid + 1)[grid_idx]; // Right
			tmp &= (tile_grid - 8)[grid_idx]; // Up
			tmp &= (tile_grid + 8)[grid_idx]; // Down
			
			if(((grid_val ^ tmp) & TILE_OWNER_BITS) == 0){
				tile_grid[grid_idx] |= TILE_LOCKED_BIT;
			} else {
				tile_grid[grid_idx] &= ~TILE_LOCKED_BIT;
			}
			
			blit_tile();
			px_coro_yield(0);
		}
	}
}

static void raise_tiles(void){
	for(grid_idx = 0; grid_idx < sizeof(tile_grid); grid_idx++){
		if(tile_grid[grid_idx] & TILE_ANIM_BITS){
			animate_up();
			animate_up();
		}
	}
	
	selected_tile_count = 0;
}

static void gameloop_player(void){
	while(true){
		if(JOY_LEFT (pad1.value)) mouse_x -= MOUSE_SPEED;
		if(JOY_RIGHT(pad1.value)) mouse_x += MOUSE_SPEED;
		if(JOY_DOWN (pad1.value)) mouse_y += MOUSE_SPEED;
		if(JOY_UP   (pad1.value)) mouse_y -= MOUSE_SPEED;
		
		if(JOY_BTN_B(pad1.release)){
			grid_idx = tile_at_mouse();
			if(grid_idx) click_tile();
			
			if(selected_tile_count == 4){
				if(check_match()){
					shuffle_tiles();
					assign_tiles();
					capture_tiles();
					raise_tiles();
					break;
				} else {
					raise_tiles();
				}
			}
		}
		
		px_coro_yield(0);
	}
}

static u8 gameloop_coro[128];
static uintptr_t gameloop_body(uintptr_t _){
	mouse_x = 0x8000, mouse_y = 0x8000;
	
	while(true){
		debug_message("BLUE ");
		player_own_bit = TILE_PLAYER1_OWN_BIT;
		gameloop_player();
		
		debug_message("GREEN ");
		player_own_bit = TILE_PLAYER2_OWN_BIT;
		gameloop_player();
	}
}

static void game_screen(void){
	memcpy(tile_grid, TILE_GRID_INIT, sizeof(tile_grid));
	memcpy(symbol_grid, SYMBOL_GRID_INIT, sizeof(symbol_grid));
	
	rand_seed = 1234;
	
	// while(count < 4){
		
	// }
	
	px_ppu_sync_disable();{
		for(ix = 1; ix <= 6; ix++){
			for(iy = 1; iy <= 6; iy++){
				grid_idx = ix + iy*8;
				blit_tile();
				px_buffer_exec();
			}
		}
	} px_ppu_sync_enable();
	
	px_debug_hex_addr = NT_ADDR(0, 0, 24);
	PX.scroll_y = 480 - 16;
	PX.scroll_x = -36;
		
	fade_from_black(PALETTE, 4);
	
	px_coro_init(gameloop_body, gameloop_coro, sizeof(gameloop_coro));
	
	while(true){
		read_gamepads();
		px_debug_hex(selected_tile_count);
		if(px_coro_resume(gameloop_coro, 0)) break;
		
		px_spr(mouse_x >> 8, mouse_y >> 8, 0x02, 0x04);
		
		// This loop is SUPER expensive. Do I care?
		for(ix = 1; ix <= 6; ix++){
			for(iy = 1; iy <= 6; iy++){
				idx = ix + iy*8;
				tmp = symbol_grid[idx];
				px_spr(SYMBOL_GRID_POSX[ix] + (tmp >> 4), SYMBOL_GRID_POSY[iy] + (tmp >> 4), SYMBOL_ATTR[tmp & 0xF], SYMBOL_CHR[tmp & 0xF]);
			}
		}
		
		// Sloppy debug draw for shuffle array
		// for(idx = 4; idx < 16; idx++){
		// 	tmp = shuffle_arr[(shuffle_cursor + idx) & 0xF];
		// 	px_spr(0x08 + 16*idx, 0xD0 + idx, SYMBOL_ATTR[tmp & 0xF], SYMBOL_CHR[tmp & 0xF]);
		// }
		
		px_profile_start();
		px_profile_end();
		px_spr_end();
		px_wait_nmi();
	}
}

void main(void){
	px_uxrom_select(0);
	joy_install(nes_stdjoy_joy);
	
	px_bg_table(0);
	px_spr_table(0);
	
	// Black out the palette.
	for(idx = 0; idx < 32; idx++) px_buffer_set_color(idx, 0x1D);
	px_wait_nmi();
	
	// Decompress the tileset into character memory.
	px_lz4_to_vram(CHR_ADDR(0, 0), CHR0);
	
	sound_init(&SOUNDS);
	music_init(&MUSIC);
	
	game_screen();
}
