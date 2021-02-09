#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <memory.h>
#include <SDL.h>
#include <time.h>
#include <string.h>
#include "SDL_FontCache/SDL_FontCache.h"

#include "Chip8.h"

enum
{
	PIXEL_SIZE = 10,
	ACTUAL_SCREEN_WIDTH = 64,
	ACTUAL_SCREEN_HEIGHT = 32,

	SCREEN_WIDTH = ACTUAL_SCREEN_WIDTH * PIXEL_SIZE,
	SCREEN_HEIGHT = ACTUAL_SCREEN_HEIGHT * PIXEL_SIZE,


	DEBUG_WIDTH = 220,
	DEBUG_HEIGHT = 200,

	INSTRUCTION_LINES = 22,
	MEMORY_LINE_WIDTH = 8,
};

static int keyCodes[] = {
	SDL_SCANCODE_X,
	SDL_SCANCODE_1,
	SDL_SCANCODE_2,
	SDL_SCANCODE_3,
	SDL_SCANCODE_Q,
	SDL_SCANCODE_W,
	SDL_SCANCODE_E,
	SDL_SCANCODE_A,
	SDL_SCANCODE_S,
	SDL_SCANCODE_D,
	SDL_SCANCODE_Z,
	SDL_SCANCODE_C,
	SDL_SCANCODE_4,
	SDL_SCANCODE_R,
	SDL_SCANCODE_F,
	SDL_SCANCODE_V,
};

static int keyNames[] = {
	"X",
	"1",
	"2",
	"3",
	"Q",
	"W",
	"E",
	"A",
	"S",
	"D",
	"Z",
	"C",
	"4",
	"R",
	"F",
	"V",
};

static void handleKeyPress( chip8_t* machine, SDL_Event* event );
static void handleKeyPressDebug( chip8_t* machine, chip8_t* prevMachine, SDL_Event* event );
static void drawScreen( SDL_Renderer* renderer, chip8_t* machine );
static void drawScreenDebug( SDL_Renderer* renderer, chip8_t* machine, chip8_t* prevMachine );
static void drawDebugInfo( SDL_Renderer* renderer, chip8_t* machine, chip8_t* prevMachine );
static void drawRegisters( SDL_Renderer* renderer, chip8_t* machine, chip8_t* prevMachine );
static void drawMemory( SDL_Renderer* renderer, chip8_t* machine, chip8_t* prevMachine );
static void drawMachineCode( SDL_Renderer* renderer, chip8_t* machine );
static void updateInstructionView( chip8_t* machine );
static void updateMemoryView(chip8_t* machine);
static void addBreakPoint( uint16_t point );
static void deleteBreakPoint( uint16_t point );
static inline bool shouldBreak( chip8_t* machine );
static inline bool isBreakpoint( uint16_t );
static void runCommand();


static bool s_debug = false;
static bool s_break = false;
static bool s_input = false;
static uint16_t s_codeAddress = 0x200;
static uint16_t s_memoryAddress = 0x000;
static FC_Font* s_fontTitle = NULL;
static FC_Font* s_fontText = NULL;

#define NUM_BREAKPOINTS 16
static uint16_t s_breakPoints[NUM_BREAKPOINTS];
static uint16_t s_skipCall = 0;

#define COMMAND_LEN 20
#define COMMAND_HISTORY 8
static int s_currentCommand = 0;
static char commandBuffer[COMMAND_HISTORY][20];

