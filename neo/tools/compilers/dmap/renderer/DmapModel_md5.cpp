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

#include "DmapModel_local.h"

static const char *MD5_SnapshotName = "_MD5_Snapshot_";

/***********************************************************************

idDmapMD5Mesh

***********************************************************************/

static int c_numVerts = 0;
static int c_numWeights = 0;
static int c_numWeightJoints = 0;

typedef struct vertexWeight_s {
	int							vert;
	int							joint;
	idVec3						offset;
	float						jointWeight;
} vertexWeight_t;

/*
====================
idDmapMD5Mesh::idDmapMD5Mesh
====================
*/
idDmapMD5Mesh::idDmapMD5Mesh() {
	scaledWeights = NULL;
	weightIndex = NULL;
	shader = NULL;
	numTris = 0;
	deformInfo = NULL;
	surfaceNum = 0;
}

/*
====================
idDmapMD5Mesh::~idDmapMD5Mesh
====================
*/
idDmapMD5Mesh::~idDmapMD5Mesh() {
	Mem_Free16(scaledWeights);
	Mem_Free16(weightIndex);
	if (deformInfo) {
		R_FreeDeformInfoDmap(deformInfo);
		deformInfo = NULL;
	}
}

/*
====================
idDmapMD5Mesh::ParseMesh
====================
*/
void idDmapMD5Mesh::ParseMesh(idLexer &parser, int numJoints, const idJointMat *joints) {
	idToken		token;
	idToken		name;
	int			num;
	int			count;
	int			jointnum;
	idStr		shaderName;
	int			i, j;
	idList<int>	tris;
	idList<int>	firstWeightForVertex;
	idList<int>	numWeightsForVertex;
	int			maxweight;
	idList<vertexWeight_t> tempWeights;

	parser.ExpectTokenString("{");

	//
	// parse name
	//
	if (parser.CheckTokenString("name")) {
		parser.ReadToken(&name);
	}

	//
	// parse shader
	//
	parser.ExpectTokenString("shader");

	parser.ReadToken(&token);
	shaderName = token;

	shader = declManager->FindMaterial(shaderName);

	//
	// parse texture coordinates
	//
	parser.ExpectTokenString("numverts");
	count = parser.ParseInt();
	if (count < 0) {
		parser.Error("Invalid size: %s", token.c_str());
	}

	texCoords.SetNum(count);
	firstWeightForVertex.SetNum(count);
	numWeightsForVertex.SetNum(count);

	numWeights = 0;
	maxweight = 0;
	for (i = 0; i < texCoords.Num(); i++) {
		parser.ExpectTokenString("vert");
		parser.ParseInt();

		parser.Parse1DMatrix(2, texCoords[i].ToFloatPtr());

		firstWeightForVertex[i] = parser.ParseInt();
		numWeightsForVertex[i] = parser.ParseInt();

		if (!numWeightsForVertex[i]) {
			parser.Error("Vertex without any joint weights.");
		}

		numWeights += numWeightsForVertex[i];
		if (numWeightsForVertex[i] + firstWeightForVertex[i] > maxweight) {
			maxweight = numWeightsForVertex[i] + firstWeightForVertex[i];
		}
	}

	//
	// parse tris
	//
	parser.ExpectTokenString("numtris");
	count = parser.ParseInt();
	if (count < 0) {
		parser.Error("Invalid size: %d", count);
	}

	tris.SetNum(count * 3);
	numTris = count;
	for (i = 0; i < count; i++) {
		parser.ExpectTokenString("tri");
		parser.ParseInt();

		tris[i * 3 + 0] = parser.ParseInt();
		tris[i * 3 + 1] = parser.ParseInt();
		tris[i * 3 + 2] = parser.ParseInt();
	}

	//
	// parse weights
	//
	parser.ExpectTokenString("numweights");
	count = parser.ParseInt();
	if (count < 0) {
		parser.Error("Invalid size: %d", count);
	}

	if (maxweight > count) {
		parser.Warning("Vertices reference out of range weights in model (%d of %d weights).", maxweight, count);
	}

	tempWeights.SetNum(count);

	for (i = 0; i < count; i++) {
		parser.ExpectTokenString("weight");
		parser.ParseInt();

		jointnum = parser.ParseInt();
		if ((jointnum < 0) || (jointnum >= numJoints)) {
			parser.Error("Joint Index out of range(%d): %d", numJoints, jointnum);
		}

		tempWeights[i].joint = jointnum;
		tempWeights[i].jointWeight = parser.ParseFloat();

		parser.Parse1DMatrix(3, tempWeights[i].offset.ToFloatPtr());
	}

	// create pre-scaled weights and an index for the vertex/joint lookup
	scaledWeights = (idVec4 *)Mem_Alloc16(numWeights * sizeof(scaledWeights[0]), TAG_DMAP);
	weightIndex = (int *)Mem_Alloc16(numWeights * 2 * sizeof(weightIndex[0]), TAG_DMAP);
	memset(weightIndex, 0, numWeights * 2 * sizeof(weightIndex[0]));

	count = 0;
	for (i = 0; i < texCoords.Num(); i++) {
		num = firstWeightForVertex[i];
		for (j = 0; j < numWeightsForVertex[i]; j++, num++, count++) {
			scaledWeights[count].ToVec3() = tempWeights[num].offset * tempWeights[num].jointWeight;
			scaledWeights[count].w = tempWeights[num].jointWeight;
			weightIndex[count * 2 + 0] = tempWeights[num].joint * sizeof(idJointMat);
		}
		weightIndex[count * 2 - 1] = 1;
	}

	tempWeights.Clear();
	numWeightsForVertex.Clear();
	firstWeightForVertex.Clear();

	parser.ExpectTokenString("}");

	// update counters
	c_numVerts += texCoords.Num();
	c_numWeights += numWeights;
	c_numWeightJoints++;
	for (i = 0; i < numWeights; i++) {
		c_numWeightJoints += weightIndex[i * 2 + 1];
	}

	//
	// build the information that will be common to all animations of this mesh:
	// silhouette edge connectivity and normal / tangent generation information
	//
	idDmapDrawVert *verts = (idDmapDrawVert *)_alloca16(texCoords.Num() * sizeof(idDmapDrawVert));
	for (i = 0; i < texCoords.Num(); i++) {
		verts[i].Clear();
		verts[i].st = texCoords[i];
	}
	TransformVerts(verts, joints);
	deformInfo = R_BuildDeformInfoDmap(texCoords.Num(), verts, tris.Num(), tris.Ptr(), shader->UseUnsmoothedTangents());
}

