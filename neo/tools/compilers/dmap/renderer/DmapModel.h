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
#ifndef __DMAPMODEL_H__
#define __DMAPMODEL_H__

/*
===============================================================================

Render Model

===============================================================================
*/

// this is used for calculating unsmoothed normals and tangents for deformed models
typedef struct dmapDominantTri_s {
	uint					v2, v3;
	float					normalizationScale[3];
} dmapDominantTri_t;

typedef struct shadowCache_s {
	idVec4						xyz;					// we use homogenous coordinate tricks
} shadowCache_t;

// our only drawing geometry type
typedef struct srfDmapTriangles_s {
	idBounds					bounds;					// for culling

	int							ambientViewCount;		// if == dmap_tr.viewCount, it is visible this view

	bool						generateNormals;		// create normals from geometry, instead of using explicit ones
	bool						tangentsCalculated;		// set when the vertex tangents have been calculated
	bool						facePlanesCalculated;	// set when the face planes have been calculated
	bool						perfectHull;			// true if there aren't any dangling edges
	bool						deformedSurface;		// if true, indexes, silIndexes, mirrorVerts, and silEdges are
	// pointers into the original surface, and should not be freed

	int							numVerts;				// number of vertices
	idDmapDrawVert *			verts;					// vertices, allocated with special allocator

	int							numIndexes;				// for shadows, this has both front and rear end caps and silhouette planes
	uint *					indexes;				// indexes, allocated with special allocator

	uint *					silIndexes;				// indexes changed to be the first vertex with same XYZ, ignoring normal and texcoords

	int							numMirroredVerts;		// this many verts at the end of the vert list are tangent mirrors
	int *						mirroredVerts;			// tri->mirroredVerts[0] is the mirror of tri->numVerts - tri->numMirroredVerts + 0

	int							numDupVerts;			// number of duplicate vertexes
	int *						dupVerts;				// pairs of the number of the first vertex and the number of the duplicate vertex

	int							numSilEdges;			// number of silhouette edges
	silEdge_t *					silEdges;				// silhouette edges

	idPlane *					facePlanes;				// [numIndexes/3] plane equations

	dmapDominantTri_t *				dominantTris;			// [numVerts] for deformed surface fast tangent calculation

	int							numShadowIndexesNoFrontCaps;	// shadow volumes with front caps omitted
	int							numShadowIndexesNoCaps;			// shadow volumes with the front and rear caps omitted

	int							shadowCapPlaneBits;		// bits 0-5 are set when that plane of the interacting light has triangles
	// projected on it, which means that if the view is on the outside of that
	// plane, we need to draw the rear caps of the shadow volume
	// turboShadows will have SHADOW_CAP_INFINITE

	shadowCache_t *				shadowVertexes;			// these will be copied to shadowCache when it is going to be drawn.
	// these are NULL when vertex programs are available

	struct srfDmapTriangles_s *		ambientSurface;			// for light interactions, point back at the original surface that generated
	// the interaction, which we will get the ambientCache from

	struct srfDmapTriangles_s *		nextDeferredFree;		// chain of tris to free next frame

	// data in vertex object space, not directly readable by the CPU
	struct dmapVertCache_s *		indexCache;				// int
	struct dmapVertCache_s *		ambientCache;			// idDrawVert	
	struct dmapVertCache_s *		shadowCache;			// shadowCache_t
} srfDmapTriangles_t;

typedef idList<srfDmapTriangles_t *> idDmapTriList;

typedef struct dmapModelSurface_s {
	int							id;
	const idMaterial *			shader;
	srfDmapTriangles_t *			geometry;
} dmapModelSurface_t;

// the init methods may be called again on an already created model when
// a reloadModels is issued

class idDmapRenderModel {
public:
	virtual						~idDmapRenderModel() {};

	// Loads static models only, dynamic models must be loaded by the modelManager
	virtual void				InitFromFile(const char *fileName) = 0;

	// renderBump uses this to load the very high poly count models, skipping the
	// shadow and tangent generation, along with some surface cleanup to make it load faster
	virtual void				PartialInitFromFile(const char *fileName) = 0;

	// this is used for dynamically created surfaces, which are assumed to not be reloadable.
	// It can be called again to clear out the surfaces of a dynamic model for regeneration.
	virtual void				InitEmpty(const char *name) = 0;

	// dynamic model instantiations will be created with this
	// the geometry data will be owned by the model, and freed when it is freed
	// the geoemtry should be raw triangles, with no extra processing
	virtual void				AddSurface(dmapModelSurface_t surface) = 0;

	// cleans all the geometry and performs cross-surface processing
	// like shadow hulls
	// Creates the duplicated back side geometry for two sided, alpha tested, lit materials
	// This does not need to be called if none of the surfaces added with AddSurface require
	// light interaction, and all the triangles are already well formed.
	virtual void				FinishSurfaces() = 0;

