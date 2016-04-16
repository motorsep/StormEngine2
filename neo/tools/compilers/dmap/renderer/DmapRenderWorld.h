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
#ifndef __DMAPRENDERWORLD_H__
#define __DMAPRENDERWORLD_H__

const int SHADERPARM_MD5_SKINSCALE = 8;	// for scaling vertex offsets on md5 models (jack skellington effect)

/*
===============================================================================

Render World

===============================================================================
*/

struct dmapRenderEntity_s;
typedef bool(*dmapDeferredEntityCallback_t)(dmapRenderEntity_s*, const struct dmapRenderView_s*);

typedef struct dmapRenderEntity_s {
	idDmapRenderModel *		hModel;				// this can only be null if callback is set

	int						entityNum;
	int						bodyId;

	// Entities that are expensive to generate, like skeletal models, can be
	// deferred until their bounds are found to be in view, in the frustum
	// of a shadowing light that is in view, or contacted by a trace / overlay test.
	// This is also used to do visual cueing on items in the view
	// The renderView may be NULL if the callback is being issued for a non-view related
	// source.
	// The callback function should clear renderEntity->callback if it doesn't
	// want to be called again next time the entity is referenced (ie, if the
	// callback has now made the entity valid until the next updateEntity)
	idBounds				bounds;					// only needs to be set for deferred models and md5s
	dmapDeferredEntityCallback_t	callback;

	void *					callbackData;			// used for whatever the callback wants

	// player bodies and possibly player shadows should be suppressed in views from
	// that player's eyes, but will show up in mirrors and other subviews
	// security cameras could suppress their model in their subviews if we add a way
	// of specifying a view number for a remoteRenderMap view
	int						suppressSurfaceInViewID;
	int						suppressShadowInViewID;

	// world models for the player and weapons will not cast shadows from view weapon
	// muzzle flashes
	int						suppressShadowInLightID;

	// if non-zero, the surface and shadow (if it casts one)
	// will only show up in the specific view, ie: player weapons
	int						allowSurfaceInViewID;

	// positioning
	// axis rotation vectors must be unit length for many
	// R_LocalToGlobal functions to work, so don't scale models!
	// axis vectors are [0] = forward, [1] = left, [2] = up
	idVec3					origin;
	idMat3					axis;

	// texturing
	const idMaterial *		customShader;			// if non-0, all surfaces will use this
	const idMaterial *		referenceShader;		// used so flares can reference the proper light shader
	const idDeclSkin *		customSkin;				// 0 for no remappings
	class idSoundEmitter *	referenceSound;			// for shader sound tables, allowing effects to vary with sounds
	float					shaderParms[MAX_ENTITY_SHADER_PARMS];	// can be used in any way by shader or model generation

	// networking: see WriteGUIToSnapshot / ReadGUIFromSnapshot
	class idUserInterface * gui[MAX_RENDERENTITY_GUI];

	struct renderView_s	*	remoteRenderView;		// any remote camera surfaces will use this

	int						numJoints;
	idJointMat *			joints;					// array of joints that will modify vertices.
	// NULL if non-deformable model.  NOT freed by renderer

	float					modelDepthHack;			// squash depth range so particle effects don't clip into walls

	// options to override surface shader flags (replace with material parameters?)
	bool					noSelfShadow;			// cast shadows onto other objects,but not self
	bool					noShadow;				// no shadow at all

	bool					noDynamicInteractions;	// don't create any light / shadow interactions after
	// the level load is completed.  This is a performance hack
	// for the gigantic outdoor meshes in the monorail map, so
	// all the lights in the moving monorail don't touch the meshes

	bool					weaponDepthHack;		// squash depth range so view weapons don't poke into walls
	// this automatically implies noShadow
	int						forceUpdate;			// force an update (NOTE: not a bool to keep this struct a multiple of 4 bytes)
	int						timeGroup;
	int						xrayIndex;
} dmapRenderEntity_t;

// dmapModelTrace_t is for tracing vs. visual geometry
typedef struct dmapModelTrace_s
{
	float					fraction;			// fraction of trace completed
	idVec3					point;				// end point of trace in global space
	idVec3					normal;				// hit triangle normal vector in global space
	const idMaterial* 		material;			// material of hit surface
	const dmapRenderEntity_t* 	entity;				// render entity that was hit
	int						jointNumber;		// md5 joint nearest to the hit triangle
} dmapModelTrace_t;

typedef struct dmapRenderView_s {
	// player views will set this to a non-zero integer for model suppress / allow
	// subviews (mirrors, cameras, etc) will always clear it to zero
	int						viewID;

	// sized from 0 to SCREEN_WIDTH / SCREEN_HEIGHT (640/480), not actual resolution
	int						x, y, width, height;

	float					fov_x, fov_y;
	idVec3					vieworg;
	idMat3					viewaxis;			// transformation matrix, view looks down the positive X axis

	bool					cramZNear;			// for cinematics, we want to set ZNear much lower
	bool					forceUpdate;		// for an update

	// time in milliseconds for shader effects and other time dependent rendering issues
	int						time;
	float					shaderParms[MAX_GLOBAL_SHADER_PARMS];		// can be used in any way by shader
	const idMaterial		*globalMaterial;							// used to override everything draw
} dmapRenderView_t;


