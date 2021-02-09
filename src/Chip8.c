#include "Chip8.h"
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>

static uint16_t s_opcode;
static uint8_t s_subInstruction = 0;

#define setBit(A, addr,k)		((A)[addr + (k/8)] |= (1 << (k%8)))
#define clearBit(A, addr,k)	((A)[addr + (k/8)] &=  ~(1 << (k%8)))
#define getBit(A, addr,k)		((A)[addr + (k/8)] & (1 << (k%8)))

#define getBit8(A, addr, k)	(A[addr + (k/8)] & (1 << (k%8)))

#define getOpcodeUpper(opcode) ((opcode & 0xF000) >> 12)
#define getOpcodeX(opcode) ((opcode & 0x0F00u) >> 8u)
#define getOpcodeY(opcode) ((opcode & 0x00F0u) >> 4u)
#define getOpcodeN(opcode) (opcode & 0x000Fu)
#define getOpcodeNN(opcode) (opcode & 0x00FFu)
#define getOpcodeNNN(opcode) (opcode & 0x0FFFu)

#define getX() getOpcodeX(s_opcode)
#define getY() getOpcodeY(s_opcode)
#define getN() getOpcodeN(s_opcode)
#define getNN() getOpcodeNN(s_opcode)
#define getNNN() getOpcodeNNN(s_opcode)

#define registerX machine->cpu.reg[getX()]
#define registerY machine->cpu.reg[getY()]
#define registerFlag machine->cpu.reg[0xF]

#define VIDEO_MEM_LOCATION 0xF00
#define CALL_STACK_LOCATION 0xEA0
#define CODE_START_LOCATION 0x200
#define FONTSET_LOCATION 0x50

