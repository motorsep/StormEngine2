/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.
Copyright (C) 2012 dhewg (dhewm3)
Copyright (C) 2012 Robert Beckebans
Copyright (C) 2013 Daniel Gibson

This file is part of the Doom 3 GPL Source Code ("Doom 3 Source Code").

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.	If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "../../idlib/precompiled.h"

// DG: SDL.h somehow needs the following functions, so #undef those silly
//     "don't use" #defines from Str.h
#undef strncmp
#undef strcasecmp
#undef vsnprintf
// DG end

#include <SDL.h>

#include "renderer/tr_local.h"
#include "sdl_local.h"
#include "../posix/posix_public.h"

// DG: those are needed for moving/resizing windows
extern idCVar r_windowX;
extern idCVar r_windowY;
extern idCVar r_windowWidth;
extern idCVar r_windowHeight;
// DG end

const char* kbdNames[] =
{
	"english", "french", "german", "italian", "spanish", "turkish", "norwegian", NULL
};

idCVar in_keyboard( "in_keyboard", "english", CVAR_SYSTEM | CVAR_ARCHIVE | CVAR_NOCHEAT, "keyboard layout", kbdNames, idCmdSystem::ArgCompletion_String<kbdNames> );

struct kbd_poll_t
{
	int key;
	bool state;
	
	kbd_poll_t()
	{
	}
	
	kbd_poll_t( int k, bool s )
	{
		key = k;
		state = s;
	}
};

struct mouse_poll_t
{
	int action;
	int value;
	
	mouse_poll_t()
	{
	}
	
	mouse_poll_t( int a, int v )
	{
		action = a;
		value = v;
	}
};

static idList<kbd_poll_t> kbd_polls;
static idList<mouse_poll_t> mouse_polls;
static idList<sysEvent_t> event_queue;
void Sys_QueEvent( sysEventType_t type, int value, int value2, int ptrLength, void *ptr, int inputDeviceNum );

#define SDL_BFG_MAX_CONTROLLER_BUTTON_EVENTS K_JOY_DPAD_RIGHT-K_JOY1+1
#define SDL_BFG_EVENTS_MAX_CONTROLLER_EVENTS SDL_CONTROLLER_BUTTON_MAX + SDL_CONTROLLER_AXIS_MAX
struct gamepad_device_t
{
	SDL_JoystickID gamePadId;
	SDL_Joystick *gameJoyStick;
	SDL_GameController *gamePad;
	int oldState[SDL_CONTROLLER_BUTTON_MAX];
	int oldAxisState[SDL_CONTROLLER_AXIS_MAX];
	/* This is a Mapping of the Cardinal Directions for each axis */
	bool oldButtonStates[SDL_BFG_MAX_CONTROLLER_BUTTON_EVENTS];
	int activeEvents;
	struct
	{
		int event;
		int value;
	} events[ SDL_BFG_EVENTS_MAX_CONTROLLER_EVENTS ];
	gamepad_device_t()
	{
		gamePadId = -1;
		gamePad = NULL;
		activeEvents = 0;
		gameJoyStick = NULL;
		for(int i = 0;i < SDL_CONTROLLER_BUTTON_MAX; i++)
			oldState[i] = 0;
		for(int i = 0;i < SDL_CONTROLLER_AXIS_MAX; i++) {
			oldAxisState[i] = 0;
		}
		for(int i = 0; i < SDL_BFG_MAX_CONTROLLER_BUTTON_EVENTS;i++)
			oldButtonStates[i] = false;
	}
	
