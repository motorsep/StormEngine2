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
===========================================================================

idInteraction implementation

===========================================================================
*/

/*
================
R_CalcInteractionFacingDmap

Determines which triangles of the surface are facing towards the light origin.

The facing array should be allocated with one extra index than
the number of surface triangles, which will be used to handle dangling
edge silhouettes.
================
*/
void R_CalcInteractionFacingDmap(const idDmapRenderEntityLocal* ent, const srfDmapTriangles_t* tri, const idDmapRenderLightLocal* light, srfCullInfo_t& cullInfo)
{
	idVec3 localLightOrigin;

	if (cullInfo.facing != NULL) {
		return;
	}

	R_GlobalPointToLocal(ent->modelMatrix, light->globalLightOrigin, localLightOrigin);

	int numFaces = tri->numIndexes / 3;

	if (!tri->facePlanes || !tri->facePlanesCalculated) {
		R_DeriveFacePlanesDmap(const_cast<srfDmapTriangles_t *>(tri));
	}

	cullInfo.facing = (byte *)R_StaticAlloc((numFaces + 1) * sizeof(cullInfo.facing[0]));

	// calculate back face culling
	float *planeSide = (float *)_alloca16(numFaces * sizeof(float));

	// exact geometric cull against face
	dmapSIMDProcessor->Dot(planeSide, localLightOrigin, tri->facePlanes, numFaces);
	dmapSIMDProcessor->CmpGE(cullInfo.facing, planeSide, 0.0f, numFaces);

	cullInfo.facing[numFaces] = 1;	// for dangling edges to reference
}

/*
=====================
R_CalcInteractionCullBitsDmap

We want to cull a little on the sloppy side, because the pre-clipping
of geometry to the lights in dmap will give many cases that are right
at the border. We throw things out on the border, because if any one
vertex is clearly inside, the entire triangle will be accepted.
=====================
*/
void R_CalcInteractionCullBitsDmap(const idDmapRenderEntityLocal* ent, const srfDmapTriangles_t* tri, const idDmapRenderLightLocal* light, srfCullInfo_t& cullInfo)
{
	int i, frontBits;

	if (cullInfo.cullBits != NULL) {
		return;
	}

	frontBits = 0;

	// cull the triangle surface bounding box
	for (i = 0; i < 6; i++) {

		R_GlobalPlaneToLocal(ent->modelMatrix, -light->frustum[i], cullInfo.localClipPlanes[i]);

		// get front bits for the whole surface
		if (tri->bounds.PlaneDistance(cullInfo.localClipPlanes[i]) >= LIGHT_CLIP_EPSILON) {
			frontBits |= 1 << i;
		}
	}

	// if the surface is completely inside the light frustum
	if (frontBits == ((1 << 6) - 1)) {
		cullInfo.cullBits = LIGHT_CULL_ALL_FRONT;
		return;
	}

	cullInfo.cullBits = (byte *)R_StaticAlloc(tri->numVerts * sizeof(cullInfo.cullBits[0]));
	dmapSIMDProcessor->Memset(cullInfo.cullBits, 0, tri->numVerts * sizeof(cullInfo.cullBits[0]));

	float *planeSide = (float *)_alloca16(tri->numVerts * sizeof(float));

	for (i = 0; i < 6; i++) {
		// if completely infront of this clipping plane
		if (frontBits & (1 << i)) {
			continue;
		}
		dmapSIMDProcessor->Dot(planeSide, cullInfo.localClipPlanes[i], tri->verts, tri->numVerts);
		dmapSIMDProcessor->CmpLT(cullInfo.cullBits, i, planeSide, LIGHT_CLIP_EPSILON, tri->numVerts);
	}
}
