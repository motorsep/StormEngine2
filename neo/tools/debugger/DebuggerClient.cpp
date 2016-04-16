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

#include "DebuggerApp.h"

/*
================
rvDebuggerClient::rvDebuggerClient
================
*/
rvDebuggerClient::rvDebuggerClient ( )
{
	mConnected = false;
	mWaitFor   = DBMSG_UNKNOWN;
	mNextConnectTime = 0;
}

/*
================
rvDebuggerClient::~rvDebuggerClient
================
*/
rvDebuggerClient::~rvDebuggerClient ( )
{
	ClearBreakpoints ( );	
	ClearCallstack ( );
	ClearThreads ( );
}

/*
================
rvDebuggerClient::Initialize

Initialize the debugger client
================
*/
bool rvDebuggerClient::Initialize ( void )
{
	// Nothing else can run with the debugger
	com_editors = EDITOR_DEBUGGER;

	// Initialize the network connection
	if ( !mPort.InitForPort ( debugger_clientport.GetInteger() ) )
	{
		idLib::Warning( "Can't open debugger client port %d", debugger_clientport.GetInteger() );
		return false;
	}

	// Server must be running on the local host on port 28980
	Sys_StringToNetAdr ( "localhost", &mServerAdr, true );
	mServerAdr.port = debugger_serverport.GetInteger();

	return true;
}	

/*
================
rvDebuggerClient::Shutdown

Shutdown the debugger client and let the debugger server
know we are shutting down
================
*/
void rvDebuggerClient::Shutdown ( void )
{
	mPort.Close();

	if ( mConnected )
	{
		SendMessage ( DBMSG_DISCONNECT );
		mConnected = false;
	}

	com_editors &= ~EDITOR_DEBUGGER;
}

/*
================
rvDebuggerClient::ProcessMessages

Process all incomding messages from the debugger server
================
*/
bool rvDebuggerClient::ProcessMessages ( void )
{
	// Attempt to let the server know we are here.
	if ( !IsConnected() )
	{
		if ( mNextConnectTime == 0 )
			mNextConnectTime = Sys_Milliseconds();
		else if ( mNextConnectTime < Sys_Milliseconds() )
		{
			mNextConnectTime = Sys_Milliseconds() + 1000;

			SendMessage ( DBMSG_CONNECT );
		}
	}
	else
	{
		mNextConnectTime = 0;
	}

	msg_t msg;
	netadr_t adrFrom;

	// Check for pending udp packets on the debugger port
	while ( msg.ReadPacket( mPort, adrFrom ) )
	{
		// Only accept packets from the debugger server for security reasons
		if ( !Sys_CompareNetAdrBase ( adrFrom, mServerAdr ) )
		{
			continue;
		}

		unsigned short command;
		if ( !msg.Read<unsigned short>( command ) )
			continue;
		
		// Is this what we are waiting for?
		if ( command == mWaitFor )
		{
			mWaitFor = DBMSG_UNKNOWN;
		}
		
		switch ( command )
		{
			case DBMSG_CONNECT:
				mConnected = true;
				SendMessage ( DBMSG_CONNECTED );
				SendBreakpoints ( );
				break;
				
			case DBMSG_CONNECTED:
				mConnected = true;
				SendBreakpoints ( );				
				SendMessage ( DBMSG_INSPECTTHREADS );
				break;
				
			case DBMSG_DISCONNECT:
				mConnected = false;
				break;
				
			case DBMSG_BREAK:
				HandleBreak ( msg );				
				break;
				
			// Callstack being send to the client
			case DBMSG_INSPECTCALLSTACK:
				HandleInspectCallstack ( msg );
				break;
				
			// Thread list is being sent to the client
			case DBMSG_INSPECTTHREADS:
				HandleInspectThreads ( msg );
				break;
				
			case DBMSG_INSPECTVARIABLE:
				HandleInspectVariable ( msg );
				break;			
		}			
		
		// Give the window a chance to process the message
		gDebuggerApp.GetWindow().ProcessNetMessage ( msg );
	}

	return true;
}