class idDmapRenderWorld {
public:
	virtual					~idDmapRenderWorld() {};

	// The same render world can be reinitialized as often as desired
	// a NULL or empty mapName will create an empty, single area world
	virtual bool			InitFromMap(const char *mapName) = 0;

	//-------------- Entity and Light Defs -----------------

	// entityDefs and lightDefs are added to a given world to determine
	// what will be drawn for a rendered scene.  Most update work is defered
	// until it is determined that it is actually needed for a given view.
	virtual	qhandle_t		AddEntityDef(const dmapRenderEntity_t *re) = 0;
	virtual	void			UpdateEntityDef(qhandle_t entityHandle, const dmapRenderEntity_t *re) = 0;
	virtual	void			FreeEntityDef(qhandle_t entityHandle) = 0;
	virtual const dmapRenderEntity_t *GetRenderEntity(qhandle_t entityHandle) const = 0;

	virtual	qhandle_t		AddLightDef(const renderLight_t *rlight) = 0;
	virtual	void			UpdateLightDef(qhandle_t lightHandle, const renderLight_t *rlight) = 0;
	virtual	void			FreeLightDef(qhandle_t lightHandle) = 0;
	virtual const renderLight_t *GetRenderLight(qhandle_t lightHandle) const = 0;

	// Force the generation of all light / surface interactions at the start of a level
	// If this isn't called, they will all be dynamically generated
	virtual	void			GenerateAllInteractions() = 0;

	// returns true if this area model needs portal sky to draw
	virtual bool			CheckAreaForPortalSky(int areaNum) = 0;

	//-------------- Decals and Overlays  -----------------

	// Creates decals on all world surfaces that the winding projects onto.
	// The projection origin should be infront of the winding plane.
	// The decals are projected onto world geometry between the winding plane and the projection origin.
	// The decals are depth faded from the winding plane to a certain distance infront of the
	// winding plane and the same distance from the projection origin towards the winding.
	virtual void			ProjectDecalOntoWorld(const idFixedWinding &winding, const idVec3 &projectionOrigin, const bool parallel, const float fadeDepth, const idMaterial *material, const int startTime) = 0;

	// Creates decals on static models.
	virtual void			ProjectDecal(qhandle_t entityHandle, const idFixedWinding &winding, const idVec3 &projectionOrigin, const bool parallel, const float fadeDepth, const idMaterial *material, const int startTime) = 0;

	// Creates overlays on dynamic models.
	virtual void			ProjectOverlay(qhandle_t entityHandle, const idPlane localTextureAxis[2], const idMaterial *material) = 0;

	// Removes all decals and overlays from the given entity def.
	virtual void			RemoveDecals(qhandle_t entityHandle) = 0;

	//-------------- Scene Rendering -----------------

	// some calls to material functions use the current renderview time when servicing cinematics.  this function
	// ensures that any parms accessed (such as time) are properly set.
	virtual void			SetRenderView(const dmapRenderView_t *renderView) = 0;

	// rendering a scene may actually render multiple subviews for mirrors and portals, and
	// may render composite textures for gui console screens and light projections
	// It would also be acceptable to render a scene multiple times, for "rear view mirrors", etc
	virtual void			RenderScene(const dmapRenderView_t *renderView) = 0;

	//-------------- Portal Area Information -----------------

	// returns the number of portals
	virtual int				NumPortals(void) const = 0;

	// returns 0 if no portal contacts the bounds
	// This is used by the game to identify portals that are contained
	// inside doors, so the connection between areas can be topologically
	// terminated when the door shuts.
	virtual	qhandle_t		FindPortal(const idBounds &b) const = 0;

	// doors explicitly close off portals when shut
	// multiple bits can be set to block multiple things, ie: ( PS_VIEW | PS_LOCATION | PS_AIR )
	virtual	void			SetPortalState(qhandle_t portal, int blockingBits) = 0;
	virtual int				GetPortalState(qhandle_t portal) = 0;

	// returns true only if a chain of portals without the given connection bits set
	// exists between the two areas (a door doesn't separate them, etc)
	virtual	bool			AreasAreConnected(int areaNum1, int areaNum2, portalConnection_t connection) = 0;

	// returns the number of portal areas in a map, so game code can build information
	// tables for the different areas
	virtual	int				NumAreas(void) const = 0;

	// Will return -1 if the point is not in an area, otherwise
	// it will return 0 <= value < NumAreas()
	virtual int				PointInArea(const idVec3 &point) const = 0;

