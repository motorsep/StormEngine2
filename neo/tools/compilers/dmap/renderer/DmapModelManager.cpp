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

#include "DmapModel_local.h"
#include "Dmap_tr_local.h"	// just for R_FreeWorldInteractions and R_CreateWorldInteractions

class idDmapRenderModelManagerLocal : public idDmapRenderModelManager {
public:
	idDmapRenderModelManagerLocal();
	virtual					~idDmapRenderModelManagerLocal() {}

	virtual void			Init();
	virtual void			Shutdown();
	virtual idDmapRenderModel *	AllocModel();
	virtual void			FreeModel(idDmapRenderModel *model);
	virtual idDmapRenderModel *	FindModel(const char *modelName);
	virtual idDmapRenderModel *	CheckModel(const char *modelName);
	virtual idDmapRenderModel *	DefaultModel();
	virtual void			AddModel(idDmapRenderModel *model);
	virtual void			RemoveModel(idDmapRenderModel *model);
	virtual void			ReloadModels(bool forceAll = false);
	virtual void			FreeModelVertexCaches();
	virtual void			WritePrecacheCommands(idFile *file);
	virtual void			BeginLevelLoad();
	virtual void			EndLevelLoad();

	virtual	void			PrintMemInfo(MemInfo_t *mi);

private:
	idList<idDmapRenderModel*>	models;
	idHashIndex				hash;
	idDmapRenderModel *			defaultModel;
	idDmapRenderModel *			beamModel;
	idDmapRenderModel *			spriteModel;
	idDmapRenderModel *			trailModel;
	bool					insideLevelLoad;		// don't actually load now

	idDmapRenderModel *			GetModel(const char *modelName, bool createIfNotFound);

	static void				PrintModel_f(const idCmdArgs &args);
	static void				ListModels_f(const idCmdArgs &args);
	static void				ReloadModels_f(const idCmdArgs &args);
	static void				TouchModel_f(const idCmdArgs &args);
};

idDmapRenderModelManagerLocal	Dmap_localModelManager;
idDmapRenderModelManager *		dmapRenderModelManager = &Dmap_localModelManager;


/*
==============
idDmapRenderModelManagerLocal::idDmapRenderModelManagerLocal
==============
*/
idDmapRenderModelManagerLocal::idDmapRenderModelManagerLocal() {
	defaultModel = NULL;
	beamModel = NULL;
	spriteModel = NULL;
	insideLevelLoad = false;
	trailModel = NULL;
}

/*
==============
idDmapRenderModelManagerLocal::PrintModel_f
==============
*/
void idDmapRenderModelManagerLocal::PrintModel_f(const idCmdArgs &args) {
	idDmapRenderModel	*model;

	if (args.Argc() != 2) {
		common->Printf("usage: printModel <modelName>\n");
		return;
	}

	model = dmapRenderModelManager->CheckModel(args.Argv(1));
	if (!model) {
		common->Printf("model \"%s\" not found\n", args.Argv(1));
		return;
	}

	model->Print();
}

/*
==============
idDmapRenderModelManagerLocal::ListModels_f
==============
*/
void idDmapRenderModelManagerLocal::ListModels_f(const idCmdArgs &args) {
	int		totalMem = 0;
	int		inUse = 0;

	common->Printf(" mem   srf verts tris\n");
	common->Printf(" ---   --- ----- ----\n");

	for (int i = 0; i < Dmap_localModelManager.models.Num(); i++) {
		idDmapRenderModel	*model = Dmap_localModelManager.models[i];

		if (!model->IsLoaded()) {
			continue;
		}
		model->List();
		totalMem += model->Memory();
		inUse++;
	}

	common->Printf(" ---   --- ----- ----\n");
	common->Printf(" mem   srf verts tris\n");

	common->Printf("%i loaded models\n", inUse);
	common->Printf("total memory: %4.1fM\n", (float)totalMem / (1024 * 1024));
}

/*
==============
idDmapRenderModelManagerLocal::ReloadModels_f
==============
*/
void idDmapRenderModelManagerLocal::ReloadModels_f(const idCmdArgs &args) {
	if (idStr::Icmp(args.Argv(1), "all") == 0) {
		Dmap_localModelManager.ReloadModels(true);
	}
	else {
		Dmap_localModelManager.ReloadModels(false);
	}
}

