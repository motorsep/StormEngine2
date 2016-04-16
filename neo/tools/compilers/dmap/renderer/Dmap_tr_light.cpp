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

static const float CHECK_BOUNDS_EPSILON = 1.0f;

/*
==================
R_IssueEntityDefCallbackDmap
==================
*/
bool R_IssueEntityDefCallbackDmap(idDmapRenderEntityLocal *def) {
	bool update;
	idBounds	oldBounds;
	const bool checkBounds = r_checkBounds.GetBool();

	if (checkBounds) {
		oldBounds = def->referenceBounds;
	}

	def->archived = false;		// will need to be written to the demo file
	dmap_tr.pc.c_entityDefCallbacks++;
	if (dmap_tr.viewDef) {
		update = def->parms.callback(&def->parms, &dmap_tr.viewDef->renderView);
	}
	else {
		update = def->parms.callback(&def->parms, NULL);
	}

	if (!def->parms.hModel) {
		common->Error("R_IssueEntityDefCallbackDmap: dynamic entity callback didn't set model");
		return false;
	}

	if (checkBounds) {
		if (oldBounds[0][0] > def->referenceBounds[0][0] + CHECK_BOUNDS_EPSILON ||
			oldBounds[0][1] > def->referenceBounds[0][1] + CHECK_BOUNDS_EPSILON ||
			oldBounds[0][2] > def->referenceBounds[0][2] + CHECK_BOUNDS_EPSILON ||
			oldBounds[1][0] < def->referenceBounds[1][0] - CHECK_BOUNDS_EPSILON ||
			oldBounds[1][1] < def->referenceBounds[1][1] - CHECK_BOUNDS_EPSILON ||
			oldBounds[1][2] < def->referenceBounds[1][2] - CHECK_BOUNDS_EPSILON) {
			common->Printf("entity %i callback extended reference bounds\n", def->index);
		}
	}

	return update;
}

/*
=============
R_SetEntityDefViewEntityDmap

If the entityDef isn't already on the viewEntity list, create
a viewEntity and add it to the list with an empty scissor rect.

This does not instantiate dynamic models for the entity yet.
=============
*/
dmapViewEntity_t *R_SetEntityDefViewEntityDmap(idDmapRenderEntityLocal *def) {
	dmapViewEntity_t		*vModel;

	if (def->viewCount ==dmap_tr.viewCount) {
		return def->viewEntity;
	}
	def->viewCount =dmap_tr.viewCount;

	// set the model and modelview matricies
	vModel = (dmapViewEntity_t *)R_ClearedFrameAlloc(sizeof(*vModel));
	vModel->entityDef = def;

	// the scissorRect will be expanded as the model bounds is accepted into visible portal chains
	vModel->scissorRect.Clear();

	// copy the model and weapon depth hack for back-end use
	vModel->modelDepthHack = def->parms.modelDepthHack;
	vModel->weaponDepthHack = def->parms.weaponDepthHack;

	R_AxisToModelMatrix(def->parms.axis, def->parms.origin, vModel->modelMatrix);

	// we may not have a viewDef if we are just creating shadows at entity creation time
	if (dmap_tr.viewDef) {
		myGlMultMatrix(vModel->modelMatrix,dmap_tr.viewDef->worldSpace.modelViewMatrix, vModel->modelViewMatrix);

		vModel->next =dmap_tr.viewDef->viewEntitys;
		dmap_tr.viewDef->viewEntitys = vModel;
	}

	def->viewEntity = vModel;

	return vModel;
}

/*
====================
R_TestPointInViewLight
====================
*/
static const float INSIDE_LIGHT_FRUSTUM_SLOP = 32;
// this needs to be greater than the dist from origin to corner of near clip plane
static bool R_TestPointInViewLight(const idVec3 &org, const idDmapRenderLightLocal *light) {
	int		i;
	idVec3	local;

	for (i = 0; i < 6; i++) {
		float d = light->frustum[i].Distance(org);
		if (d > INSIDE_LIGHT_FRUSTUM_SLOP) {
			return false;
		}
	}

	return true;
}

/*
===================
R_PointInFrustum

Assumes positive sides face outward
===================
*/
static bool R_PointInFrustum(idVec3 &p, idPlane *planes, int numPlanes) {
	for (int i = 0; i < numPlanes; i++) {
		float d = planes[i].Distance(p);
		if (d > 0) {
			return false;
		}
	}
	return true;
}