	gamepad_device_t( SDL_GameController * v )
	{
		gamePad = v;
		activeEvents = 0;
		gameJoyStick = SDL_GameControllerGetJoystick(v);
		gamePadId = SDL_JoystickInstanceID(gameJoyStick);
		for(int i = 0;i < SDL_CONTROLLER_BUTTON_MAX; i++)
			oldState[i] = 0;
		for(int i = 0;i < SDL_CONTROLLER_AXIS_MAX; i++) {
			oldAxisState[i] = 0;
		}
		for(int i = 0; i < SDL_BFG_MAX_CONTROLLER_BUTTON_EVENTS;i++)
			oldButtonStates[i] = false;
	}
	void PushButton( int inputDeviceNum, int key, bool value ) {
		int curKey = 0;
		if(K_JOY1 <= key && key <=K_JOY_DPAD_RIGHT) {
			curKey = key-K_JOY1;
			if(oldButtonStates[curKey] != value) {
				oldButtonStates[curKey] = value;
				Sys_QueEvent( SE_KEY, key, value, 0, NULL, inputDeviceNum );
			}
		}
	}
	void PostInputEvent( int inputDeviceNum, int event, int value, int range = 16384 )
	{
		// These events are used for GUI button presses
		if( ( J_ACTION1 <= event  ) && ( event <= J_ACTION_MAX ) )
		{
			PushButton( inputDeviceNum, K_JOY1 + ( event - J_ACTION1 ), value );
		}
		else if( ( event >= J_DPAD_UP ) && ( event <= J_DPAD_RIGHT ) )
		{
			PushButton( inputDeviceNum, K_JOY_DPAD_UP + ( event - J_DPAD_UP ), value );
		}
		else if( event >= J_AXIS_MIN && event <= J_AXIS_MAX )
		{
			int axis = event - J_AXIS_MIN;
			int percent = ( value * 16 ) / range;
			if( oldAxisState[axis] != percent )
			{
				oldAxisState[axis] = percent;
				Sys_QueEvent( SE_JOYSTICK, axis, percent, 0, NULL, inputDeviceNum );
			}
			if( event == J_AXIS_LEFT_X )
			{
				PushButton( inputDeviceNum, K_JOY_STICK1_LEFT, ( value < -range ) );
				PushButton( inputDeviceNum, K_JOY_STICK1_RIGHT, ( value > range ) );
			}
			else if( event == J_AXIS_LEFT_Y )
			{
				PushButton( inputDeviceNum, K_JOY_STICK1_UP, ( value < -range ) );
				PushButton( inputDeviceNum, K_JOY_STICK1_DOWN, ( value > range ) );
			}
			else if( event == J_AXIS_RIGHT_X )
			{
				PushButton( inputDeviceNum, K_JOY_STICK2_LEFT, ( value < -range ) );
				PushButton( inputDeviceNum, K_JOY_STICK2_RIGHT, ( value > range ) );
			}
			else if( event == J_AXIS_RIGHT_Y )
			{
				PushButton( inputDeviceNum, K_JOY_STICK2_UP, ( value < -range ) );
				PushButton( inputDeviceNum, K_JOY_STICK2_DOWN, ( value > range ) );
			}
			else if( ( event >= J_DPAD_UP ) && ( event <= J_DPAD_RIGHT ) )
			{
				PushButton( inputDeviceNum, K_JOY_DPAD_UP + ( event - J_DPAD_UP ), value != 0 );
			}
			else if( event == J_AXIS_LEFT_TRIG )
			{
				PushButton( inputDeviceNum, K_JOY_TRIGGER1, ( value > range ) );
			}
			else if( event == J_AXIS_RIGHT_TRIG )
			{
				PushButton( inputDeviceNum, K_JOY_TRIGGER2, ( value > range ) );
			}
		}
	
		// These events are used for actual game input
		events[activeEvents].event = event;
		events[activeEvents].value = value;
		activeEvents++;
	}

	int	pollEvents(int controllerID) {
		static const int sdl2BFG_ButtonRemap[SDL_CONTROLLER_BUTTON_MAX] =	
		{
			/* SEE: SDL_GameControllerButton in SDL_gamecontroller.h */
			J_ACTION1,		J_ACTION2		,		// A, B
			J_ACTION3,		J_ACTION4,		// X, Y
			J_ACTION10,		-1,				// Back, Unused ( Guide )
			J_ACTION9,						// Start
			J_ACTION7,		J_ACTION8,		// Left Stick Down, Right Stick Down
			J_ACTION5,		J_ACTION6,		// Black, White (Left Shoulder, Right Shoulder)
			J_DPAD_UP,		J_DPAD_DOWN,	// Up, Down
			J_DPAD_LEFT,	J_DPAD_RIGHT,	// Left, Right
		};
		static const int sdl2BFG_AxisRemap[SDL_CONTROLLER_AXIS_MAX] =
		{
			J_AXIS_LEFT_X,		J_AXIS_LEFT_Y,
			J_AXIS_RIGHT_X,		J_AXIS_RIGHT_Y,
			J_AXIS_LEFT_TRIG,	J_AXIS_RIGHT_TRIG
		};
		activeEvents = 0;
		int currentState = 0;
		for(int i = 0;i < SDL_CONTROLLER_BUTTON_MAX; i++) {
			if(-1 < sdl2BFG_ButtonRemap[i]) {
				currentState = SDL_GameControllerGetButton(gamePad,(SDL_GameControllerButton)i);
				PostInputEvent(controllerID,sdl2BFG_ButtonRemap[i],currentState);
			}
		}
		int currentAxisState = 0;

		for(int i = 0;i < SDL_CONTROLLER_AXIS_MAX; i++) {
			currentAxisState = SDL_GameControllerGetAxis(gamePad,(SDL_GameControllerAxis)i);
			PostInputEvent(controllerID,sdl2BFG_AxisRemap[i],currentAxisState);
		}
		return activeEvents;
	}
	int		ReturnInputEvent( const int n, int& action, int& value )
	{
		if(-1 < n && n < activeEvents) {
			action = events[n].event;
			value  = events[n].value;
			return 1;
		}
		return 0;
	}
};

static const int MAX_JOYSTICKS = 4; /* Limit for Most consoles is 4 Controllers */
static idList<gamepad_device_t> joystick_Devices;

