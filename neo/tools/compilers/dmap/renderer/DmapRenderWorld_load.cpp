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

#include "DmapModelManager.h"

#include "Dmap_tr_local.h"

/*
================
idDmapRenderWorldLocal::FreeWorld
================
*/
void idDmapRenderWorldLocal::FreeWorld() {
	int i;

	// this will free all the lightDefs and entityDefs
	FreeDefs();

	// free all the portals and check light/model references
	for (i = 0; i < numPortalAreas; i++) {
		dmapPortalArea_t	*area;
		dmapPortal_t		*portal, *nextPortal;

		area = &portalAreas[i];
		for (portal = area->portals; portal; portal = nextPortal) {
			nextPortal = portal->next;
			delete portal->w;
			R_StaticFree(portal);
		}

		// there shouldn't be any remaining lightRefs or entityRefs
		if (area->lightRefs.areaNext != &area->lightRefs) {
			common->Error("FreeWorld: unexpected remaining lightRefs");
		}
		if (area->entityRefs.areaNext != &area->entityRefs) {
			common->Error("FreeWorld: unexpected remaining entityRefs");
		}
	}

	if (portalAreas) {
		R_StaticFree(portalAreas);
		portalAreas = NULL;
		numPortalAreas = 0;
		R_StaticFree(areaScreenRect);
		areaScreenRect = NULL;
	}

	if (doublePortals) {
		R_StaticFree(doublePortals);
		doublePortals = NULL;
		numInterAreaPortals = 0;
	}

	if (areaNodes) {
		R_StaticFree(areaNodes);
		areaNodes = NULL;
	}

	// free all the inline idRenderModels
	for (i = 0; i < localModels.Num(); i++) {
		dmapRenderModelManager->RemoveModel(localModels[i]);
		delete localModels[i];
	}
	localModels.Clear();

	areaReferenceAllocator.Shutdown();
	interactionAllocator.Shutdown();
	areaNumRefAllocator.Shutdown();

	mapName = "<FREED>";
}

/*
================
idDmapRenderWorldLocal::TouchWorldModels
================
*/
void idDmapRenderWorldLocal::TouchWorldModels(void) {
	int i;

	for (i = 0; i < localModels.Num(); i++) {
		dmapRenderModelManager->CheckModel(localModels[i]->Name());
	}
}

/*
================
idDmapRenderWorldLocal::ParseModel
================
*/
idDmapRenderModel *idDmapRenderWorldLocal::ParseModel(idLexer *src) {
	idDmapRenderModel	*model;
	idToken			token;
	int				i, j;
	srfDmapTriangles_t	*tri;
	dmapModelSurface_t	surf;

	src->ExpectTokenString("{");

	// parse the name
	src->ExpectAnyToken(&token);

	model = dmapRenderModelManager->AllocModel();
	model->InitEmpty(token);

	int numSurfaces = src->ParseInt();
	if (numSurfaces < 0) {
		src->Error("R_ParseModel: bad numSurfaces");
	}

	for (i = 0; i < numSurfaces; i++) {
		src->ExpectTokenString("{");

		src->ExpectAnyToken(&token);

		surf.shader = declManager->FindMaterial(token);

		((idMaterial*)surf.shader)->AddReference();

		tri = R_AllocStaticTriSurfDmap();
		surf.geometry = tri;

		tri->numVerts = src->ParseInt();
		tri->numIndexes = src->ParseInt();

		R_AllocStaticTriSurfVertsDmap(tri, tri->numVerts);
		for (j = 0; j < tri->numVerts; j++) {
			float	vec[8];

			src->Parse1DMatrix(8, vec);

			tri->verts[j].xyz[0] = vec[0];
			tri->verts[j].xyz[1] = vec[1];
			tri->verts[j].xyz[2] = vec[2];
			tri->verts[j].st[0] = vec[3];
			tri->verts[j].st[1] = vec[4];
			tri->verts[j].normal[0] = vec[5];
			tri->verts[j].normal[1] = vec[6];
			tri->verts[j].normal[2] = vec[7];
		}

		R_AllocStaticTriSurfIndexesDmap(tri, tri->numIndexes);
		for (j = 0; j < tri->numIndexes; j++) {
			tri->indexes[j] = src->ParseInt();
		}
		src->ExpectTokenString("}");

		// add the completed surface to the model
		model->AddSurface(surf);
	}

	src->ExpectTokenString("}");

	model->FinishSurfaces();

	return model;
}

