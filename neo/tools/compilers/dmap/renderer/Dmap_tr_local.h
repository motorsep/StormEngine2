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
#ifndef __DMAP_TR_LOCAL_H__
#define __DMAP_TR_LOCAL_H__

#include "../../../../renderer/tr_local.h"

#include "DmapModelDecal.h"
#include "DmapModelOverlay.h"
#include "DmapInteraction.h"

class idDmapRenderWorldLocal;

struct shadowFrustum_t {
	int		numPlanes;		// this is always 6 for now
	idPlane	planes[6];
	// positive sides facing inward
	// plane 5 is always the plane the projection is going to, the
	// other planes are just clip planes
	// all planes are in global coordinates

	bool	makeClippedPlanes;
	// a projected light with a single frustum needs to make sil planes
	// from triangles that clip against side planes, but a point light
	// that has adjacent frustums doesn't need to
};

// areas have references to hold all the lights and entities in them
typedef struct dmapAreaReference_s {
	struct dmapAreaReference_s *areaNext;				// chain in the area
	struct dmapAreaReference_s *areaPrev;
	struct dmapAreaReference_s *ownerNext;				// chain on either the entityDef or lightDef
	idDmapRenderEntityLocal *		entity;					// only one of entity / light will be non-NULL
	idDmapRenderLightLocal *	light;					// only one of entity / light will be non-NULL
	struct dmapPortalArea_s	*	area;					// so owners can find all the areas they are in
} dmapAreaReference_t;

// deformable meshes precalculate as much as possible from a base frame, then generate
// complete srfTriangles_t from just a new set of vertexes
typedef struct dmapDeformInfo_s {
	int				numSourceVerts;

	// numOutputVerts may be smaller if the input had duplicated or degenerate triangles
	// it will often be larger if the input had mirrored texture seams that needed
	// to be busted for proper tangent spaces
	int				numOutputVerts;

	int				numMirroredVerts;
	int *			mirroredVerts;

	int				numIndexes;
	uint *			indexes;

	uint *			silIndexes;

	int				numDupVerts;
	int *			dupVerts;

	int				numSilEdges;
	silEdge_t *		silEdges;

	dmapDominantTri_t *	dominantTris;
} dmapDeformInfo_t;


// a viewEntity is created whenever a idRenderEntityLocal is considered for inclusion
// in the current view, but it may still turn out to be culled.
// viewEntity are allocated on the frame temporary stack memory
// a viewEntity contains everything that the back end needs out of a idRenderEntityLocal,
// which the front end may be modifying simultaniously if running in SMP mode.
// A single entityDef can generate multiple dmapViewEntity_t in a single frame, as when seen in a mirror
typedef struct dmapViewEntity_s {
	struct dmapViewEntity_s	*next;

	// back end should NOT reference the entityDef, because it can change when running SMP
	idDmapRenderEntityLocal	*entityDef;

	// for scissor clipping, local inside renderView viewport
	// scissorRect.Empty() is true if the dmapViewEntity_t was never actually
	// seen through any portals, but was created for shadow casting.
	// a viewEntity can have a non-empty scissorRect, meaning that an area
	// that it is in is visible, and still not be visible.
	idScreenRect		scissorRect;

	bool				weaponDepthHack;
	float				modelDepthHack;

	float				modelMatrix[16];		// local coords to global coords
	float				modelViewMatrix[16];	// local coords to eye coords
} dmapViewEntity_t;

