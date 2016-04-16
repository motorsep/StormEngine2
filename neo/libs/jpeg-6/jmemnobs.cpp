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
#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"
#include "jmemsys.h"     /* import the system-dependent declarations */

/*
 * Memory allocation and ri.Freeing are controlled by the regular library
 * routines ri.Malloc() and ri.Free().
 */

GLOBAL void *
jpeg_get_small( j_common_ptr cinfo, size_t sizeofobject ) {
    return (void *) malloc( sizeofobject );
}

GLOBAL void
jpeg_free_small( j_common_ptr cinfo, void * object, size_t sizeofobject ) {
    free( object );
}


/*
 * "Large" objects are treated the same as "small" ones.
 * NB: although we include FAR keywords in the routine declarations,
 * this file won't actually work in 80x86 small/medium model; at least,
 * you probably won't be able to process useful-size images in only 64KB.
 */

GLOBAL void FAR *
jpeg_get_large( j_common_ptr cinfo, size_t sizeofobject ) {
    return (void FAR *) malloc( sizeofobject );
}

GLOBAL void
jpeg_free_large( j_common_ptr cinfo, void FAR * object, size_t sizeofobject ) {
    free( object );
}


/*
 * This routine computes the total memory space available for allocation.
 * Here we always say, "we got all you want bud!"
 */

GLOBAL long
jpeg_mem_available( j_common_ptr cinfo, long min_bytes_needed,
                    long max_bytes_needed, long already_allocated ) {
    return max_bytes_needed;
}


/*
 * Backing store (temporary file) management.
 * Since jpeg_mem_available always promised the moon,
 * this should never be called and we can just error out.
 */

GLOBAL void
jpeg_open_backing_store( j_common_ptr cinfo, backing_store_ptr info,
                         long total_bytes_needed ) {
    ERREXIT( cinfo, JERR_NO_BACKING_STORE );
}


/*
 * These routines take care of any system-dependent initialization and
 * cleanup required.  Here, there isn't any.
 */

GLOBAL long
jpeg_mem_init( j_common_ptr cinfo ) {
    return 0;       /* just set max_memory_to_use to 0 */
}

GLOBAL void
jpeg_mem_term( j_common_ptr cinfo ) {
    /* no work */
}
