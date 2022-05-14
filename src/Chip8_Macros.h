#pragma once
#ifndef CHIP_8_MACROS_H
#define CHIP_8_MACROS_H

#define getOpcodeUpper(opcode) ((opcode & 0xF000) >> 12)
#define getOpcodeX(opcode) ((opcode & 0x0F00u) >> 8u)
#define getOpcodeY(opcode) ((opcode & 0x00F0u) >> 4u)
#define getOpcodeN(opcode) (opcode & 0x000Fu)
#define getOpcodeNN(opcode) (opcode & 0x00FFu)
#define getOpcodeNNN(opcode) (opcode & 0x0FFFu)


#endif