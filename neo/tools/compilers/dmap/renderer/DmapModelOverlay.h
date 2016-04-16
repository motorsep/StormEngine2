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
#ifndef __DMAPMODELOVERLAY_H__
#define __DMAPMODELOVERLAY_H__

/*
===============================================================================

Render model overlay for adding decals on top of dynamic models.

===============================================================================
*/

const int MAX_OVERLAY_SURFACES = 16;

typedef struct dmapOverlayVertex_s {
	int							vertexNum;
	float						st[2];
} dmapOverlayVertex_t;

typedef struct dmapOverlaySurface_s {
	int							surfaceNum;
	int							surfaceId;
	int							numIndexes;
	uint *						indexes;
	int							numVerts;
	dmapOverlayVertex_t *			verts;
} dmapOverlaySurface_t;

typedef struct dmapOverlayMaterial_s {
	const idMaterial *			material;
	idList<dmapOverlaySurface_t *>	surfaces;
} dmapOverlayMaterial_t;


class idDmapRenderModelOverlay {
public:
	idDmapRenderModelOverlay();
	~idDmapRenderModelOverlay();

	static idDmapRenderModelOverlay *Alloc(void);
	static void					Free(idDmapRenderModelOverlay *overlay);

	// Projects an overlay onto deformable geometry and can be added to
	// a render entity to allow decals on top of dynamic models.
	// This does not generate tangent vectors, so it can't be used with
	// light interaction shaders. Materials for overlays should always
	// be clamped, because the projected texcoords can run well off the
	// texture since no new clip vertexes are generated.
	void						CreateOverlay(const idDmapRenderModel *model, const idPlane localTextureAxis[2], const idMaterial *material);

	// Creates new model surfaces for baseModel, which should be a static instantiation of a dynamic model.
	void						AddOverlaySurfacesToModel(idDmapRenderModel *baseModel);

	// Removes overlay surfaces from the model.
	static void					RemoveOverlaySurfacesFromModel(idDmapRenderModel *baseModel);

	void						ReadFromDemoFile(class idDemoFile *f);
	void						WriteToDemoFile(class idDemoFile *f) const;

private:
	idList<dmapOverlayMaterial_t *>	materials;

	void						FreeSurface(dmapOverlaySurface_t *surface);
};

#endif /* !__DMAPMODELOVERLAY_H__ */