int			eventHead = 0;

// RB begin
static int SDL_KeyToDoom3Key( SDL_Keycode key, bool& isChar )
{
	isChar = false;
	
	if( key >= SDLK_SPACE && key < SDLK_DELETE )
	{
		isChar = true;
		//return key;// & 0xff;
	}
	
	switch( key )
	{
		case SDLK_ESCAPE:
			return K_ESCAPE;
			
		case SDLK_SPACE:
			return K_SPACE;
			
			//case SDLK_EXCLAIM:
			/*
			SDLK_QUOTEDBL:
			SDLK_HASH:
			SDLK_DOLLAR:
			SDLK_AMPERSAND:
			SDLK_QUOTE		= 39,
			SDLK_LEFTPAREN		= 40,
			SDLK_RIGHTPAREN		= 41,
			SDLK_ASTERISK		= 42,
			SDLK_PLUS		= 43,
			SDLK_COMMA		= 44,
			SDLK_MINUS		= 45,
			SDLK_PERIOD		= 46,
			SDLK_SLASH		= 47,
			*/
		case SDLK_SLASH:
			return K_SLASH;// this is the '/' key on the keyboard
		case SDLK_QUOTE:
			return K_APOSTROPHE; // This is the "'" key.
		case SDLK_0:
			return K_0;
			
		case SDLK_1:
			return K_1;
			
		case SDLK_2:
			return K_2;
			
		case SDLK_3:
			return K_3;
			
		case SDLK_4:
			return K_4;
			
		case SDLK_5:
			return K_5;
			
		case SDLK_6:
			return K_6;
			
		case SDLK_7:
			return K_7;
			
		case SDLK_8:
			return K_8;
			
		case SDLK_9:
			return K_9;
			
			// DG: add some missing keys..
		case SDLK_UNDERSCORE:
			return K_UNDERLINE;
			
		case SDLK_MINUS:
			return K_MINUS;
			
		case SDLK_COMMA:
			return K_COMMA;
			
		case SDLK_COLON:
			return K_COLON;
			
		case SDLK_SEMICOLON:
			return K_SEMICOLON;
			
		case SDLK_PERIOD:
			return K_PERIOD;
			
		case SDLK_AT:
			return K_AT;
			
		case SDLK_EQUALS:
			return K_EQUALS;
			// DG end
			
			/*
			SDLK_COLON		= 58,
			SDLK_SEMICOLON		= 59,
			SDLK_LESS		= 60,
			SDLK_EQUALS		= 61,
			SDLK_GREATER		= 62,
			SDLK_QUESTION		= 63,
			SDLK_AT			= 64,
			*/
			/*
			   Skip uppercase letters
			 */
			/*
			SDLK_LEFTBRACKET	= 91,
			SDLK_BACKSLASH		= 92,
			SDLK_RIGHTBRACKET	= 93,
			SDLK_CARET		= 94,
			SDLK_UNDERSCORE		= 95,
			SDLK_BACKQUOTE		= 96,
			*/
		case SDLK_RIGHTBRACKET:
			return K_RBRACKET;
		case SDLK_LEFTBRACKET:
			return K_LBRACKET;
		case SDLK_BACKSLASH:
			return K_BACKSLASH;
		case SDLK_a:
			return K_A;
			
		case SDLK_b:
			return K_B;
			
		case SDLK_c:
			return K_C;
			
		case SDLK_d:
			return K_D;
			
		case SDLK_e:
			return K_E;
			
		case SDLK_f:
			return K_F;
			
		case SDLK_g:
			return K_G;
			
		case SDLK_h:
			return K_H;
			
		case SDLK_i:
			return K_I;
			
		case SDLK_j:
			return K_J;
			
		case SDLK_k:
			return K_K;
			
		case SDLK_l:
			return K_L;
			
		case SDLK_m:
			return K_M;
			
		case SDLK_n:
			return K_N;
			
		case SDLK_o:
			return K_O;
			
		case SDLK_p:
			return K_P;
			
		case SDLK_q:
			return K_Q;
			
		case SDLK_r:
			return K_R;
			
		case SDLK_s:
			return K_S;
			
		case SDLK_t:
			return K_T;
			
		case SDLK_u:
			return K_U;
			
		case SDLK_v:
			return K_V;
			
		case SDLK_w:
			return K_W;
			
		case SDLK_x:
			return K_X;
			
		case SDLK_y:
			return K_Y;
			
		case SDLK_z:
			return K_Z;
			
		case SDLK_RETURN:
			return K_ENTER;
			
		case SDLK_BACKSPACE:
			return K_BACKSPACE;
			
		case SDLK_PAUSE:
			return K_PAUSE;
			
			// DG: add tab key support
		case SDLK_TAB:
			return K_TAB;
			// DG end
			
			//case SDLK_APPLICATION:
			//	return K_COMMAND;
		case SDLK_CAPSLOCK:
			return K_CAPSLOCK;
			
		case SDLK_SCROLLLOCK:
			return K_SCROLL;
			
		case SDLK_POWER:
			return K_POWER;
			
		case SDLK_UP:
			return K_UPARROW;
			
		case SDLK_DOWN:
			return K_DOWNARROW;
			
		case SDLK_LEFT:
			return K_LEFTARROW;
			
		case SDLK_RIGHT:
			return K_RIGHTARROW;
			
		case SDLK_LGUI:
			return K_LWIN;
			
		case SDLK_RGUI:
			return K_RWIN;
			//case SDLK_MENU:
			//	return K_MENU;
			
		case SDLK_LALT:
			return K_LALT;
			
		case SDLK_RALT:
			return K_RALT;
			
		case SDLK_RCTRL:
			return K_RCTRL;
			
		case SDLK_LCTRL:
			return K_LCTRL;
			
		case SDLK_RSHIFT:
			return K_RSHIFT;
			
		case SDLK_LSHIFT:
			return K_LSHIFT;
			
		case SDLK_INSERT:
			return K_INS;
			
		case SDLK_DELETE:
			return K_DEL;
			
		case SDLK_PAGEDOWN:
			return K_PGDN;
			
		case SDLK_PAGEUP:
			return K_PGUP;
			
		case SDLK_HOME:
			return K_HOME;
			
		case SDLK_END:
			return K_END;
			
		case SDLK_F1:
			return K_F1;
			
		case SDLK_F2:
			return K_F2;
			
		case SDLK_F3:
			return K_F3;
			
		case SDLK_F4:
			return K_F4;
			
		case SDLK_F5:
			return K_F5;
			
		case SDLK_F6:
			return K_F6;
			
		case SDLK_F7:
			return K_F7;
			
		case SDLK_F8:
			return K_F8;
			
		case SDLK_F9:
			return K_F9;
			
		case SDLK_F10:
			return K_F10;
			
		case SDLK_F11:
			return K_F11;
			
		case SDLK_F12:
			return K_F12;
			// K_INVERTED_EXCLAMATION;
			
		case SDLK_F13:
			return K_F13;
			
		case SDLK_F14:
			return K_F14;
			
		case SDLK_F15:
			return K_F15;
			
		case SDLK_KP_7:
			return K_KP_7;
			
		case SDLK_KP_8:
			return K_KP_8;
			
		case SDLK_KP_9:
			return K_KP_9;
			
		case SDLK_KP_4:
			return K_KP_4;
			
		case SDLK_KP_5:
			return K_KP_5;
			
		case SDLK_KP_6:
			return K_KP_6;
			
		case SDLK_KP_1:
			return K_KP_1;
			
		case SDLK_KP_2:
			return K_KP_2;
			
		case SDLK_KP_3:
			return K_KP_3;
			
		case SDLK_KP_ENTER:
			return K_KP_ENTER;
			
		case SDLK_KP_0:
			return K_KP_0;
			
		case SDLK_KP_PERIOD:
			return K_KP_DOT;
			
		case SDLK_KP_DIVIDE:
			return K_KP_SLASH;
			// K_SUPERSCRIPT_TWO;


			
		case SDLK_KP_MINUS:
			return K_KP_MINUS;
			// K_ACUTE_ACCENT;
			
		case SDLK_KP_PLUS:
			return K_KP_PLUS;
			
		case SDLK_NUMLOCKCLEAR:
			return K_NUMLOCK;
			
		case SDLK_KP_MULTIPLY:
			return K_KP_STAR;
			
		case SDLK_KP_EQUALS:
			return K_KP_EQUALS;
			
			// K_MASCULINE_ORDINATOR;
			// K_GRAVE_A;
			// K_AUX1;
			// K_CEDILLA_C;
			// K_GRAVE_E;
			// K_AUX2;
			// K_AUX3;
			// K_AUX4;
			// K_GRAVE_I;
			// K_AUX5;
			// K_AUX6;
			// K_AUX7;
			// K_AUX8;
			// K_TILDE_N;
			// K_GRAVE_O;
			// K_AUX9;
			// K_AUX10;
			// K_AUX11;
			// K_AUX12;
			// K_AUX13;
			// K_AUX14;
			// K_GRAVE_U;
			// K_AUX15;
			// K_AUX16;
			
		case SDLK_PRINTSCREEN:
			return K_PRINTSCREEN;
			
		case SDLK_MODE:
			return K_RALT;
	}
	
	return 0;
}
// RB end