/*
==============
idDmapRenderModelManagerLocal::TouchModel_f

Precache a specific model
==============
*/
void idDmapRenderModelManagerLocal::TouchModel_f(const idCmdArgs &args) {
	const char	*model = args.Argv(1);

	if (!model[0]) {
		common->Printf("usage: touchModel <modelName>\n");
		return;
	}

	common->Printf("touchModel %s\n", model);
	common->UpdateScreen(false);
	idDmapRenderModel *m = dmapRenderModelManager->CheckModel(model);
	if (!m) {
		common->Printf("...not found\n");
	}
}

/*
=================
idDmapRenderModelManagerLocal::WritePrecacheCommands
=================
*/
void idDmapRenderModelManagerLocal::WritePrecacheCommands(idFile *f) {
	for (int i = 0; i < models.Num(); i++) {
		idDmapRenderModel	*model = models[i];

		if (!model) {
			continue;
		}
		if (!model->IsReloadable()) {
			continue;
		}

		char	str[1024];
		sprintf(str, "touchModel %s\n", model->Name());
		common->Printf("%s", str);
		f->Printf("%s", str);
	}
}

/*
=================
idDmapRenderModelManagerLocal::Init
=================
*/
void idDmapRenderModelManagerLocal::Init() {
	/*
	cmdSystem->AddCommand("listModels", ListModels_f, CMD_FL_RENDERER, "lists all models");
	cmdSystem->AddCommand("printModel", PrintModel_f, CMD_FL_RENDERER, "prints model info", idCmdSystem::ArgCompletion_ModelName);
	cmdSystem->AddCommand("reloadModels", ReloadModels_f, CMD_FL_RENDERER | CMD_FL_CHEAT, "reloads models");
	cmdSystem->AddCommand("touchModel", TouchModel_f, CMD_FL_RENDERER, "touches a model", idCmdSystem::ArgCompletion_ModelName);
	*/
	insideLevelLoad = false;

	// create a default model
	idDmapRenderModelStatic *model = new idDmapRenderModelStatic;
	model->InitEmpty("_DEFAULT");
	model->MakeDefaultModel();
	model->SetLevelLoadReferenced(true);
	defaultModel = model;
	AddModel(model);

	// create the beam model
	idDmapRenderModelStatic *beam = new idDmapRenderModelBeam;
	beam->InitEmpty("_BEAM");
	beam->SetLevelLoadReferenced(true);
	beamModel = beam;
	AddModel(beam);

	idDmapRenderModelStatic *sprite = new idDmapRenderModelSprite;
	sprite->InitEmpty("_SPRITE");
	sprite->SetLevelLoadReferenced(true);
	spriteModel = sprite;
	AddModel(sprite);
}

/*
=================
idDmapRenderModelManagerLocal::Shutdown
=================
*/
void idDmapRenderModelManagerLocal::Shutdown() {
	models.DeleteContents(true);
	hash.Free();
}

/*
=================
idDmapRenderModelManagerLocal::GetModel
=================
*/
idDmapRenderModel *idDmapRenderModelManagerLocal::GetModel(const char *modelName, bool createIfNotFound) {
	idStr		canonical;
	idStr		extension;

	if (!modelName || !modelName[0]) {
		return NULL;
	}

	canonical = modelName;
	canonical.ToLower();

	// see if it is already present
	int key = hash.GenerateKey(modelName, false);
	for (int i = hash.First(key); i != -1; i = hash.Next(i)) {
		idDmapRenderModel *model = models[i];

		if (canonical.Icmp(model->Name()) == 0) {
			if (!model->IsLoaded()) {
				// reload it if it was purged
				model->LoadModel();
			}
			else if (insideLevelLoad && !model->IsLevelLoadReferenced()) {
				// we are reusing a model already in memory, but
				// touch all the materials to make sure they stay
				// in memory as well
				model->TouchData();
			}
			model->SetLevelLoadReferenced(true);
			return model;
		}
	}

	// see if we can load it

	// determine which subclass of idDmapRenderModel to initialize

	idDmapRenderModel	*model;

	canonical.ExtractFileExtension(extension);

	if ((extension.Icmp("ase") == 0) || (extension.Icmp("lwo") == 0) || (extension.Icmp("flt") == 0)) {
		model = new idDmapRenderModelStatic;
		model->InitFromFile(modelName);
	}
	else if (extension.Icmp("ma") == 0) {
		model = new idDmapRenderModelStatic;
		model->InitFromFile(modelName);
	}
	else if (extension.Icmp(MD5_MESH_EXT) == 0) {
		model = new idDmapRenderModelMD5;
		model->InitFromFile(modelName);
	}
	else if (extension.Icmp("md3") == 0) {
		model = new idDmapRenderModelMD3;
		model->InitFromFile(modelName);
	}
	else if (extension.Icmp("prt") == 0) {
		model = new idDmapRenderModelPrt;
		model->InitFromFile(modelName);
	}
	else if (extension.Icmp("liquid") == 0) {
		model = new idDmapRenderModelLiquid;
		model->InitFromFile(modelName);
	}
	else {

		if (extension.Length()) {
			common->Warning("unknown model type '%s'", canonical.c_str());
		}

		if (!createIfNotFound) {
			return NULL;
		}

		idDmapRenderModelStatic	*smodel = new idDmapRenderModelStatic;
		smodel->InitEmpty(modelName);
		smodel->MakeDefaultModel();

		model = smodel;
	}

	model->SetLevelLoadReferenced(true);

	if (!createIfNotFound && model->IsDefaultModel()) {
		delete model;
		model = NULL;

		return NULL;
	}

	AddModel(model);

	return model;
}

