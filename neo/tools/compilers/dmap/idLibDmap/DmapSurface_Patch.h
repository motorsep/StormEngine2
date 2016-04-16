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
#ifndef __DMAPSURFACE_PATCH_H__
#define __DMAPSURFACE_PATCH_H__

/*
===============================================================================

Bezier patch surface.

===============================================================================
*/

class idDmapSurface_Patch : public idDmapSurface
{

public:
	idDmapSurface_Patch();
	idDmapSurface_Patch(int maxPatchWidth, int maxPatchHeight);
	idDmapSurface_Patch(const idDmapSurface_Patch& patch);
	~idDmapSurface_Patch();

	void				SetSize(int patchWidth, int patchHeight);
	int					GetWidth() const;
	int					GetHeight() const;

	// subdivide the patch mesh based on error
	void				Subdivide(float maxHorizontalError, float maxVerticalError, float maxLength, bool genNormals = false);
	// subdivide the patch up to an explicit number of horizontal and vertical subdivisions
	void				SubdivideExplicit(int horzSubdivisions, int vertSubdivisions, bool genNormals, bool removeLinear = false);

protected:
	int					width;			// width of patch
	int					height;			// height of patch
	int					maxWidth;		// maximum width allocated for
	int					maxHeight;		// maximum height allocated for
	bool				expanded;		// true if vertices are spaced out

private:
	// put the approximation points on the curve
	void				PutOnCurve();
	// remove columns and rows with all points on one line
	void				RemoveLinearColumnsRows();
	// resize verts buffer
	void				ResizeExpanded(int height, int width);
	// space points out over maxWidth * maxHeight buffer
	void				Expand();
	// move all points to the start of the verts buffer
	void				Collapse();
	// project a point onto a vector to calculate maximum curve error
	void				ProjectPointOntoVector(const idVec3& point, const idVec3& vStart, const idVec3& vEnd, idVec3& vProj);
	// generate normals
	void				GenerateNormals();
	// generate triangle indexes
	void				GenerateIndexes();
	// lerp point from two patch point
	void				LerpVert(const idDmapDrawVert& a, const idDmapDrawVert& b, idDmapDrawVert& out) const;
	// sample a single 3x3 patch
	void				SampleSinglePatchPoint(const idDmapDrawVert ctrl[3][3], float u, float v, idDmapDrawVert* out) const;
	void				SampleSinglePatch(const idDmapDrawVert ctrl[3][3], int baseCol, int baseRow, int width, int horzSub, int vertSub, idDmapDrawVert* outVerts) const;
};

/*
=================
idDmapSurface_Patch::idDmapSurface_Patch
=================
*/
ID_INLINE idDmapSurface_Patch::idDmapSurface_Patch()
{
	height = width = maxHeight = maxWidth = 0;
	expanded = false;
}

/*
=================
idDmapSurface_Patch::idDmapSurface_Patch
=================
*/
ID_INLINE idDmapSurface_Patch::idDmapSurface_Patch(int maxPatchWidth, int maxPatchHeight)
{
	width = height = 0;
	maxWidth = maxPatchWidth;
	maxHeight = maxPatchHeight;
	verts.SetNum(maxWidth * maxHeight);
	expanded = false;
}

/*
=================
idDmapSurface_Patch::idDmapSurface_Patch
=================
*/
ID_INLINE idDmapSurface_Patch::idDmapSurface_Patch(const idDmapSurface_Patch& patch)
{
	(*this) = patch;
}

/*
=================
idDmapSurface_Patch::~idDmapSurface_Patch
=================
*/
ID_INLINE idDmapSurface_Patch::~idDmapSurface_Patch()
{
}

/*
=================
idDmapSurface_Patch::GetWidth
=================
*/
ID_INLINE int idDmapSurface_Patch::GetWidth() const
{
	return width;
}

/*
=================
idDmapSurface_Patch::GetHeight
=================
*/
ID_INLINE int idDmapSurface_Patch::GetHeight() const
{
	return height;
}

#endif /* !__DMAPSURFACE_PATCH_H__ */
