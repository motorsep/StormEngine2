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
#ifndef __DMAPMODEL_LOCAL_H__
#define __DMAPMODEL_LOCAL_H__

#include "DmapModel.h"

/*
===============================================================================

Static model

===============================================================================
*/

class idJointMat;

class idDmapRenderModelStatic : public idDmapRenderModel {
public:
	// the inherited public interface
	static idDmapRenderModel *		Alloc();

	idDmapRenderModelStatic();
	virtual						~idDmapRenderModelStatic();

	virtual void				InitFromFile(const char *fileName);
	virtual void				PartialInitFromFile(const char *fileName);
	virtual void				PurgeModel();
	virtual void				Reset() {};
	virtual void				LoadModel();
	virtual bool				IsLoaded() const;
	virtual void				SetLevelLoadReferenced(bool referenced);
	virtual bool				IsLevelLoadReferenced();
	virtual void				TouchData();
	virtual void				InitEmpty(const char *name);
	virtual void				AddSurface(dmapModelSurface_t surface);
	virtual void				FinishSurfaces();
	virtual void				FreeVertexCache();
	virtual const char *		Name() const;
	virtual void				Print() const;
	virtual void				List() const;
	virtual int					Memory() const;
	virtual ID_TIME_T				Timestamp() const;
	virtual int					NumSurfaces() const;
	virtual int					NumBaseSurfaces() const;
	virtual const dmapModelSurface_t *Surface(int surfaceNum) const;
	virtual srfDmapTriangles_t *	AllocSurfaceTriangles(int numVerts, int numIndexes) const;
	virtual void				FreeSurfaceTriangles(srfDmapTriangles_t *tris) const;
	virtual srfDmapTriangles_t *	ShadowHull() const;
	virtual bool				IsStaticWorldModel() const;
	virtual dynamicModel_t		IsDynamicModel() const;
	virtual bool				IsDefaultModel() const;
	virtual bool				IsReloadable() const;
	virtual idDmapRenderModel *		InstantiateDynamicModel(const struct dmapRenderEntity_s *ent, const struct dmapViewDef_s *view, idDmapRenderModel *cachedModel);
	virtual int					NumJoints(void) const;
	virtual const idMD5Joint *	GetJoints(void) const;
	virtual jointHandle_t		GetJointHandle(const char *name) const;
	virtual const char *		GetJointName(jointHandle_t handle) const;
	virtual const idJointQuat *	GetDefaultPose(void) const;
	virtual int					NearestJoint(int surfaceNum, int a, int b, int c) const;
	virtual idBounds			Bounds(const struct dmapRenderEntity_s *ent) const;
	virtual void				ReadFromDemoFile(class idDemoFile *f);
	virtual void				WriteToDemoFile(class idDemoFile *f);
	virtual float				DepthHack() const;

	void						MakeDefaultModel();

	bool						LoadASE(const char *fileName);
	bool						LoadLWO(const char *fileName);
	bool						LoadFLT(const char *fileName);
	bool						LoadMA(const char *filename);

	bool						ConvertASEToModelSurfaces(const struct aseModel_s *ase);
	bool						ConvertLWOToModelSurfaces(const struct st_lwObject *lwo);
	bool						ConvertMAToModelSurfaces(const struct maModel_s *ma);

	struct aseModel_s *			ConvertLWOToASE(const struct st_lwObject *obj, const char *fileName);

	bool						DeleteSurfaceWithId(int id);
	void						DeleteSurfacesWithNegativeId(void);
	bool						FindSurfaceWithId(int id, int &surfaceNum);

public:
	idList<dmapModelSurface_t>		surfaces;
	idBounds					bounds;
	int							overlaysAdded;

protected:
	int							lastModifiedFrame;
	int							lastArchivedFrame;

