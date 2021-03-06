//
//  main.c
//  CHIP8
//
//  Created by Valtteri Koskivuori on 29/07/16.
//  Copyright © 2016 Valtteri Koskivuori. All rights reserved.
//

#include <SDL2/SDL.h>
#include "CPU.h"
#include <time.h>

bool emulatorRunning = true;

void (*signal(int signo, void (*func )(int)))(int);
typedef void sigfunc(int);
sigfunc *signal(int, sigfunc*);

void sig_handler(int sig) {
	if (sig == SIGINT) {
		emulatorRunning = false;
	}
}

void destroy_window(SDL_Window *window) {
	if (window != NULL) {
		SDL_DestroyWindow(window);
	}
}

void destroy_renderer(SDL_Renderer *renderer) {
	if (renderer != NULL) {
		SDL_DestroyRenderer(renderer);
	}
}

void destroy_texture(SDL_Texture *texture) {
	if (texture != NULL) {
		SDL_DestroyTexture(texture);
	}
}

void render(SDL_Renderer *renderer) {
	//Clear the window
	SDL_SetRenderDrawColor(renderer, 0x0, 0x0, 0x0, 0x0);
	SDL_RenderClear(renderer);
	
	//Render the data
	char data[2048] = {0};
	get_current_frame(data, 2048);
	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 0);
	for (int y = 0; y < 32; ++y) {
		for (int x = 0; x < 64; ++x) {
			if (data[(y*64) + x] == 0) {
				//Don't draw ¯\_(ツ)_/¯
			} else {
				SDL_RenderDrawPoint(renderer, x, y);
			}
		}
	}
	SDL_RenderPresent(renderer);
}

/*
 KEYMAPPING
 CHIP8 HEX MAP
 +-+-+-+-+
 |1|2|3|C|
 +-+-+-+-+
 |4|5|6|D|
 +-+-+-+-+
 |7|8|9|E|
 +-+-+-+-+
 |A|0|B|F|
 +-+-+-+-+
 HOST KEYBOARD
 +-+-+-+-+
 |1|2|3|4|
 +-+-+-+-+
 |Q|W|E|R|
 +-+-+-+-+
 |A|S|D|F|
 +-+-+-+-+
 |Z|X|C|V|
 +-+-+-+-+
 
 */

void set_input() {
	//Get keyboard input, then send that to the CPU
	
	SDL_PumpEvents();
	const Uint8 *keys = SDL_GetKeyboardState(NULL);
	byte input = 0xFF;
	
	//Prevent keys from sticking when pressing more than one at a time
	cpu_set_keys(0xFF);
	
	if (keys[SDL_SCANCODE_1]) {
		input = 0x1;
	}
	if (keys[SDL_SCANCODE_2]) {
		input = 0x2;
	}
	if (keys[SDL_SCANCODE_3]) {
		input = 0x3;
	}
	if (keys[SDL_SCANCODE_4]) {
		input = 0xC;
	}
	if (keys[SDL_SCANCODE_Q]) {
		input = 0x4;
	}
	if (keys[SDL_SCANCODE_W]) {
		input = 0x5;
	}
	if (keys[SDL_SCANCODE_E]) {
		input = 0x6;
	}
	if (keys[SDL_SCANCODE_R]) {
		input = 0xD;
	}
	if (keys[SDL_SCANCODE_A]) {
		input = 0x7;
	}
	if (keys[SDL_SCANCODE_S]) {
		input = 0x8;
	}
	if (keys[SDL_SCANCODE_D]) {
		input = 0x9;
	}
	if (keys[SDL_SCANCODE_F]) {
		input = 0xE;
	}
	if (keys[SDL_SCANCODE_Z]) {
		input = 0xA;
	}
	if (keys[SDL_SCANCODE_X]) {
		input = 0x0;
	}
	if (keys[SDL_SCANCODE_C]) {
		input = 0xB;
	}
	if (keys[SDL_SCANCODE_V]) {
		input = 0xF;
	}
	
	cpu_set_keys(input);
}



int main(int argc, char *argv[]) {
	int windowWidth = 64;
	int windowHeight = 32;
	int windowScale = 16; //How big the pixels are
	
	SDL_Window *window = NULL;
	SDL_Renderer *renderer = NULL;
	
	
	//Init SDL
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
		fprintf(stdout, "SDL couldn't initialize, error %s",SDL_GetError());
		return false;
	}
	
	SDL_WindowFlags flags = SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI;
	
	//Init window
	window = SDL_CreateWindow("CHIP-8 by VKoskiv 2016-2019",
							  SDL_WINDOWPOS_UNDEFINED,
							  SDL_WINDOWPOS_UNDEFINED,
							  windowWidth * (windowScale/2),
							  windowHeight * (windowScale/2), flags);
	if (window == NULL) {
		fprintf(stdout, "Window couldn't be created, error %s", SDL_GetError());
		return false;
	}
	
	//Init renderer
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	if (renderer == NULL) {
		fprintf(stdout, "Renderer couldn't be created, error %s", SDL_GetError());
		return false;
	}
	SDL_RenderSetScale(renderer, windowScale, windowScale);
	
	//Initialize the emulator
	cpu_initialize();
	
	if (argc == 2) {
		switch (cpu_load_rom(argv[1])) {
			case -1:
				fprintf(stderr, "Couldn't find the ROM file! (Check working dir/path)\n");
				return -1;
				break;
			case -2:
				fprintf(stderr, "ROM too big\n");
				return -1;
				break;
				
			default:
				fprintf(stderr, " bytes loaded.\n");
				break;
		}
	} else {
		fprintf(stderr, "Please provide a ROM filepath as argument!\n");
		return -1;
	}
	
	//Emulation loop
	do {
		//Check for CTRL-C
		if (signal(SIGINT, sig_handler) == SIG_ERR) {
			fprintf(stderr, "Couldn't catch SIGINT\n");
		}
		//Run the CPU cycle
		cpu_emulate_cycle();
		//Draw if needed
		if (cpu_is_drawflag_set()) {
			render(renderer);
		}
		//Check if CPU has halted
		if (cpu_has_halted()) {
			emulatorRunning = false;
		}
		//Set keyboard input to CPU
		set_input();
		//Delay
		struct timespec ts;
		int ms;
		
		if (CPU_DEBUG && delayEnabled) {
			ms = delayDebug;
		}
		else {
			ms = delayNormal;
		}
		ts.tv_sec = ms / 1000;
		ts.tv_nsec = (ms % 1000) * 1000000;
		nanosleep(&ts, NULL);
	} while (emulatorRunning);
	
	destroy_renderer(renderer);
	destroy_window(window);
	
	return 0;
}

