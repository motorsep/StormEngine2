/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2012 Robert Beckebans
Copyright (C) 2012 Daniel Gibson

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

#pragma hdrstop
#include "precompiled.h"

//===============================================================
//
//	memory allocation all in one place
//
//===============================================================
#include <stdlib.h>
#undef new

// foresthale 2014-05-29: ultra-paranoid debug heap that leaks, just for finding errors
//#define DEBUGHEAP
#ifdef DEBUGHEAP
// storage limits for the debug heap
#define DEBUGHEAP_BUFFER (0x40000000)
#define DEBUGHEAP_LIST (1<<20)
// just a basic heap - grows, never shrinks
static unsigned char *debugheapstart = NULL;
static unsigned char *debugheapend;
static unsigned char *debugheapcur;
// each allocation that is made is tracked as a pair of pointers (start and end) and a live flag, we can check sentinels for corruption
static unsigned int debugheaplivecount = 0;
static unsigned char *debugheaplivepointers[DEBUGHEAP_LIST][2];
static unsigned char debugheapliveflag[DEBUGHEAP_LIST];
static mutexHandle_t debugheapmutex;

void Mem_Printf( const char *fmt, ...)
{
	char buf[1024];
	va_list ap;
	
	va_start( ap, fmt );
	vsprintf( buf, fmt, ap );
	va_end( ap );

#ifdef WIN32
	buf[1023] = 0;
	OutputDebugString(buf);
#else
	printf("%s", buf);
#endif
}

void* Mem_Alloc16( const size_t size, const memTag_t tag )
{
	size_t paddedsize = ((size + 15) & ~15);
	unsigned char *m;

	if (debugheapstart == NULL)
	{
		Sys_MutexCreate(debugheapmutex);
#if 0
		debugheapstart = (unsigned char *)malloc(DEBUGHEAP_BUFFER);
		debugheapend = debugheapstart + DEBUGHEAP_BUFFER;
#else
		// the buffer itself
		static unsigned char debugheapbuffer[DEBUGHEAP_BUFFER];
		debugheapstart = debugheapbuffer;
		debugheapend = debugheapbuffer + sizeof(debugheapbuffer);
#endif
		debugheapcur = debugheapstart;
	}

	if (size == 0)
	{
		//Mem_Printf("DEBUGHEAP Mem_Alloc16(%i,%i) size == 0\n", (int)size, (int)tag);
		return NULL;
	}

	Sys_MutexLock(debugheapmutex, true);
	if (debugheaplivecount == DEBUGHEAP_LIST)
	{
		Mem_Printf("DEBUGHEAP Mem_Alloc16(%i,%i) debugheaplivecount == DEBUGHEAP_LIST\n", (int)size, (int)tag);
		Sys_MutexUnlock(debugheapmutex);
		return NULL;
	}
	if (debugheapcur+size > debugheapend)
	{
		Mem_Printf("DEBUGHEAP Mem_Alloc16(%i,%i) debugheapcur+size > DEBUGHEAP_BUFFER\n", (int)size, (int)tag);
		Sys_MutexUnlock(debugheapmutex);
		return NULL;
	}

	// advance the heap in 16 byte increments, so that the return addresses are always aligned (as expected)
	m = debugheapcur;
	debugheapcur += paddedsize;

	// add the new range to the live pointer data
	debugheaplivepointers[debugheaplivecount][0] = m;
	debugheaplivepointers[debugheaplivecount][1] = m + size;
	debugheapliveflag[debugheaplivecount] = 1; // allocated - this will be changed to 0 on free
	debugheaplivecount++;

	// fill the memory with intentional garbage
	for (size_t i = 0;i < paddedsize;i += 4)
		*((int*)(m + i)) = 0xDEADBEEF;

	// we're done with the mutex now
	Sys_MutexUnlock(debugheapmutex);

	return m;
}

void Mem_Free16( void* ptr )
{
	unsigned char *m = (unsigned char *)ptr;
	if (ptr == NULL)
	{
		//Mem_Printf("Mem_Free16(%p) pointer == NULL\n", ptr);
		return;
	}
	Sys_MutexLock(debugheapmutex, true);
	if (m >= debugheapstart && m < debugheapend)
	{
		// do a binary search to find the pointer quickly - we know they are in sorted order
		int i = 0, j = debugheaplivecount, k = 0;
		for (;;)
		{
			k = (i + j) >> 1;
			if (debugheaplivepointers[k][0] < m)
			{
				if (i == k)
					break;
				i = k;
			}
			else if (debugheaplivepointers[k][0] > m)
			{
				if (j == k)
					break;
				j = k;
			}
			else
				break;
		}
		if (m >= debugheaplivepointers[k][0] && m < debugheaplivepointers[k][1])
		{
			if (m == debugheaplivepointers[k][0])
			{
				// a perfectly normal freeing operation, cool!  we don't really do that.
				if (debugheapliveflag[k])
				{
					// we'll notice this flag is cleared next time a free occurs on the same pointer
					debugheapliveflag[k] = 0;
					Sys_MutexUnlock(debugheapmutex);
					return;
				}
				else
					Mem_Printf("DEBUGHEAP: Mem_Free16(%p) pointer is a duplicate free!\n", ptr);
			}
			else
				Mem_Printf("DEBUGHEAP: Mem_Free16(%p) pointer is not at the start of the block! (%p ... %p)\n", ptr, debugheaplivepointers[k][0], debugheaplivepointers[k][1]);
		}
		Mem_Printf("DEBUGHEAP: Mem_Free16(%p) pointer is in debugheap but does not match any range!\n", ptr);
	}
	else
		Mem_Printf("DEBUGHEAP: Mem_Free16(%p) pointer isn't even in the debugheap range! (%p ... %p)\n", ptr, debugheapstart, debugheapend);
	Sys_MutexUnlock(debugheapmutex);
}

#else // DEBUGHEAP

/*
==================
Mem_Alloc16
==================
*/
// RB: 64 bit fixes, changed int to size_t
void* Mem_Alloc16( const size_t size, const memTag_t tag )
// RB end
{
	if( !size )
	{
		return NULL;
	}
	const size_t paddedSize = ( size + 15 ) & ~15;
#ifdef _WIN32
	// this should work with MSVC and mingw, as long as __MSVCRT_VERSION__ >= 0x0700
	return _aligned_malloc( paddedSize, 16 );
#else // not _WIN32
	// DG: the POSIX solution for linux etc
	void* ret;
	posix_memalign( &ret, 16, paddedSize );
	return ret;
	// DG end
#endif // _WIN32
}

/*
==================
Mem_Free16
==================
*/
void Mem_Free16( void* ptr )
{
	if( ptr == NULL )
	{
		return;
	}
#ifdef _WIN32
	_aligned_free( ptr );
#else // not _WIN32
	// DG: Linux/POSIX compatibility
	// can use normal free() for aligned memory
	free( ptr );
	// DG end
#endif // _WIN32
}

#endif // !DEBUGHEAP

/*
==================
Mem_ClearedAlloc
==================
*/
void* Mem_ClearedAlloc( const size_t size, const memTag_t tag )
{
	void* mem = Mem_Alloc( size, tag );
	SIMDProcessor->Memset( mem, 0, size );
	return mem;
}

/*
==================
Mem_CopyString
==================
*/
char* Mem_CopyString( const char* in )
{
	char* out = ( char* )Mem_Alloc( strlen( in ) + 1, TAG_STRING );
	strcpy( out, in );
	return out;
}