// viewDefs are allocated on the frame temporary stack memory
typedef struct dmapViewDef_s {
	// specified in the call to DrawScene()
	dmapRenderView_t		renderView;

	float				projectionMatrix[16];
	dmapViewEntity_t		worldSpace;

	idDmapRenderWorldLocal *renderWorld;

	float				floatTime;

	idVec3				initialViewAreaOrigin;
	// Used to find the portalArea that view flooding will take place from.
	// for a normal view, the initialViewOrigin will be renderView.viewOrg,
	// but a mirror may put the projection origin outside
	// of any valid area, or in an unconnected area of the map, so the view
	// area must be based on a point just off the surface of the mirror / subview.
	// It may be possible to get a failed portal pass if the plane of the
	// mirror intersects a portal, and the initialViewAreaOrigin is on
	// a different side than the renderView.viewOrg is.

	bool				isSubview;				// true if this view is not the main view
	bool				isMirror;				// the portal is a mirror, invert the face culling
	bool				isXraySubview;

	bool				isEditor;

	int					numClipPlanes;			// mirrors will often use a single clip plane
	idPlane				clipPlanes[MAX_CLIP_PLANES];		// in world space, the positive side
	// of the plane is the visible side
	idScreenRect		viewport;				// in real pixels and proper Y flip

	idScreenRect		scissor;
	// for scissor clipping, local inside renderView viewport
	// subviews may only be rendering part of the main view
	// these are real physical pixel values, possibly scaled and offset from the
	// renderView x/y/width/height

	struct dmapViewDef_s *	superView;				// never go into an infinite subview loop
	struct drawSurf_s *	subviewSurface;

	// drawSurfs are the visible surfaces of the viewEntities, sorted
	// by the material sort parameter
	drawSurf_t **		drawSurfs;				// we don't use an idList for this, because
	int					numDrawSurfs;			// it is allocated in frame temporary memory
	int					maxDrawSurfs;			// may be resized

	struct dmapViewLight_s	*viewLights;			// chain of all viewLights effecting view
	struct dmapViewEntity_s	*viewEntitys;			// chain of all viewEntities effecting view, including off screen ones casting shadows
	// we use viewEntities as a check to see if a given view consists solely
	// of 2D rendering, which we can optimize in certain ways.  A 2D view will
	// not have any viewEntities

	idPlane				frustum[5];				// positive sides face outward, [4] is the front clip plane
	idFrustum			viewFrustum;

	int					areaNum;				// -1 = not in a valid area

	bool *				connectedAreas;
	// An array in frame temporary memory that lists if an area can be reached without
	// crossing a closed door.  This is used to avoid drawing interactions
	// when the light is behind a closed door.

} dmapViewDef_t;

// viewLights are allocated on the frame temporary stack memory
// a viewLight contains everything that the back end needs out of an idRenderLightLocal,
// which the front end may be modifying simultaniously if running in SMP mode.
// a viewLight may exist even without any surfaces, and may be relevent for fogging,
// but should never exist if its volume does not intersect the view frustum
typedef struct dmapViewLight_s {
	struct dmapViewLight_s *	next;

	// back end should NOT reference the lightDef, because it can change when running SMP
	idDmapRenderLightLocal *	lightDef;

	// for scissor clipping, local inside renderView viewport
	// scissorRect.Empty() is true if the dmapViewEntity_t was never actually
	// seen through any portals
	idScreenRect			scissorRect;

	// if the view isn't inside the light, we can use the non-reversed
	// shadow drawing, avoiding the draws of the front and rear caps
	bool					viewInsideLight;

	// true if globalLightOrigin is inside the view frustum, even if it may
	// be obscured by geometry.  This allows us to skip shadows from non-visible objects
	bool					viewSeesGlobalLightOrigin;

	// if !viewInsideLight, the corresponding bit for each of the shadowFrustum
	// projection planes that the view is on the negative side of will be set,
	// allowing us to skip drawing the projected caps of shadows if we can't see the face
	int						viewSeesShadowPlaneBits;

	idVec3					globalLightOrigin;			// global light origin used by backend
	idPlane					lightProject[4];			// light project used by backend
	idPlane					fogPlane;					// fog plane for backend fog volume rendering
	const srfDmapTriangles_t *	frustumTris;				// light frustum for backend fog volume rendering
	const idMaterial *		lightShader;				// light shader used by backend
	const float	*			shaderRegisters;			// shader registers used by backend
	idImage *				falloffImage;				// falloff image used by backend

	const struct drawSurf_s	*globalShadows;				// shadow everything
	const struct drawSurf_s	*localInteractions;			// don't get local shadows
	const struct drawSurf_s	*localShadows;				// don't shadow local Surfaces
	const struct drawSurf_s	*globalInteractions;		// get shadows from everything
	const struct drawSurf_s	*translucentInteractions;	// get shadows from everything
} dmapViewLight_t;

