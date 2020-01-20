#include <stdlib.h>
#include <string.h>

#include "pixler.h"
#include "common.h"

#define BG_COLOR 0x31
static const u8 PALETTE[] = {
	BG_COLOR, 0x00, 0x10, 0x20,
	BG_COLOR, 0x06, 0x16, 0x26,
	BG_COLOR, 0x09, 0x19, 0x29,
	BG_COLOR, 0x01, 0x11, 0x21,
	
	BG_COLOR, 0x00, 0x10, 0x20,
	BG_COLOR, 0x06, 0x16, 0x26,
	BG_COLOR, 0x09, 0x19, 0x29,
	BG_COLOR, 0x01, 0x11, 0x21,
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

static Gamestate splash_screen(void){
	register u8 x = 32, y = 32;
	register s16 sin = 0, cos = 0x3FFF;
	
	px_ppu_sync_disable();{
		// Load the splash tilemap into nametable 0.
		px_lz4_to_vram(NT_ADDR(0, 0, 0), MAP_SPLASH);
	} px_ppu_sync_enable();
	
	music_play(0);
	
	fade_from_black(PALETTE, 4);
	
	while(true){
		read_gamepads();
		if(JOY_LEFT (pad1.value)) x -= 1;
		if(JOY_RIGHT(pad1.value)) x += 1;
		if(JOY_DOWN (pad1.value)) y += 1;
		if(JOY_UP   (pad1.value)) y -= 1;
		if(JOY_BTN_A(pad1.press)) sound_play(SOUND_JUMP);
		
		// Draw a sprite.
		px_spr(x, y, PX_SPR_BEHIND, 'o');
		
		PX.scroll_y = 480 + (sin >> 9);
		sin += cos >> 6;
		cos -= sin >> 6;
		
		px_spr_end();
		px_wait_nmi();
	}
	
	return splash_screen();
}

void main(void){
	// Set up CC65 joystick driver.
	joy_install(nes_stdjoy_joy);
	
	// Set which tiles to use for the background and sprites.
	px_bg_table(0);
	px_spr_table(0);
	
	// Not using bank switching, but a good idea to set a reliable value at boot.
	px_uxrom_select(0);
	
	// Black out the palette.
	for(idx = 0; idx < 32; idx++) px_buffer_set_color(idx, 0x1D);
	px_wait_nmi();
	
	// Decompress the tileset into character memory.
	px_lz4_to_vram(CHR_ADDR(0, 0), CHR0);
	
	sound_init(SOUNDS);
	music_init(MUSIC);
	
	// Jump to the splash screen state.
	splash_screen();
}