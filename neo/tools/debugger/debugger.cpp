/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2014-2016 Robert Beckebans
Copyright (C) 2014-2016 Kot in Action Creative Artel

This file is part of the Doom 3 BFG Edition GPL Source Code ("Doom 3 BFG Edition Source Code").

Doom 3 BFG Edition Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 BFG Edition Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 BFG Edition Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 BFG Edition Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 BFG Edition Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/
#include "precompiled.h"
#pragma hdrstop

#include "../../sys/win32/rc/debugger_resource.h"
#include "DebuggerApp.h"
#include "DebuggerServer.h"

DWORD CALLBACK DebuggerThread ( LPVOID param );

rvDebuggerApp					gDebuggerApp;
HWND							gDebuggerWindow = NULL;
bool							gDebuggerSuspend = false;
bool							gDebuggerConnnected = false;
HANDLE							gDebuggerGameThread = NULL;

rvDebuggerServer*				gDebuggerServer			= NULL;
HANDLE							gDebuggerServerThread   = NULL;
DWORD							gDebuggerServerThreadID = 0;
bool							gDebuggerServerQuit     = false;

/*
================
DebuggerMain

Main entry point for the debugger application
================
*/
void DebuggerClientInit( const char *cmdline )
{
	// See if the debugger is already running
	if ( rvDebuggerWindow::Activate ( ) )
	{
		goto DebuggerClientInitDone;
	}

	if ( !gDebuggerApp.Initialize ( win32.hInstance ) )
	{
		goto DebuggerClientInitDone;
	}

	//gDebuggerApp.Run ( );

DebuggerClientInitDone:
	;

	//common->Quit();
}

void DebuggerClientUpdate()
{
	gDebuggerApp.RunOnce ( );
}

/*
================
DebuggerServerThread

Thread proc for the debugger server
================
*/
DWORD CALLBACK DebuggerServerThread ( LPVOID param )
{
	assert ( gDebuggerServer );
	
	while ( !gDebuggerServerQuit )
	{
		gDebuggerServer->ProcessMessages ( );
		Sleep ( 1 );
	}

	return 0;
}

/*
================
DebuggerServerInit

Starts up the debugger server
================
*/
bool DebuggerServerInit ( void )
{
	// Allocate the new debugger server
	if ( gDebuggerServer == NULL )
		gDebuggerServer = new rvDebuggerServer;
	
	// Start the debugger server thread
	if ( gDebuggerServerThread == NULL )
	{
		gDebuggerServerThread = CreateThread ( NULL, 0, DebuggerServerThread, 0, 0, &gDebuggerServerThreadID );

		// Initialize the debugger server
		if ( !gDebuggerServer->Initialize() )
		{
			DebuggerServerShutdown();

			delete gDebuggerServer;
			gDebuggerServer = NULL;
			return false;
		}
	}	
	return true;
}

/*
================
DebuggerServerShutdown

Shuts down the debugger server
================
*/
void DebuggerServerShutdown ( void )
{
	if ( gDebuggerServerThread )
	{
		// Signal the debugger server to quit
		gDebuggerServerQuit = true;
		
		// Wait for the thread to finish
		WaitForSingleObject ( gDebuggerServerThread, INFINITE );
		
		// Shutdown the server now
		gDebuggerServer->Shutdown();

		delete gDebuggerServer;
		gDebuggerServer = NULL;
		
		// Cleanup the thread handle
		CloseHandle ( gDebuggerServerThread );
		gDebuggerServerThread = NULL;
	}
}

/*
================
DebuggerServerCheckBreakpoint

Check to see if there is a breakpoint associtated with this statement
================
*/
void DebuggerServerCheckBreakpoint ( idInterpreter* interpreter, idProgram* program, int instructionPointer )
{
	if ( !gDebuggerServer )
	{
		return;
	}
	
	gDebuggerServer->CheckBreakpoints ( interpreter, program, instructionPointer );
}

bool DebuggerServerIsSuspended()
{
	return gDebuggerServer && gDebuggerServer->IsSuspended();
}

/*
================
DebuggerServerPrint

Sends a print message to the debugger client
================
*/
void DebuggerServerPrint ( const char* text )
{
	if ( !gDebuggerServer )
	{
		return;
	}
	
	gDebuggerServer->Print ( text );
}

idCVar debugger_serverport( "debugger_serverport", "27980", CVAR_GAME | CVAR_INTEGER, "" );
idCVar debugger_clientport( "debugger_clientport", "27981", CVAR_GAME | CVAR_INTEGER, "" );
