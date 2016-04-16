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

#include "../../d3xp/gamesys/Event.h"
#include "../../d3xp/gamesys/Class.h"
#include "../../d3xp/script/Script_Program.h"
#include "../../d3xp/script/Script_Interpreter.h"
#include "../../d3xp/script/Script_Thread.h"
#include "../../d3xp/script/Script_Compiler.h"
#include "DebuggerApp.h"
#include "DebuggerServer.h"

/*
================
rvDebuggerServer::rvDebuggerServer
================
*/
rvDebuggerServer::rvDebuggerServer ( )
{
	mInitialized		= false;
	mConnected			= false;
	mBreakNext			= false;
	mBreak				= false;
	mBreakStepOver		= false;
	mBreakStepInto		= false;
	mGameThread			= NULL;
	mLastStatementLine	= -1;
	mBreakStepOverFunc1 = NULL;
	mBreakStepOverFunc2 = NULL;
	mBreakProgram		= NULL;
	mBreakInterpreter	= NULL;
	mBreakStepOverDepth = 0;
	mBreakInstructionPointer = 0;
	mLastThreadSyncTime = 0;
}

/*
================
rvDebuggerServer::~rvDebuggerServer
================
*/
rvDebuggerServer::~rvDebuggerServer ( )
{
}

/*
================
rvDebuggerServer::Initialize

Initialize the debugger server.  This function should be called before the
debugger server is used.
================
*/
bool rvDebuggerServer::Initialize ( void )
{
	// Initialize the network connection
	if ( !mPort.InitForPort ( debugger_serverport.GetInteger() ) )
	{
		idLib::Warning( "Can't open debugger server port %d", debugger_serverport.GetInteger() );
		return false;
	}

	// Get a copy of the game thread handle so we can suspend the thread on a break
	DuplicateHandle ( GetCurrentProcess(), GetCurrentThread ( ), GetCurrentProcess(), &mGameThread, 0, FALSE, DUPLICATE_SAME_ACCESS );

	// Create a critical section to ensure that the shared thread
	// variables are protected
	InitializeCriticalSection ( &mCriticalSection );

	// Server must be running on the local host on port 28980
	Sys_StringToNetAdr ( "localhost", &mClientAdr, true );
	mClientAdr.port = debugger_clientport.GetInteger();
	
	// Attempt to let the server know we are here.  The server may not be running so this
	// message will just get ignored.
	SendMessage ( DBMSG_CONNECT );
	
	mInitialized = true;

	return true;
}	

void rvDebuggerServer::OSPathToRelativePath( const char *osPath, idStr &qpath )
{
	if ( strchr( osPath, ':' ) ) 
	{
		qpath = fileSystem->OSPathToRelativePath( osPath );
	}
	else
	{
		qpath = osPath;
	}
}

/*
================
rvDebuggerServer::Shutdown

Shutdown the debugger server.
================
*/
void rvDebuggerServer::Shutdown ( void )
{
	// Let the debugger client know we are shutting down
	if ( mConnected )
	{
		SendMessage ( DBMSG_DISCONNECT );
		mConnected = false;
	}

	if ( mInitialized )
	{
		mInitialized = false;

		mPort.Close();

		// dont need the crit section anymore
		DeleteCriticalSection ( &mCriticalSection );
	}
}

