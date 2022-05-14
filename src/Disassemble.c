#include "Chip8_Macros.h"
#include "Diassemble.h"
#include "Chip8.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>


typedef void (*INSTRUCTION_TXT)(uint16_t opcode, char* str );

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

uint16_t* addresses = NULL;

static INSTRUCTION_TXT s_instructionsTXT[16] = {
	OP0_TXT,OP1_TXT,OP2_TXT,OP3_TXT,OP4_TXT,OP5_TXT,OP6_TXT,OP7_TXT,
	OP8_TXT,OP9_TXT,OPA_TXT,OPB_TXT,OPC_TXT,OPD_TXT,OPE_TXT,OPF_TXT
};

typedef struct label_s
{
	uint16_t address;
	uint16_t id;
} label_t;

#define MAX_LABELS 100
#define LINE_SIZE 80

static bool s_addLabel = false;
static int s_labelCount = 0;
static int s_linesCapacity = 200;
static int s_linesSize = 0;
char* s_lines = NULL;

static uint16_t* s_labels = NULL;

static uint16_t tryAddLabel( uint16_t address );
static int fetchLabel( uint16_t address );

void disassembleCode( const uint8_t* memory, const uint16_t end, const char* outFilename )
{
	FILE* out = fopen( outFilename, "w+" );

	if ( ! out )
		return;

	s_linesCapacity = 200;
	s_linesSize = 0;
	size_t size = LINE_SIZE * ((size_t)end);
	s_lines = malloc( size );
	

	if ( ! s_lines )
	{
		fclose( out );
		return;
	}

	memset( s_lines, 0, size );

	s_labels = malloc( sizeof( label_t ) * MAX_LABELS );
	if ( ! s_labels )
	{
		fclose( out );
		free( s_lines );
		return;
	}

	s_addLabel = true;
	char line[LINE_SIZE];
	for ( uint16_t i = 0; i < end; i+=2 )
	{
		uint16_t opcode = getOpcodeRawAddress( memory, i );
		disassembleInstruction( opcode, s_lines + (size_t)i * LINE_SIZE );
	}

	for ( uint16_t i = 0; i < end; i += 2 )
	{
		//Note these labels are found in order of them appearing
		//in the code rather than the order of the addresses.
		int label = fetchLabel( i + 0x200 );
		if ( label != -1 )
			fprintf( out, ".label%d\n", label );

		const char* str = s_lines + (size_t)i * LINE_SIZE;
		if ( str[0] )
			fprintf( out, "%s\n", str );
	}

	printf( "Code disassembled to %s.\n", outFilename );

	fclose( out );
	free( s_lines );
	s_addLabel = false;
}

//Should be hash table but, this is small enough doesn't
//really matter. Even if the entire memory was full with
//unique jumps it would only be 4 mil.
int fetchLabel( uint16_t address )
{
	for ( int i = 0; i < s_labelCount; i++ )
	{
		if ( address == s_labels[i] )
			return i;
	}

	return -1;
}

uint16_t tryAddLabel( uint16_t address )
{
	for ( int i = 0; i < s_labelCount; i++ )
	{
		if ( address == s_labels[i] )
			return i;
	}

	int label = s_labelCount;
	s_labels[label] = address;
	s_labelCount++;

	return label;
}


void disassembleInstruction( uint16_t opcode, char* str )
{
	int index = getOpcodeUpper( opcode );

	assert( index < 0x10 );
	s_instructionsTXT[index]( opcode, str );
}

void OP0_TXT( uint16_t opcode, char* str )
{
	if ( getOpcodeN( opcode ) == 0 )
	{
		sprintf( str, "CLR" );
	}
	else if ( getOpcodeN( opcode ) == 0xE )
	{
		sprintf( str, "RET" );
	}
	else if ( ! s_addLabel )
	{
		sprintf( str, "??? (0x%04X)", opcode );
	}
}

void OP1_TXT( uint16_t opcode, char* str )
{
	if ( ! s_addLabel )
		sprintf( str, "JMP  0x%03X", getOpcodeNNN( opcode ) );
	else
	{
		uint16_t address = getOpcodeNNN( opcode );
		if ( address < 0x200 )
			sprintf( str, "JMP  0x%03X", address);
		else
			sprintf( str, "JMP  label%d", tryAddLabel(address) );
	}
}

void OP2_TXT( uint16_t opcode, char* str )
{
	if ( ! s_addLabel )
		sprintf( str, "CALL 0x%03X", getOpcodeNNN( opcode ) );
	else
	{
		uint16_t address = getOpcodeNNN( opcode );
		if ( address < 0x200 )
			sprintf( str, "CALL 0x%03X", address );
		else
			sprintf( str, "CALL label%d", tryAddLabel( address ) );
	}
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
		if ( ! s_addLabel  )
			sprintf( str, "??? (0x%04X)", opcode );
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
	if ( ! s_addLabel )
		sprintf( str, "MJMP 0x%03X", getOpcodeNNN( opcode ) );
	else
	{
		uint16_t address = getOpcodeNNN( opcode );
		if ( address < 0x200 )
			sprintf( str, "MJMP 0x%03X", address );
		else
			sprintf( str, "MJMP label%d", tryAddLabel( address ) );
	}
}

void OPC_TXT( uint16_t opcode, char* str )
{
	sprintf( str, "RND  V%01X, 0x%02X", getOpcodeX( opcode ), getOpcodeNN( opcode ) );
}

void OPD_TXT( uint16_t opcode, char* str )
{
	sprintf( str, "DRAW V%01X, V%01X, 0x%01X", getOpcodeX( opcode ), getOpcodeY( opcode ), getOpcodeN( opcode ) );
}

void OPE_TXT( uint16_t opcode, char* str )
{
	if ( getOpcodeN( opcode ) == 0xE )
	{
		sprintf( str, "SK   V%01X", getOpcodeX( opcode ) );
	}
	else if ( getOpcodeN( opcode ) == 0x1 )
	{
		sprintf( str, "SNK  V%01X", getOpcodeX( opcode ) );
	}
	else if ( ! s_addLabel )
	{
		sprintf( str, "??? (0x%04X)", opcode );
	}
}

void OPF_TXT( uint16_t opcode, char* str )
{
	switch ( getOpcodeNN( opcode ) )
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
		if ( ! s_addLabel )
			sprintf( str, "??? (0x%04X)", opcode );
	}

}