/*
====================
idDmapMD5Mesh::TransformVerts
====================
*/
void idDmapMD5Mesh::TransformVerts(idDmapDrawVert *verts, const idJointMat *entJoints) {
	dmapSIMDProcessor->TransformVerts(verts, texCoords.Num(), entJoints, scaledWeights, weightIndex, numWeights);
}

/*
====================
idDmapMD5Mesh::TransformScaledVerts

Special transform to make the mesh seem fat or skinny.  May be used for zombie deaths
====================
*/
void idDmapMD5Mesh::TransformScaledVerts(idDmapDrawVert *verts, const idJointMat *entJoints, float scale) {
	idVec4 *scaledWeights = (idVec4 *)_alloca16(numWeights * sizeof(scaledWeights[0]));
	dmapSIMDProcessor->Mul(scaledWeights[0].ToFloatPtr(), scale, scaledWeights[0].ToFloatPtr(), numWeights * 4);
	dmapSIMDProcessor->TransformVerts(verts, texCoords.Num(), entJoints, scaledWeights, weightIndex, numWeights);
}

/*
====================
idDmapMD5Mesh::UpdateSurface
====================
*/
void idDmapMD5Mesh::UpdateSurface(const struct dmapRenderEntity_s *ent, const idJointMat *entJoints, dmapModelSurface_t *surf) {
	int i, base;
	srfDmapTriangles_t *tri;

	dmap_tr.pc.c_deformedSurfaces++;
	dmap_tr.pc.c_deformedVerts += deformInfo->numOutputVerts;
	dmap_tr.pc.c_deformedIndexes += deformInfo->numIndexes;

	surf->shader = shader;

	if (surf->geometry) {
		// if the number of verts and indexes are the same we can re-use the triangle surface
		// the number of indexes must be the same to assure the correct amount of memory is allocated for the facePlanes
		if (surf->geometry->numVerts == deformInfo->numOutputVerts && surf->geometry->numIndexes == deformInfo->numIndexes) {
			R_FreeStaticTriSurfVertexCachesDmap(surf->geometry);
		}
		else {
			R_FreeStaticTriSurfDmap(surf->geometry);
			surf->geometry = R_AllocStaticTriSurfDmap();
		}
	}
	else {
		surf->geometry = R_AllocStaticTriSurfDmap();
	}

	tri = surf->geometry;

	// note that some of the data is references, and should not be freed
	tri->deformedSurface = true;
	tri->tangentsCalculated = false;
	tri->facePlanesCalculated = false;

	tri->numIndexes = deformInfo->numIndexes;
	tri->indexes = deformInfo->indexes;
	tri->silIndexes = deformInfo->silIndexes;
	tri->numMirroredVerts = deformInfo->numMirroredVerts;
	tri->mirroredVerts = deformInfo->mirroredVerts;
	tri->numDupVerts = deformInfo->numDupVerts;
	tri->dupVerts = deformInfo->dupVerts;
	tri->numSilEdges = deformInfo->numSilEdges;
	tri->silEdges = deformInfo->silEdges;
	tri->dominantTris = deformInfo->dominantTris;
	tri->numVerts = deformInfo->numOutputVerts;

	if (tri->verts == NULL) {
		R_AllocStaticTriSurfVertsDmap(tri, tri->numVerts);
		for (i = 0; i < deformInfo->numSourceVerts; i++) {
			tri->verts[i].Clear();
			tri->verts[i].st = texCoords[i];
		}
	}

	if (ent->shaderParms[SHADERPARM_MD5_SKINSCALE] != 0.0f) {
		TransformScaledVerts(tri->verts, entJoints, ent->shaderParms[SHADERPARM_MD5_SKINSCALE]);
	}
	else {
		TransformVerts(tri->verts, entJoints);
	}

	// replicate the mirror seam vertexes
	base = deformInfo->numOutputVerts - deformInfo->numMirroredVerts;
	for (i = 0; i < deformInfo->numMirroredVerts; i++) {
		tri->verts[base + i] = tri->verts[deformInfo->mirroredVerts[i]];
	}

	R_BoundTriSurfDmap(tri);

	// If a surface is going to be have a lighting interaction generated, it will also have to call
	// R_DeriveTangents() to get normals, tangents, and face planes.  If it only
	// needs shadows generated, it will only have to generate face planes.  If it only
	// has ambient drawing, or is culled, no additional work will be necessary
	if (!r_useDeferredTangents.GetBool()) {
		// set face planes, vertex normals, tangents
		R_DeriveTangentsDmap(tri);
	}
}