/*
=============
R_SetLightDefViewLightDmap

If the lightDef isn't already on the viewLight list, create
a viewLight and add it to the list with an empty scissor rect.
=============
*/
dmapViewLight_t *R_SetLightDefViewLightDmap(idDmapRenderLightLocal *light) {
	dmapViewLight_t *vLight;

	if (light->viewCount == dmap_tr.viewCount) {
		return light->viewLight;
	}
	light->viewCount = dmap_tr.viewCount;

	// add to the view light chain
	vLight = (dmapViewLight_t *)R_ClearedFrameAlloc(sizeof(*vLight));
	vLight->lightDef = light;

	// the scissorRect will be expanded as the light bounds is accepted into visible portal chains
	vLight->scissorRect.Clear();

	// calculate the shadow cap optimization states
	vLight->viewInsideLight = R_TestPointInViewLight(dmap_tr.viewDef->renderView.vieworg, light);
	if (!vLight->viewInsideLight) {
		vLight->viewSeesShadowPlaneBits = 0;
		for (int i = 0; i < light->numShadowFrustums; i++) {
			float d = light->shadowFrustums[i].planes[5].Distance(dmap_tr.viewDef->renderView.vieworg);
			if (d < INSIDE_LIGHT_FRUSTUM_SLOP) {
				vLight->viewSeesShadowPlaneBits |= 1 << i;
			}
		}
	}
	else {
		// this should not be referenced in this case
		vLight->viewSeesShadowPlaneBits = 63;
	}

	// see if the light center is in view, which will allow us to cull invisible shadows
	vLight->viewSeesGlobalLightOrigin = R_PointInFrustum(light->globalLightOrigin, dmap_tr.viewDef->frustum, 4);

	// copy data used by backend
	vLight->globalLightOrigin = light->globalLightOrigin;
	vLight->lightProject[0] = light->lightProject[0];
	vLight->lightProject[1] = light->lightProject[1];
	vLight->lightProject[2] = light->lightProject[2];
	vLight->lightProject[3] = light->lightProject[3];
	vLight->fogPlane = light->frustum[5];
	vLight->frustumTris = light->frustumTris;
	vLight->falloffImage = light->falloffImage;
	vLight->lightShader = light->lightShader;
	vLight->shaderRegisters = NULL;		// allocated and evaluated in R_AddLightSurfaces

	// link the view light
	vLight->next = dmap_tr.viewDef->viewLights;
	dmap_tr.viewDef->viewLights = vLight;

	light->viewLight = vLight;

	return vLight;
}



