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
#include "jdct.h"        /* Private declarations for DCT subsystem */

#ifdef DCT_FLOAT_SUPPORTED


/*
 * This module is specialized to the case DCTSIZE = 8.
 */

#if DCTSIZE != 8
Sorry, this code only copes with 8 x8 DCTs.  /* deliberate syntax err */
    #endif


/*
 * Perform the forward DCT on one block of samples.
 */

GLOBAL void
jpeg_fdct_float( FAST_FLOAT * data ) {
    FAST_FLOAT tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
    FAST_FLOAT tmp10, tmp11, tmp12, tmp13;
    FAST_FLOAT z1, z2, z3, z4, z5, z11, z13;
    FAST_FLOAT * dataptr;
    int ctr;

    /* Pass 1: process rows. */

    dataptr = data;
    for ( ctr = DCTSIZE - 1; ctr >= 0; ctr-- ) {
        tmp0 = dataptr[0] + dataptr[7];
        tmp7 = dataptr[0] - dataptr[7];
        tmp1 = dataptr[1] + dataptr[6];
        tmp6 = dataptr[1] - dataptr[6];
        tmp2 = dataptr[2] + dataptr[5];
        tmp5 = dataptr[2] - dataptr[5];
        tmp3 = dataptr[3] + dataptr[4];
        tmp4 = dataptr[3] - dataptr[4];

        /* Even part */

        tmp10 = tmp0 + tmp3;/* phase 2 */
        tmp13 = tmp0 - tmp3;
        tmp11 = tmp1 + tmp2;
        tmp12 = tmp1 - tmp2;

        dataptr[0] = tmp10 + tmp11;/* phase 3 */
        dataptr[4] = tmp10 - tmp11;

        z1 = ( tmp12 + tmp13 ) * ( (FAST_FLOAT) 0.707106781 );/* c4 */
        dataptr[2] = tmp13 + z1;/* phase 5 */
        dataptr[6] = tmp13 - z1;

        /* Odd part */

        tmp10 = tmp4 + tmp5;/* phase 2 */
        tmp11 = tmp5 + tmp6;
        tmp12 = tmp6 + tmp7;

        /* The rotator is modified from fig 4-8 to avoid extra negations. */
        z5 = ( tmp10 - tmp12 ) * ( (FAST_FLOAT) 0.382683433 );/* c6 */
        z2 = ( (FAST_FLOAT) 0.541196100 ) * tmp10 + z5;/* c2-c6 */
        z4 = ( (FAST_FLOAT) 1.306562965 ) * tmp12 + z5;/* c2+c6 */
        z3 = tmp11 * ( (FAST_FLOAT) 0.707106781 );/* c4 */

        z11 = tmp7 + z3;    /* phase 5 */
        z13 = tmp7 - z3;

        dataptr[5] = z13 + z2;/* phase 6 */
        dataptr[3] = z13 - z2;
        dataptr[1] = z11 + z4;
        dataptr[7] = z11 - z4;

        dataptr += DCTSIZE; /* advance pointer to next row */
    }

    /* Pass 2: process columns. */

    dataptr = data;
    for ( ctr = DCTSIZE - 1; ctr >= 0; ctr-- ) {
        tmp0 = dataptr[DCTSIZE * 0] + dataptr[DCTSIZE * 7];
        tmp7 = dataptr[DCTSIZE * 0] - dataptr[DCTSIZE * 7];
        tmp1 = dataptr[DCTSIZE * 1] + dataptr[DCTSIZE * 6];
        tmp6 = dataptr[DCTSIZE * 1] - dataptr[DCTSIZE * 6];
        tmp2 = dataptr[DCTSIZE * 2] + dataptr[DCTSIZE * 5];
        tmp5 = dataptr[DCTSIZE * 2] - dataptr[DCTSIZE * 5];
        tmp3 = dataptr[DCTSIZE * 3] + dataptr[DCTSIZE * 4];
        tmp4 = dataptr[DCTSIZE * 3] - dataptr[DCTSIZE * 4];

        /* Even part */

        tmp10 = tmp0 + tmp3;/* phase 2 */
        tmp13 = tmp0 - tmp3;
        tmp11 = tmp1 + tmp2;
        tmp12 = tmp1 - tmp2;

        dataptr[DCTSIZE * 0] = tmp10 + tmp11;/* phase 3 */
        dataptr[DCTSIZE * 4] = tmp10 - tmp11;

        z1 = ( tmp12 + tmp13 ) * ( (FAST_FLOAT) 0.707106781 );/* c4 */
        dataptr[DCTSIZE * 2] = tmp13 + z1;/* phase 5 */
        dataptr[DCTSIZE * 6] = tmp13 - z1;

        /* Odd part */

        tmp10 = tmp4 + tmp5;/* phase 2 */
        tmp11 = tmp5 + tmp6;
        tmp12 = tmp6 + tmp7;

        /* The rotator is modified from fig 4-8 to avoid extra negations. */
        z5 = ( tmp10 - tmp12 ) * ( (FAST_FLOAT) 0.382683433 );/* c6 */
        z2 = ( (FAST_FLOAT) 0.541196100 ) * tmp10 + z5;/* c2-c6 */
        z4 = ( (FAST_FLOAT) 1.306562965 ) * tmp12 + z5;/* c2+c6 */
        z3 = tmp11 * ( (FAST_FLOAT) 0.707106781 );/* c4 */

        z11 = tmp7 + z3;    /* phase 5 */
        z13 = tmp7 - z3;

        dataptr[DCTSIZE * 5] = z13 + z2;/* phase 6 */
        dataptr[DCTSIZE * 3] = z13 - z2;
        dataptr[DCTSIZE * 1] = z11 + z4;
        dataptr[DCTSIZE * 7] = z11 - z4;

        dataptr++;      /* advance pointer to next column */
    }
}

#endif /* DCT_FLOAT_SUPPORTED */