	idStr						name;
	srfDmapTriangles_t *			shadowHull;
	bool						isStaticWorldModel;
	bool						defaulted;
	bool						purged;					// eventually we will have dynamic reloading
	bool						fastLoad;				// don't generate tangents and shadow data
	bool						reloadable;				// if not, reloadModels won't check timestamp
	bool						levelLoadReferenced;	// for determining if it needs to be freed
	ID_TIME_T						timeStamp;

	static idCVar				r_mergeModelSurfaces;	// combine model surfaces with the same material
	static idCVar				r_slopVertex;			// merge xyz coordinates this far apart
	static idCVar				r_slopTexCoord;			// merge texture coordinates this far apart
	static idCVar				r_slopNormal;			// merge normals that dot less than this
};

/*
===============================================================================

MD5 animated model

===============================================================================
*/

class idDmapMD5Mesh {
	friend class				idDmapRenderModelMD5;

public:
	idDmapMD5Mesh();
	~idDmapMD5Mesh();

	void						ParseMesh(idLexer &parser, int numJoints, const idJointMat *joints);
	void						UpdateSurface(const struct dmapRenderEntity_s *ent, const idJointMat *joints, dmapModelSurface_t *surf);
	idBounds					CalcBounds(const idJointMat *joints);
	int							NearestJoint(int a, int b, int c) const;
	int							NumVerts(void) const;
	int							NumTris(void) const;
	int							NumWeights(void) const;

private:
	idList<idVec2>				texCoords;			// texture coordinates
	int							numWeights;			// number of weights
	idVec4 *					scaledWeights;		// joint weights
	int *						weightIndex;		// pairs of: joint offset + bool true if next weight is for next vertex
	const idMaterial *			shader;				// material applied to mesh
	int							numTris;			// number of triangles
	struct dmapDeformInfo_s *		deformInfo;			// used to create srfDmapTriangles_t from base frames and new vertexes
	int							surfaceNum;			// number of the static surface created for this mesh

	void						TransformVerts(idDmapDrawVert *verts, const idJointMat *joints);
	void						TransformScaledVerts(idDmapDrawVert *verts, const idJointMat *joints, float scale);
};

class idDmapRenderModelMD5 : public idDmapRenderModelStatic {
public:
	virtual void				InitFromFile(const char *fileName);
	virtual dynamicModel_t		IsDynamicModel() const;
	virtual idBounds			Bounds(const struct dmapRenderEntity_s *ent) const;
	virtual void				Print() const;
	virtual void				List() const;
	virtual void				TouchData();
	virtual void				PurgeModel();
	virtual void				LoadModel();
	virtual int					Memory() const;
	virtual idDmapRenderModel *		InstantiateDynamicModel(const struct dmapRenderEntity_s *ent, const struct dmapViewDef_s *view, idDmapRenderModel *cachedModel);
	virtual int					NumJoints(void) const;
	virtual const idMD5Joint *	GetJoints(void) const;
	virtual jointHandle_t		GetJointHandle(const char *name) const;
	virtual const char *		GetJointName(jointHandle_t handle) const;
	virtual const idJointQuat *	GetDefaultPose(void) const;
	virtual int					NearestJoint(int surfaceNum, int a, int b, int c) const;

private:
	idList<idMD5Joint>			joints;
	idList<idJointQuat>			defaultPose;
	idList<idDmapMD5Mesh>			meshes;

	void						CalculateBounds(const idJointMat *joints);
	void						GetFrameBounds(const dmapRenderEntity_t *ent, idBounds &bounds) const;
	void						DrawJoints(const dmapRenderEntity_t *ent, const struct dmapViewDef_s *view) const;
	void						ParseJoint(idLexer &parser, idMD5Joint *joint, idJointQuat *defaultPose);
};

/*
===============================================================================

MD3 animated model

===============================================================================
*/

struct md3Header_s;
struct md3Surface_s;