static void PushConsoleEvent( const char* s )
{
	char* b;
	size_t len;
	
	len = strlen( s ) + 1;
	b = ( char* )Mem_Alloc( len, TAG_EVENTS );
	strcpy( b, s );
	
	SDL_Event event;
	
	event.type = SDL_USEREVENT;
	event.user.code = SE_CONSOLE;
	event.user.data1 = ( void* )len;
	event.user.data2 = b;
	
	SDL_PushEvent( &event );
}


int sys_HandleSDL_Events(void *userdata, SDL_Event *event);
/*
=================
Sys_InitInput
=================
*/
void Sys_InitInput()
{
	common->Printf( "\n------- Input Initialization -------\n" );
	kbd_polls.SetGranularity( 64 );
	mouse_polls.SetGranularity( 64 );
	event_queue.SetGranularity( 256 );
	joystick_Devices.SetGranularity( MAX_JOYSTICKS );
	in_keyboard.SetModified();
	/* Initialize Game Controller API */
	SDL_Init( SDL_INIT_JOYSTICK |SDL_INIT_GAMECONTROLLER );
	/* Initialize Event Filter */
	SDL_SetEventFilter(sys_HandleSDL_Events, NULL);
	idStr ControllerPath = Sys_DefaultBasePath();
	ControllerPath.Append("/base/gamecontrollerdb.txt");
	common->Printf( "Loading controller Mapping file \"%s\"\n",ControllerPath.c_str());
	SDL_GameControllerAddMappingsFromFile(ControllerPath.c_str());
	ControllerPath = Sys_DefaultSavePath();
	ControllerPath.Append("/gamecontrollerdb.txt");
	common->Printf( "Loading controller Mapping file \"%s\"\n",ControllerPath.c_str());
	SDL_GameControllerAddMappingsFromFile(ControllerPath.c_str());
    char guid[64];
    SDL_GameController *gamecontroller;
    for (int i = 0; i < SDL_NumJoysticks(); ++i) {
        const char *name;
        SDL_JoystickGetGUIDString(SDL_JoystickGetDeviceGUID(i),
                                  guid, sizeof (guid));

        if ( SDL_IsGameController(i) )
        {
            name = SDL_GameControllerNameForIndex(i);
			if(joystick_Devices.Num() < MAX_JOYSTICKS) {
				common->Printf( "Adding Controller \"%s\" ( GUID: %s )\n",name, guid);
		        gamecontroller = SDL_GameControllerOpen(i);
				joystick_Devices.Append(gamepad_device_t( gamecontroller ));
			} else{
				common->Printf( "Detected Controller Not used: %s ( GUID: %s )\n",name, guid);
			}
        }
    }
}