/*
=================
idDmapRenderModelManagerLocal::AllocModel
=================
*/
idDmapRenderModel *idDmapRenderModelManagerLocal::AllocModel() {
	return new idDmapRenderModelStatic();
}

/*
=================
idDmapRenderModelManagerLocal::FreeModel
=================
*/
void idDmapRenderModelManagerLocal::FreeModel(idDmapRenderModel *model) {
	if (!model) {
		return;
	}
	if (!dynamic_cast<idDmapRenderModelStatic *>(model)) {
		common->Error("idDmapRenderModelManager::FreeModel: model '%s' is not a static model", model->Name());
		return;
	}
	if (model == defaultModel) {
		common->Error("idDmapRenderModelManager::FreeModel: can't free the default model");
		return;
	}
	if (model == beamModel) {
		common->Error("idDmapRenderModelManager::FreeModel: can't free the beam model");
		return;
	}
	if (model == spriteModel) {
		common->Error("idDmapRenderModelManager::FreeModel: can't free the sprite model");
		return;
	}

	R_CheckForEntityDefsUsingModelDmap(model);

	delete model;
}

/*
=================
idDmapRenderModelManagerLocal::FindModel
=================
*/
idDmapRenderModel *idDmapRenderModelManagerLocal::FindModel(const char *modelName) {
	return GetModel(modelName, true);
}

/*
=================
idDmapRenderModelManagerLocal::CheckModel
=================
*/
idDmapRenderModel *idDmapRenderModelManagerLocal::CheckModel(const char *modelName) {
	return GetModel(modelName, false);
}

/*
=================
idDmapRenderModelManagerLocal::DefaultModel
=================
*/
idDmapRenderModel *idDmapRenderModelManagerLocal::DefaultModel() {
	return defaultModel;
}

/*
=================
idDmapRenderModelManagerLocal::AddModel
=================
*/
void idDmapRenderModelManagerLocal::AddModel(idDmapRenderModel *model) {
	hash.Add(hash.GenerateKey(model->Name(), false), models.Append(model));
}

/*
=================
idDmapRenderModelManagerLocal::RemoveModel
=================
*/
void idDmapRenderModelManagerLocal::RemoveModel(idDmapRenderModel *model) {
	int index = models.FindIndex(model);
	hash.RemoveIndex(hash.GenerateKey(model->Name(), false), index);
	models.RemoveIndex(index);
}

/*
=================
idDmapRenderModelManagerLocal::ReloadModels
=================
*/
void idDmapRenderModelManagerLocal::ReloadModels(bool forceAll) {
	if (forceAll) {
		common->Printf("Reloading all model files...\n");
	}
	else {
		common->Printf("Checking for changed model files...\n");
	}

	R_FreeDerivedData();

	// skip the default model at index 0
	for (int i = 1; i < models.Num(); i++) {
		idDmapRenderModel	*model = models[i];

		// we may want to allow world model reloading in the future, but we don't now
		if (!model->IsReloadable()) {
			continue;
		}

		if (!forceAll) {
			// check timestamp
			ID_TIME_T current;

			fileSystem->ReadFile(model->Name(), NULL, &current);
			if (current <= model->Timestamp()) {
				continue;
			}
		}

		common->DPrintf("reloading %s.\n", model->Name());

		model->LoadModel();
	}

	// we must force the world to regenerate, because models may
	// have changed size, making their references invalid
	R_ReCreateWorldReferences();
}