//=======================================================================

// a request for frame memory will never fail
// (until malloc fails), but it may force the
// allocation of a new memory block that will
// be discontinuous with the existing memory
typedef struct frameMemoryBlock_s {
	struct frameMemoryBlock_s *next;
	int		size;
	int		used;
	int		poop;			// so that base is 16 byte aligned
	byte	base[4];	// dynamically allocated as [size]
} frameMemoryBlock_t;

// all of the information needed by the back end must be
// contained in a frameData_t.  This entire structure is
// duplicated so the front and back end can run in parallel
// on an SMP machine (OBSOLETE: this capability has been removed)
typedef struct {
	// one or more blocks of memory for all frame
	// temporary allocations
	frameMemoryBlock_t	*memory;

	// alloc will point somewhere into the memory chain
	frameMemoryBlock_t	*alloc;

	srfDmapTriangles_t *	firstDeferredFreeTriSurf;
	srfDmapTriangles_t *	lastDeferredFreeTriSurf;

	int					memoryHighwater;	// max used on any frame

	// the currently building command list
	// commands can be inserted at the front if needed, as for required
	// dynamically generated textures
	emptyCommand_t	*cmdHead, *cmdTail;		// may be of other command type based on commandId
} frameData_t;

extern	frameData_t	*dmapFrameData;

//=======================================================================


class idDmapRenderLightLocal : public idRenderLight
{
public:
	idDmapRenderLightLocal();

	virtual void			FreeRenderLight();
	virtual void			UpdateRenderLight(const renderLight_t *re, bool forceUpdate = false);
	virtual void			GetRenderLight(renderLight_t *re);
	virtual void			ForceUpdate();
	virtual int				GetIndex();

	renderLight_t			parms;					// specification

	bool					lightHasMoved;			// the light has changed its position since it was
	// first added, so the prelight model is not valid

	float					modelMatrix[16];		// this is just a rearrangement of parms.axis and parms.origin

	idDmapRenderWorldLocal *	world;
	int						index;					// in world lightdefs

	int						areaNum;				// if not -1, we may be able to cull all the light's
	// interactions if !viewDef->connectedAreas[areaNum]

	int						lastModifiedFrameNum;	// to determine if it is constantly changing,
	// and should go in the dynamic frame memory, or kept
	// in the cached memory
	bool					archived;				// for demo writing


	// derived information
	idPlane					lightProject[4];

	const idMaterial *		lightShader;			// guaranteed to be valid, even if parms.shader isn't
	idImage *				falloffImage;

	idVec3					globalLightOrigin;		// accounting for lightCenter and parallel


	idPlane					frustum[6];				// in global space, positive side facing out, last two are front/back
	idWinding *				frustumWindings[6];		// used for culling
	srfDmapTriangles_t *	frustumTris;			// triangulated frustumWindings[]

	int						numShadowFrustums;		// one for projected lights, usually six for point lights
	shadowFrustum_t			shadowFrustums[6];

	int						viewCount;				// if == dmap_tr.viewCount, the light is on the viewDef->viewLights list
	struct dmapViewLight_s *	viewLight;

	dmapAreaReference_t *		references;				// each area the light is present in will have a lightRef
	idInteraction *			firstInteraction;		// doubly linked list
	idInteraction *			lastInteraction;

	struct dmapDoublePortal_s *	foggedPortals;
};

class idDmapRenderEntityLocal : public idRenderEntity {
public:
	idDmapRenderEntityLocal();

	virtual void			FreeRenderEntity();
	virtual void			UpdateRenderEntity(const renderEntity_t *re, bool forceUpdate = false);
	virtual void			GetRenderEntity(renderEntity_t *re);
	virtual void			ForceUpdate();
	virtual int				GetIndex();