/*
====================
idDmapMD5Mesh::CalcBounds
====================
*/
idBounds idDmapMD5Mesh::CalcBounds(const idJointMat *entJoints) {
	idBounds	bounds;
	idDmapDrawVert *verts = (idDmapDrawVert *)_alloca16(texCoords.Num() * sizeof(idDmapDrawVert));

	TransformVerts(verts, entJoints);

	dmapSIMDProcessor->MinMax(bounds[0], bounds[1], verts, texCoords.Num());

	return bounds;
}

/*
====================
idDmapMD5Mesh::NearestJoint
====================
*/
int idDmapMD5Mesh::NearestJoint(int a, int b, int c) const {
	int i, bestJoint, vertNum, weightVertNum;
	float bestWeight;

	// duplicated vertices might not have weights
	if (a >= 0 && a < texCoords.Num()) {
		vertNum = a;
	}
	else if (b >= 0 && b < texCoords.Num()) {
		vertNum = b;
	}
	else if (c >= 0 && c < texCoords.Num()) {
		vertNum = c;
	}
	else {
		// all vertices are duplicates which shouldn't happen
		return 0;
	}

	// find the first weight for this vertex
	weightVertNum = 0;
	for (i = 0; weightVertNum < vertNum; i++) {
		weightVertNum += weightIndex[i * 2 + 1];
	}

	// get the joint for the largest weight
	bestWeight = scaledWeights[i].w;
	bestJoint = weightIndex[i * 2 + 0] / sizeof(idJointMat);
	for (; weightIndex[i * 2 + 1] == 0; i++) {
		if (scaledWeights[i].w > bestWeight) {
			bestWeight = scaledWeights[i].w;
			bestJoint = weightIndex[i * 2 + 0] / sizeof(idJointMat);
		}
	}
	return bestJoint;
}

