#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <stdlib.h>
#include <SDL.h>
#define SDL_MAIN_HANDLED
#define WINDOW_WIDTH 600
const char* keycode[]= {"X","1","2","3","Q","W","E","A","S","D","Z","C","4", "R", "F", "V"};
Uint8 keystates[] = { SDL_SCANCODE_X, SDL_SCANCODE_1, SDL_SCANCODE_2, SDL_SCANCODE_3, SDL_SCANCODE_Q, SDL_SCANCODE_Q, 
SDL_SCANCODE_E, SDL_SCANCODE_A, SDL_SCANCODE_S, SDL_SCANCODE_D, SDL_SCANCODE_Z, SDL_SCANCODE_C, SDL_SCANCODE_4,
SDL_SCANCODE_R, SDL_SCANCODE_F, SDL_SCANCODE_V};

typedef struct CPUState {
	uint8_t V[16];
	uint16_t I;
	uint16_t* SP;
	uint16_t PC;
	uint8_t delay;
	uint8_t sound;
	uint8_t* memory;
	uint8_t screen[32][64];
}
CPUState;
CPUState* initCPU() {
	CPUState* s =(CPUState*)malloc(sizeof(CPUState));
	if(s!=NULL){
	s->memory = calloc(4096, 1);
	s->PC = 512;
	s->SP = calloc(15, sizeof(uint16_t));
	for (int i = 0; i < 16; i++)
	{
		s->V[i] = 0;
	}
	}
	return s;
}
void loadgame(CPUState* cpu, char filepath[]) {
	FILE* ptr = NULL;
	errno_t err;
	err = fopen_s(&ptr, filepath, "rb"); //"D:\\Downloads\\test_opcode.ch8"
	if (!err && ptr != NULL) {

		fseek(ptr, 0l, SEEK_END);
		int numbytes = ftell(ptr);
		fseek(ptr, 0l, SEEK_SET);

		uint8_t* arr = (uint8_t*)calloc(numbytes, sizeof(uint8_t));
		if (arr == NULL) {
			return;
		}
		fread(arr, sizeof(uint8_t), numbytes, ptr);
		if (numbytes < 4096 - 512) {
			for (int i = 0; i < numbytes; i++) {
				(cpu->memory)[512 + i] = arr[i];
			}
		}
		else {
			printf("File too large");

		}
	}
	else {
		printf("cannot open file");
	}
}
void initfont(CPUState* cpu) {
	uint8_t fonts[] = { 0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
0x20, 0x60, 0x20, 0x20, 0x70, // 1
0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
0x90, 0x90, 0xF0, 0x10, 0x10, // 4
0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
0xF0, 0x10, 0x20, 0x40, 0x40, // 7
0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
0xF0, 0x90, 0xF0, 0x90, 0x90, // A
0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
0xF0, 0x80, 0x80, 0x80, 0xF0, // C
0xE0, 0x90, 0x90, 0x90, 0xE0, // D
0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
0xF0, 0x80, 0xF0, 0x80, 0x80  // F
	};
	for (int i = 0; i < 16*5; i++)
	{
		(cpu->memory)[i] = fonts[i];
	}
}
void updatescreen(SDL_Renderer* renderer, CPUState* cpu) {
	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
	
	SDL_RenderClear(renderer);
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	for (int i = 0; i < 32; i++) {
		for (int j = 0;j < 64; j++) {
			if (cpu->screen[i][j] != 0) {
				SDL_RenderDrawPoint(renderer, j, i);
			}
		}
	}
	SDL_RenderPresent(renderer);
}
void execute(CPUState* cpu, uint8_t firstbyte, uint8_t secondbyte, SDL_Renderer* renderer, SDL_Event* event,Uint8** keystate) {
	uint8_t hexone = firstbyte >> 4; 
	switch (hexone) {
		case 0x0:
			if (firstbyte== 0x00 && secondbyte ==0xE0) {
				printf("Opcode: 00E0, Clear Screen \n");
				for (int i = 0; i < 32; i++)
				{
					for (int j = 0; j < 64; j++)
					{
						cpu->screen[i][j] = 1;
					}
				}
				updatescreen(renderer, cpu);
			}
			else if (firstbyte == 0x00 && secondbyte == 0xEE) {
				printf("Opcode: 00EE, return from subroutine \n");
				int i = 0;
				while (1) {
					if (cpu->SP[i] == 0) {
						cpu->PC = cpu->SP[i - 1];
						cpu->SP[i - 1] = 0;
						break;
					}
					i++;

				}
			}
			break;
		case 0x1:
			printf("Opcode: %02X%02X, jump to %X%X \n", firstbyte, secondbyte, firstbyte & 0x0F,secondbyte);
			cpu->PC = 256 * (firstbyte & 0x0f) + secondbyte;
			break;
		case 0x2:
			printf("Opcode: %02X%02X, calls subroutine at %X%X \n", firstbyte, secondbyte, firstbyte & 0x0F, secondbyte);
			int i = 0;
			while (1) {
				if (cpu->SP[i] == 0) {
					cpu->SP[i] = cpu->PC;
					cpu->PC = (firstbyte & 0x0F) * 256 + secondbyte;
					break;
				}
				i++;

			}
			break;
		case 0x3:
			printf("Opcode: %02X%02X, skips next instruction if V%X = %X \n", firstbyte, secondbyte, firstbyte & 0x0F, secondbyte);
			if (cpu->V[firstbyte & 0x0F]==secondbyte) {
				cpu->PC += 2;
			}
			break;
		case 0x4:
			printf("Opcode: %02X%02X, skips next instruction if V%X != %X \n", firstbyte, secondbyte, firstbyte & 0x0F, secondbyte);
			if (cpu->V[firstbyte & 0x0F] != secondbyte) {
				cpu->PC += 2;
			}
			break;
		case 0x5:
			printf("Opcode: %02X%02X, skips next instruction if V%X == V%X \n", firstbyte, secondbyte, firstbyte & 0x0F, secondbyte >> 4);
			if (cpu->V[firstbyte & 0x0F] == cpu->V[secondbyte>>4]) {
				cpu->PC += 2;
			}
			break;
		case 0x6:
			printf("Opcode: %02X%02X, sets V%X to %X \n", firstbyte, secondbyte, firstbyte & 0x0F, secondbyte);
			cpu->V[firstbyte & 0x0F] = secondbyte;
			break;
		case 0x7:
			printf("Opcode: %02X%02X, adds %X to V%X \n", firstbyte, secondbyte, secondbyte, firstbyte & 0x0F);
			cpu->V[firstbyte & 0x0f] += secondbyte;
			break;
		case 0x8:
			switch (secondbyte & 0x0F) {
			case 0x0:
				printf("Opcode: %02X%02X, sets V%X to value of V%X \n", firstbyte, secondbyte, firstbyte & 0x0F, secondbyte >> 4);
				cpu->V[firstbyte & 0x0f] = cpu->V[secondbyte >> 4];
				break;
			case 0x1:
				printf("Opcode: %02X%02X, sets V%X to V%X or V%X \n", firstbyte, secondbyte, firstbyte & 0x0F, firstbyte & 0x0F, secondbyte >> 4);
				cpu->V[firstbyte & 0x0f] = cpu->V[firstbyte&0x0F]|cpu->V[secondbyte >> 4];
				break;
			case 0x2:
				printf("Opcode: %02X%02X, sets V%X to V%X and V%X \n", firstbyte, secondbyte, firstbyte & 0x0F, firstbyte & 0x0F, secondbyte >> 4);
				cpu->V[firstbyte & 0x0f] = cpu->V[firstbyte & 0x0F] & cpu->V[secondbyte >> 4];
				break;
			case 0x3:
				printf("Opcode: %02X%02X, sets V%X to V%X xor V%X \n", firstbyte, secondbyte, firstbyte & 0x0F, firstbyte & 0x0F, secondbyte >> 4);
				cpu->V[firstbyte & 0x0f] = cpu->V[firstbyte & 0x0F] ^ cpu->V[secondbyte >> 4];
				break;
			case 0x4:
				printf("Opcode: %02X%02X, adds V%X to V%X, VF set to 1 if carry and 0 if not\n", firstbyte, secondbyte, secondbyte >> 4, firstbyte & 0x0F);
				if ((cpu->V[firstbyte & 0x0F] + cpu->V[secondbyte >> 4])>=256) {
					cpu->V[0x0F]= 1;
				}
				else {
					cpu->V[0xF] = 0;
				}
				cpu->V[firstbyte & 0x0F] += cpu->V[secondbyte >> 4];
				break;
			case 0x5:
				printf("Opcode: %02X%02X, V%X subtracted from V%X, VF 0 if borrow 1 if not \n", firstbyte, secondbyte, secondbyte >> 4, firstbyte & 0x0F);
				if (cpu->V[firstbyte & 0x0F] >= cpu->V[secondbyte >> 4]) {
					cpu->V[0xF] = 1;
				}
				else {
					cpu->V[0xF] = 0;
				}
				cpu->V[firstbyte & 0x0F] -= cpu->V[secondbyte >> 4];
				break;
			case 0x6:
				printf("Opcode: %02X%02X, Stores least significant bit of V%X in VF then shifts V%X to right by 1\n", firstbyte, secondbyte, firstbyte & 0x0F, firstbyte & 0x0);
				cpu->V[0xF] = cpu->V[firstbyte & 0x0F] & 0x01;
				cpu->V[firstbyte & 0x0F] = cpu->V[firstbyte & 0x0F] >> 1;
				break;
			case 0x7:
				printf("Opcode: %02X%02X, sets V%X to V%X - V%X, VF 0 if borrow 1 if not \n", firstbyte, secondbyte, firstbyte & 0x0F, secondbyte >> 4, firstbyte & 0x0);
				if (cpu->V[firstbyte & 0x0F] <= cpu->V[secondbyte >> 4]) {
					cpu->V[0xF] = 1;
				}
				else {
					cpu->V[0xF] = 0;
				}
				cpu->V[firstbyte & 0x0F] = cpu->V[secondbyte >> 4] - cpu->V[firstbyte&0x0F];
				break;
			case 0xE:
				printf("Opcode: %02X%02X, Stores most significant bit of V%d in VF then shifts V%d to left by 1 \n", firstbyte, secondbyte, firstbyte & 0x0F, firstbyte & 0x0);
				cpu->V[0xF] = cpu->V[firstbyte & 0x0F] >>3;
				cpu->V[firstbyte & 0x0F] = cpu->V[firstbyte & 0x0F] << 1;
				break;
			default:
				printf("invalid opcode");
			}
			break;
		case 0x9:
			if ((secondbyte & 0x0F) == 0x0) {
				printf("Opcode: %02X%02X, skips the next instruction if V%X does not equal V%X \n", firstbyte, secondbyte, firstbyte & 0x0F, secondbyte >> 4);
				if (cpu->V[firstbyte & 0x0F] != cpu->V[secondbyte >> 4]) {
					cpu->PC += 2;
				}
			}
			else {
				printf("invalid opcode");

			}
			break;
		case 0xA:
			printf("Opcode: %02X%02X, Sets I to adress %X%X \n", firstbyte, secondbyte, firstbyte & 0x0F, secondbyte);
			cpu->I = 256 * (firstbyte & 0x0F) + secondbyte;
			break;
		case 0xB:
			printf("Opcode: %02X%02X, jumps to the adress %X%Xplus V0 \n", firstbyte, secondbyte, firstbyte & 0x0F, secondbyte >> 4);
			cpu->PC= 256 * (firstbyte & 0x0F) + secondbyte+ cpu->V[0x0];
			break;
		case 0xC:
			printf("Opcode: %02X%02X, sets V%X to the result of a bitwise and operation on a random nuumber and %X \n", firstbyte, secondbyte, firstbyte & 0x0F, secondbyte);
			cpu->V[firstbyte & 0x0F] = (rand() % 256) & secondbyte;
			break;
		case 0xD:
			printf("Opcode: %02X%02X, draws a sprite at coordinate V%X,V%X that has width of 8 pixels and height of %X+1 pixels\n", firstbyte, secondbyte, firstbyte & 0x0F, secondbyte >> 4, secondbyte& 0x0F);
			int count = 0;
			int x = cpu->V[firstbyte & 0x0F] % 64;
			int y = cpu->V[secondbyte >> 4] % 32;
			cpu->V[0xf] = 0;
			for (int i = 0; i < (secondbyte & 0x0f); i++)
			{
				if (y + i >= 32) {
					break;
				}
				int xcount = 0;
				for (unsigned int j = 0x80; j !=0 ; j>>=1)
				{
					if (x + xcount >= 64) {
						break;
					}
					if ((cpu->memory[cpu->I + i] & j) != 0) {
						if (cpu->screen[y + i][x + xcount] != 0) {
							cpu->screen[y + i][x + xcount] = 0;
							
						}
						else {
							cpu->screen[y + i][x + xcount] = 1;
							cpu->V[0xf] = 1;
						}
						
					}
					xcount++;
				}
			}
			updatescreen(renderer, cpu);
			break;
		case 0xE:
			switch (secondbyte) {
			case 0x9E:
				printf("Opcode: %02X%02X, skips the next instruction if the key stored in V%X is pressed\n", firstbyte, secondbyte, firstbyte & 0x0F);
				/*
				while (SDL_PollEvent(event) != 0) {
					if ((*event).type == SDL_KEYDOWN) {
						char* keyname = SDL_GetKeyName((*event).key.keysym.sym);
						if (strcmp(keyname, keycode[cpu->V[firstbyte & 0x0F]])==0) {
							cpu->PC += 2;
						}
					}
				}
				*/
				while (SDL_PollEvent(event) != 0) {

				}
				
				if ((*keystate)[keystates[cpu->V[firstbyte & 0x0F]]]) {
					printf("skip \n");
					cpu->PC += 2;
				}
				break;
				
			case 0xA1:
				printf("Opcode: %02X%02X, skips the next instruction if the key stored in V%X is not pressed\n", firstbyte, secondbyte, firstbyte & 0x0F);
				while (SDL_PollEvent(event) != 0) {

				}
				
				if ((*keystate)[keystates[cpu->V[firstbyte & 0x0F]]]==0) {
					printf("skip \n");
					cpu->PC += 2;
				}
				break;
			default:
				printf("invalid opcode");
				break;
			}
			break;
		case 0xF:
			switch (secondbyte) {
			case 0x07:
				printf("Opcode: %02X%02X, sets V%X to the value of the delay timer \n", firstbyte, secondbyte, firstbyte & 0x0F);
				cpu->V[firstbyte & 0x0F] = cpu->delay;
				break;
			case 0x0A:
				printf("Opcode: %02X%02X, a key press is awaited, then stored in V%X \n", firstbyte, secondbyte, firstbyte & 0x0F);
				while (1) {
					if (SDL_PollEvent(event) == 1) {
						if ((*event).type == SDL_KEYDOWN) {
							char* keyname = SDL_GetKeyName((*event).key.keysym.sym);
							for (int i = 0; i < 16; i++) {
								if (strcmp(keyname, keycode[i])==0){
									
									cpu->V[firstbyte & 0x0F] = i;
									break;
								}
							}
							break;
						}
					}
					
				}
				break;
			case 0x15:
				printf("Opcode: %02X%02X, sets the delay timer to V%X \n", firstbyte, secondbyte, firstbyte & 0x0F);
				cpu->delay = cpu->V[firstbyte & 0x0F];
				break;
			case 0x18:
				printf("Opcode: %02X%02X, sets the sound timer to V%X \n", firstbyte, secondbyte, firstbyte & 0x0F);
				cpu->sound = cpu->V[firstbyte & 0x0F];
				break;
			case 0x1E:
				printf("Opcode: %02X%02X, adds V%X to I\n", firstbyte, secondbyte, firstbyte & 0x0F);
				cpu->I += cpu->V[firstbyte & 0x0F];
				break;
			case 0x29:
				printf("Opcode: %02X%02X, sets I  to the location of the sprite for the character in V%X  \n", firstbyte, secondbyte, firstbyte & 0x0F);
				cpu->I = cpu->V[firstbyte & 0x0F] * 5;
				break;
			case 0x33:
				printf("Opcode: %02X%02X, stores binary representation of V%X at I \n", firstbyte, secondbyte, firstbyte & 0x0F);
				int temp = cpu->V[firstbyte & 0x0F];
				cpu->memory[cpu->I + 2] = temp % 10;
				temp /= 10;
				cpu->memory[cpu->I + 1] = temp % 10;
				temp /= 10;
				cpu->memory[cpu->I ] = temp % 10;
				break;
			case 0x55:
				printf("Opcode: %02X%02X, stores V0 to V%X in memory starting at I \n", firstbyte, secondbyte, firstbyte & 0x0F);
				if ((firstbyte & 0x0F) == 0) {
					cpu->memory[cpu->I] = cpu->V[0];
				}
				else {
					for (int i = 0; i < (firstbyte & 0x0F)+1; i++)
					{
						cpu->memory[cpu->I + i] = cpu->V[i];
					}
				}
				break;
			case 0x65:
				printf("Opcode: %02X%02X, fills V0 to V%X with memory starting at I \n", firstbyte, secondbyte, firstbyte & 0x0F);
				if ((firstbyte & 0x0F) == 0) {
					cpu->V[0] = cpu->memory[cpu->I];
				}
				else {
					for (int i = 0; i < (firstbyte & 0x0F)+1; i++)
					{
						cpu->V[i] = cpu->memory[cpu->I + i];
					}
				}
				break;
			default:
				printf("invalid opcode");
			}
			break;
		default:
			printf("error, opcode not valid");
	}

}
int fetch(CPUState* cpu, SDL_Renderer* renderer, SDL_Event* event,Uint8** keystate){
	if (cpu->PC >= 4096){
		printf("done \n");
		return 0;
	}
	else {
		uint8_t firstbyte = (cpu->memory)[(*cpu).PC];
		uint8_t secondbyte = (cpu->memory)[(*cpu).PC + 1];
		
		(*cpu).PC += 2;
		execute(cpu, firstbyte, secondbyte, renderer, event,keystate);
		return 1;
	}
}


int main(int argc, char* argv[]) {
	srand(time(NULL));
	CPUState cpu = *initCPU();
	loadgame(&cpu, "D:\\Downloads\\Space Invaders.ch8");
	initfont(&cpu);
	SDL_Event event;
	SDL_Renderer* renderer;
	SDL_Window* window;
	SDL_Init(SDL_INIT_VIDEO);
	SDL_CreateWindowAndRenderer(640, 320, 0, &window, &renderer);
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderClear(renderer);
	SDL_RenderSetScale(renderer, 10, 10);
	SDL_RenderPresent(renderer);
	Uint8* keystate = SDL_GetKeyboardState(NULL);
	for (int i = 0; i < 4096; i++)
	{
		printf("%X ", (cpu.memory)[i]);
	}
	Uint32 timer = 0;
	while (fetch(&cpu, renderer, &event,&keystate)!=0)
	{
		if ((SDL_GetTicks()-timer)>15) {
			timer = SDL_GetTicks;
			if (cpu.delay > 0) {
				cpu.delay--;
			}
		}
		//delay decrement
		SDL_Delay(1);
	}
	while (1) {
		if (SDL_PollEvent(&event) && event.type == SDL_QUIT)
			break;
	}
}