/*
================
rvDebuggerServer::ProcessMessages

Process all incoming network messages from the debugger client
================
*/
bool rvDebuggerServer::ProcessMessages ( void )
{
	// Attempt to let the server know we are here.
	if ( IsConnected() )
	{
		if ( mLastThreadSyncTime != idInterpreter::sLastScriptExecuteTime )
		{
			mLastThreadSyncTime = idInterpreter::sLastScriptExecuteTime;
			HandleInspectThreads( msg_t() );
		}
	}

	msg_t msg;
	netadr_t adrFrom;

	// Check for pending udp packets on the debugger port
	while ( msg.ReadPacket( mPort, adrFrom ) )
	{
		// Only accept packets from the debugger server for security reasons
		if ( !Sys_CompareNetAdrBase ( adrFrom, mClientAdr ) )
		{
			continue;
		}
			
		unsigned short command;
		if ( !msg.Read<unsigned short>( command ) )
			continue;
		
		switch ( command )
		{
			case DBMSG_CONNECT:
				mConnected = true;
				SendMessage ( DBMSG_CONNECTED );
				break;
				
			case DBMSG_CONNECTED:
				mConnected = true;
				break;
				
			case DBMSG_DISCONNECT:
				ClearBreakpoints ( );
				Resume ( );				
				mConnected = false;
				break;
				
			case DBMSG_ADDBREAKPOINT:
				HandleAddBreakpoint ( msg );
				break;
				
			case DBMSG_REMOVEBREAKPOINT:
				HandleRemoveBreakpoint ( msg );
				break;
				
			case DBMSG_RESUME:
				Resume ( );
				break;
				
			case DBMSG_BREAK:
				mBreakNext = true;
				break;
				
			case DBMSG_STEPOVER:
				mBreakStepOver = true;
				mBreakStepOverDepth = mBreakInterpreter->GetCallstackDepth ( );
				mBreakStepOverFunc1 = mBreakInterpreter->GetCallstack()[mBreakInterpreter->GetCallstackDepth()].f;
				if ( mBreakInterpreter->GetCallstackDepth() > 0 )
				{
					mBreakStepOverFunc2 = mBreakInterpreter->GetCallstack()[mBreakInterpreter->GetCallstackDepth()-1].f;
				}
				else
				{
					mBreakStepOverFunc2 = NULL;
				}
				Resume ( );
				break;

			case DBMSG_STEPINTO:
				mBreakStepInto = true;
				Resume ( );
				break;
				
			case DBMSG_INSPECTVARIABLE:
				HandleInspectVariable ( msg );
				break;
				
			case DBMSG_INSPECTCALLSTACK:
				HandleInspectCallstack ( msg );
				break;
				
			case DBMSG_INSPECTTHREADS:
				HandleInspectThreads ( msg );
				break;

			case DBMSG_CHECKPORT:
				HandleCheckPort ( msg );
				break;
		}		
	}

	return true;
}

/*
================
rvDebuggerServer::SendMessage

Send a message with no data to the debugger server.
================
*/
void rvDebuggerServer::SendMessage ( EDebuggerMessage dbmsg )
{
	msg_t	 msg;
	msg.Write<unsigned short>( dbmsg );
	msg.SendPacket( mPort, mClientAdr );
}

/*
================
rvDebuggerServer::HandleAddBreakpoint

Handle the DBMSG_ADDBREAKPOINT message being sent by the debugger client.  This
message is handled by adding a new breakpoint to the breakpoint list with the
data supplied in the message.
================
*/
void rvDebuggerServer::HandleAddBreakpoint ( msg_t& msg )
{
	bool onceOnly;
	int lineNumber;
	int id;
	char filename[MAX_PATH];
	
	// Read the breakpoint info
	msg.Read<bool>( onceOnly );
	msg.Read<int>( lineNumber );
	msg.Read<int>( id );
	msg.ReadString( filename, MAX_PATH );
	
	// Since breakpoints are used by both threads we need to 
	// protect them with a crit section	
	EnterCriticalSection ( &mCriticalSection );
	mBreakpoints.Append ( new rvDebuggerBreakpoint ( filename, lineNumber, id ) );
	LeaveCriticalSection ( &mCriticalSection );
}

