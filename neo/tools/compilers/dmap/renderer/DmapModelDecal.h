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
#ifndef __DMAPMODELDECAL_H__
#define __DMAPMODELDECAL_H__

/*
===============================================================================

Decals are lightweight primitives for bullet / blood marks on static
geometry. Decals with common materials will be merged together, but
additional decals will be allocated as needed. The material should not
be one that receives lighting, because no interactions are generated
for these lightweight surfaces.

FIXME:	Decals on models in portalled off areas do not get freed
until the area becomes visible again.

===============================================================================
*/

typedef struct decalProjectionInfo_s {
	idVec3						projectionOrigin;
	idBounds					projectionBounds;
	idPlane						boundingPlanes[6];
	idPlane						fadePlanes[2];
	idPlane						textureAxis[2];
	const idMaterial *			material;
	bool						parallel;
	float						fadeDepth;
	int							startTime;
	bool						force;
} decalProjectionInfo_t;

class idDmapRenderModelDecal {
public:
	idDmapRenderModelDecal(void);
	~idDmapRenderModelDecal(void);

	static idDmapRenderModelDecal *	Alloc(void);
	static void					Free(idDmapRenderModelDecal *decal);

	// Creates decal projection info.
	static bool					CreateProjectionInfo(decalProjectionInfo_t &info, const idFixedWinding &winding, const idVec3 &projectionOrigin, const bool parallel, const float fadeDepth, const idMaterial *material, const int startTime);

	// Transform the projection info from global space to local.
	static void					GlobalProjectionInfoToLocal(decalProjectionInfo_t &localInfo, const decalProjectionInfo_t &info, const idVec3 &origin, const idMat3 &axis);

	// Creates a deal on the given model.
	void						CreateDecal(const idDmapRenderModel *model, const decalProjectionInfo_t &localInfo);

	// Remove decals that are completely faded away.
	static idDmapRenderModelDecal *	RemoveFadedDecals(idDmapRenderModelDecal *decals, int time);

	// Updates the vertex colors, removing any faded indexes,
	// then copy the verts to temporary vertex cache and adds a drawSurf.
	void						AddDecalDrawSurf(struct dmapViewEntity_s *space);

	// Returns the next decal in the chain.
	idDmapRenderModelDecal *		Next(void) const { return nextDecal; }

	void						ReadFromDemoFile(class idDemoFile *f);
	void						WriteToDemoFile(class idDemoFile *f) const;

private:
	static const int			MAX_DECAL_VERTS = 40;
	static const int			MAX_DECAL_INDEXES = 60;

	const idMaterial *			material;
	srfDmapTriangles_t			tri;
	idDmapDrawVert				verts[MAX_DECAL_VERTS];
	float						vertDepthFade[MAX_DECAL_VERTS];
	uint						indexes[MAX_DECAL_INDEXES];
	int							indexStartTime[MAX_DECAL_INDEXES];
	idDmapRenderModelDecal *	nextDecal;

	// Adds the winding triangles to the appropriate decal in the
	// chain, creating a new one if necessary.
	void						AddWinding(const idWinding &w, const idMaterial *decalMaterial, const idPlane fadePlanes[2], float fadeDepth, int startTime);

	// Adds depth faded triangles for the winding to the appropriate
	// decal in the chain, creating a new one if necessary.
	// The part of the winding at the front side of both fade planes is not faded.
	// The parts at the back sides of the fade planes are faded with the given depth.
	void						AddDepthFadedWinding(const idWinding &w, const idMaterial *decalMaterial, const idPlane fadePlanes[2], float fadeDepth, int startTime);
};
#endif /* !__DMAPMODELDECAL_H__ */