/*
=================
Sys_ShutdownInput
=================
*/
void Sys_ShutdownInput()
{
	kbd_polls.Clear();
	mouse_polls.Clear();
	event_queue.Clear();
}

/*
===========
Sys_InitScanTable
===========
*/
// Windows has its own version due to the tools
#ifndef _WIN32
void Sys_InitScanTable()
{
}
#endif

/*
===============
Sys_GetConsoleKey
===============
*/
unsigned char Sys_GetConsoleKey( bool shifted )
{
	static unsigned char keys[2] = { '`', '~' };
	
	if( in_keyboard.IsModified() )
	{
		idStr lang = in_keyboard.GetString();
		
		if( lang.Length() )
		{
			if( !lang.Icmp( "french" ) )
			{
				keys[0] = '<';
				keys[1] = '>';
			}
			else if( !lang.Icmp( "german" ) )
			{
				keys[0] = '^';
				keys[1] = 176; // °
			}
			else if( !lang.Icmp( "italian" ) )
			{
				keys[0] = '\\';
				keys[1] = '|';
			}
			else if( !lang.Icmp( "spanish" ) )
			{
				keys[0] = 186; // º
				keys[1] = 170; // ª
			}
			else if( !lang.Icmp( "turkish" ) )
			{
				keys[0] = '"';
				keys[1] = 233; // é
			}
			else if( !lang.Icmp( "norwegian" ) )
			{
				keys[0] = 124; // |
				keys[1] = 167; // §
			}
		}
		
		in_keyboard.ClearModified();
	}
	
	return shifted ? keys[1] : keys[0];
}

/*
===============
Sys_MapCharForKey
===============
*/
unsigned char Sys_MapCharForKey( int key )
{
	return key & 0xff;
}

/*
===============
Sys_GrabMouseCursor
===============
*/
void Sys_GrabMouseCursor( bool grabIt )
{
	int flags;
	
	if( grabIt )
	{
		// DG: disabling the cursor is now done once in GLimp_Init() because it should always be disabled
		flags = GRAB_ENABLE | GRAB_SETSTATE;
		// DG end
	}
	else
	{
		flags = GRAB_SETSTATE;
	}
	
	GLimp_GrabInput( flags );
}

/*
================

Sys_GetEvent
================
*/
sysEvent_t Sys_GetEvent()
{
	SDL_Event ev;
	sysEvent_t res = { };
	int eventNum = event_queue.Num();
	
	static const sysEvent_t res_none = { SE_NONE, 0, 0, 0, NULL };
	if(eventNum && eventHead < eventNum ) {
		res = event_queue[eventHead];
		eventHead++;
		return res;
	} else{
		return res_none;
	}
		
}

