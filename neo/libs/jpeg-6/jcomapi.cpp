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


/*
 * Abort processing of a JPEG compression or decompression operation,
 * but don't destroy the object itself.
 *
 * For this, we merely clean up all the nonpermanent memory pools.
 * Note that temp files (virtual arrays) are not allowed to belong to
 * the permanent pool, so we will be able to close all temp files here.
 * Closing a data source or destination, if necessary, is the application's
 * responsibility.
 */

GLOBAL void
jpeg_abort( j_common_ptr cinfo ) {
    int pool;

    /* Releasing pools in reverse order might help avoid fragmentation
     * with some (brain-damaged) malloc libraries.
     */
    for ( pool = JPOOL_NUMPOOLS - 1; pool > JPOOL_PERMANENT; pool-- ) {
        ( *cinfo->mem->free_pool )( cinfo, pool );
    }

    /* Reset overall state for possible reuse of object */
    cinfo->global_state = ( cinfo->is_decompressor ? DSTATE_START : CSTATE_START );
}


/*
 * Destruction of a JPEG object.
 *
 * Everything gets deallocated except the master jpeg_compress_struct itself
 * and the error manager struct.  Both of these are supplied by the application
 * and must be freed, if necessary, by the application.  (Often they are on
 * the stack and so don't need to be freed anyway.)
 * Closing a data source or destination, if necessary, is the application's
 * responsibility.
 */

GLOBAL void
jpeg_destroy( j_common_ptr cinfo ) {
    /* We need only tell the memory manager to release everything. */
    /* NB: mem pointer is NULL if memory mgr failed to initialize. */
    if ( cinfo->mem != NULL ) {
        ( *cinfo->mem->self_destruct )( cinfo );
    }
    cinfo->mem = NULL;      /* be safe if jpeg_destroy is called twice */
    cinfo->global_state = 0;/* mark it destroyed */
}


/*
 * Convenience routines for allocating quantization and Huffman tables.
 * (Would jutils.c be a more reasonable place to put these?)
 */

GLOBAL JQUANT_TBL *
jpeg_alloc_quant_table( j_common_ptr cinfo ) {
    JQUANT_TBL * tbl;

    tbl = (JQUANT_TBL *)
          ( *cinfo->mem->alloc_small )( cinfo, JPOOL_PERMANENT, SIZEOF( JQUANT_TBL ) );
    tbl->sent_table = FALSE;/* make sure this is false in any new table */
    return tbl;
}


GLOBAL JHUFF_TBL *
jpeg_alloc_huff_table( j_common_ptr cinfo ) {
    JHUFF_TBL * tbl;

    tbl = (JHUFF_TBL *)
          ( *cinfo->mem->alloc_small )( cinfo, JPOOL_PERMANENT, SIZEOF( JHUFF_TBL ) );
    tbl->sent_table = FALSE;/* make sure this is false in any new table */
    return tbl;
}
