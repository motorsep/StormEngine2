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
 * Master selection of compression modules.
 * This is done once at the start of processing an image.  We determine
 * which modules will be used and give them appropriate initialization calls.
 */

GLOBAL void
jinit_compress_master( j_compress_ptr cinfo ) {
    /* Initialize master control (includes parameter checking/processing) */
    jinit_c_master_control( cinfo, FALSE /* full compression */ );

    /* Preprocessing */
    if ( !cinfo->raw_data_in ) {
        jinit_color_converter( cinfo );
        jinit_downsampler( cinfo );
        jinit_c_prep_controller( cinfo, FALSE /* never need full buffer here */ );
    }
    /* Forward DCT */
    jinit_forward_dct( cinfo );
    /* Entropy encoding: either Huffman or arithmetic coding. */
    if ( cinfo->arith_code ) {
        ERREXIT( cinfo, JERR_ARITH_NOTIMPL );
    } else {
        if ( cinfo->progressive_mode ) {
#ifdef C_PROGRESSIVE_SUPPORTED
            jinit_phuff_encoder( cinfo );
#else
            ERREXIT( cinfo, JERR_NOT_COMPILED );
#endif
        } else {
            jinit_huff_encoder( cinfo );
        }
    }

    /* Need a full-image coefficient buffer in any multi-pass mode. */
    jinit_c_coef_controller( cinfo,
                            ( cinfo->num_scans > 1 || cinfo->optimize_coding ) );
    jinit_c_main_controller( cinfo, FALSE /* never need full buffer here */ );

    jinit_marker_writer( cinfo );

    /* We can now tell the memory manager to allocate virtual arrays. */
    ( *cinfo->mem->realize_virt_arrays )( (j_common_ptr) cinfo );

    /* Write the datastream header (SOI) immediately.
     * Frame and scan headers are postponed till later.
     * This lets application insert special markers after the SOI.
     */
    ( *cinfo->marker->write_file_header )( cinfo );
}