/*
================
idDmapRenderWorldLocal::ParseShadowModel
================
*/
idDmapRenderModel *idDmapRenderWorldLocal::ParseShadowModel(idLexer *src) {
	idDmapRenderModel	*model;
	idToken			token;
	int				j;
	srfDmapTriangles_t	*tri;
	dmapModelSurface_t	surf;

	src->ExpectTokenString("{");

	// parse the name
	src->ExpectAnyToken(&token);

	model = dmapRenderModelManager->AllocModel();
	model->InitEmpty(token);

	surf.shader = dmap_tr.defaultMaterial;

	tri = R_AllocStaticTriSurfDmap();
	surf.geometry = tri;

	tri->numVerts = src->ParseInt();
	tri->numShadowIndexesNoCaps = src->ParseInt();
	tri->numShadowIndexesNoFrontCaps = src->ParseInt();
	tri->numIndexes = src->ParseInt();
	tri->shadowCapPlaneBits = src->ParseInt();

	R_AllocStaticTriSurfShadowVertsDmap(tri, tri->numVerts);
	tri->bounds.Clear();
	for (j = 0; j < tri->numVerts; j++) {
		float	vec[8];

		src->Parse1DMatrix(3, vec);
		tri->shadowVertexes[j].xyz[0] = vec[0];
		tri->shadowVertexes[j].xyz[1] = vec[1];
		tri->shadowVertexes[j].xyz[2] = vec[2];
		tri->shadowVertexes[j].xyz[3] = 1;		// no homogenous value

		tri->bounds.AddPoint(tri->shadowVertexes[j].xyz.ToVec3());
	}

	R_AllocStaticTriSurfIndexesDmap(tri, tri->numIndexes);
	for (j = 0; j < tri->numIndexes; j++) {
		tri->indexes[j] = src->ParseInt();
	}

	// add the completed surface to the model
	model->AddSurface(surf);

	src->ExpectTokenString("}");

	// we do NOT do a model->FinishSurfaceces, because we don't need sil edges, planes, tangents, etc.
	//	model->FinishSurfaces();

	return model;
}

/*
================
idDmapRenderWorldLocal::SetupAreaRefs
================
*/
void idDmapRenderWorldLocal::SetupAreaRefs() {
	int		i;

	connectedAreaNum = 0;
	for (i = 0; i < numPortalAreas; i++) {
		portalAreas[i].areaNum = i;
		portalAreas[i].lightRefs.areaNext =
			portalAreas[i].lightRefs.areaPrev =
			&portalAreas[i].lightRefs;
		portalAreas[i].entityRefs.areaNext =
			portalAreas[i].entityRefs.areaPrev =
			&portalAreas[i].entityRefs;
	}
}

