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

//#include "sys/platform.h"
#include "precompiled.h"
#pragma hdrstop

#include <stdio.h>
#include "renderer/jpeg_memory_src.h"

#ifdef HAVE_JPEG_MEM_SRC
void jpeg_memory_src(j_decompress_ptr cinfo, unsigned char *inbuffer, unsigned long insize) {
	jpeg_mem_src(cinfo, inbuffer, insize);
}
#else
static void init_mem_source(j_decompress_ptr cinfo) {
	/* no work necessary here */
}

static boolean fill_mem_input_buffer(j_decompress_ptr cinfo) {
	static JOCTET mybuffer[4];

	/* The whole JPEG data is expected to reside in the supplied memory
	 * buffer, so any request for more data beyond the given buffer size
	 * is treated as an error.
	 */
	WARNMS(cinfo, JWRN_JPEG_EOF);
	/* Insert a fake EOI marker */
	mybuffer[0] = (JOCTET) 0xFF;
	mybuffer[1] = (JOCTET) JPEG_EOI;

	cinfo->src->next_input_byte = mybuffer;
	cinfo->src->bytes_in_buffer = 2;

	return TRUE;
}

static void skip_input_data(j_decompress_ptr cinfo, long num_bytes) {
	struct jpeg_source_mgr *src = cinfo->src;

	if (num_bytes > 0) {
		while (num_bytes > (long)src->bytes_in_buffer) {
			num_bytes -= (long)src->bytes_in_buffer;
			(void)(*src->fill_input_buffer) (cinfo);
			/* note we assume that fill_input_buffer will never return FALSE,
			 * so suspension need not be handled.
			 */
		}
		src->next_input_byte += (size_t)num_bytes;
		src->bytes_in_buffer -= (size_t)num_bytes;
	}
}

static void term_source(j_decompress_ptr cinfo)
{
	/* no work necessary here */
}

void jpeg_memory_src(j_decompress_ptr cinfo, unsigned char *inbuffer, unsigned long insize) {
	struct jpeg_source_mgr *src;

	if (inbuffer == NULL || insize == 0)	/* Treat empty input as fatal error */
		ERREXIT(cinfo, JERR_INPUT_EMPTY);

	/* The source object is made permanent so that a series of JPEG images
	 * can be read from the same buffer by calling jpeg_mem_src only before
	 * the first one.
	 */
	if (cinfo->src == NULL) {	/* first time for this JPEG object? */
		cinfo->src = (struct jpeg_source_mgr *)
			(*cinfo->mem->alloc_small)((j_common_ptr)cinfo, JPOOL_PERMANENT,
										sizeof(struct jpeg_source_mgr));
	}

	src = cinfo->src;
	src->init_source = init_mem_source;
	src->fill_input_buffer = fill_mem_input_buffer;
	src->skip_input_data = skip_input_data;
	src->resync_to_restart = jpeg_resync_to_restart; /* use default method */
	src->term_source = term_source;
	src->bytes_in_buffer = (size_t)insize;
	src->next_input_byte = (JOCTET *)inbuffer;
}
#endif