/*
================
rvDebuggerServer::HandleRemoveBreakpoint

Handle the DBMSG_REMOVEBREAKPOINT message being sent by the debugger client.  This
message is handled by removing the breakpoint that matches the given id from the 
list.
================
*/
void rvDebuggerServer::HandleRemoveBreakpoint ( msg_t& msg )
{
	int i;
	int id;
	
	// ID that we are to remove
	msg.Read( id );

	// Since breakpoints are used by both threads we need to 
	// protect them with a crit section	
	EnterCriticalSection ( &mCriticalSection );
	
	// Find the breakpoint that matches the given id and remove it from the list
	for ( i = 0; i < mBreakpoints.Num(); i ++ )
	{
		if ( mBreakpoints[i]->GetID ( ) == id )
		{
			delete mBreakpoints[i];
			mBreakpoints.RemoveIndex ( i );
			break;
		}
	}

	LeaveCriticalSection ( &mCriticalSection );
}

/*
================
rvDebuggerServer::MSG_WriteCallstackFunc

Writes a single callstack entry to the given message
================
*/
void rvDebuggerServer::MSG_WriteCallstackFunc ( msg_t& msg, const prstack_t* stack )
{
	const function_t*	func = stack->f;

	// If the function is unknown then just fill in with default data.
	if ( !func )
	{
		msg.WriteString( "<UNKNOWN>" );
		msg.WriteString( "<UNKNOWN>" );
		msg.Write<int>( 0 );
		return;
	}
	else
	{
		msg.WriteString( va("%s( ??? )", func->Name() ) );
	}

	// Use the calling statement as the filename and linenumber where
	// the call was made from
	const statement_t* st = &mBreakProgram->GetStatement ( stack->s );
	if ( st )
	{
		idStr qpath;
		OSPathToRelativePath(mBreakProgram->GetFilename( st->file ), qpath);		
		qpath.BackSlashesToSlashes ( );

		msg.WriteString( qpath.c_str() );
		msg.Write<int>( st->linenumber );
	}
	else
	{
		msg.WriteString( "<UNKNOWN>" );
		msg.Write<int>( 0 );
	}						
}

/*
================
rvDebuggerServer::HandleInspectCallstack

Handle an incoming inspect callstack message by sending a message
back to the client with the callstack data.
================
*/
void rvDebuggerServer::HandleInspectCallstack ( msg_t& in_msg )
{
	msg_t msg;
	msg.Write<unsigned short>( DBMSG_INSPECTCALLSTACK );
	msg.Write<unsigned short>( mBreakInterpreter->GetCallstackDepth() );
	
	// write out the current function
	prstack_t temp;
	temp.f = mBreakInterpreter->GetCurrentFunction ( );
	temp.s = 0;
	temp.stackbase = 0;
	MSG_WriteCallstackFunc ( msg, &temp );	

	// Run through all of the callstack and write each to the msg		
	for ( int i = mBreakInterpreter->GetCallstackDepth ( ) - 1; i > 0; i -- )
	{
		MSG_WriteCallstackFunc ( msg, mBreakInterpreter->GetCallstack ( ) + i );	
	}
		
	msg.SendPacket( mPort, mClientAdr );
}

/*
================
rvDebuggerServer::HandleInspectThreads

Send the list of the current threads in the interpreter back to the debugger client
================
*/
void rvDebuggerServer::HandleInspectThreads ( msg_t& in_msg )
{
	msg_t  msg;
	msg.Write<unsigned short>( DBMSG_INSPECTTHREADS );
	msg.Write<unsigned short>( idThread::GetThreads().Num() );

	// Loop through all of the threads and write their name and number to the message
	for ( int i = 0; i < idThread::GetThreads().Num(); i ++ )
	{
		idThread* thread = idThread::GetThreads()[i];
	
		unsigned short threadFlags = 0;
		if ( mBreakInterpreter && thread == mBreakInterpreter->GetThread ( ) )
			threadFlags |= TFL_CURRENT;
		if ( thread->IsDoneProcessing() )
			threadFlags |= TFL_DONE;
		if ( thread->IsWaiting() )
			threadFlags |= TFL_WAITING;
		if ( thread->IsDying() )
			threadFlags |= TFL_DYING;
		
		msg.WriteString( thread->GetThreadName ( ) );
		msg.Write<int>( thread->GetThreadNum ( ) );
		msg.Write<unsigned short>( threadFlags );

		thread->DebugInfo( msg );
	}
	
	// Send off the inspect threads packet to the debugger client
	msg.SendPacket( mPort, mClientAdr );
}