	// fills the *areas array with the numbers of the areas the bounds cover
	// returns the total number of areas the bounds cover
	virtual int				BoundsInAreas(const idBounds &bounds, int *areas, int maxAreas) const = 0;

	// Used by the sound system to do area flowing
	virtual	int				NumPortalsInArea(int areaNum) = 0;

	// returns one portal from an area
	virtual exitPortal_t	GetPortal(int areaNum, int portalNum) = 0;

	//-------------- Tracing  -----------------

	// Checks a ray trace against any gui surfaces in an entity, returning the
	// fraction location of the trace on the gui surface, or -1,-1 if no hit.
	// This doesn't do any occlusion testing, simply ignoring non-gui surfaces.
	// start / end are in global world coordinates.
	virtual guiPoint_t		GuiTrace(qhandle_t entityHandle, const idVec3 start, const idVec3 end) const = 0;

	// Traces vs the render model, possibly instantiating a dynamic version, and returns true if something was hit
	virtual bool			ModelTrace(dmapModelTrace_t &trace, qhandle_t entityHandle, const idVec3 &start, const idVec3 &end, const float radius) const = 0;

	// Traces vs the whole rendered world. FIXME: we need some kind of material flags.
	virtual bool			Trace(dmapModelTrace_t &trace, const idVec3 &start, const idVec3 &end, const float radius, bool skipDynamic = true, bool skipPlayer = false) const = 0;

	// Traces vs the world model bsp tree.
	virtual bool			FastWorldTrace(dmapModelTrace_t &trace, const idVec3 &start, const idVec3 &end) const = 0;

	//-------------- Demo Control  -----------------

	// Writes a loadmap command to the demo, and clears archive counters.
	virtual void			StartWritingDemo(idDemoFile *demo) = 0;
	virtual void			StopWritingDemo() = 0;

	// Returns true when demoRenderView has been filled in.
	// adds/updates/frees entityDefs and lightDefs based on the current demo file
	// and returns the renderView to be used to render this frame.
	// a demo file may need to be advanced multiple times if the framerate
	// is less than 30hz
	// demoTimeOffset will be set if a new map load command was processed before
	// the next renderScene
	virtual bool			ProcessDemoCommand(idDemoFile *readDemo, dmapRenderView_t *demoRenderView, int *demoTimeOffset) = 0;

	// this is used to regenerate all interactions ( which is currently only done during influences ), there may be a less
	// expensive way to do it
	virtual void			RegenerateWorld() = 0;

	//-------------- Debug Visualization  -----------------

	// Line drawing for debug visualization
	virtual void			DebugClearLines(int time) = 0;		// a time of 0 will clear all lines and text
	virtual void			DebugLine(const idVec4 &color, const idVec3 &start, const idVec3 &end, const int lifetime = 0, const bool depthTest = false) = 0;
	virtual void			DebugArrow(const idVec4 &color, const idVec3 &start, const idVec3 &end, int size, const int lifetime = 0) = 0;
	virtual void			DebugWinding(const idVec4 &color, const idWinding &w, const idVec3 &origin, const idMat3 &axis, const int lifetime = 0, const bool depthTest = false) = 0;
	virtual void			DebugCircle(const idVec4 &color, const idVec3 &origin, const idVec3 &dir, const float radius, const int numSteps, const int lifetime = 0, const bool depthTest = false) = 0;
	virtual void			DebugSphere(const idVec4 &color, const idSphere &sphere, const int lifetime = 0, bool depthTest = false) = 0;
	virtual void			DebugBounds(const idVec4 &color, const idBounds &bounds, const idVec3 &org = vec3_origin, const int lifetime = 0) = 0;
	virtual void			DebugBox(const idVec4 &color, const idBox &box, const int lifetime = 0) = 0;
	virtual void			DebugFrustum(const idVec4 &color, const idFrustum &frustum, const bool showFromOrigin = false, const int lifetime = 0) = 0;
	virtual void			DebugCone(const idVec4 &color, const idVec3 &apex, const idVec3 &dir, float radius1, float radius2, const int lifetime = 0) = 0;
	virtual void			DebugAxis(const idVec3 &origin, const idMat3 &axis) = 0;

	// Polygon drawing for debug visualization.
	virtual void			DebugClearPolygons(int time) = 0;		// a time of 0 will clear all polygons
	virtual void			DebugPolygon(const idVec4 &color, const idWinding &winding, const int lifeTime = 0, const bool depthTest = false) = 0;

	// Text drawing for debug visualization.
	virtual void			DrawText(const char *text, const idVec3 &origin, float scale, const idVec4 &color, const idMat3 &viewAxis, const int align = 1, const int lifetime = 0, bool depthTest = false) = 0;


	virtual void 			ClearQuads(int time) = 0;															// ########################### SR
	virtual void 			DrawQuad(const idVec4 &color, const idWinding &winding, float lifeTime, float fadeTime) = 0;	// ########################### SR 

};

#endif /* !__DMAPRENDERWORLD_H__ */
