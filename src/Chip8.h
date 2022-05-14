#pragma once
#ifndef CHIP8_H
#define CHIP8_H

#include <stdint.h>
#include <stdbool.h>

typedef struct cpu_s
{
	uint8_t reg[16];
	uint8_t snd;
	uint8_t dly;
	uint16_t sp;
	uint16_t pc;
	uint16_t ptr;
} cpu_t;

typedef struct chip8_s
{
	cpu_t cpu;
	uint8_t memory[0x1000];
} chip8_t;

static inline uint16_t getOpcodeRawAddress( uint8_t* memory, uint16_t address )
{
	uint16_t lower;
	uint16_t upper;
	upper = memory[address];
	lower = memory[address + 1];
	return (upper << 8) | lower;
}

static inline uint16_t getOpcode( chip8_t* machine, uint16_t address )
{
	return getOpcodeRawAddress( machine->memory, address );
}


#define KEY_LOCATION 0xEF0
#define NUM_KEYS 16

extern chip8_t* createMachine();
extern bool peekCall( chip8_t* machine );
extern void doOneClock( chip8_t* machine );
extern void doOneInstructionDebug( chip8_t* machine, chip8_t* prevMachine );
extern void doOneClockDebug( chip8_t* machine, chip8_t* prevMachine );
extern uint8_t* readCode( const char* filename, int* len );
extern bool loadRom(uint8_t* memory, const char* filename);
extern void destroyMachine(chip8_t* machine);

#endif