/*
================
idDmapRenderWorldLocal::ParseInterAreaPortals
================
*/
void idDmapRenderWorldLocal::ParseInterAreaPortals(idLexer *src) {
	int i, j;

	src->ExpectTokenString("{");

	numPortalAreas = src->ParseInt();
	if (numPortalAreas < 0) {
		src->Error("R_ParseInterAreaPortals: bad numPortalAreas");
		return;
	}
	portalAreas = (dmapPortalArea_t *)R_ClearedStaticAlloc(numPortalAreas * sizeof(portalAreas[0]));
	areaScreenRect = (idScreenRect *)R_ClearedStaticAlloc(numPortalAreas * sizeof(idScreenRect));

	// set the doubly linked lists
	SetupAreaRefs();

	numInterAreaPortals = src->ParseInt();
	if (numInterAreaPortals < 0) {
		src->Error("R_ParseInterAreaPortals: bad numInterAreaPortals");
		return;
	}

	doublePortals = (dmapDoublePortal_t *)R_ClearedStaticAlloc(numInterAreaPortals *
		sizeof(doublePortals[0]));

	for (i = 0; i < numInterAreaPortals; i++) {
		int		numPoints, a1, a2;
		idWinding	*w;
		dmapPortal_t	*p;

		numPoints = src->ParseInt();
		a1 = src->ParseInt();
		a2 = src->ParseInt();

		w = new idWinding(numPoints);
		w->SetNumPoints(numPoints);
		for (j = 0; j < numPoints; j++) {
			src->Parse1DMatrix(3, (*w)[j].ToFloatPtr());
			// no texture coordinates
			(*w)[j][3] = 0;
			(*w)[j][4] = 0;
		}

		// add the portal to a1
		p = (dmapPortal_t *)R_ClearedStaticAlloc(sizeof(*p));
		p->intoArea = a2;
		p->doublePortal = &doublePortals[i];
		p->w = w;
		p->w->GetPlane(p->plane);

		p->next = portalAreas[a1].portals;
		portalAreas[a1].portals = p;

		doublePortals[i].portals[0] = p;

		// reverse it for a2
		p = (dmapPortal_t *)R_ClearedStaticAlloc(sizeof(*p));
		p->intoArea = a1;
		p->doublePortal = &doublePortals[i];
		p->w = w->Reverse();
		p->w->GetPlane(p->plane);

		p->next = portalAreas[a2].portals;
		portalAreas[a2].portals = p;

		doublePortals[i].portals[1] = p;
	}

	src->ExpectTokenString("}");
}

/*
================
idDmapRenderWorldLocal::ParseNodes
================
*/
void idDmapRenderWorldLocal::ParseNodes(idLexer *src) {
	int			i;

	src->ExpectTokenString("{");

	numAreaNodes = src->ParseInt();
	if (numAreaNodes < 0) {
		src->Error("R_ParseNodes: bad numAreaNodes");
	}
	areaNodes = (areaNode_t *)R_ClearedStaticAlloc(numAreaNodes * sizeof(areaNodes[0]));

	for (i = 0; i < numAreaNodes; i++) {
		areaNode_t	*node;

		node = &areaNodes[i];

		src->Parse1DMatrix(4, node->plane.ToFloatPtr());
		node->children[0] = src->ParseInt();
		node->children[1] = src->ParseInt();
	}

	src->ExpectTokenString("}");
}

/*
================
idDmapRenderWorldLocal::CommonChildrenArea_r
================
*/
int idDmapRenderWorldLocal::CommonChildrenArea_r(areaNode_t *node) {
	int	nums[2];

	for (int i = 0; i < 2; i++) {
		if (node->children[i] <= 0) {
			nums[i] = -1 - node->children[i];
		}
		else {
			nums[i] = CommonChildrenArea_r(&areaNodes[node->children[i]]);
		}
	}

	// solid nodes will match any area
	if (nums[0] == AREANUM_SOLID) {
		nums[0] = nums[1];
	}
	if (nums[1] == AREANUM_SOLID) {
		nums[1] = nums[0];
	}

	int	common;
	if (nums[0] == nums[1]) {
		common = nums[0];
	}
	else {
		common = CHILDREN_HAVE_MULTIPLE_AREAS;
	}

	node->commonChildrenArea = common;

	return common;
}

/*
=================
idDmapRenderWorldLocal::ClearWorld

Sets up for a single area world
=================
*/
void idDmapRenderWorldLocal::ClearWorld() {
	numPortalAreas = 1;
	portalAreas = (dmapPortalArea_t *)R_ClearedStaticAlloc(sizeof(portalAreas[0]));
	areaScreenRect = (idScreenRect *)R_ClearedStaticAlloc(sizeof(idScreenRect));

	SetupAreaRefs();

	// even though we only have a single area, create a node
	// that has both children pointing at it so we don't need to
	//
	areaNodes = (areaNode_t *)R_ClearedStaticAlloc(sizeof(areaNodes[0]));
	areaNodes[0].plane[3] = 1;
	areaNodes[0].children[0] = -1;
	areaNodes[0].children[1] = -1;
}