/*
===================
R_EntityDefDynamicModelDmap

Issues a deferred entity callback if necessary.
If the model isn't dynamic, it returns the original.
Returns the cached dynamic model if present, otherwise creates
it and any necessary overlays
===================
*/
idDmapRenderModel *R_EntityDefDynamicModelDmap(idDmapRenderEntityLocal *def) {
	bool callbackUpdate;

	// allow deferred entities to construct themselves
	if (def->parms.callback) {
		callbackUpdate = R_IssueEntityDefCallbackDmap(def);
	}
	else {
		callbackUpdate = false;
	}

	idDmapRenderModel *model = def->parms.hModel;

	if (!model) {
		common->Error("R_EntityDefDynamicModelDmap: NULL model");
	}

	if (model->IsDynamicModel() == DM_STATIC) {
		def->dynamicModel = NULL;
		def->dynamicModelFrameCount = 0;
		return model;
	}

	// continously animating models (particle systems, etc) will have their snapshot updated every single view
	if (callbackUpdate || (model->IsDynamicModel() == DM_CONTINUOUS && def->dynamicModelFrameCount != dmap_tr.frameCount)) {
		R_ClearEntityDefDynamicModelDmap(def);
	}

	// if we don't have a snapshot of the dynamic model, generate it now
	if (!def->dynamicModel) {

		// instantiate the snapshot of the dynamic model, possibly reusing memory from the cached snapshot
		def->cachedDynamicModel = model->InstantiateDynamicModel(&def->parms, (struct dmapViewDef_s*)dmap_tr.viewDef, def->cachedDynamicModel);

		if (def->cachedDynamicModel) {

			// add any overlays to the snapshot of the dynamic model
			if (def->overlay && !r_skipOverlays.GetBool()) {
				def->overlay->AddOverlaySurfacesToModel(def->cachedDynamicModel);
			}
			else {
				idDmapRenderModelOverlay::RemoveOverlaySurfacesFromModel(def->cachedDynamicModel);
			}

			if (r_checkBounds.GetBool()) {
				idBounds b = def->cachedDynamicModel->Bounds();
				if (b[0][0] < def->referenceBounds[0][0] - CHECK_BOUNDS_EPSILON ||
					b[0][1] < def->referenceBounds[0][1] - CHECK_BOUNDS_EPSILON ||
					b[0][2] < def->referenceBounds[0][2] - CHECK_BOUNDS_EPSILON ||
					b[1][0] > def->referenceBounds[1][0] + CHECK_BOUNDS_EPSILON ||
					b[1][1] > def->referenceBounds[1][1] + CHECK_BOUNDS_EPSILON ||
					b[1][2] > def->referenceBounds[1][2] + CHECK_BOUNDS_EPSILON) {
					common->Printf("entity %i dynamic model exceeded reference bounds\n", def->index);
				}
			}
		}

		def->dynamicModel = def->cachedDynamicModel;
		def->dynamicModelFrameCount = dmap_tr.frameCount;
	}

	// set model depth hack value
	if (def->dynamicModel && model->DepthHack() != 0.0f && dmap_tr.viewDef) {
		idPlane eye, clip;
		idVec3 ndc;
		R_TransformModelToClip(def->parms.origin, dmap_tr.viewDef->worldSpace.modelViewMatrix, dmap_tr.viewDef->projectionMatrix, eye, clip);
		R_TransformClipToDevice(clip, ndc);
		def->parms.modelDepthHack = model->DepthHack() * (1.0f - ndc.z);
	}

	// FIXME: if any of the surfaces have deforms, create a frame-temporary model with references to the
	// undeformed surfaces.  This would allow deforms to be light interacting.

	return def->dynamicModel;
}