/*
====================
idDmapMD5Mesh::NumVerts
====================
*/
int idDmapMD5Mesh::NumVerts(void) const {
	return texCoords.Num();
}

/*
====================
idDmapMD5Mesh::NumTris
====================
*/
int	idDmapMD5Mesh::NumTris(void) const {
	return numTris;
}

/*
====================
idDmapMD5Mesh::NumWeights
====================
*/
int	idDmapMD5Mesh::NumWeights(void) const {
	return numWeights;
}

/***********************************************************************

idDmapRenderModelMD5

***********************************************************************/

/*
====================
idDmapRenderModelMD5::ParseJoint
====================
*/
void idDmapRenderModelMD5::ParseJoint(idLexer &parser, idMD5Joint *joint, idJointQuat *defaultPose) {
	idToken	token;
	int		num;

	//
	// parse name
	//
	parser.ReadToken(&token);
	joint->name = token;

	//
	// parse parent
	//
	num = parser.ParseInt();
	if (num < 0) {
		joint->parent = NULL;
	}
	else {
		if (num >= joints.Num() - 1) {
			parser.Error("Invalid parent for joint '%s'", joint->name.c_str());
		}
		joint->parent = &joints[num];
	}

	//
	// parse default pose
	//
	parser.Parse1DMatrix(3, defaultPose->t.ToFloatPtr());
	parser.Parse1DMatrix(3, defaultPose->q.ToFloatPtr());
	defaultPose->q.w = defaultPose->q.CalcW();
}

/*
====================
idDmapRenderModelMD5::InitFromFile
====================
*/
void idDmapRenderModelMD5::InitFromFile(const char *fileName) {
	name = fileName;
	LoadModel();
}

/*
====================
idDmapRenderModelMD5::LoadModel

used for initial loads, reloadModel, and reloading the data of purged models
Upon exit, the model will absolutely be valid, but possibly as a default model
====================
*/
void idDmapRenderModelMD5::LoadModel() {
	int			version;
	int			i;
	int			num;
	int			parentNum;
	idToken		token;
	idLexer		parser(LEXFL_ALLOWPATHNAMES | LEXFL_NOSTRINGESCAPECHARS);
	idJointQuat	*pose;
	idMD5Joint	*joint;
	idJointMat *poseMat3;

	if (!purged) {
		PurgeModel();
	}
	purged = false;

	if (!parser.LoadFile(name)) {
		MakeDefaultModel();
		return;
	}

	parser.ExpectTokenString(MD5_VERSION_STRING);
	version = parser.ParseInt();

	if (version != MD5_VERSION) {
		parser.Error("Invalid version %d.  Should be version %d\n", version, MD5_VERSION);
	}

	//
	// skip commandline
	//
	parser.ExpectTokenString("commandline");
	parser.ReadToken(&token);

	// parse num joints
	parser.ExpectTokenString("numJoints");
	num = parser.ParseInt();
	joints.SetGranularity(1);
	joints.SetNum(num);
	defaultPose.SetGranularity(1);
	defaultPose.SetNum(num);
	poseMat3 = (idJointMat *)_alloca16(num * sizeof(*poseMat3));

	// parse num meshes
	parser.ExpectTokenString("numMeshes");
	num = parser.ParseInt();
	if (num < 0) {
		parser.Error("Invalid size: %d", num);
	}
	meshes.SetGranularity(1);
	meshes.SetNum(num);

	//
	// parse joints
	//
	parser.ExpectTokenString("joints");
	parser.ExpectTokenString("{");
	pose = defaultPose.Ptr();
	joint = joints.Ptr();
	for (i = 0; i < joints.Num(); i++, joint++, pose++) {
		ParseJoint(parser, joint, pose);
		poseMat3[i].SetRotation(pose->q.ToMat3());
		poseMat3[i].SetTranslation(pose->t);
		if (joint->parent) {
			parentNum = joint->parent - joints.Ptr();
			pose->q = (poseMat3[i].ToMat3() * poseMat3[parentNum].ToMat3().Transpose()).ToQuat();
			pose->t = (poseMat3[i].ToVec3() - poseMat3[parentNum].ToVec3()) * poseMat3[parentNum].ToMat3().Transpose();
		}
	}
	parser.ExpectTokenString("}");

	for (i = 0; i < meshes.Num(); i++) {
		parser.ExpectTokenString("mesh");
		meshes[i].ParseMesh(parser, defaultPose.Num(), poseMat3);
	}

	//
	// calculate the bounds of the model
	//
	CalculateBounds(poseMat3);

	// set the timestamp for reloadmodels
	fileSystem->ReadFile(name, NULL, &timeStamp);
}