#define FONTSET_SET_SIZE 80
uint8_t fontset[FONTSET_SET_SIZE] =
{
	0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
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




typedef void (*INSTRUCTION)(chip8_t*);
typedef void (*INSTRUCTION_TXT)(uint16_t opcode, char* str);


static void OP0( chip8_t* machine );
static void OP1( chip8_t* machine );
static void OP2( chip8_t* machine );
static void OP3( chip8_t* machine );
static void OP4( chip8_t* machine );
static void OP5( chip8_t* machine );
static void OP6( chip8_t* machine );
static void OP7( chip8_t* machine );
static void OP8( chip8_t* machine );
static void OP9( chip8_t* machine );
static void OPA( chip8_t* machine );
static void OPB( chip8_t* machine );
static void OPC( chip8_t* machine );
static void OPD( chip8_t* machine );
static void OPE( chip8_t* machine );
static void OPF( chip8_t* machine );

static void OP0_TXT( uint16_t opcode, char* str );
static void OP1_TXT( uint16_t opcode, char* str );
static void OP2_TXT( uint16_t opcode, char* str );
static void OP3_TXT( uint16_t opcode, char* str );
static void OP4_TXT( uint16_t opcode, char* str );
static void OP5_TXT( uint16_t opcode, char* str );
static void OP6_TXT( uint16_t opcode, char* str );
static void OP7_TXT( uint16_t opcode, char* str );
static void OP8_TXT( uint16_t opcode, char* str );
static void OP9_TXT( uint16_t opcode, char* str );
static void OPA_TXT( uint16_t opcode, char* str );
static void OPB_TXT( uint16_t opcode, char* str );
static void OPC_TXT( uint16_t opcode, char* str );
static void OPD_TXT( uint16_t opcode, char* str );
static void OPE_TXT( uint16_t opcode, char* str );
static void OPF_TXT( uint16_t opcode, char* str );

static void waitKey( chip8_t* machine, int key );

static INSTRUCTION s_instructions[16] = {
	OP0,OP1,OP2,OP3,OP4,OP5,OP6,OP7,
	OP8,OP9,OPA,OPB,OPC,OPD,OPE,OPF
};

static INSTRUCTION_TXT s_instructionsTXT[16] = {
	OP0_TXT,OP1_TXT,OP2_TXT,OP3_TXT,OP4_TXT,OP5_TXT,OP6_TXT,OP7_TXT,
	OP8_TXT,OP9_TXT,OPA_TXT,OPB_TXT,OPC_TXT,OPD_TXT,OPE_TXT,OPF_TXT
};

chip8_t* createMachine()
{

	chip8_t* machine = malloc( sizeof( chip8_t ) );

	if ( machine )
	{
		memset( machine->cpu.reg, 0x0, 16 );
		machine->cpu.dly = 0;
		machine->cpu.snd = 0;
		machine->cpu.ptr = 0;
		machine->cpu.pc = CODE_START_LOCATION;
		machine->cpu.sp = CALL_STACK_LOCATION;
		memset( machine->memory + VIDEO_MEM_LOCATION, 0x0, 256 );
		memset( machine->memory + KEY_LOCATION, 0x0, NUM_KEYS );
		memcpy( machine->memory + FONTSET_LOCATION, fontset, FONTSET_SET_SIZE );
	}

	

	return machine;
}

void loadRom( chip8_t* machine, const char* filename )
{
	FILE* file;
	file = fopen( filename, "rb" );

	if ( ! file )
	{
		fprintf( stderr, "ERROR: Could not open file.\n" );
		exit( 1 );
	}

	unsigned char* buffer = malloc( 0x800 );

	if ( ! buffer )
	{
		fprintf( stderr, "ERROR: Could not create buffer.\n" );
		fclose( file );
		free( buffer );
		return;
	}

	int len = fread( buffer, 1, 0x800, file );
	memcpy( machine->memory + 0x200, buffer, len );

	free( buffer );
	fclose( file );
}

bool peekCall( chip8_t* machine )
{
	return getOpcodeUpper( getOpcode( machine, machine->cpu.pc ) ) == 0x2;
}

void doOneClock( chip8_t* machine )
{
	s_subInstruction = (s_subInstruction + 1) % 3;

	static int counter = 0;
	counter = (counter + 1) % 10;

	machine->cpu.dly -= machine->cpu.dly != 0 && counter == 0;
	machine->cpu.snd -= machine->cpu.snd != 0 && counter == 0;

	switch ( s_subInstruction )
	{
	case 0:
		machine->cpu.pc += 2;
		break;
	case 1:
		s_opcode = getOpcode( machine, machine->cpu.pc );
		break;
	case 2:
		s_instructions[getOpcodeUpper(s_opcode)]( machine );
	}
}

void doOneClockDebug( chip8_t* machine, chip8_t* prevMachine )
{
	memcpy( prevMachine, machine, sizeof( chip8_t ) );
	doOneClock( machine );
}

void doOneInstructionDebug( chip8_t* machine, chip8_t* prevMachine, bool stepIn )
{
	memcpy( prevMachine, machine, sizeof( chip8_t ) );

	do
	{
		doOneClock( machine );
	} while ( s_subInstruction != 0 );
}

void destroyMachine( chip8_t* machine )
{
	free( machine );
}

void OP0( chip8_t* machine )
{
	if ( getN() == 0 )
	{
		//Clear display
		memset( machine->memory + VIDEO_MEM_LOCATION, 0x0, 256 );
	}
	else if ( getN() == 0xE )
	{
		//Return from routine.
		machine->cpu.pc = *(uint16_t*)(machine->memory + machine->cpu.sp);
		machine->cpu.sp -= 2;
	}
}

void OP1( chip8_t* machine )
{
	machine->cpu.pc = getNNN() - 2;
}

void OP2( chip8_t* machine )
{
	machine->cpu.sp += 2;
	*(uint16_t*)(machine->memory + machine->cpu.sp) = machine->cpu.pc;
	machine->cpu.pc = getNNN() - 2;
}

void OP3( chip8_t* machine )
{
	machine->cpu.pc += (registerX == getNN()) * 2;
}

void OP4( chip8_t* machine )
{
	machine->cpu.pc += (registerX != getNN()) * 2;
}

void OP5( chip8_t* machine )
{
	machine->cpu.pc += (registerX == registerY) * 2;
}

void OP6( chip8_t* machine )
{
	registerX = getNN();
}

void OP7( chip8_t* machine )
{
	registerX += getNN();
}

void OP8( chip8_t* machine )
{
	uint16_t sum;
	switch ( getN() )
	{
	case 0x0:
		registerX = registerY;
		break;
	case 0x1:
		registerX |= registerY;
		break;
	case 0x2:
		registerX &= registerY;
		break;
	case 0x3:
		registerX ^= registerY;
		break;
	case 0x4:
		sum = registerX + registerY;
		registerFlag = (sum & 0xFF00u) != 0;
		registerX = sum;
		break;
	case 0x5:
		sum = registerX - registerY;
		registerFlag = (sum & 0xFF00u) != 0;
		registerX = sum;
		break;
	case 0x6:
		registerFlag = (0x1u & registerX);
		registerX >>= 1;
		break;
	case 0x7:
		sum = registerY - registerX;
		registerFlag = (sum & 0xFF00u) != 0;
		registerX = sum;
		break;
	case 0xE:
		registerFlag = (0x10000000u & registerX) != 0;
		registerX <<= 1;
		break;
	}
}

void OP9( chip8_t* machine )
{
	machine->cpu.pc += (registerX != registerY) * 2;
}

void OPA( chip8_t* machine )
{
	machine->cpu.ptr = getNNN();
}

void OPB( chip8_t* machine )
{
	machine->cpu.pc = getNNN() + machine->cpu.reg[0] - 2;
}

void OPC( chip8_t* machine )
{
	uint8_t num = rand() & 0xFF;
	registerX = num & getNN();
}

void OPD( chip8_t* machine )
{
	uint8_t x = registerX;
	uint8_t y = registerY;
	uint8_t n = getN();

	registerFlag = 0;

	uint16_t size = n * 8;
	for ( int i = 0; i < size; i++ )
	{
		if ( getBit( machine->memory, machine->cpu.ptr, i ) )
		{
			uint16_t xcoord = x + 7 - (i % 8);
			uint16_t ycoord = y + (i / 8);

			xcoord = xcoord % 64;
			ycoord = ycoord % 32;

			int index = xcoord + ycoord * 64;
			//int index = x + (i % 8) + (y + (i/8)) * 64;
			if ( getBit( machine->memory, VIDEO_MEM_LOCATION, index ) )
			{
				clearBit( machine->memory, VIDEO_MEM_LOCATION, index );
				registerFlag = 1;
			}
			else
				setBit( machine->memory, VIDEO_MEM_LOCATION, index );
		}
	}

}

void OPE( chip8_t* machine )
{
	if ( getN() == 0xE )
	{
		machine->cpu.pc += (machine->memory[KEY_LOCATION + registerX] == 1) * 2;

	}
	else if ( getN() == 0x1 )
	{
		machine->cpu.pc += (machine->memory[KEY_LOCATION + registerX] == 0) * 2 ;
	}
	
}

void OPF( chip8_t* machine )
{
	switch ( getNN() )
	{
	case 0x07:
		registerX = machine->cpu.dly;
		return;
	case 0x0A:
		for ( int i = 0; i < NUM_KEYS; i++ )
		{
			if ( machine->memory[KEY_LOCATION + i] )
			{
				registerX = machine->memory[KEY_LOCATION + i];
				return;
			}
		}
		machine->cpu.pc -= 2; //wait
		return;

	case 0x15:
		machine->cpu.dly = registerX;
		return;
	case 0x18:
		machine->cpu.snd = registerX;
		return;
	case 0x1E:
		machine->cpu.ptr += registerX;
		return;
	case 0x29:
		machine->cpu.ptr = FONTSET_LOCATION + (5 * registerX);
		return;
	case 0x33:
	{
		uint8_t value = registerX;
		machine->memory[machine->cpu.ptr + 2] = value % 10;
		value /= 10;

		machine->memory[machine->cpu.ptr + 1] = value % 10;
		value /= 10;

		machine->memory[machine->cpu.ptr] = value % 10;
		return;
	}
	case 0x55:
		for ( int i = 0; i <= getX(); i++ )
		{
			machine->memory[machine->cpu.ptr + i] = machine->cpu.reg[i];
		}
		return;
	case 0x65:
		for ( int i = 0; i <= getX(); i++ )
		{
			machine->cpu.reg[i] = machine->memory[machine->cpu.ptr + i];
		}
		return;
	}
}


void disassembleInstruction( uint16_t opcode, char* str )
{
	int index = getOpcodeUpper( opcode );
	if ( index >= 0 && index < sizeof( s_instructions ) / sizeof( INSTRUCTION_TXT ) )
	{
		s_instructionsTXT[index]( opcode, str );
	}
	else
	{
		sprintf( str, "???" );
	}
}

void OP0_TXT( uint16_t opcode, char* str )
{
	if ( getOpcodeN(opcode) == 0 )
	{
		sprintf( str, "CLR" );
	}
	else if ( getOpcodeN(opcode) == 0xE )
	{
		sprintf( str, "RET" );
	}
	else
	{
		sprintf( str, "???" );
	}
}

void OP1_TXT( uint16_t opcode, char* str )
{
	sprintf( str, "JMP  0x%03X", getOpcodeNNN(opcode) );
}

void OP2_TXT( uint16_t opcode, char* str )
{
	sprintf( str, "CALL 0x%03X", getOpcodeNNN( opcode ) );
}

void OP3_TXT( uint16_t opcode, char* str )
{
	sprintf( str, "SE   V%01X, 0x%02X", getOpcodeX( opcode ), getOpcodeNN( opcode ) );
}

void OP4_TXT( uint16_t opcode, char* str )
{
	sprintf( str, "SNE  V%01X, 0x%02X", getOpcodeX( opcode ), getOpcodeNN( opcode ) );
}

void OP5_TXT( uint16_t opcode, char* str )
{
	sprintf( str, "SE  V%01X, V%01X", getOpcodeX( opcode ), getOpcodeY( opcode ) );
}

void OP6_TXT( uint16_t opcode, char* str )
{
	sprintf( str, "MOV  V%01X, 0x%02X", getOpcodeX( opcode ), getOpcodeNN( opcode ) );
}

void OP7_TXT( uint16_t opcode, char* str )
{
	sprintf( str, "ADD  V%01X, 0x%02X", getOpcodeX( opcode ), getOpcodeNN( opcode ) );
}

void OP8_TXT( uint16_t opcode, char* str )
{
	switch ( getOpcodeN( opcode ) )
	{
	case 0x0:
		sprintf( str, "MOV  V%01X, V%01X", getOpcodeX( opcode ), getOpcodeY( opcode ) );
		return;
	case 0x1:
		sprintf( str, "OR   V%01X, V%01X", getOpcodeX( opcode ), getOpcodeY( opcode ) );
		return;
	case 0x2:
		sprintf( str, "AND  V%01X, V%01X", getOpcodeX( opcode ), getOpcodeY( opcode ) );
		return;
	case 0x3:
		sprintf( str, "XOR  V%01X, V%01X", getOpcodeX( opcode ), getOpcodeY( opcode ) );
		return;
	case 0x4:
		sprintf( str, "ADD  V%01X, V%01X", getOpcodeX( opcode ), getOpcodeY( opcode ) );
		return;
	case 0x5:
		sprintf( str, "SUB  V%01X, V%01X", getOpcodeX( opcode ), getOpcodeY( opcode ) );
		return;
	case 0x6:
		sprintf( str, "SHR  V%01X", getOpcodeX( opcode ) );
		return;
	case 0x7:
		sprintf( str, "SUBO V%01X, V%01X", getOpcodeX( opcode ), getOpcodeY( opcode ) );
		return;
	case 0xE:
		sprintf( str, "SHL  V%01X", getOpcodeX( opcode ) );
		return;
	default:
		sprintf( str, "???");
	}
}

void OP9_TXT( uint16_t opcode, char* str )
{
	sprintf( str, "SNE  V%01X, V%01X", getOpcodeX( opcode ), getOpcodeY( opcode ) );
}

void OPA_TXT( uint16_t opcode, char* str )
{
	sprintf( str, "MOV  PTR, 0x%03X", getOpcodeNNN( opcode ) );
}

void OPB_TXT( uint16_t opcode, char* str )
{
	sprintf( str, "MJMP V%01X", getOpcodeX( opcode ) );
}

void OPC_TXT( uint16_t opcode, char* str )
{
	sprintf( str, "RND  V%01X, 0x%02X", getOpcodeX( opcode ), getOpcodeNN(opcode) );
}

void OPD_TXT( uint16_t opcode, char* str )
{
	sprintf( str, "DRAW V%01X, V%01X, 0x%01X", getOpcodeX( opcode ), getOpcodeY( opcode ), getOpcodeN(opcode) );
}

void OPE_TXT( uint16_t opcode, char* str )
{
	if ( getOpcodeN(opcode) == 0xE )
	{
		sprintf( str, "SK   V%01X", getOpcodeX(opcode) );
	}
	else if ( getOpcodeN(opcode) == 0x1 )
	{
		sprintf( str, "SNK  V%01X", getOpcodeX( opcode ) );
	}
	else
	{
		sprintf( str, "???" );
	}
}

void OPF_TXT( uint16_t opcode, char* str )
{
	switch ( getOpcodeNN(opcode) )
	{
	case 0x07:
		sprintf( str, "MOV  V%01X, DLY", getOpcodeX( opcode ) );
		return;
	case 0x0A:
		sprintf( str, "WTK  V%01X", getOpcodeX( opcode ) );
		return;
	case 0x15:
		sprintf( str, "MOV  DLY, V%01X", getOpcodeX( opcode ) );
		return;
	case 0x18:
		sprintf( str, "MOV  SND, V%01X", getOpcodeX( opcode ) );
		return;
	case 0x1E:
		sprintf( str, "ADD  PTR, V%01X", getOpcodeX( opcode ) );
		return;
	case 0x29:
		sprintf( str, "SPT  V%01X", getOpcodeX( opcode ) );
		return;
	case 0x33:
		sprintf( str, "BCD  V%01X", getOpcodeX( opcode ) );
		return;
	case 0x55:
		sprintf( str, "DUMP V%01X", getOpcodeX( opcode ) );
		return;
	case 0x65:
		sprintf( str, "LOAD V%01X", getOpcodeX( opcode ) );
		return;
	default:
		sprintf( str, "???" );
	}
	
}