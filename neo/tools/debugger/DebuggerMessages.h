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
#ifndef DEBUGGERMESSAGES_H_
#define DEBUGGERMESSAGES_H_

enum EDebuggerMessage
{
	DBMSG_UNKNOWN,
	DBMSG_CONNECT,
	DBMSG_CONNECTED,
	DBMSG_DISCONNECT,
	DBMSG_ADDBREAKPOINT,
	DBMSG_REMOVEBREAKPOINT,
	DBMSG_HITBREAKPOINT,
	DBMSG_RESUME,
	DBMSG_RESUMED,
	DBMSG_BREAK,
	DBMSG_PRINT,
	DBMSG_INSPECTVARIABLE,
	DBMSG_INSPECTCALLSTACK,
	DBMSG_INSPECTTHREADS,
	DBMSG_STEPOVER,
	DBMSG_STEPINTO,
	DBMSG_CHECKPORT,
	DBMSG_PORTINUSE,
};

enum ThreadFlags
{
	TFL_CURRENT		= (1<<0),
	TFL_DONE		= (1<<1),
	TFL_WAITING		= (1<<2),
	TFL_DYING		= (1<<3),
	TFL_WAIT_TIME	= (1<<4),
	TFL_WAIT_ENTITY	= (1<<5),
};

extern idCVar debugger_serverport;
extern idCVar debugger_clientport;

#endif // DEBUGGER_MESSAGES_H_