/*
==============
idDmapRenderModelMD5::Print
==============
*/
void idDmapRenderModelMD5::Print() const {
	const idDmapMD5Mesh	*mesh;
	int			i;

	common->Printf("%s\n", name.c_str());
	common->Printf("Dynamic model.\n");
	common->Printf("Generated smooth normals.\n");
	common->Printf("    verts  tris weights material\n");
	int	totalVerts = 0;
	int	totalTris = 0;
	int	totalWeights = 0;
	for (mesh = meshes.Ptr(), i = 0; i < meshes.Num(); i++, mesh++) {
		totalVerts += mesh->NumVerts();
		totalTris += mesh->NumTris();
		totalWeights += mesh->NumWeights();
		common->Printf("%2i: %5i %5i %7i %s\n", i, mesh->NumVerts(), mesh->NumTris(), mesh->NumWeights(), mesh->shader->GetName());
	}
	common->Printf("-----\n");
	common->Printf("%4i verts.\n", totalVerts);
	common->Printf("%4i tris.\n", totalTris);
	common->Printf("%4i weights.\n", totalWeights);
	common->Printf("%4i joints.\n", joints.Num());
}

/*
==============
idDmapRenderModelMD5::List
==============
*/
void idDmapRenderModelMD5::List() const {
	int			i;
	const idDmapMD5Mesh	*mesh;
	int			totalTris = 0;
	int			totalVerts = 0;

	for (mesh = meshes.Ptr(), i = 0; i < meshes.Num(); i++, mesh++) {
		totalTris += mesh->numTris;
		totalVerts += mesh->NumVerts();
	}
	common->Printf(" %4ik %3i %4i %4i %s(MD5)", Memory() / 1024, meshes.Num(), totalVerts, totalTris, Name());

	if (defaulted) {
		common->Printf(" (DEFAULTED)");
	}

	common->Printf("\n");
}

/*
====================
idDmapRenderModelMD5::CalculateBounds
====================
*/
void idDmapRenderModelMD5::CalculateBounds(const idJointMat *entJoints) {
	int			i;
	idDmapMD5Mesh	*mesh;

	bounds.Clear();
	for (mesh = meshes.Ptr(), i = 0; i < meshes.Num(); i++, mesh++) {
		bounds.AddBounds(mesh->CalcBounds(entJoints));
	}
}

/*
====================
idDmapRenderModelMD5::Bounds

This calculates a rough bounds by using the joint radii without
transforming all the points
====================
*/
idBounds idDmapRenderModelMD5::Bounds(const dmapRenderEntity_t *ent) const {
#if 0
	// we can't calculate a rational bounds without an entity,
	// because joints could be positioned to deform it into an
	// arbitrarily large shape
	if (!ent) {
		common->Error("idDmapRenderModelMD5::Bounds: called without entity");
	}
#endif

	if (!ent) {
		// this is the bounds for the reference pose
		return bounds;
	}

	return ent->bounds;
}

/*
====================
idDmapRenderModelMD5::DrawJoints
====================
*/
void idDmapRenderModelMD5::DrawJoints(const dmapRenderEntity_t *ent, const struct dmapViewDef_s *view) const {
	
	assert(0);
	/*int					i;
	int					num;
	idVec3				pos;
	const idJointMat	*joint;
	const idMD5Joint	*md5Joint;
	int					parentNum;

	num = ent->numJoints;
	joint = ent->joints;
	md5Joint = joints.Ptr();
	for (i = 0; i < num; i++, joint++, md5Joint++) {
		pos = ent->origin + joint->ToVec3() * ent->axis;
		if (md5Joint->parent) {
			parentNum = md5Joint->parent - joints.Ptr();
			session->rw->DebugLine(colorWhite, ent->origin + ent->joints[parentNum].ToVec3() * ent->axis, pos);
		}

		session->rw->DebugLine(colorRed, pos, pos + joint->ToMat3()[0] * 2.0f * ent->axis);
		session->rw->DebugLine(colorGreen, pos, pos + joint->ToMat3()[1] * 2.0f * ent->axis);
		session->rw->DebugLine(colorBlue, pos, pos + joint->ToMat3()[2] * 2.0f * ent->axis);
	}

	idBounds bounds;

	bounds.FromTransformedBounds(ent->bounds, vec3_zero, ent->axis);
	session->rw->DebugBounds(colorMagenta, bounds, ent->origin);

	if ((r_jointNameScale.GetFloat() != 0.0f) && (bounds.Expand(128.0f).ContainsPoint(view->renderView.vieworg - ent->origin))) {
		idVec3	offset(0, 0, r_jointNameOffset.GetFloat());
		float	scale;

		scale = r_jointNameScale.GetFloat();
		joint = ent->joints;
		num = ent->numJoints;
		for (i = 0; i < num; i++, joint++) {
			pos = ent->origin + joint->ToVec3() * ent->axis;
			session->rw->DrawText(joints[i].name, pos + offset, scale, colorWhite, view->renderView.viewaxis, 1);
		}
	}*/
}