/*
=================
idDmapRenderWorldLocal::FreeDefs

dump all the interactions
=================
*/
void idDmapRenderWorldLocal::FreeDefs() {
	int		i;

	generateAllInteractionsCalled = false;

	if (interactionTable) {
		R_StaticFree(interactionTable);
		interactionTable = NULL;
	}

	// free all lightDefs
	for (i = 0; i < lightDefs.Num(); i++) {
		idDmapRenderLightLocal	*light;

		light = lightDefs[i];
		if (light && light->world == this) {
			FreeLightDef(i);
			lightDefs[i] = NULL;
		}
	}

	// free all entityDefs
	for (i = 0; i < entityDefs.Num(); i++) {
		idDmapRenderEntityLocal	*mod;

		mod = entityDefs[i];
		if (mod && mod->world == this) {
			FreeEntityDef(i);
			entityDefs[i] = NULL;
		}
	}
}

/*
=================
idDmapRenderWorldLocal::InitFromMap

A NULL or empty name will make a world without a map model, which
is still useful for displaying a bare model
=================
*/
bool idDmapRenderWorldLocal::InitFromMap(const char *name) {
	idLexer *		src;
	idToken			token;
	idStr			filename;
	idDmapRenderModel *	lastModel;

	// if this is an empty world, initialize manually
	if (!name || !name[0]) {
		FreeWorld();
		mapName.Clear();
		ClearWorld();
		return true;
	}


	// load it
	filename = name;
	filename.SetFileExtension(PROC_FILE_EXT);

	// if we are reloading the same map, check the timestamp
	// and try to skip all the work
	ID_TIME_T currentTimeStamp;
	fileSystem->ReadFile(filename, NULL, &currentTimeStamp);

	if (name == mapName) {
		if (currentTimeStamp != FILE_NOT_FOUND_TIMESTAMP && currentTimeStamp == mapTimeStamp) {
			common->Printf("idDmapRenderWorldLocal::InitFromMap: retaining existing map\n");
			FreeDefs();
			TouchWorldModels();
			AddWorldModelEntities();
			ClearPortalStates();
			return true;
		}
		common->Printf("idDmapRenderWorldLocal::InitFromMap: timestamp has changed, reloading.\n");
	}

	FreeWorld();

	src = new idLexer(filename, LEXFL_NOSTRINGCONCAT | LEXFL_NODOLLARPRECOMPILE);
	if (!src->IsLoaded()) {
		common->Printf("idDmapRenderWorldLocal::InitFromMap: %s not found\n", filename.c_str());
		ClearWorld();
		return false;
	}


	mapName = name;
	mapTimeStamp = currentTimeStamp;

	// if we are writing a demo, archive the load command
	if (common->WriteDemo()) {
		WriteLoadMap();
	}

	if (!src->ReadToken(&token) || token.Icmp(PROC_FILE_ID)) {
		common->Printf("idDmapRenderWorldLocal::InitFromMap: bad id '%s' instead of '%s'\n", token.c_str(), PROC_FILE_ID);
		delete src;
		return false;
	}

	// parse the file
	while (1) {
		if (!src->ReadToken(&token)) {
			break;
		}

		if (token == "model") {
			lastModel = ParseModel(src);

			// add it to the model manager list
			dmapRenderModelManager->AddModel(lastModel);

			// save it in the list to free when clearing this map
			localModels.Append(lastModel);
			continue;
		}

		if (token == "shadowModel") {
			lastModel = ParseShadowModel(src);

			// add it to the model manager list
			dmapRenderModelManager->AddModel(lastModel);

			// save it in the list to free when clearing this map
			localModels.Append(lastModel);
			continue;
		}

		if (token == "interAreaPortals") {
			ParseInterAreaPortals(src);
			continue;
		}

		if (token == "nodes") {
			ParseNodes(src);
			continue;
		}

		src->Error("idDmapRenderWorldLocal::InitFromMap: bad token \"%s\"", token.c_str());
	}

	delete src;

	// if it was a trivial map without any areas, create a single area
	if (!numPortalAreas) {
		ClearWorld();
	}

	// find the points where we can early-our of reference pushing into the BSP tree
	CommonChildrenArea_r(&areaNodes[0]);

	AddWorldModelEntities();
	ClearPortalStates();

	// done!
	return true;
}