/*
=================
R_AddDrawSurfDmap
=================
*/
void R_AddDrawSurfDmap(const srfDmapTriangles_t *tri, const dmapViewEntity_t *space, const dmapRenderEntity_t *renderEntity,
	const idMaterial *shader, const idScreenRect &scissor) {
	assert(0);
	/*drawSurf_t		*drawSurf;
	const float		*shaderParms;
	static float	refRegs[MAX_EXPRESSION_REGISTERS];	// don't put on stack, or VC++ will do a page touch
	float			generatedShaderParms[MAX_ENTITY_SHADER_PARMS];

	drawSurf = (drawSurf_t *)R_FrameAlloc(sizeof(*drawSurf));
	drawSurf->geo = tri;
	drawSurf->space = space;
	drawSurf->material = shader;
	drawSurf->scissorRect = scissor;
	drawSurf->sort = shader->GetSort() + dmap_tr.sortOffset;
	drawSurf->dsFlags = 0;

	// bumping this offset each time causes surfaces with equal sort orders to still
	// deterministically draw in the order they are added
	dmap_tr.sortOffset += 0.000001f;

	// if it doesn't fit, resize the list
	if (dmap_tr.viewDef->numDrawSurfs == dmap_tr.viewDef->maxDrawSurfs) {
		drawSurf_t	**old = dmap_tr.viewDef->drawSurfs;
		int			count;

		if (dmap_tr.viewDef->maxDrawSurfs == 0) {
			dmap_tr.viewDef->maxDrawSurfs = INITIAL_DRAWSURFS;
			count = 0;
		}
		else {
			count = dmap_tr.viewDef->maxDrawSurfs * sizeof(dmap_tr.viewDef->drawSurfs[0]);
			dmap_tr.viewDef->maxDrawSurfs *= 2;
		}
		dmap_tr.viewDef->drawSurfs = (drawSurf_t **)R_FrameAlloc(dmap_tr.viewDef->maxDrawSurfs * sizeof(dmap_tr.viewDef->drawSurfs[0]));
		memcpy(dmap_tr.viewDef->drawSurfs, old, count);
	}
	dmap_tr.viewDef->drawSurfs[dmap_tr.viewDef->numDrawSurfs] = drawSurf;
	dmap_tr.viewDef->numDrawSurfs++;

	// process the shader expressions for conditionals / color / texcoords
	const float	*constRegs = shader->ConstantRegisters();
	if (constRegs) {
		// shader only uses constant values
		drawSurf->shaderRegisters = constRegs;
	}
	else {
		float *regs = (float *)R_FrameAlloc(shader->GetNumRegisters() * sizeof(float));
		drawSurf->shaderRegisters = regs;

		// a reference shader will take the calculated stage color value from another shader
		// and use that for the parm0-parm3 of the current shader, which allows a stage of
		// a light model and light flares to pick up different flashing tables from
		// different light shaders
		if (renderEntity->referenceShader) {
			// evaluate the reference shader to find our shader parms
			const shaderStage_t *pStage;

			renderEntity->referenceShader->EvaluateRegisters(refRegs, renderEntity->shaderParms, dmap_tr.viewDef, renderEntity->referenceSound);
			pStage = renderEntity->referenceShader->GetStage(0);

			memcpy(generatedShaderParms, renderEntity->shaderParms, sizeof(generatedShaderParms));
			generatedShaderParms[0] = refRegs[pStage->color.registers[0]];
			generatedShaderParms[1] = refRegs[pStage->color.registers[1]];
			generatedShaderParms[2] = refRegs[pStage->color.registers[2]];

			shaderParms = generatedShaderParms;
		}
		else {
			// evaluate with the entityDef's shader parms
			shaderParms = renderEntity->shaderParms;
		}

		float oldFloatTime = 0.0f;
		int oldTime = 0;

		if (space->entityDef && space->entityDef->parms.timeGroup) {
			oldFloatTime = dmap_tr.viewDef->floatTime;
			oldTime = dmap_tr.viewDef->renderView.time;

			dmap_tr.viewDef->floatTime = game->GetTimeGroupTime(space->entityDef->parms.timeGroup) * 0.001;
			dmap_tr.viewDef->renderView.time = game->GetTimeGroupTime(space->entityDef->parms.timeGroup);
		}

		shader->EvaluateRegisters(regs, shaderParms, dmap_tr.viewDef, renderEntity->referenceSound);

		if (space->entityDef && space->entityDef->parms.timeGroup) {
			dmap_tr.viewDef->floatTime = oldFloatTime;
			dmap_tr.viewDef->renderView.time = oldTime;
		}
	}

	// check for deformations
	R_DeformDrawSurf(drawSurf);

	// skybox surfaces need a dynamic texgen
	switch (shader->Texgen()) {
	case TG_SKYBOX_CUBE:
		R_SkyboxTexGen(drawSurf, dmap_tr.viewDef->renderView.vieworg);
		break;
	case TG_WOBBLESKY_CUBE:
		R_WobbleskyTexGen(drawSurf, dmap_tr.viewDef->renderView.vieworg);
		break;
	}

	// check for gui surfaces
	idUserInterface	*gui = NULL;

	if (!space->entityDef) {
		gui = shader->GlobalGui();
	}
	else {
		int guiNum = shader->GetEntityGui() - 1;
		if (guiNum >= 0 && guiNum < MAX_RENDERENTITY_GUI) {
			gui = renderEntity->gui[guiNum];
		}
		if (gui == NULL) {
			gui = shader->GlobalGui();
		}
	}

	if (gui) {
		// force guis on the fast time
		float oldFloatTime;
		int oldTime;

		oldFloatTime = dmap_tr.viewDef->floatTime;
		oldTime = dmap_tr.viewDef->renderView.time;

		dmap_tr.viewDef->floatTime = game->GetTimeGroupTime(1) * 0.001;
		dmap_tr.viewDef->renderView.time = game->GetTimeGroupTime(1);

		idBounds ndcBounds;

		if (!R_PreciseCullSurface(drawSurf, ndcBounds)) {
			// did we ever use this to forward an entity color to a gui that didn't set color?
			//			memcpy( dmap_tr.guiShaderParms, shaderParms, sizeof( dmap_tr.guiShaderParms ) );
			R_RenderGuiSurf(gui, drawSurf);
		}

		dmap_tr.viewDef->floatTime = oldFloatTime;
		dmap_tr.viewDef->renderView.time = oldTime;
	}

	// we can't add subviews at this point, because that would
	// increment dmap_tr.viewCount, messing up the rest of the surface
	// adds for this view
	*/
}