/*
=================
idDmapRenderModelManagerLocal::FreeModelVertexCaches
=================
*/
void idDmapRenderModelManagerLocal::FreeModelVertexCaches() {
	for (int i = 0; i < models.Num(); i++) {
		idDmapRenderModel *model = models[i];
		model->FreeVertexCache();
	}
}

/*
=================
idDmapRenderModelManagerLocal::BeginLevelLoad
=================
*/
void idDmapRenderModelManagerLocal::BeginLevelLoad() {
	insideLevelLoad = true;

	for (int i = 0; i < models.Num(); i++) {
		idDmapRenderModel *model = models[i];

		if (com_purgeAll.GetBool() && model->IsReloadable()) {
			R_CheckForEntityDefsUsingModelDmap(model);
			model->PurgeModel();
		}

		model->SetLevelLoadReferenced(false);
	}

	// purge unused triangle surface memory
	R_PurgeTriSurfData(dmapFrameData);
}

/*
=================
idDmapRenderModelManagerLocal::EndLevelLoad
=================
*/
void idDmapRenderModelManagerLocal::EndLevelLoad() {
	common->Printf("----- idDmapRenderModelManagerLocal::EndLevelLoad -----\n");

	int start = Sys_Milliseconds();

	insideLevelLoad = false;
	int	purgeCount = 0;
	int	keepCount = 0;
	int	loadCount = 0;

	// purge any models not touched
	for (int i = 0; i < models.Num(); i++) {
		idDmapRenderModel *model = models[i];

		// foresthale 2014-05-28: Brian Harris suggested the editors should never purge assets, because of potential for crashes on improperly refcounted assets
		if (!model->IsLevelLoadReferenced() && model->IsLoaded() && model->IsReloadable() && !(com_editors)) {

			//			common->Printf( "purging %s\n", model->Name() );

			purgeCount++;

			R_CheckForEntityDefsUsingModelDmap(model);

			model->PurgeModel();

		}
		else {

			//			common->Printf( "keeping %s\n", model->Name() );

			keepCount++;
		}
	}

	// purge unused triangle surface memory
	R_PurgeTriSurfData(dmapFrameData);

	// load any new ones
	for (int i = 0; i < models.Num(); i++) {
		idDmapRenderModel *model = models[i];

		if (model->IsLevelLoadReferenced() && !model->IsLoaded() && model->IsReloadable()) {

			loadCount++;
			model->LoadModel();

			if ((loadCount & 15) == 0) {
				assert(0);
				//session->PacifierUpdate();
			}
		}
	}

	// _D3XP added this
	int	end = Sys_Milliseconds();
	common->Printf("%5i models purged from previous level, ", purgeCount);
	common->Printf("%5i models kept.\n", keepCount);
	if (loadCount) {
		common->Printf("%5i new models loaded in %5.1f seconds\n", loadCount, (end - start) * 0.001);
	}
}

/*
=================
idDmapRenderModelManagerLocal::PrintMemInfo
=================
*/
void idDmapRenderModelManagerLocal::PrintMemInfo(MemInfo_t *mi) {
	int i, j, totalMem = 0;
	int *sortIndex;
	idFile *f;

	f = fileSystem->OpenFileWrite(mi->filebase + "_models.txt");
	if (!f) {
		return;
	}

	// sort first
	sortIndex = new int[Dmap_localModelManager.models.Num()];

	for (i = 0; i < Dmap_localModelManager.models.Num(); i++) {
		sortIndex[i] = i;
	}

	for (i = 0; i < Dmap_localModelManager.models.Num() - 1; i++) {
		for (j = i + 1; j < Dmap_localModelManager.models.Num(); j++) {
			if (Dmap_localModelManager.models[sortIndex[i]]->Memory() <  Dmap_localModelManager.models[sortIndex[j]]->Memory()) {
				int temp = sortIndex[i];
				sortIndex[i] = sortIndex[j];
				sortIndex[j] = temp;
			}
		}
	}

	// print next
	for (int i = 0; i < Dmap_localModelManager.models.Num(); i++) {
		idDmapRenderModel	*model = Dmap_localModelManager.models[sortIndex[i]];
		int mem;

		if (!model->IsLoaded()) {
			continue;
		}

		mem = model->Memory();
		totalMem += mem;
		f->Printf("%s %s\n", idStr::FormatNumber(mem).c_str(), model->Name());
	}

	delete sortIndex;
	mi->modelAssetsTotal = totalMem;

	f->Printf("\nTotal model bytes allocated: %s\n", idStr::FormatNumber(totalMem).c_str());
	fileSystem->CloseFile(f);
}