	// overlays are extra polygons that deform with animating models for blood and damage marks
	virtual void			ProjectOverlay(const idPlane localTextureAxis[2], const idMaterial *material);
	virtual void			RemoveDecals();

	dmapRenderEntity_t		parms;

	float					modelMatrix[16];		// this is just a rearrangement of parms.axis and parms.origin

	idDmapRenderWorldLocal *	world;
	int						index;					// in world entityDefs

	int						lastModifiedFrameNum;	// to determine if it is constantly changing,
	// and should go in the dynamic frame memory, or kept
	// in the cached memory
	bool					archived;				// for demo writing

	idDmapRenderModel *		dynamicModel;			// if parms.model->IsDynamicModel(), this is the generated data
	int						dynamicModelFrameCount;	// continuously animating dynamic models will recreate
	// dynamicModel if this doesn't == dmap_tr.viewCount
	idDmapRenderModel *		cachedDynamicModel;

	idBounds				referenceBounds;		// the local bounds used to place entityRefs, either from parms or a model

	// a dmapViewEntity_t is created whenever a idDmapRenderEntityLocal is considered for inclusion
	// in a given view, even if it turns out to not be visible
	int						viewCount;				// if dmap_tr.viewCount == viewCount, viewEntity is valid,
	// but the entity may still be off screen
	struct dmapViewEntity_s *	viewEntity;				// in frame temporary memory

	int						visibleCount;
	// if dmap_tr.viewCount == visibleCount, at least one ambient
	// surface has actually been added by R_AddAmbientDrawsurfs
	// note that an entity could still be in the view frustum and not be visible due
	// to portal passing

	idDmapRenderModelDecal *	decals;					// chain of decals that have been projected on this model
	idDmapRenderModelOverlay *	overlay;				// blood overlays on animated models

	dmapAreaReference_t *	entityRefs;				// chain of all references
	idInteraction *			firstInteraction;		// doubly linked list
	idInteraction *			lastInteraction;

	bool					needsPortalSky;
};

/*
** Most renderer globals are defined here.
** backend functions should never modify any of these fields,
** but may read fields that aren't dynamically modified
** by the frontend.
*/
class idDmapRenderSystemLocal : public idRenderSystem
{
public:
	// external functions
	virtual void			Init();
	virtual void			Shutdown();
	virtual void			ResetGuiModels();
	virtual void			InitOpenGL();
	virtual void			ShutdownOpenGL();
	virtual bool			IsOpenGLRunning() const;
	virtual bool			IsFullScreen() const;
	virtual stereo3DMode_t	GetStereo3DMode() const;
	virtual bool			HasQuadBufferSupport() const;
	virtual bool			IsStereoScopicRenderingSupported() const;
	virtual stereo3DMode_t	GetStereoScopicRenderingMode() const;
	virtual void			EnableStereoScopicRendering(const stereo3DMode_t mode) const;
	virtual int				GetWidth() const;
	virtual int				GetHeight() const;
	virtual float			GetPixelAspect() const;
	virtual float			GetPhysicalScreenWidthInCentimeters() const;
	virtual idRenderWorld* 	AllocRenderWorld();
	virtual void			FreeRenderWorld(idRenderWorld* rw);
	virtual void			BeginLevelLoad();
	virtual void			EndLevelLoad();
	virtual void			LoadLevelImages();
	virtual void			Preload(const idPreloadManifest& manifest, const char* mapName);
	virtual void			BeginAutomaticBackgroundSwaps(autoRenderIconType_t icon = AUTORENDER_DEFAULTICON);
	virtual void			EndAutomaticBackgroundSwaps();
	virtual bool			AreAutomaticBackgroundSwapsRunning(autoRenderIconType_t* usingAlternateIcon = NULL) const;

	virtual idFont* 		RegisterFont(const char* fontName);
	virtual void			ResetFonts();
	virtual void			PrintMemInfo(MemInfo_t* mi);