int main(int argc, const char** argv)
{
	for ( int i = 0; i < COMMAND_HISTORY; i++ )
	{
		memset( commandBuffer[i], 0, COMMAND_LEN );
	}
	
	int clocks = 1;
	int instructionsPerFrame = 6;
	const char* filename = NULL;

	for ( int i = 1; i < argc; i++ )
	{
		if ( strcmp( "--debug", argv[i] ) == 0 || strcmp( "-d", argv[i] ) == 0 )
		{
			s_debug = true;
		}
		else if ( strcmp( "--break", argv[i] ) == 0 || strcmp( "-b", argv[i] ) == 0 )
		{
			s_break = true;
		}
		else if ( strstr( argv[i], "--clocks=" ) != 0 || strstr( argv[i], "-c=" ) != 0 )
		{
			char buffer[50];
			strcpy( buffer, argv[i] );
			strtok( buffer, "=" );
			char* value = strtok( NULL, buffer );

			if ( ! sscanf( value, "%x", &instructionsPerFrame ) )
			{
				instructionsPerFrame = 6;
			}
		}
		else
		{
			if ( ! filename )
				filename = argv[i];
		}
	}

	//filename = "tetris.rom";

	if ( ! filename )
	{
		fprintf( stderr, "ERROR: No file supplied.\n" );
		return 1;
	}



	int width = SCREEN_WIDTH;
	int height = SCREEN_HEIGHT;

	if ( s_debug )
	{
		width += DEBUG_WIDTH;
		height += DEBUG_HEIGHT;
	}


	chip8_t* machine = createMachine();

	memset( machine->memory + 0xF00, 0x0, 256 );

	chip8_t* prevMachine = NULL;
	if ( s_debug )
	{
		prevMachine = createMachine();
	}

	loadRom( machine, filename );

	SDL_Window* window = SDL_CreateWindow(
		filename,
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		width,
		height,
		SDL_WINDOW_SHOWN
	);

	

	if ( ! window )
	{
		fprintf( stderr, "ERROR: Could not create window: %s\n", SDL_GetError() );
		destroyMachine( machine );
		return 1;
	}

	SDL_Renderer* renderer = SDL_CreateRenderer( window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC );

	if ( ! renderer )
	{
		fprintf( stderr, "ERROR: Could not create renderer: %s\n", SDL_GetError() );
		SDL_DestroyWindow( window );
		destroyMachine( machine );
		return 1;
	}

	if ( s_debug )
	{
		s_fontTitle = FC_CreateFont();
		s_fontText = FC_CreateFont();
		bool success = true;
		success &= FC_LoadFont( s_fontTitle, renderer, "novem__.ttf", 20, FC_MakeColor( 255, 255, 255, 255 ), TTF_STYLE_NORMAL );
		success &= FC_LoadFont( s_fontText, renderer, "novem__.ttf", 15, FC_MakeColor( 255, 255, 255, 255 ), TTF_STYLE_NORMAL );

		if ( ! success )
		{
			fprintf( stderr, "ERROR: Could not load font: %s\n", SDL_GetError() );
			SDL_DestroyWindow( window );
			SDL_DestroyRenderer( renderer );
			destroyMachine( machine );
			return 1;
		}
	}


	int previousClock = clock();
	int previousDrawClock = clock();

	double drawClockTime = 1.0 / 60.0;
	double clockTime = 1.0 / (double)clocks;
	SDL_Event event;

	bool running = true;
	while ( running )
	{
		while ( SDL_PollEvent( &event ) )
		{
			if ( event.type == SDL_QUIT )
			{
				running = false;
				break;
			}
			else
			{
				if ( s_debug )
					handleKeyPressDebug( machine, prevMachine, &event );
				else
					handleKeyPress( machine, &event );
			}
		}
		SDL_RenderClear( renderer );

		//Draw
		if ( s_debug )
		{
			for ( int i = 0; ! s_break &&  i < instructionsPerFrame; i++ )
			{
				doOneInstructionDebug( machine, prevMachine );
				updateInstructionView( machine );
				updateMemoryView( machine );

				if ( shouldBreak( machine ) )
				{
					s_break = true;
					if ( machine->cpu.pc == s_skipCall )
						s_skipCall = 0;
				}
					
			}
			drawScreenDebug( renderer, machine, prevMachine );
		}
		else
		{
			for ( int i = 0 ; i < instructionsPerFrame * 3; i++ )
				doOneClock( machine );

			drawScreen( renderer, machine );
			previousDrawClock = clock();
		}
			
		

		SDL_RenderPresent( renderer );
	}


	SDL_DestroyWindow( window );
	SDL_DestroyRenderer( renderer );

	destroyMachine( machine );
	destroyMachine( prevMachine );
	return 0;
}

void handleKeyPress( chip8_t* machine, SDL_Event* event )
{
	uint8_t* keysPressed = SDL_GetKeyboardState( NULL );

	for ( int i = 0; i < sizeof( keyCodes ) / sizeof( int ); i++ )
	{
		machine->memory[KEY_LOCATION + i] = keysPressed[keyCodes[i]];
	}
}