void Sys_QueEvent( sysEventType_t type, int value, int value2, int ptrLength, void *ptr, int inputDeviceNum )
{
	sysEvent_t eventData = { };
	eventData.evType = type;
	eventData.evValue = value;
	eventData.evValue2 = value2;
	eventData.evPtrLength = ptrLength;
	eventData.evPtr = ptr;
	eventData.inputDevice = inputDeviceNum;
	event_queue.Append(eventData);
}
int sys_HandleSDL_Events(void *userdata, SDL_Event *event)
{
	sysEvent_t res = { };
	int key;
	static const sysEvent_t res_none = { SE_NONE, 0, 0, 0, NULL };
	switch( event->type )
	{
		case SDL_WINDOWEVENT:
			switch( event->window.event )
			{
				case SDL_WINDOWEVENT_FOCUS_GAINED:
				{
					// unset modifier, in case alt-tab was used to leave window and ALT is still set
					// as that can cause fullscreen-toggling when pressing enter...
					SDL_Keymod currentmod = SDL_GetModState();
					int newmod = KMOD_NONE;
					if( currentmod & KMOD_CAPS ) // preserve capslock
						newmod |= KMOD_CAPS;
						
					SDL_SetModState( ( SDL_Keymod )newmod );
					
					// DG: un-pause the game when focus is gained, that also re-grabs the input
					//     disabling the cursor is now done once in GLimp_Init() because it should always be disabled
					cvarSystem->SetCVarBool( "com_pause", false );
					// DG end
					break;
				}
				
				case SDL_WINDOWEVENT_FOCUS_LOST:
					// DG: pause the game when focus is lost, that also un-grabs the input
					cvarSystem->SetCVarBool( "com_pause", true );
					// DG end
					break;
					
					// DG: handle resizing and moving of window
				case SDL_WINDOWEVENT_RESIZED:
				{
					int w = event->window.data1;
					int h = event->window.data2;
					r_windowWidth.SetInteger( w );
					r_windowHeight.SetInteger( h );
					
					glConfig.nativeScreenWidth = w;
					glConfig.nativeScreenHeight = h;
					break;
				}
				
				case SDL_WINDOWEVENT_MOVED:
				{
					int x = event->window.data1;
					int y = event->window.data2;
					r_windowX.SetInteger( x );
					r_windowY.SetInteger( y );
					break;
				}
				// DG end
			}
			
			return 0;
		
		case SDL_KEYDOWN:
			if( event->key.keysym.sym == SDLK_RETURN && ( event->key.keysym.mod & KMOD_ALT ) > 0 )
			{
				// DG: go to fullscreen on current display, instead of always first display
				int fullscreen = 0;
				if( ! renderSystem->IsFullScreen() )
				{
					// this will be handled as "fullscreen on current window"
					// r_fullscreen 1 means "fullscreen on first window" in d3 bfg
					fullscreen = -2;
				}
				cvarSystem->SetCVarInteger( "r_fullscreen", fullscreen );
				// DG end
				PushConsoleEvent( "vid_restart" );
				return 0;
			}
			
			// DG: ctrl-g to un-grab mouse - yeah, left ctrl shoots, then just use right ctrl :)
			if( event->key.keysym.sym == SDLK_g && ( event->key.keysym.mod & KMOD_CTRL ) > 0 )
			{
				bool grab = cvarSystem->GetCVarBool( "in_nograb" );
				grab = !grab;
				cvarSystem->SetCVarBool( "in_nograb", grab );
				return 0;
			}
			// DG end
			
			// fall through
		case SDL_KEYUP:
		{
			bool isChar;
			char c;
			// DG: special case for SDL_SCANCODE_GRAVE - the console key under Esc
			if( event->key.keysym.scancode == SDL_SCANCODE_GRAVE )
			{
				key = K_GRAVE;
				c = K_BACKSPACE; // bad hack to get empty console inputline..

			} // DG end, the original code is in the else case
			else
			{
				key = SDL_KeyToDoom3Key( event->key.keysym.sym, isChar );
				
				if( key == 0 )
				{
					unsigned char uc = event->key.keysym.scancode & 0xff;
					// check if its an unmapped console key
					if( uc == Sys_GetConsoleKey( false ) || uc == Sys_GetConsoleKey( true ) )
					{
						key = K_GRAVE;
						c = K_BACKSPACE; // bad hack to get empty console inputline..
					}
					else
					{
						if( event->type == SDL_KEYDOWN ) // FIXME: don't complain if this was an ASCII char and the console is open?
							common->Warning( "unmapped SDL key %d (0x%x) scancode %d", event->key.keysym.sym, event->	key.keysym.scancode, event->key.keysym.scancode );
						return 0;
					}
				}
			}
			
			Sys_QueEvent( SE_KEY, key, event->key.state == SDL_PRESSED ? 1 : 0, 0, NULL, 0 );
			kbd_polls.Append( kbd_poll_t( key, event->key.state == SDL_PRESSED ) );
			
			if( key == K_BACKSPACE && event->key.state == SDL_PRESSED ) {
				//c = key;
				Sys_QueEvent( SE_CHAR, K_BACKSPACE, 0, 0, NULL, 0 );
			}
			//Sys_QueEvent( SE_CHAR, c, 0, 0, NULL, 0 );
			return 0;

		}
		
		case SDL_TEXTINPUT:
			if( event->text.text && *event->text.text )
			{
				if( !event->text.text[1] ) 
					Sys_QueEvent( SE_CHAR, *event->text.text, 0, 0, NULL, 0 );
				else {
					char* s = NULL;
					size_t s_pos = 0;
					s = strdup( event->text.text );
					while( s !=NULL )
					{
						Sys_QueEvent( SE_CHAR, s[s_pos], 0, 0, NULL, 0 );
						s_pos++;
						if( !s[s_pos] )
						{
							free( s );
							s = NULL;
							s_pos = 0;
						}
					}
					return 0;
				}
			}
			
			return 1;
			
		case SDL_MOUSEMOTION:
			// DG: return event with absolute mouse-coordinates when in menu
			// to fix cursor problems in windowed mode
			if( game && game->Shell_IsActive() )
			{
				Sys_QueEvent( SE_MOUSE_ABSOLUTE, event->motion.x, event->motion.y, 0, NULL, 0 );
			}
			else // this is the old, default behavior
			{
				Sys_QueEvent( SE_MOUSE, event->motion.xrel, event->motion.yrel, 0, NULL, 0 );
			}
			// DG end
			
			mouse_polls.Append( mouse_poll_t( M_DELTAX, event->motion.xrel ) );
			mouse_polls.Append( mouse_poll_t( M_DELTAY, event->motion.yrel ) );
			
			return 0;
			
		case SDL_MOUSEWHEEL:
			if( event->wheel.y > 0 )
			{
				mouse_polls.Append( mouse_poll_t( M_DELTAZ, 1 ) );
				Sys_QueEvent( SE_KEY, K_MWHEELUP, 1, 0, NULL, 0 );
				/* Immediately Queue Not Pressed Event */
				Sys_QueEvent( SE_KEY, K_MWHEELUP, 0, 0, NULL, 0 );
			}
			else
			{
				mouse_polls.Append( mouse_poll_t( M_DELTAZ, -1 ) );
				Sys_QueEvent( SE_KEY, K_MWHEELDOWN, 1, 0, NULL, 0 );
				/* Immediately Queue Not Pressed Event */
				Sys_QueEvent( SE_KEY, K_MWHEELDOWN, 0, 0, NULL, 0 );
			}
			return 0;
			
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
			switch( event->button.button )
			{
				case SDL_BUTTON_LEFT:
					Sys_QueEvent( SE_KEY, K_MOUSE1, event->button.state == SDL_PRESSED ? 1 : 0, 0, NULL, 0 );
					mouse_polls.Append( mouse_poll_t( M_ACTION1, event->button.state == SDL_PRESSED ? 1 : 0 ) );
					break;
				case SDL_BUTTON_MIDDLE:
					Sys_QueEvent( SE_KEY, K_MOUSE3, event->button.state == SDL_PRESSED ? 1 : 0, 0, NULL, 0 );
					mouse_polls.Append( mouse_poll_t( M_ACTION3, event->button.state == SDL_PRESSED ? 1 : 0 ) );
					break;
				case SDL_BUTTON_RIGHT:
					Sys_QueEvent( SE_KEY, K_MOUSE2, event->button.state == SDL_PRESSED ? 1 : 0, 0, NULL, 0 );
					mouse_polls.Append( mouse_poll_t( M_ACTION2, event->button.state == SDL_PRESSED ? 1 : 0 ) );
					break;
			}
			return 0;
		case SDL_JOYAXISMOTION:
		case SDL_JOYBALLMOTION:          /**< Joystick trackball motion */
		case SDL_JOYHATMOTION:           /**< Joystick hat position change */
		case SDL_JOYBUTTONDOWN:          /**< Joystick button pressed */
		case SDL_JOYBUTTONUP:            /**< Joystick button released */
		case SDL_JOYDEVICEADDED:         /**< A new joystick has been inserted into the system */
		case SDL_JOYDEVICEREMOVED:       /**< An opened joystick has been removed */
			/* Always Pass these events on to SDL*/
	        return 1;

		case SDL_CONTROLLERDEVICEADDED:
			/* TODO: Handle what happens when a gamepad is added */
			/* 			Sys_QueEvent( SE_KEY, key, value, 0, NULL, inputDeviceNum ); */
			common->Printf( "Controller Device Connected: %u\n", event->cdevice.which );
			{
				SDL_GameController *gamecontroller = NULL;
				gamecontroller = SDL_GameControllerOpen(event->cdevice.which);
				common->Printf( "Controller Connected %s\n",
					SDL_GameControllerNameForIndex(event->cdevice.which) );
				joystick_Devices.Append(gamepad_device_t(gamecontroller));
			}
	        return 1;
	    case SDL_CONTROLLERDEVICEREMOVED:
			/* TODO: Handle what happens when a gamepad is removed */
			for(int i = 0; i< joystick_Devices.Num();i++) {
				if(event->cdevice.which == joystick_Devices[i].gamePadId) {
					common->Printf( "Controller Device Removed: %u\n", event->cdevice.which );
					common->Printf( "Controller Device Name %s\n",
						SDL_GameControllerName(joystick_Devices[i].gamePad) );
					SDL_GameControllerClose(joystick_Devices[i].gamePad);
					joystick_Devices.RemoveIndex(i);
					break;
				}
			}
	        return 1;
	    case SDL_CONTROLLERBUTTONDOWN:
	    case SDL_CONTROLLERBUTTONUP:
	    case SDL_CONTROLLERAXISMOTION:
			/* The Actual per-Device events are handled when joystick devices get polled. */
	        return 1;
		case SDL_QUIT:
			PushConsoleEvent( "quit" );
			return 0;
			
		case SDL_USEREVENT:
			switch( event->user.code )
			{
				case SE_CONSOLE:
					//res.evType = SE_CONSOLE;
					//res.evPtrLength = ( intptr_t )event->user.data1;
					//res.evPtr = event->user.data2;
					Sys_QueEvent( SE_CONSOLE, 0, 0, ( intptr_t )event->user.data1, event->user.data2, 0 );
					return 0;
				default:
					common->Warning( "unknown user event %u", event->user.code );
					return 1;
			}
		default:
			common->Warning( "unknown event %u", event->type );
			return 1;
	}
	// Event Not Handled, Add to Main queue.
	return 1;
}