/*
================
rvDebuggerClient::HandleBreak

Handle the DBMSG_BREAK message send from the server.  This message is handled
by caching the file and linenumber where the break occured. 
================
*/
void rvDebuggerClient::HandleBreak ( msg_t& msg )
{
	char filename[MAX_PATH];

	mBreak = true;

	// Line number
	msg.Read<int>( mBreakLineNumber );
	
	// Filename
	msg.ReadString( filename, MAX_PATH );
	mBreakFilename = filename;
	
	// Clear the variables
	mVariables.Clear ( );

	// Request the callstack and threads
	SendMessage ( DBMSG_INSPECTCALLSTACK );
	WaitFor ( DBMSG_INSPECTCALLSTACK, 2000 );
	
	SendMessage ( DBMSG_INSPECTTHREADS );
	WaitFor ( DBMSG_INSPECTTHREADS, 2000 );
}

/*
================
rvDebuggerClient::InspectVariable

Instructs the client to inspect the given variable at the given callstack depth.  The
variable is inspected by sending a DBMSG_INSPECTVARIABLE message to the server which
will in turn respond back to the client with the variable value
================
*/
void rvDebuggerClient::InspectVariable ( const char* name, int callstackDepth )
{
	msg_t msg;
	msg.Write<unsigned short>( DBMSG_INSPECTVARIABLE );
	msg.Write<int>( (mCallstack.Num()-callstackDepth) );
	msg.WriteString( name );
	msg.SendPacket( mPort, mServerAdr );
}

/*
================
rvDebuggerClient::HandleInspectCallstack

Handle the message DBMSG_INSPECTCALLSTACK being sent from the server.  This message
is handled by adding the callstack entries to a list for later lookup.
================
*/
void rvDebuggerClient::HandleInspectCallstack ( msg_t& msg )
{	
	ClearCallstack ( );

	// Read all of the callstack entries specfied in the message
	unsigned short depth = 0;	
	for ( msg.Read<unsigned short>( depth ); depth > 0; depth -- )
	{
		rvDebuggerCallstack & entry = mCallstack.Alloc();
	
		char temp[1024];

		// Function name
		msg.ReadString( temp, 1024 );
		entry.mFunction = temp;

		// Filename
		msg.ReadString( temp, 1024 );
		entry.mFilename = temp;

		// Line Number
		msg.Read<int>( entry.mLineNumber );
	}
}

/*
================
rvDebuggerClient::HandleInspectThreads

Handle the message DBMSG_INSPECTTHREADS being sent from the server.  This message
is handled by adding the list of threads to a list for later lookup.
================
*/
void rvDebuggerClient::HandleInspectThreads ( msg_t& msg )
{
	const int currentTime = timeGetTime();

	// Loop over the number of threads in the message
	unsigned short count = 0;
	msg.Read<unsigned short>( count );

	mThreads.SetNum( 0 );

	for ( int i = 0; i < count; ++i )
	{
		rvDebuggerThread & entry = mThreads.Alloc();

		// Thread name
		char temp[2048] = {};
		msg.ReadString( temp, 2048 );
		entry.mName = temp;

		msg.Read<int>( entry.mID );
		msg.Read<unsigned short>( entry.mFlags );
		msg.ReadString( temp, 2048 );
		entry.mFunction = temp;
		msg.Read<int>( entry.mWaitTime );
		msg.ReadString( temp, 2048 );
		entry.mWaitMsg = temp;
	}
}

/*
================
rvDebuggerClient::HandleInspectVariable

Handle the message DBMSG_INSPECTVARIABLE being sent from the server.  This message
is handled by adding the inspected variable to a dictionary for later lookup
================
*/
void rvDebuggerClient::HandleInspectVariable ( msg_t& msg )
{
	char	var[1024];
	char	value[1024];
	int		callDepth = 0;
	
	msg.Read<int>( callDepth );
	msg.ReadString( var, 1024 );
	msg.ReadString( value, 1024 );
		
	mVariables.Set ( va("%d:%s", mCallstack.Num()-callDepth, var), value );	
}

/*
================
rvDebuggerClient::WaitFor

Waits the given amount of time for the specified message to be received by the 
debugger client.
================
*/
bool rvDebuggerClient::WaitFor ( EDebuggerMessage msg, int time )
{
	int start;
	
	// Cant wait if not connected
	if ( !mConnected )
	{
		return false;
	}
	
	start    = Sys_Milliseconds ( );
	mWaitFor = msg;
	
	while ( mWaitFor != DBMSG_UNKNOWN && Sys_Milliseconds()-start < time )
	{
		ProcessMessages ( );
		Sleep ( 0 );
	}
	
	if ( mWaitFor != DBMSG_UNKNOWN )
	{
		mWaitFor = DBMSG_UNKNOWN;
		return false;
	}
	
	return true;
}