void rvDebuggerServer::HandleCheckPort( msg_t& in_msg )
{
	unsigned short portNum = 0;
	if ( in_msg.Read<unsigned short>( portNum ) )
	{
		if ( portNum == mPort.GetPort() )
		{
			msg_t  msg;
			msg.Write<unsigned short>( DBMSG_PORTINUSE );
			msg.Write<unsigned short>( portNum );
			msg.SendPacket( mPort, mClientAdr );
		}
	}
}

void rvDebuggerServer::HandlePortInUse( msg_t& in_msg )
{
	/*mPort.Close();
	mPort.InitForPort();*/
}

/*
================
rvDebuggerServer::HandleInspectVariable

Respondes to a request from the debugger client to inspect the value of a given variable
================
*/
void rvDebuggerServer::HandleInspectVariable ( msg_t& in_msg )
{
	char varname[256];
	int  scopeDepth;
	
	if ( !mBreak )
	{
		return;
	}
	
	in_msg.Read<int>( scopeDepth );
	in_msg.ReadString( varname, 256 );

	msg_t msg;
	msg.Write<unsigned short>( DBMSG_INSPECTVARIABLE );
		
	idStr varvalue;
	if ( !mBreakInterpreter->GetRegisterValue ( varname, varvalue, scopeDepth ) )
	{
		varvalue = "???";
	}
	
	msg.Write<int>( scopeDepth );	
	msg.WriteString( varname );
	msg.WriteString( varvalue.c_str() );
	msg.SendPacket( mPort, mClientAdr );
}

/*
================
rvDebuggerServer::CheckBreakpoints

Check to see if any breakpoints have been hit.  This includes "break next", 
"step into", and "step over" break points
================
*/
void rvDebuggerServer::CheckBreakpoints	( idInterpreter* interpreter, idProgram* program, int instructionPointer )
{
	if ( !mConnected ) {
		return;
	}

	// Grab the current statement and the filename that it came from
	const statement_t* st = &program->GetStatement ( instructionPointer );
	const char* filename = program->GetFilename ( st->file );
	
	// Operate on lines, not statements
	if ( mLastStatementLine == st->linenumber && mLastStatementFile == filename )
	{
		return;
	}

	// Save the last visited line and file so we can prevent
	// double breaks on lines with more than one statement
	mLastStatementFile = idStr( filename );
	mLastStatementLine = st->linenumber;

	// Reset stepping when the last function on the callstack is returned from
	if ( st->op == OP_RETURN && interpreter->GetCallstackDepth ( ) <= 1 )
	{
		mBreakStepOver = false;
		mBreakStepInto = false;	
	}

	// See if we are supposed to break on the next script line
	if ( mBreakNext )
	{
		Break ( interpreter, program, instructionPointer );
		return;
	}

	// Only break on the same callstack depth and thread as the break over	
	if ( mBreakStepOver )
	{			
		if ( ( interpreter->GetCurrentFunction ( ) == mBreakStepOverFunc1 || 
		       interpreter->GetCurrentFunction ( ) == mBreakStepOverFunc2    )&& 
		     ( interpreter->GetCallstackDepth ( )  <= mBreakStepOverDepth ) )
		{
			Break ( interpreter, program, instructionPointer );
			return;
		}							
	}
				
	// See if we are supposed to break on the next line
	if ( mBreakStepInto )
	{
		// Break
		Break ( interpreter, program, instructionPointer );
		return;
	}

	idStr qpath;
	OSPathToRelativePath(filename,qpath);	
	qpath.BackSlashesToSlashes ( );

	EnterCriticalSection ( &mCriticalSection );

	// Check all the breakpoints
	for ( int i = 0; i < mBreakpoints.Num ( ); i ++ )
	{
		rvDebuggerBreakpoint* bp = mBreakpoints[i];
		
		// Skip if not match of the line number
		if ( st->linenumber != bp->GetLineNumber ( ) )
		{
			continue;
		}

		// Skip if no match of the filename
		if ( idStr::Icmp ( bp->GetFilename(), qpath ) )
		{
			continue;		
		}
	
		// Pop out of the critical section so we dont get stuck	
		LeaveCriticalSection ( &mCriticalSection );
		
		// We hit a breakpoint, so break
		Break ( interpreter, program, instructionPointer );
		
		// Back into the critical section since we are going to have to leave it 
		EnterCriticalSection ( &mCriticalSection );
		
		break;
	}
	
	LeaveCriticalSection ( &mCriticalSection );	
}