	// frees all the data, but leaves the class around for dangling references,
	// which can regenerate the data with LoadModel()
	virtual void				PurgeModel() = 0;

	// resets any model information that needs to be reset on a same level load etc..
	// currently only implemented for liquids
	virtual void				Reset() = 0;

	// used for initial loads, reloadModel, and reloading the data of purged models
	// Upon exit, the model will absolutely be valid, but possibly as a default model
	virtual void				LoadModel() = 0;

	// internal use
	virtual bool				IsLoaded() const = 0;
	virtual void				SetLevelLoadReferenced(bool referenced) = 0;
	virtual bool				IsLevelLoadReferenced() = 0;

	// models that are already loaded at level start time
	// will still touch their data to make sure they
	// are kept loaded
	virtual void				TouchData() = 0;

	// dump any ambient caches on the model surfaces
	virtual void				FreeVertexCache() = 0;

	// returns the name of the model
	virtual const char	*		Name() const = 0;

	// prints a detailed report on the model for printModel
	virtual void				Print() const = 0;

	// prints a single line report for listModels
	virtual void				List() const = 0;

	// reports the amount of memory (roughly) consumed by the model
	virtual int					Memory() const = 0;

	// for reloadModels
	virtual ID_TIME_T				Timestamp() const = 0;

	// returns the number of surfaces
	virtual int					NumSurfaces() const = 0;

	// NumBaseSurfaces will not count any overlays added to dynamic models
	virtual int					NumBaseSurfaces() const = 0;

	// get a pointer to a surface
	virtual const dmapModelSurface_t *Surface(int surfaceNum) const = 0;

	// Allocates surface triangles.
	// Allocates memory for srfDmapTriangles_t::verts and srfDmapTriangles_t::indexes
	// The allocated memory is not initialized.
	// srfDmapTriangles_t::numVerts and srfDmapTriangles_t::numIndexes are set to zero.
	virtual srfDmapTriangles_t *	AllocSurfaceTriangles(int numVerts, int numIndexes) const = 0;

	// Frees surfaces triangles.
	virtual void				FreeSurfaceTriangles(srfDmapTriangles_t *tris) const = 0;

	// created at load time by stitching together all surfaces and sharing
	// the maximum number of edges.  This may be incorrect if a skin file
	// remaps surfaces between shadow casting and non-shadow casting, or
	// if some surfaces are noSelfShadow and others aren't
	virtual srfDmapTriangles_t	*	ShadowHull() const = 0;

	// models of the form "_area*" may have a prelight shadow model associated with it
	virtual bool				IsStaticWorldModel() const = 0;

	// models parsed from inside map files or dynamically created cannot be reloaded by
	// reloadmodels
	virtual bool				IsReloadable() const = 0;

	// md3, md5, particles, etc
	virtual dynamicModel_t		IsDynamicModel() const = 0;

	// if the load failed for any reason, this will return true
	virtual bool				IsDefaultModel() const = 0;

	// dynamic models should return a fast, conservative approximation
	// static models should usually return the exact value
	virtual idBounds			Bounds(const struct dmapRenderEntity_s *ent = NULL) const = 0;

	// returns value != 0.0f if the model requires the depth hack
	virtual float				DepthHack() const = 0;

	// returns a static model based on the definition and view
	// currently, this will be regenerated for every view, even though
	// some models, like character meshes, could be used for multiple (mirror)
	// views in a frame, or may stay static for multiple frames (corpses)
	// The renderer will delete the returned dynamic model the next view
	// This isn't const, because it may need to reload a purged model if it
	// wasn't precached correctly.
	virtual idDmapRenderModel *		InstantiateDynamicModel(const struct dmapRenderEntity_s *ent, const struct dmapViewDef_s *view, idDmapRenderModel *cachedModel) = 0;

	// Returns the number of joints or 0 if the model is not an MD5
	virtual int					NumJoints(void) const = 0;

	// Returns the MD5 joints or NULL if the model is not an MD5
	virtual const idMD5Joint *	GetJoints(void) const = 0;

	// Returns the handle for the joint with the given name.
	virtual jointHandle_t		GetJointHandle(const char *name) const = 0;

	// Returns the name for the joint with the given handle.
	virtual const char *		GetJointName(jointHandle_t handle) const = 0;

	// Returns the default animation pose or NULL if the model is not an MD5.
	virtual const idJointQuat *	GetDefaultPose(void) const = 0;

	// Returns number of the joint nearest to the given triangle.
	virtual int					NearestJoint(int surfaceNum, int a, int c, int b) const = 0;

	// Writing to and reading from a demo file.
	virtual void				ReadFromDemoFile(class idDemoFile *f) = 0;
	virtual void				WriteToDemoFile(class idDemoFile *f) = 0;
};

#endif /* !__DMAPMODEL_H__ */