/*
================
rvDebuggerClient::FindBreakpoint

Searches for a breakpoint that maches the given filename and linenumber
================
*/
rvDebuggerBreakpoint* rvDebuggerClient::FindBreakpoint ( const char* filename, int linenumber )
{
	for ( int i = 0; i < mBreakpoints.Num(); i ++ )
	{
		rvDebuggerBreakpoint* bp = &mBreakpoints[i];
		
		if ( linenumber == bp->GetLineNumber ( ) && !idStr::Icmp ( bp->GetFilename ( ), filename ) )
		{
			return bp;
		}
	}
	
	return NULL;
}

/*
================
rvDebuggerClient::ClearBreakpoints

Removes all breakpoints from the client and server
================
*/
void rvDebuggerClient::ClearBreakpoints ( void )
{
	for ( int i = 0; i < GetBreakpointCount(); i ++ )
	{
		SendRemoveBreakpoint ( mBreakpoints[i] );
	}	
	mBreakpoints.Clear ( );
}

/*
================
rvDebuggerClient::AddBreakpoint

Adds a breakpoint to the client and server with the give nfilename and linenumber
================
*/
int rvDebuggerClient::AddBreakpoint ( const char* filename, int lineNumber, bool onceOnly )
{
	int index = mBreakpoints.Append ( rvDebuggerBreakpoint ( filename, lineNumber ) );		
	
	SendAddBreakpoint ( mBreakpoints[index] );
	
	return index;
}

/*
================
rvDebuggerClient::RemoveBreakpoint

Removes the breakpoint with the given ID from the client and server
================
*/
bool rvDebuggerClient::RemoveBreakpoint ( int bpID )
{
	for ( int index = 0; index < GetBreakpointCount(); index ++ )
	{	
		if ( mBreakpoints[index].GetID ( ) == bpID )
		{
			SendRemoveBreakpoint ( mBreakpoints[index] );			
			mBreakpoints.RemoveIndex ( index );
			return true;
		}
	}
	
	return false;
}

/*
================
rvDebuggerClient::SendMessage

Send a message with no data to the debugger server
================
*/
void rvDebuggerClient::SendMessage ( EDebuggerMessage dbmsg )
{
	msg_t msg;
	msg.Write<unsigned short>( dbmsg );
	msg.SendPacket( mPort, mServerAdr );
}

/*
================
rvDebuggerClient::SendBreakpoints

Send all breakpoints to the debugger server
================
*/
void rvDebuggerClient::SendBreakpoints ( void )
{
	if ( !mConnected )
	{
		return;
	}
	
	// Send all the breakpoints to the server
	for ( int i = 0; i < mBreakpoints.Num(); i ++ )
	{
		SendAddBreakpoint ( mBreakpoints[i] );
	}
}

/*
================
rvDebuggerClient::SendAddBreakpoint

Send an individual breakpoint over to the debugger server
================
*/
void rvDebuggerClient::SendAddBreakpoint ( rvDebuggerBreakpoint& bp, bool onceOnly )
{
	if ( !mConnected )
	{
		return;
	}
	
	msg_t msg;
	msg.Write<unsigned short>( DBMSG_ADDBREAKPOINT );
	msg.Write<bool>( onceOnly );
	msg.Write<int>( bp.GetLineNumber ( ) );
	msg.Write<int>( bp.GetID ( ) );
	msg.WriteString( bp.GetFilename() );
	msg.SendPacket( mPort, mServerAdr );
}

/*
================
rvDebuggerClient::SendRemoveBreakpoint

Sends a remove breakpoint message to the debugger server
================
*/
void rvDebuggerClient::SendRemoveBreakpoint ( rvDebuggerBreakpoint& bp )
{
	if ( !mConnected )
	{
		return;
	}
	
	msg_t msg;
	msg.Write<unsigned short>( DBMSG_REMOVEBREAKPOINT );
	msg.Write<int>( bp.GetID() );
	msg.SendPacket( mPort, mServerAdr );
}

/*
================
rvDebuggerClient::ClearCallstack

Clear all callstack entries
================
*/
void rvDebuggerClient::ClearCallstack ( void )
{
	mCallstack.Clear ( );
}

/*
================
rvDebuggerClient::ClearThreads

Clear all thread entries
================
*/
void rvDebuggerClient::ClearThreads ( void )
{
	mThreads.Clear ( );
}