	virtual void			SetColor(const idVec4& color);
	virtual uint32			GetColor();
	virtual void			SetGLState(const uint64 glState);
	virtual void			DrawFilled(const idVec4& color, float x, float y, float w, float h);
	virtual void			DrawStretchPic(float x, float y, float w, float h, float s1, float t1, float s2, float t2, const idMaterial* material);
	virtual void			DrawStretchPic(const idVec4& topLeft, const idVec4& topRight, const idVec4& bottomRight, const idVec4& bottomLeft, const idMaterial* material);
	virtual void			DrawStretchTri(const idVec2& p1, const idVec2& p2, const idVec2& p3, const idVec2& t1, const idVec2& t2, const idVec2& t3, const idMaterial* material);
	virtual idDrawVert* 	AllocTris(int numVerts, const triIndex_t* indexes, int numIndexes, const idMaterial* material, const stereoDepthType_t stereoType = STEREO_DEPTH_TYPE_NONE);
	virtual void			DrawSmallChar(int x, int y, int ch);
	virtual void			DrawSmallStringExt(int x, int y, const char* string, const idVec4& setColor, bool forceColor);
	virtual void			DrawBigChar(int x, int y, int ch);
	virtual void			DrawBigStringExt(int x, int y, const char* string, const idVec4& setColor, bool forceColor);

	virtual void			WriteDemoPics();
	virtual void			WriteEndFrame();
	virtual void			DrawDemoPics();
	virtual const emptyCommand_t* 	SwapCommandBuffers(uint64* frontEndMicroSec, uint64* backEndMicroSec, uint64* shadowMicroSec, uint64* gpuMicroSec);

	virtual void			SwapCommandBuffers_FinishRendering(uint64* frontEndMicroSec, uint64* backEndMicroSec, uint64* shadowMicroSec, uint64* gpuMicroSec);
	virtual const emptyCommand_t* 	SwapCommandBuffers_FinishCommandBuffers();

	virtual void			RenderCommandBuffers(const emptyCommand_t* commandBuffers);
	virtual void			TakeScreenshot(int width, int height, const char* fileName, int downSample, renderView_t* ref);
	virtual void			TakeScreenshot(int width, int height, idFile* outFile, int blends, renderView_t* ref);

	virtual void			CropRenderSize(int width, int height);
	virtual void			CaptureRenderToImage(const char* imageName, bool clearColorAfterCopy = false);
	virtual void			CaptureRenderToFile(const char* fileName, bool fixAlpha);
	virtual void			UnCrop();
	virtual bool			UploadImage(const char* imageName, const byte* data, int width, int height);



public:
	// internal functions
	idDmapRenderSystemLocal();
	~idDmapRenderSystemLocal();

	void					Clear();
	void					GetCroppedViewport(idScreenRect* viewport);
	void					PerformResolutionScaling(int& newWidth, int& newHeight);
	int						GetFrameCount() const
	{
		return frameCount;
	};

	void OnFrame();

public:
	// renderer globals
	bool					registered;		// cleared at shutdown, set at InitOpenGL

	bool					takingScreenshot;

	int						frameCount;		// incremented every frame
	int						viewCount;		// incremented every view (twice a scene if subviewed)
	// and every R_MarkFragments call

	float					frameShaderTime;	// shader time for all non-world 2D rendering

	idVec4					ambientLightVector;	// used for "ambient bump mapping"

	idList<idDmapRenderWorldLocal*>worlds;

	idDmapRenderWorldLocal* 	primaryWorld;
	dmapRenderView_t			primaryRenderView;
	dmapViewDef_t* 				primaryView;
	// many console commands need to know which world they should operate on

	const idMaterial* 		whiteMaterial;
	const idMaterial* 		charSetMaterial;
	const idMaterial* 		defaultPointLight;
	const idMaterial* 		defaultProjectedLight;
	const idMaterial* 		defaultMaterial;
	idImage* 				testImage;
	idCinematic* 			testVideo;
	int						testVideoStartTime;