void handleKeyPressDebug( chip8_t* machine, chip8_t* prevMachine, SDL_Event* event )
{
	handleKeyPress(machine, event);

	if ( event->type == SDL_KEYDOWN )
	{
		switch ( event->key.keysym.sym )
		{
		case SDLK_F10:
			if ( s_break )
			{
				if ( peekCall( machine ) )
				{
					s_skipCall = machine->cpu.pc + 0x2;
					s_break = false;
				}

				doOneInstructionDebug( machine, prevMachine );
				updateInstructionView( machine );
				updateMemoryView( machine );

				
			}
			break;
		case SDLK_F11:
			if ( s_break )
			{
				doOneInstructionDebug( machine, prevMachine );
				updateInstructionView( machine );
				updateMemoryView( machine );
			}
			break;
		case SDLK_PAGEDOWN:
			s_codeAddress += 2;
			break;
		case SDLK_PAGEUP:
			s_codeAddress -= 2;
			break;
		case SDLK_PAUSE:
			s_break = ! s_break;
			break;
			break;
		case SDLK_UP:
			if ( s_input )
				strcpy( commandBuffer[s_currentCommand], commandBuffer[(s_currentCommand - 1) % COMMAND_HISTORY] );
			else
				s_memoryAddress -= MEMORY_LINE_WIDTH;


			break;
		case SDLK_DOWN:
			if ( s_input )
				strcpy( commandBuffer[s_currentCommand], commandBuffer[(s_currentCommand + 1) % COMMAND_HISTORY] );
			else
				s_memoryAddress += MEMORY_LINE_WIDTH;
			break;
		case SDLK_LEFT:
			s_memoryAddress -= 1;
			break;
		case SDLK_RIGHT:
			s_memoryAddress += 1;
			break;
		case SDLK_END:
			s_input = ! s_input;
			break;
		case SDLK_RETURN:
			runCommand();
			s_currentCommand = (s_currentCommand + 1) % COMMAND_HISTORY;
			memset( commandBuffer[s_currentCommand], 0, COMMAND_LEN );
			break;
		case SDLK_BACKSPACE:
		{
			int len = strlen( commandBuffer[s_currentCommand] ) - 1;
			if ( len >= 0 )
				commandBuffer[s_currentCommand][len] = 0;

			SDL_StartTextInput();
			break;
		}
		}
	}
	else if ( event->type == SDL_TEXTINPUT && s_input )
	{
		int len = strlen( commandBuffer[s_currentCommand] );
		if ( len < COMMAND_LEN - 2 )
			strcat( commandBuffer[s_currentCommand], event->text.text );
	}
}

void updateInstructionView( chip8_t* machine )
{
	int displayDiff = (int)machine->cpu.pc - (int)s_codeAddress;
	if ( displayDiff >= INSTRUCTION_LINES * 2 || displayDiff < 0 )
	{
		s_codeAddress = machine->cpu.pc;
	}
}

void updateMemoryView( chip8_t* machine )
{
	int displayDiff = (int)machine->cpu.ptr - (int)s_memoryAddress;
	if ( displayDiff >= MEMORY_LINE_WIDTH * MEMORY_LINE_WIDTH || displayDiff < 0 )
	{
		s_memoryAddress = machine->cpu.ptr;
	}
}

void runCommand()
{
	char temp[100];
	strcpy( temp, commandBuffer[s_currentCommand] );

	char* command = strtok( temp, " " );

	if ( ! command )
		return;

	if ( strcmp(command, "addbreak") == 0 )
	{
		char* value = strtok( NULL, temp );
		int address = 0;
		if ( ! sscanf( value, "%x", &address ) )
		{
			address = 0;
		}

		addBreakPoint( address );
	}
	else if ( strcmp( command, "delbreak" ) == 0 )
	{
		char* value = strtok( NULL, temp );
		int address = 0;
		if ( ! sscanf( value, "%x", &address ) )
		{
			address = 0;
		}

		deleteBreakPoint( address );
	}
}

