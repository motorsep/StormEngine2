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
#pragma hdrstop
#include "precompiled.h"

#include "Dmap_tr_local.h"

/*
==========================================================================================

GUI SHADERS

==========================================================================================
*/

/*
================
R_SurfaceToTextureAxisDmap

Calculates two axis for the surface sutch that a point dotted against
the axis will give a 0.0 to 1.0 range in S and T when inside the gui surface
================
*/
void R_SurfaceToTextureAxisDmap(const srfDmapTriangles_t *tri, idVec3 &origin, idVec3 axis[3]) {
	float		area, inva;
	float		d0[5], d1[5];
	idDmapDrawVert	*a, *b, *c;
	float		bounds[2][2];
	float		boundsOrg[2];
	int			i, j;
	float		v;

	// find the bounds of the texture
	bounds[0][0] = bounds[0][1] = 999999;
	bounds[1][0] = bounds[1][1] = -999999;
	for (i = 0; i < tri->numVerts; i++) {
		for (j = 0; j < 2; j++) {
			v = tri->verts[i].st[j];
			if (v < bounds[0][j]) {
				bounds[0][j] = v;
			}
			if (v > bounds[1][j]) {
				bounds[1][j] = v;
			}
		}
	}

	// use the floor of the midpoint as the origin of the
	// surface, which will prevent a slight misalignment
	// from throwing it an entire cycle off
	boundsOrg[0] = floor((bounds[0][0] + bounds[1][0]) * 0.5);
	boundsOrg[1] = floor((bounds[0][1] + bounds[1][1]) * 0.5);


	// determine the world S and T vectors from the first drawSurf triangle
	a = tri->verts + tri->indexes[0];
	b = tri->verts + tri->indexes[1];
	c = tri->verts + tri->indexes[2];

	VectorSubtract(b->xyz, a->xyz, d0);
	d0[3] = b->st[0] - a->st[0];
	d0[4] = b->st[1] - a->st[1];
	VectorSubtract(c->xyz, a->xyz, d1);
	d1[3] = c->st[0] - a->st[0];
	d1[4] = c->st[1] - a->st[1];

	area = d0[3] * d1[4] - d0[4] * d1[3];
	if (area == 0.0) {
		origin.Zero();
		axis[0].Zero();
		axis[1].Zero();
		axis[2].Zero();
		return;	// degenerate
	}
	inva = 1.0 / area;

	axis[0][0] = (d0[0] * d1[4] - d0[4] * d1[0]) * inva;
	axis[0][1] = (d0[1] * d1[4] - d0[4] * d1[1]) * inva;
	axis[0][2] = (d0[2] * d1[4] - d0[4] * d1[2]) * inva;

	axis[1][0] = (d0[3] * d1[0] - d0[0] * d1[3]) * inva;
	axis[1][1] = (d0[3] * d1[1] - d0[1] * d1[3]) * inva;
	axis[1][2] = (d0[3] * d1[2] - d0[2] * d1[3]) * inva;

	idPlane plane;
	plane.FromPoints(a->xyz, b->xyz, c->xyz);
	axis[2][0] = plane[0];
	axis[2][1] = plane[1];
	axis[2][2] = plane[2];

	// take point 0 and project the vectors to the texture origin
	VectorMA(a->xyz, boundsOrg[0] - a->st[0], axis[0], origin);
	VectorMA(origin, boundsOrg[1] - a->st[1], axis[1], origin);
}