	idImage* 				ambientCubeImage;	// hack for testing dependent ambient lighting

	dmapViewDef_t* 				viewDef;

	performanceCounters_t	pc;					// performance counters

	dmapViewEntity_t			identitySpace;		// can use if we don't know viewDef->worldSpace is valid

	idScreenRect			renderCrops[MAX_RENDER_CROPS];
	int						currentRenderCrop;

	// GUI drawing variables for surface creation
	int						guiRecursionLevel;		// to prevent infinite overruns
	uint32					currentColorNativeBytesOrder;
	uint64					currentGLState;
	class idGuiModel* 		guiModel;

	idList<idFont*, TAG_FONT>		fonts;

	unsigned short			gammaTable[256];	// brightness / gamma modify this

	srfDmapTriangles_t* 		unitSquareTriangles;
	srfDmapTriangles_t* 		zeroOneCubeTriangles;
	srfDmapTriangles_t* 		testImageTriangles;

	// these are allocated at buffer swap time, but
	// the back end should only use the ones in the backEnd stucture,
	// which are copied over from the frame that was just swapped.
	drawSurf_t				unitSquareSurface_;
	drawSurf_t				zeroOneCubeSurface_;
	drawSurf_t				testImageSurface_;

	idParallelJobList* 		frontEndJobList;

	unsigned				timerQueryId;		// for GL_TIME_ELAPSED_EXT queries


	// foresthale 2014-03-01: screenshots need to override the results of GetWidth() and GetHeight()
	int						screenshotOverrideWidth;
	int						screenshotOverrideHeight;
};

extern idDmapRenderSystemLocal	dmap_tr;

//
// cvars
//
extern idCVar r_useTurboShadow;				// 1 = use the infinite projection with W technique for dynamic shadows
extern idCVar r_useShadowVertexProgram;		// 1 = do the shadow projection in the vertex program on capable cards
extern idCVar r_useShadowProjectedCull;		// 1 = discard triangles outside light volume before shadowing
extern idCVar r_useCulling;					// 0 = none, 1 = sphere, 2 = sphere + box
extern idCVar r_useEntityCulling;			// 0 = none, 1 = box
extern idCVar r_useLightCulling;			// 0 = none, 1 = box, 2 = exact clip of polyhedron faces
extern idCVar r_useDeferredTangents;	// 1 = don't always calc tangents after deform

extern idCVar		com_purgeAll;

/*
====================================================================

MAIN

====================================================================
*/

void R_InitMaterialsDmap();
bool R_CullLocalBox(const idBounds &bounds, const float modelMatrix[16], int numPlanes, const idPlane *planes);
void myGlMultMatrix(const float *a, const float *b, float *out);

/*
============================================================

RENDERWORLD_DEFS

============================================================
*/

void R_DeriveLightDataDmap(idDmapRenderLightLocal* light);
void R_CheckForEntityDefsUsingModelDmap(idDmapRenderModel* model);
void R_FreeEntityDefDerivedDataDmap(idDmapRenderEntityLocal* def, bool keepDecals, bool keepCachedDynamicModel);

/*
============================================================

TR_STENCILSHADOWS

"facing" should have one more element than tri->numIndexes / 3, which should be set to 1

============================================================
*/

void R_MakeShadowFrustums(idDmapRenderLightLocal *def);

typedef enum {
	SG_DYNAMIC,		// use infinite projections
	SG_STATIC,		// clip to bounds
	SG_OFFLINE		// perform very time consuming optimizations
} shadowGen_t;

srfDmapTriangles_t *R_CreateShadowVolume(const idDmapRenderEntityLocal *ent,
	const srfDmapTriangles_t *tri, const idDmapRenderLightLocal *light,
	shadowGen_t optimize, srfCullInfo_t &cullInfo);

/*
============================================================

TR_TURBOSHADOW

Fast, non-clipped overshoot shadow volumes

"facing" should have one more element than tri->numIndexes / 3, which should be set to 1
calling this function may modify "facing" based on culling

============================================================
*/