void drawScreen( SDL_Renderer* renderer, chip8_t* machine )
{
	static SDL_Rect rectangle;
	rectangle.w = PIXEL_SIZE;
	rectangle.h = PIXEL_SIZE;

	for ( int i = 0; i < ACTUAL_SCREEN_WIDTH * ACTUAL_SCREEN_HEIGHT; i++ )
	{
		rectangle.x = (i % ACTUAL_SCREEN_WIDTH) * PIXEL_SIZE;
		rectangle.y = (i / ACTUAL_SCREEN_WIDTH) * PIXEL_SIZE;

		uint8_t shift = (i % 8);
		uint8_t byte = machine->memory[i/8 + 0xF00];

		uint8_t color = (byte >> shift) & 1;
		SDL_SetRenderDrawColor( renderer, color * 255, color * 255, color * 255, 255 );
		SDL_RenderFillRect( renderer, &rectangle );
	}


	SDL_SetRenderDrawColor( renderer, 0, 0, 0, 255 );
}

void addBreakPoint( uint16_t point )
{
	for ( int i = 0; i < NUM_BREAKPOINTS; i++ )
	{
		if ( ! s_breakPoints[i] )
		{
			s_breakPoints[i] = point;
			return;
		}
	}
}

void deleteBreakPoint( uint16_t point )
{
	for ( int i = 0; i < NUM_BREAKPOINTS; i++ )
	{
		if ( s_breakPoints[i] == point )
		{
			s_breakPoints[i] = 0;
			return;
		}
	}
}

bool shouldBreak( chip8_t* machine )
{

	return isBreakpoint( machine->cpu.pc ) || s_skipCall == machine->cpu.pc;
}

bool isBreakpoint( uint16_t point )
{
	for ( int i = 0; i < NUM_BREAKPOINTS; i++ )
	{
		if ( point == s_breakPoints[i] )
			return true;
	}
	return false;
}

void drawScreenDebug( SDL_Renderer* renderer, chip8_t* machine, chip8_t* prevMachine )
{
	drawDebugInfo( renderer, machine, prevMachine );
	drawScreen( renderer, machine );
}

void drawDebugInfo( SDL_Renderer* renderer, chip8_t* machine, chip8_t* prevMachine )
{
	SDL_SetRenderDrawColor( renderer, 0, 0, 255, 255 );
	static SDL_Rect bottomWindow;
	static SDL_Rect rightWindow;

	bottomWindow.w = SCREEN_WIDTH + DEBUG_WIDTH;
	bottomWindow.h = DEBUG_HEIGHT;
	bottomWindow.x = 0;
	bottomWindow.y = SCREEN_HEIGHT;

	rightWindow.w = DEBUG_WIDTH;
	rightWindow.h = SCREEN_HEIGHT;
	rightWindow.x = SCREEN_WIDTH;
	rightWindow.y = 0;

	SDL_RenderFillRect( renderer, &bottomWindow );
	SDL_RenderFillRect( renderer, &rightWindow );

	drawRegisters( renderer, machine, prevMachine );
	drawMachineCode( renderer, machine );
	drawMemory( renderer, machine, prevMachine, machine->cpu.ptr );
	
	int j = s_currentCommand;
	for ( int i = 0; i < COMMAND_HISTORY; i++ )
	{
		j = (j + 1) % COMMAND_HISTORY;

		const char* format = "  %s";
		if ( s_currentCommand == j )
			format = "> %s";

		FC_Draw( s_fontText, renderer, 470, SCREEN_HEIGHT + 20 + 20 * (COMMAND_HISTORY + j - s_currentCommand), format, commandBuffer[j] );
	}

	if ( s_input )
		FC_Draw( s_fontText, renderer, 480, SCREEN_HEIGHT + 20 , "(Text Input Mode)" );
}