void Sys_Handle_SDL_ControllerAxisEvent( const SDL_ControllerAxisEvent sdlEvent )
{

}
/*
================
Sys_ClearEvents
================
*/
void Sys_ClearEvents()
{
	SDL_Event ev;
	
	while( SDL_PollEvent( &ev ) )
		;
		
	kbd_polls.SetNum( 0 );
	mouse_polls.SetNum( 0 );
	event_queue.SetNum(0);
	eventHead = 0;
}

/*
================
Sys_GenerateEvents
================
*/
void Sys_GenerateEvents()
{
	char* s = Posix_ConsoleInput();
	
	if( s )
		PushConsoleEvent( s );
		
	SDL_PumpEvents();
}

/*
================
Sys_PollKeyboardInputEvents
================
*/
int Sys_PollKeyboardInputEvents()
{
	return kbd_polls.Num();
}

/*
================
Sys_ReturnKeyboardInputEvent
================
*/
int Sys_ReturnKeyboardInputEvent( const int n, int& key, bool& state )
{
	if( n >= kbd_polls.Num() )
		return 0;
		
	key = kbd_polls[n].key;
	state = kbd_polls[n].state;
	return 1;
}

/*
================
Sys_EndKeyboardInputEvents
================
*/
void Sys_EndKeyboardInputEvents()
{
	kbd_polls.SetNum( 0 );
}