class idDmapRenderModelMD3 : public idDmapRenderModelStatic {
public:
	virtual void				InitFromFile(const char *fileName);
	virtual dynamicModel_t		IsDynamicModel() const;
	virtual idDmapRenderModel *		InstantiateDynamicModel(const struct dmapRenderEntity_s *ent, const struct dmapViewDef_s *view, idDmapRenderModel *cachedModel);
	virtual idBounds			Bounds(const struct dmapRenderEntity_s *ent) const;

private:
	int							index;			// model = tr.models[model->index]
	int							dataSize;		// just for listing purposes
	struct md3Header_s *		md3;			// only if type == MOD_MESH
	int							numLods;

	void						LerpMeshVertexes(srfDmapTriangles_t *tri, const struct md3Surface_s *surf, const float backlerp, const int frame, const int oldframe) const;
};

/*
===============================================================================

Liquid model

===============================================================================
*/

class idDmapRenderModelLiquid : public idDmapRenderModelStatic {
public:
	idDmapRenderModelLiquid();

	virtual void				InitFromFile(const char *fileName);
	virtual dynamicModel_t		IsDynamicModel() const;
	virtual idDmapRenderModel *		InstantiateDynamicModel(const struct dmapRenderEntity_s *ent, const struct dmapViewDef_s *view, idDmapRenderModel *cachedModel);
	virtual idBounds			Bounds(const struct dmapRenderEntity_s *ent) const;

	virtual void				Reset();
	void						IntersectBounds(const idBounds &bounds, float displacement);

private:
	dmapModelSurface_t				GenerateSurface(float lerp);
	void						WaterDrop(int x, int y, float *page);
	void						Update(void);

	int							verts_x;
	int							verts_y;
	float						scale_x;
	float						scale_y;
	int							time;
	int							liquid_type;
	int							update_tics;
	int							seed;

	idRandom					random;

	const idMaterial *			shader;
	struct dmapDeformInfo_s	*		deformInfo;		// used to create srfDmapTriangles_t from base frames
	// and new vertexes

	float						density;
	float						drop_height;
	int							drop_radius;
	float						drop_delay;

	idList<float>				pages;
	float *						page1;
	float *						page2;

	idList<idDmapDrawVert>			verts;

	int							nextDropTime;

};

/*
===============================================================================

PRT model

===============================================================================
*/

class idDmapRenderModelPrt : public idDmapRenderModelStatic {
public:
	idDmapRenderModelPrt();

	virtual void				InitFromFile(const char *fileName);
	virtual void				TouchData();
	virtual dynamicModel_t		IsDynamicModel() const;
	virtual idDmapRenderModel *		InstantiateDynamicModel(const struct dmapRenderEntity_s *ent, const struct dmapViewDef_s *view, idDmapRenderModel *cachedModel);
	virtual idBounds			Bounds(const struct dmapRenderEntity_s *ent) const;
	virtual float				DepthHack() const;
	virtual int					Memory() const;

private:
	const idDeclParticle *		particleSystem;
};

/*
===============================================================================

Beam model

===============================================================================
*/

class idDmapRenderModelBeam : public idDmapRenderModelStatic {
public:
	virtual dynamicModel_t		IsDynamicModel() const;
	virtual bool				IsLoaded() const;
	virtual idDmapRenderModel *		InstantiateDynamicModel(const struct dmapRenderEntity_s *ent, const struct dmapViewDef_s *view, idDmapRenderModel *cachedModel);
	virtual idBounds			Bounds(const struct dmapRenderEntity_s *ent) const;
};


/*
================================================================================

idRenderModelSprite

================================================================================
*/
class idDmapRenderModelSprite : public idDmapRenderModelStatic {
public:
	virtual	dynamicModel_t	IsDynamicModel() const;
	virtual	bool			IsLoaded() const;
	virtual	idDmapRenderModel *	InstantiateDynamicModel(const struct dmapRenderEntity_s *ent, const struct dmapViewDef_s *view, idDmapRenderModel *cachedModel);
	virtual	idBounds		Bounds(const struct dmapRenderEntity_s *ent) const;
};

#endif /* !__DMAPMODEL_LOCAL_H__ */
