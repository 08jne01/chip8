#pragma once
#ifndef DISASSEMBLE_H
#define DISASSEMBLE_H
#include <stdint.h>
#include <stdbool.h>

extern void disassembleCode( const uint8_t* memory, const uint16_t end, const char* outFilename );
extern void disassembleInstruction( uint16_t opcode, char* str );


#endif