/*
================
Sys_PollMouseInputEvents
================
*/
int Sys_PollMouseInputEvents( int mouseEvents[MAX_MOUSE_EVENTS][2] )
{
	int numEvents = mouse_polls.Num();
	
	if( numEvents > MAX_MOUSE_EVENTS )
	{
		numEvents = MAX_MOUSE_EVENTS;
	}
	
	for( int i = 0; i < numEvents; i++ )
	{
		const mouse_poll_t& mp = mouse_polls[i];
		
		mouseEvents[i][0] = mp.action;
		mouseEvents[i][1] = mp.value;
	}
	
	mouse_polls.SetNum( 0 );
	
	return numEvents;
}

//=====================================================================================
//	Joystick Input Handling
//=====================================================================================

int activeController = -1;
int Sys_PollJoystickInputEvents( int deviceNum )
{
	if( -1 < deviceNum && deviceNum < joystick_Devices.Num()) {
		if(SDL_GameControllerGetAttached(joystick_Devices[deviceNum].gamePad))
		{
			activeController = deviceNum;
			return joystick_Devices[deviceNum].pollEvents(deviceNum);

		}
	}
	return 0;//win32.g_Joystick.PollInputEvents( deviceNum );
}


int Sys_ReturnJoystickInputEvent( const int n, int& action, int& value )
{

	if(-1 < activeController ) {
		return joystick_Devices[activeController].ReturnInputEvent(n,action,value);
	}
	return 0;//win32.g_Joystick.ReturnInputEvent( n, action, value );
}


void Sys_EndJoystickInputEvents()
{
	activeController = -1;
}

void Sys_SetRumble( int device, int low, int hi )
{
	if( -1 < device && device < joystick_Devices.Num()) {
		//joystick_Devices[deviceNum].pollEvents(deviceNum);
	}
}