srfDmapTriangles_t *R_CreateVertexProgramTurboShadowVolume(const idDmapRenderEntityLocal *ent,
	const srfDmapTriangles_t *tri, const idDmapRenderLightLocal *light,
	srfCullInfo_t &cullInfo);

srfDmapTriangles_t *R_CreateTurboShadowVolume(const idDmapRenderEntityLocal *ent,
	const srfDmapTriangles_t *tri, const idDmapRenderLightLocal *light,
	srfCullInfo_t &cullInfo);

/*
============================================================

util/shadowopt3

dmap time optimization of shadow volumes, called from R_CreateShadowVolume

============================================================
*/


typedef struct {
	idVec3	*verts;			// includes both front and back projections, caller should free
	int		numVerts;
	uint	*indexes;	// caller should free

	// indexes must be sorted frontCap, rearCap, silPlanes so the caps can be removed
	// when the viewer is in a position that they don't need to see them
	int		numFrontCapIndexes;
	int		numRearCapIndexes;
	int		numSilPlaneIndexes;
	int		totalIndexes;
} optimizedShadow_t;

optimizedShadow_t SuperOptimizeOccluders(idVec4 *verts, uint *indexes, int numIndexes,
	idPlane projectionPlane, idVec3 projectionOrigin);

void CleanupOptimizedShadowTris(srfDmapTriangles_t *tri);

/*
============================================================

TR_FRONTEND_ADDMODELS

============================================================
*/

idDmapRenderModel* R_EntityDefDynamicModelDmap(idDmapRenderEntityLocal* def);
void R_ClearEntityDefDynamicModelDmap(idDmapRenderEntityLocal* def);

/*
=============================================================

TR_FRONTEND_GUISURF

=============================================================
*/

void R_SurfaceToTextureAxisDmap(const srfDmapTriangles_t* tri, idVec3& origin, idVec3 axis[3]);

/*
============================================================

TR_TRISURF

============================================================
*/

#define USE_TRI_DATA_ALLOCATOR

void				R_InitTriSurfDataDmap(void);
void				R_ShutdownTriSurfData(void); 
srfDmapTriangles_t* R_AllocStaticTriSurfDmap();
void				R_AllocStaticTriSurfPlanesDmap(srfDmapTriangles_t *tri, int numIndexes);
void				R_AllocStaticTriSurfVertsDmap(srfDmapTriangles_t* tri, int numVerts);
void				R_AllocStaticTriSurfIndexesDmap(srfDmapTriangles_t* tri, int numIndexes);
void				R_AllocStaticTriSurfSilEdgesDmap(srfDmapTriangles_t* tri, int numSilEdges);
void				R_AllocStaticTriSurfMirroredVertsDmap(srfDmapTriangles_t* tri, int numMirroredVerts);
void				R_AllocStaticTriSurfDupVertsDmap(srfDmapTriangles_t* tri, int numDupVerts);
void				R_AllocStaticTriSurfDominantTrisDmap(srfDmapTriangles_t* tri, int numVerts);
void				R_AllocStaticTriSurfShadowVertsDmap(srfDmapTriangles_t *tri, int numVerts);
void				R_ResizeStaticTriSurfIndexesDmap(srfDmapTriangles_t *tri, int numIndexes);
void				R_ResizeStaticTriSurfShadowVertsDmap(srfDmapTriangles_t *tri, int numVerts);
void				R_RangeCheckIndexesDmap(const srfDmapTriangles_t* tri);
void				R_CreateSilIndexesDmap(srfDmapTriangles_t* tri);
void				R_RemoveDegenerateTrianglesDmap(srfDmapTriangles_t* tri);
void				R_AllocStaticTriSurfSilIndexesDmap(srfDmapTriangles_t* tri, int numIndexes);
void				R_RemoveDegenerateTrianglesDmap(srfDmapTriangles_t* tri);
void				R_FreeStaticTriSurfSilIndexesDmap(srfDmapTriangles_t* tri);
void				R_FreeStaticTriSurfDmap(srfDmapTriangles_t* tri);
void				R_FreeStaticTriSurfVertexCachesDmap(srfDmapTriangles_t* tri);
void				R_CleanupTrianglesDmap(srfDmapTriangles_t* tri, bool createNormals, bool identifySilEdges, bool useUnsmoothedTangents);
void				R_BoundTriSurfDmap(srfDmapTriangles_t* tri);
void				R_ResizeStaticTriSurfVertsDmap(srfDmapTriangles_t* tri, int numVerts);
void				R_DeriveTangentsDmap(srfDmapTriangles_t *tri, bool allocFacePlanes = true);
void				R_DeriveFacePlanesDmap(srfDmapTriangles_t *tri);		// also called by renderbump
void				R_PurgeTriSurfData(frameData_t *frame);
int					R_TriSurfMemoryDmap(const srfDmapTriangles_t *tri);
srfDmapTriangles_t *R_CopyStaticTriSurfDmap(const srfDmapTriangles_t *tri);
void				R_ReverseTrianglesDmap(srfDmapTriangles_t *tri);
srfDmapTriangles_t	*R_MergeTrianglesDmap(const srfDmapTriangles_t *tri1, const srfDmapTriangles_t *tri2);
dmapDeformInfo_t	*R_BuildDeformInfoDmap(int numVerts, const idDmapDrawVert *verts, int numIndexes, const int *indexes, bool useUnsmoothedTangents);
void				R_FreeDeformInfoDmap(dmapDeformInfo_t *deformInfo);