/*
================
rvDebuggerServer::Break

Halt execution of the game threads and inform the debugger client that
the game has been halted
================
*/
void rvDebuggerServer::Break ( idInterpreter* interpreter, idProgram* program, int instructionPointer )
{
	msg_t				msg;

	// Clear all the break types
	mBreakStepOver = false;
	mBreakStepInto = false;
	mBreakNext     = false;
	
	// Grab the current statement and the filename that it came from
	const statement_t* st = &program->GetStatement ( instructionPointer );
	const char* filename = program->GetFilename ( st->file );

	idStr qpath;
	OSPathToRelativePath(filename, qpath);	
	qpath.BackSlashesToSlashes ( );

	// Give the mouse cursor back to the world
	Sys_GrabMouseCursor( false );	
	
	// Set the break variable so we know the main thread is stopped
	mBreak = true;
	mBreakProgram = program;
	mBreakInterpreter = interpreter;
	mBreakInstructionPointer = instructionPointer;
	
	// Inform the debugger of the breakpoint hit
	msg.Write<unsigned short>( DBMSG_BREAK );
	msg.Write<int>( st->linenumber );
	msg.WriteString( qpath.c_str() );
	msg.SendPacket( mPort, mClientAdr );

#if(0)
	// Suspend the game thread.  Since this will be called from within the main game thread
	// execution wont return until after the thread is resumed
	SuspendThread ( mGameThread );						
	
	// Let the debugger client know that we have started back up again
	SendMessage ( DBMSG_RESUMED );

	// This is to give some time between the keypress that 
	// told us to resume and the setforeground window. 
	// would just flash
	Sleep ( 150 );
#endif

	// Bring the window back to the foreground	
	SetForegroundWindow ( win32.hWnd );
	SetActiveWindow ( win32.hWnd );
	UpdateWindow ( win32.hWnd );
	SetFocus ( win32.hWnd );	
	
	// Give the mouse cursor back to the game
	Sys_GrabMouseCursor( true );	

	// Clear all commands that were generated before we went into suspended mode.  This is
	// to ensure we dont have mouse downs with no ups because the context was changed.
	idKeyInput::ClearStates();
}

/*
================
rvDebuggerServer::Resume

Resume execution of the game.
================
*/
void rvDebuggerServer::Resume ( void )
{
	// Cant resume if not paused
	if ( !mBreak )
	{
		return;
	}
	
	mBreak = false;

	// Start the game thread back up	
	//ResumeThread ( mGameThread );
	SendMessage ( DBMSG_RESUMED );
}

/*
================
rvDebuggerServer::ClearBreakpoints

Remove all known breakpoints
================
*/
void rvDebuggerServer::ClearBreakpoints	( void )
{
	for ( int i = 0; i < mBreakpoints.Num(); i ++ )
	{
		delete mBreakpoints[i];
	}
	
	mBreakpoints.Clear ( );
}

/*
================
rvDebuggerServer::Print

Sends a console print message over to the debugger client
================
*/
void rvDebuggerServer::Print ( const char* text )
{
	if ( !mConnected )
	{
		return;
	}

	msg_t	 msg;
	msg.Write<unsigned short>( DBMSG_PRINT );
	msg.WriteString( text );
	msg.SendPacket( mPort, mClientAdr );
}	