/*
====================
idDmapRenderModelMD5::InstantiateDynamicModel
====================
*/
idDmapRenderModel *idDmapRenderModelMD5::InstantiateDynamicModel(const struct dmapRenderEntity_s *ent, const struct dmapViewDef_s *view, idDmapRenderModel *cachedModel) {
	int					i, surfaceNum;
	idDmapMD5Mesh			*mesh;
	idDmapRenderModelStatic	*staticModel;

	if (cachedModel && !r_useCachedDynamicModels.GetBool()) {
		delete cachedModel;
		cachedModel = NULL;
	}

	if (purged) {
		common->DWarning("model %s instantiated while purged", Name());
		LoadModel();
	}

	if (!ent->joints) {
		common->Printf("idDmapRenderModelMD5::InstantiateDynamicModel: NULL joints on renderEntity for '%s'\n", Name());
		delete cachedModel;
		return NULL;
	}
	else if (ent->numJoints != joints.Num()) {
		common->Printf("idDmapRenderModelMD5::InstantiateDynamicModel: renderEntity has different number of joints than model for '%s'\n", Name());
		delete cachedModel;
		return NULL;
	}

	dmap_tr.pc.c_generateMd5++;

	if (cachedModel) {
		assert(dynamic_cast<idDmapRenderModelStatic *>(cachedModel) != NULL);
		assert(idStr::Icmp(cachedModel->Name(), MD5_SnapshotName) == 0);
		staticModel = static_cast<idDmapRenderModelStatic *>(cachedModel);
	}
	else {
		staticModel = new idDmapRenderModelStatic;
		staticModel->InitEmpty(MD5_SnapshotName);
	}

	staticModel->bounds.Clear();

	if (r_showSkel.GetInteger()) {
		if ((view != NULL) && (!r_skipSuppress.GetBool() || !ent->suppressSurfaceInViewID || (ent->suppressSurfaceInViewID != view->renderView.viewID))) {
			// only draw the skeleton
			DrawJoints(ent, view);
		}

		if (r_showSkel.GetInteger() > 1) {
			// turn off the model when showing the skeleton
			staticModel->InitEmpty(MD5_SnapshotName);
			return staticModel;
		}
	}

	// create all the surfaces
	for (mesh = meshes.Ptr(), i = 0; i < meshes.Num(); i++, mesh++) {
		// avoid deforming the surface if it will be a nodraw due to a skin remapping
		// FIXME: may have to still deform clipping hulls
		const idMaterial *shader = mesh->shader;

		shader = R_RemapShaderBySkin(shader, ent->customSkin, ent->customShader);

		if (!shader || (!shader->IsDrawn() && !shader->SurfaceCastsShadow())) {
			staticModel->DeleteSurfaceWithId(i);
			mesh->surfaceNum = -1;
			continue;
		}

		dmapModelSurface_t *surf;

		if (staticModel->FindSurfaceWithId(i, surfaceNum)) {
			mesh->surfaceNum = surfaceNum;
			surf = &staticModel->surfaces[surfaceNum];
		}
		else {

			// Remove Overlays before adding new surfaces
			idDmapRenderModelOverlay::RemoveOverlaySurfacesFromModel(staticModel);

			mesh->surfaceNum = staticModel->NumSurfaces();
			surf = &staticModel->surfaces.Alloc();
			surf->geometry = NULL;
			surf->shader = NULL;
			surf->id = i;
		}

		mesh->UpdateSurface(ent, ent->joints, surf);

		staticModel->bounds.AddPoint(surf->geometry->bounds[0]);
		staticModel->bounds.AddPoint(surf->geometry->bounds[1]);
	}

	return staticModel;
}