/*
============================================================

LIGHT

============================================================
*/

dmapViewEntity_t *R_SetEntityDefViewEntityDmap(idDmapRenderEntityLocal *def);
bool R_IssueEntityDefCallbackDmap(idDmapRenderEntityLocal *def);
dmapViewLight_t *R_SetLightDefViewLightDmap(idDmapRenderLightLocal *def);
void R_AddDrawSurfDmap(const srfDmapTriangles_t *tri, const dmapViewEntity_t *space, const dmapRenderEntity_t *renderEntity,
	const idMaterial *shader, const idScreenRect &scissor);

/*
============================================================

LIGHTRUN

============================================================
*/

void R_RegenerateWorld_f(const idCmdArgs &args);

void R_CreateEntityRefsDmap(idDmapRenderEntityLocal *def);
void R_CreateLightRefsDmap(idDmapRenderLightLocal *light);
void R_CreateLightDefFogPortalsDmap(idDmapRenderLightLocal *ldef);

void R_FreeEntityDefDecalsDmap(idDmapRenderEntityLocal *def);
void R_FreeEntityDefOverlayDmap(idDmapRenderEntityLocal *def);
void R_FreeLightDefDerivedDataDmap(idDmapRenderLightLocal *light);
void R_FreeEntityDefFadedDecalsDmap(idDmapRenderEntityLocal *def, int time);

/*
=============================================================

Draw Quads ######################################################## SR

=============================================================
*/

void RB_AddQuad(const idVec4 &color, const idWinding &winding, float lifeTime, float fadeTime);
void RB_ClearQuads(int time);

/*
=============================================================

TR_TRACE

=============================================================
*/

localTrace_t R_LocalTraceDmap(const idVec3& start, const idVec3& end, const float radius, const srfDmapTriangles_t* tri);

/*
=============================================================

TR_BACKEND_RENDERTOOLS

=============================================================
*/
void RB_SetGL2D(void);

/*
============================================================

POLYTOPE

============================================================
*/

srfDmapTriangles_t *R_PolytopeSurfaceDmap(int numPlanes, const idPlane *planes, idWinding **windings);

// returns the frustum planes in world space
void R_RenderLightFrustum(const struct renderLight_s &renderLight, idPlane lightFrustum[6]);

#include "DmapRenderWorld_local.h"

#endif /* !__DMAP_TR_LOCAL_H__ */