void drawRegisters( SDL_Renderer* renderer, chip8_t* machine, chip8_t* prevMachine )
{
	FC_Draw( s_fontTitle, renderer, 10, SCREEN_HEIGHT + 10, "Registers" );

	const static SDL_Color white = { 255, 255, 255, 255 };
	const static SDL_Color red = { 255, 50, 50, 255 };
	const int x = 5;

	for ( int i = 0; i < 0x8; i++ )
	{
		SDL_Color color = machine->cpu.reg[i] == prevMachine->cpu.reg[i] ? white : red;

		FC_DrawColor( s_fontText, renderer, x, SCREEN_HEIGHT + 40 + i * 20, color, "V%x: 0x%02X", i, machine->cpu.reg[i] );
	}

	for ( int i = 0; i < 0x8; i++ )
	{
		SDL_Color color = machine->cpu.reg[i + 0x8] == prevMachine->cpu.reg[i + 0x8] ? white : red;

		FC_Draw( s_fontText, renderer, x + 80, SCREEN_HEIGHT + 40 + i * 20, "V%X: 0x%02X", i + 0x8, machine->cpu.reg[i + 0x8] );
	}

	SDL_Color color = machine->cpu.pc == prevMachine->cpu.pc ? white : red;
	FC_DrawColor( s_fontText, renderer, x + 160, SCREEN_HEIGHT + 40, color, "PC:  0x%04X", machine->cpu.pc );

	color = machine->cpu.ptr == prevMachine->cpu.ptr ? white : red;
	FC_DrawColor( s_fontText, renderer, x + 160, SCREEN_HEIGHT + 40 + 20, color, "PTR: 0x%04X", machine->cpu.ptr );

	color = machine->cpu.sp == prevMachine->cpu.sp ? white : red;
	FC_DrawColor( s_fontText, renderer, x + 160, SCREEN_HEIGHT + 40 + 40, color, "SP:  0x%04X", machine->cpu.sp );

	color = machine->cpu.dly == prevMachine->cpu.dly ? white : red;
	FC_DrawColor( s_fontText, renderer, x + 160, SCREEN_HEIGHT + 40 + 60, color, "DLY: 0x%02X", machine->cpu.dly );

	color = machine->cpu.snd == prevMachine->cpu.snd ? white : red;
	FC_DrawColor( s_fontText, renderer, x + 160, SCREEN_HEIGHT + 40 + 80, color, "SND: 0x%02X", machine->cpu.snd );
}

void drawMachineCode( SDL_Renderer* renderer, chip8_t* machine )
{
	const static SDL_Color white = { 255, 255, 255, 255 };
	const static SDL_Color red = { 255, 50, 50, 255 };
	const static SDL_Color green = { 50, 255, 50, 255 };

	if ( s_break )
		FC_Draw( s_fontTitle, renderer, SCREEN_WIDTH + 10, 10, "Disassembly" );
	else
		FC_Draw( s_fontTitle, renderer, SCREEN_WIDTH + 10, 10, "Disassembly (running)" );

	char buffer[100];
	for ( int i = 0; i < 22; i++ )
	{
		uint16_t opcodeAddress = s_codeAddress + i * 2;
		SDL_Color color = white;
		if ( opcodeAddress == machine->cpu.pc )
		{
			color = red;
		}
		else if ( isBreakpoint( opcodeAddress ) )
		{
			color = green;
		}
		uint16_t opcode = getOpcode( machine, opcodeAddress );

		disassembleInstruction( opcode, buffer );
		FC_DrawColor( s_fontText, renderer, SCREEN_WIDTH + 10, 40 + i * 20, color, "(0x%03X) %s", opcodeAddress, buffer );
	}
}

static void drawMemory( SDL_Renderer* renderer, chip8_t* machine, chip8_t* prevMachine ) 
{
	const static SDL_Color white = { 255, 255, 255, 255 };
	const static SDL_Color red = { 255, 50, 50, 255 };
	const int x = 260;

	FC_Draw( s_fontTitle, renderer, x, SCREEN_HEIGHT + 10, "Memory" );
	char buffer[100];
	char value[10];
	for ( int i = 0; i < MEMORY_LINE_WIDTH; i++ )
	{
		sprintf( buffer, "0x%03X", s_memoryAddress + i * MEMORY_LINE_WIDTH );
		FC_Draw( s_fontText, renderer, x, SCREEN_HEIGHT + 40 + i * 20, "0x%03X", s_memoryAddress + i * MEMORY_LINE_WIDTH );
		for ( int j = 0; j < MEMORY_LINE_WIDTH; j++ )
		{
			uint16_t ptr = s_memoryAddress + i * MEMORY_LINE_WIDTH + j;
			SDL_Color color = machine->memory[ptr] == prevMachine->memory[ptr] ? white : red;
			sprintf( value, " %02X", machine->memory[ptr] );
			FC_DrawColor( s_fontText, renderer, x + 40 + j * 20, SCREEN_HEIGHT + 40 + i * 20, color, value );
			
			//strcat( buffer, value );
		}
		//FC_DrawColor( s_fontText, renderer, 280, SCREEN_HEIGHT + 40 + i * 20, white, "%s", buffer );
	}
}