/*
====================
idDmapRenderModelMD5::IsDynamicModel
====================
*/
dynamicModel_t idDmapRenderModelMD5::IsDynamicModel() const {
	return DM_CACHED;
}

/*
====================
idDmapRenderModelMD5::NumJoints
====================
*/
int idDmapRenderModelMD5::NumJoints(void) const {
	return joints.Num();
}

/*
====================
idDmapRenderModelMD5::GetJoints
====================
*/
const idMD5Joint *idDmapRenderModelMD5::GetJoints(void) const {
	return joints.Ptr();
}

/*
====================
idDmapRenderModelMD5::GetDefaultPose
====================
*/
const idJointQuat *idDmapRenderModelMD5::GetDefaultPose(void) const {
	return defaultPose.Ptr();
}

/*
====================
idDmapRenderModelMD5::GetJointHandle
====================
*/
jointHandle_t idDmapRenderModelMD5::GetJointHandle(const char *name) const {
	const idMD5Joint *joint;
	int	i;

	joint = joints.Ptr();
	for (i = 0; i < joints.Num(); i++, joint++) {
		if (idStr::Icmp(joint->name.c_str(), name) == 0) {
			return (jointHandle_t)i;
		}
	}

	return INVALID_JOINT;
}

/*
=====================
idDmapRenderModelMD5::GetJointName
=====================
*/
const char *idDmapRenderModelMD5::GetJointName(jointHandle_t handle) const {
	if ((handle < 0) || (handle >= joints.Num())) {
		return "<invalid joint>";
	}

	return joints[handle].name;
}

/*
====================
idDmapRenderModelMD5::NearestJoint
====================
*/
int idDmapRenderModelMD5::NearestJoint(int surfaceNum, int a, int b, int c) const {
	int i;
	const idDmapMD5Mesh *mesh;

	if (surfaceNum > meshes.Num()) {
		common->Error("idDmapRenderModelMD5::NearestJoint: surfaceNum > meshes.Num()");
	}

	for (mesh = meshes.Ptr(), i = 0; i < meshes.Num(); i++, mesh++) {
		if (mesh->surfaceNum == surfaceNum) {
			return mesh->NearestJoint(a, b, c);
		}
	}
	return 0;
}

/*
====================
idDmapRenderModelMD5::TouchData

models that are already loaded at level start time
will still touch their materials to make sure they
are kept loaded
====================
*/
void idDmapRenderModelMD5::TouchData() {
	idDmapMD5Mesh	*mesh;
	int			i;

	for (mesh = meshes.Ptr(), i = 0; i < meshes.Num(); i++, mesh++) {
		declManager->FindMaterial(mesh->shader->GetName());
	}
}

/*
===================
idDmapRenderModelMD5::PurgeModel

frees all the data, but leaves the class around for dangling references,
which can regenerate the data with LoadModel()
===================
*/
void idDmapRenderModelMD5::PurgeModel() {
	purged = true;
	joints.Clear();
	defaultPose.Clear();
	meshes.Clear();
}

/*
===================
idDmapRenderModelMD5::Memory
===================
*/
int	idDmapRenderModelMD5::Memory() const {
	int		total, i;

	assert(0);
	total = sizeof(*this);
	/*total += joints.MemoryUsed() + defaultPose.MemoryUsed() + meshes.MemoryUsed();

	// count up strings
	for (i = 0; i < joints.Num(); i++) {
		total += joints[i].name.DynamicMemoryUsed();
	}

	// count up meshes
	for (i = 0; i < meshes.Num(); i++) {
		const idDmapMD5Mesh *mesh = &meshes[i];

		total += mesh->texCoords.MemoryUsed() + mesh->numWeights * (sizeof(mesh->scaledWeights[0]) + sizeof(mesh->weightIndex[0]) * 2);

		// sum up deform info
		total += sizeof(mesh->deformInfo);
		total += R_DeformInfoMemoryUsed(mesh->deformInfo);
	}*/
	return total;
}