/*
=====================
idDmapRenderWorldLocal::ClearPortalStates
=====================
*/
void idDmapRenderWorldLocal::ClearPortalStates() {
	int		i, j;

	// all portals start off open
	for (i = 0; i < numInterAreaPortals; i++) {
		doublePortals[i].blockingBits = PS_BLOCK_NONE;
	}

	// flood fill all area connections
	for (i = 0; i < numPortalAreas; i++) {
		for (j = 0; j < NUM_PORTAL_ATTRIBUTES; j++) {
			connectedAreaNum++;
			FloodConnectedAreas(&portalAreas[i], j);
		}
	}
}

/*
=====================
idDmapRenderWorldLocal::AddWorldModelEntities
=====================
*/
void idDmapRenderWorldLocal::AddWorldModelEntities() {
	int		i;

	// add the world model for each portal area
	// we can't just call AddEntityDef, because that would place the references
	// based on the bounding box, rather than explicitly into the correct area
	for (i = 0; i < numPortalAreas; i++) {
		idDmapRenderEntityLocal	*def;
		int			index;

		def = new idDmapRenderEntityLocal;

		// try and reuse a free spot
		index = entityDefs.FindNull();
		if (index == -1) {
			index = entityDefs.Append(def);
		}
		else {
			entityDefs[index] = def;
		}

		def->index = index;
		def->world = this;

		def->parms.hModel = dmapRenderModelManager->FindModel(va("_area%i", i));
		if (def->parms.hModel->IsDefaultModel() || !def->parms.hModel->IsStaticWorldModel()) {
			common->Error("idDmapRenderWorldLocal::InitFromMap: bad area model lookup");
		}

		idDmapRenderModel *hModel = def->parms.hModel;

		for (int j = 0; j < hModel->NumSurfaces(); j++) {
			const dmapModelSurface_t *surf = hModel->Surface(j);
			// Portal sky addition
			if ((surf->shader->GetName() == idStr("textures/smf/portal_sky")) || (surf->shader->GetName() == idStr("textures/editor/portal_sky"))) {
				def->needsPortalSky = true;
			}
		}

		def->referenceBounds = def->parms.hModel->Bounds();

		def->parms.axis[0][0] = 1;
		def->parms.axis[1][1] = 1;
		def->parms.axis[2][2] = 1;

		R_AxisToModelMatrix(def->parms.axis, def->parms.origin, def->modelMatrix);

		// in case an explicit shader is used on the world, we don't
		// want it to have a 0 alpha or color
		def->parms.shaderParms[0] =
			def->parms.shaderParms[1] =
			def->parms.shaderParms[2] =
			def->parms.shaderParms[3] = 1;

		AddEntityRefToArea(def, &portalAreas[i]);
	}
}

/*
=====================
CheckAreaForPortalSky
=====================
*/
bool idDmapRenderWorldLocal::CheckAreaForPortalSky(int areaNum) {
	dmapAreaReference_t	*ref;

	assert(areaNum >= 0 && areaNum < numPortalAreas);

	for (ref = portalAreas[areaNum].entityRefs.areaNext; ref->entity; ref = ref->areaNext) {
		assert(ref->area == &portalAreas[areaNum]);

		if (ref->entity && ref->entity->needsPortalSky) {
			return true;
		}
	}

	return false;
}
