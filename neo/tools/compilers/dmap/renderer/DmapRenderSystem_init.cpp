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
#include "DmapVertexCache.h"

idGuiModel* tr_dmapGuiModel;

idCVar r_useTurboShadow("r_useTurboShadow", "1", CVAR_RENDERER | CVAR_BOOL, "use the infinite projection with W technique for dynamic shadows");
idCVar r_useShadowProjectedCull("r_useShadowProjectedCull", "1", CVAR_RENDERER | CVAR_BOOL, "discard triangles outside light volume before shadowing");
idCVar r_useShadowVertexProgram("r_useShadowVertexProgram", "1", CVAR_RENDERER | CVAR_BOOL, "do the shadow projection in the vertex program on capable cards");
idCVar r_useCulling("r_useCulling", "2", CVAR_RENDERER | CVAR_INTEGER, "0 = none, 1 = sphere, 2 = sphere + box", 0, 2, idCmdSystem::ArgCompletion_Integer<0, 2>);
idCVar r_useEntityCulling("r_useEntityCulling", "1", CVAR_RENDERER | CVAR_BOOL, "0 = none, 1 = box");
idCVar r_useLightCulling("r_useLightCulling", "3", CVAR_RENDERER | CVAR_INTEGER, "0 = none, 1 = box, 2 = exact clip of polyhedron faces, 3 = also areas", 0, 3, idCmdSystem::ArgCompletion_Integer<0, 3>);
idCVar com_purgeAll("com_purgeAll", "0", CVAR_BOOL | CVAR_ARCHIVE | CVAR_SYSTEM, "purge everything between level loads");
idCVar r_useDeferredTangents("r_useDeferredTangents", "1", CVAR_RENDERER | CVAR_BOOL, "defer tangents calculations after deform");

/*
=================
R_InitCvarsDmap
=================
*/
void R_InitCvarsDmap()
{
	// update latched cvars here
}

/*
=================
R_InitCommandsDmap
=================
*/
void R_InitCommandsDmap()
{

}

/*
=================
R_InitMaterialsDmap
=================
*/
void R_InitMaterialsDmap()
{
	dmap_tr.defaultMaterial = declManager->FindMaterial("_default", false);
	if (!dmap_tr.defaultMaterial)
	{
		common->FatalError("_default material not found");
	}
	dmap_tr.defaultPointLight = declManager->FindMaterial("lights/defaultPointLight");
	dmap_tr.defaultProjectedLight = declManager->FindMaterial("lights/defaultProjectedLight");
	dmap_tr.whiteMaterial = declManager->FindMaterial("_white");
	dmap_tr.charSetMaterial = declManager->FindMaterial("textures/bigchars");
}

/*
=============
R_MakeFullScreenTrisDmap
=============
*/
static srfDmapTriangles_t* R_MakeFullScreenTrisDmap()
{
	// copy verts and indexes
	srfDmapTriangles_t* tri = (srfDmapTriangles_t*)Mem_ClearedAlloc(sizeof(*tri), TAG_RENDER_TOOLS);

	tri->numIndexes = 6;
	tri->numVerts = 4;

	int indexSize = tri->numIndexes * sizeof(tri->indexes[0]);
	int allocatedIndexBytes = ALIGN(indexSize, 16);
	tri->indexes = (uint*)Mem_Alloc(allocatedIndexBytes, TAG_RENDER_TOOLS);

	int vertexSize = tri->numVerts * sizeof(tri->verts[0]);
	int allocatedVertexBytes = ALIGN(vertexSize, 16);
	tri->verts = (idDmapDrawVert*)Mem_ClearedAlloc(allocatedVertexBytes, TAG_RENDER_TOOLS);

	idDmapDrawVert* verts = tri->verts;

	uint tempIndexes[6] = { 3, 0, 2, 2, 0, 1 };
	memcpy(tri->indexes, tempIndexes, indexSize);

	verts[0].xyz[0] = -1.0f;
	verts[0].xyz[1] = 1.0f;
	verts[0].st[0] = 0.0f;
	verts[0].st[1] = 1.0f;

	verts[1].xyz[0] = 1.0f;
	verts[1].xyz[1] = 1.0f;
	verts[1].st[0] = 1.0f;
	verts[1].st[1] = 1.0f;

	verts[2].xyz[0] = 1.0f;
	verts[2].xyz[1] = -1.0f;
	verts[2].st[0] = 1.0f;
	verts[2].st[1] = 0.0f;

	verts[3].xyz[0] = -1.0f;
	verts[3].xyz[1] = -1.0f;
	verts[3].st[0] = 0.0f;
	verts[3].st[1] = 0.0f;

	for (int i = 0; i < 4; i++)
	{
		verts[i].SetColor(0xffffffff);
	}


	return tri;
}


/*
=============
R_MakeZeroOneCubeTrisDmap
=============
*/
static srfDmapTriangles_t* R_MakeZeroOneCubeTrisDmap()
{
	srfDmapTriangles_t* tri = (srfDmapTriangles_t*)Mem_ClearedAlloc(sizeof(*tri), TAG_RENDER_TOOLS);

	tri->numVerts = 8;
	tri->numIndexes = 36;

	const int indexSize = tri->numIndexes * sizeof(tri->indexes[0]);
	const int allocatedIndexBytes = ALIGN(indexSize, 16);
	tri->indexes = (uint*)Mem_Alloc(allocatedIndexBytes, TAG_RENDER_TOOLS);

	const int vertexSize = tri->numVerts * sizeof(tri->verts[0]);
	const int allocatedVertexBytes = ALIGN(vertexSize, 16);
	tri->verts = (idDmapDrawVert*)Mem_ClearedAlloc(allocatedVertexBytes, TAG_RENDER_TOOLS);

	idDmapDrawVert* verts = tri->verts;

	const float low = 0.0f;
	const float high = 1.0f;

	idVec3 center(0.0f);
	idVec3 mx(low, 0.0f, 0.0f);
	idVec3 px(high, 0.0f, 0.0f);
	idVec3 my(0.0f, low, 0.0f);
	idVec3 py(0.0f, high, 0.0f);
	idVec3 mz(0.0f, 0.0f, low);
	idVec3 pz(0.0f, 0.0f, high);

	verts[0].xyz = center + mx + my + mz;
	verts[1].xyz = center + px + my + mz;
	verts[2].xyz = center + px + py + mz;
	verts[3].xyz = center + mx + py + mz;
	verts[4].xyz = center + mx + my + pz;
	verts[5].xyz = center + px + my + pz;
	verts[6].xyz = center + px + py + pz;
	verts[7].xyz = center + mx + py + pz;

	// bottom
	tri->indexes[0 * 3 + 0] = 2;
	tri->indexes[0 * 3 + 1] = 3;
	tri->indexes[0 * 3 + 2] = 0;
	tri->indexes[1 * 3 + 0] = 1;
	tri->indexes[1 * 3 + 1] = 2;
	tri->indexes[1 * 3 + 2] = 0;
	// back
	tri->indexes[2 * 3 + 0] = 5;
	tri->indexes[2 * 3 + 1] = 1;
	tri->indexes[2 * 3 + 2] = 0;
	tri->indexes[3 * 3 + 0] = 4;
	tri->indexes[3 * 3 + 1] = 5;
	tri->indexes[3 * 3 + 2] = 0;
	// left
	tri->indexes[4 * 3 + 0] = 7;
	tri->indexes[4 * 3 + 1] = 4;
	tri->indexes[4 * 3 + 2] = 0;
	tri->indexes[5 * 3 + 0] = 3;
	tri->indexes[5 * 3 + 1] = 7;
	tri->indexes[5 * 3 + 2] = 0;
	// right
	tri->indexes[6 * 3 + 0] = 1;
	tri->indexes[6 * 3 + 1] = 5;
	tri->indexes[6 * 3 + 2] = 6;
	tri->indexes[7 * 3 + 0] = 2;
	tri->indexes[7 * 3 + 1] = 1;
	tri->indexes[7 * 3 + 2] = 6;
	// front
	tri->indexes[8 * 3 + 0] = 3;
	tri->indexes[8 * 3 + 1] = 2;
	tri->indexes[8 * 3 + 2] = 6;
	tri->indexes[9 * 3 + 0] = 7;
	tri->indexes[9 * 3 + 1] = 3;
	tri->indexes[9 * 3 + 2] = 6;
	// top
	tri->indexes[10 * 3 + 0] = 4;
	tri->indexes[10 * 3 + 1] = 7;
	tri->indexes[10 * 3 + 2] = 6;
	tri->indexes[11 * 3 + 0] = 5;
	tri->indexes[11 * 3 + 1] = 4;
	tri->indexes[11 * 3 + 2] = 6;

	for (int i = 0; i < 4; i++)
	{
		verts[i].SetColor(0xffffffff);
	}

	return tri;
}


/*
================
R_MakeTestImageTrianglesDmap

Initializes the Test Image Triangles
================
*/
srfDmapTriangles_t* R_MakeTestImageTrianglesDmap()
{
	srfDmapTriangles_t* tri = (srfDmapTriangles_t*)Mem_ClearedAlloc(sizeof(*tri), TAG_RENDER_TOOLS);

	tri->numIndexes = 6;
	tri->numVerts = 4;

	int indexSize = tri->numIndexes * sizeof(tri->indexes[0]);
	int allocatedIndexBytes = ALIGN(indexSize, 16);
	tri->indexes = (uint*)Mem_Alloc(allocatedIndexBytes, TAG_RENDER_TOOLS);

	int vertexSize = tri->numVerts * sizeof(tri->verts[0]);
	int allocatedVertexBytes = ALIGN(vertexSize, 16);
	tri->verts = (idDmapDrawVert*)Mem_ClearedAlloc(allocatedVertexBytes, TAG_RENDER_TOOLS);

	ALIGNTYPE16 uint tempIndexes[6] = { 3, 0, 2, 2, 0, 1 };
	memcpy(tri->indexes, tempIndexes, indexSize);

	idDmapDrawVert* tempVerts = tri->verts;
	tempVerts[0].xyz[0] = 0.0f;
	tempVerts[0].xyz[1] = 0.0f;
	tempVerts[0].xyz[2] = 0;
	tempVerts[0].st[0] = 0.0f;
	tempVerts[0].st[1] = 0.0f;

	tempVerts[1].xyz[0] = 1.0f;
	tempVerts[1].xyz[1] = 0.0f;
	tempVerts[1].xyz[2] = 0;
	tempVerts[1].st[0] = 1.0f;
	tempVerts[1].st[1] = 0.0f;

	tempVerts[2].xyz[0] = 1.0f;
	tempVerts[2].xyz[1] = 1.0f;
	tempVerts[2].xyz[2] = 0;
	tempVerts[2].st[0] = 1.0f;
	tempVerts[2].st[1] = 1.0f;

	tempVerts[3].xyz[0] = 0.0f;
	tempVerts[3].xyz[1] = 1.0f;
	tempVerts[3].xyz[2] = 0;
	tempVerts[3].st[0] = 0.0f;
	tempVerts[3].st[1] = 1.0f;

	for (int i = 0; i < 4; i++)
	{
		tempVerts[i].SetColor(0xFFFFFFFF);
	}
	return tri;
}

void idDmapRenderSystemLocal::OnFrame()
{
	assert(0);
}


/*
===============
idDmapRenderSystemLocal::Clear
===============
*/
void idDmapRenderSystemLocal::Clear()
{
	registered = false;
	frameCount = 0;
	viewCount = 0;
	frameShaderTime = 0.0f;
	ambientLightVector.Zero();
	worlds.Clear();
	primaryWorld = NULL;
	memset(&primaryRenderView, 0, sizeof(primaryRenderView));
	primaryView = NULL;
	defaultMaterial = NULL;
	testImage = NULL;
	ambientCubeImage = NULL;
	viewDef = NULL;
	memset(&pc, 0, sizeof(pc));
	memset(&identitySpace, 0, sizeof(identitySpace));
	memset(renderCrops, 0, sizeof(renderCrops));
	currentRenderCrop = 0;
	currentColorNativeBytesOrder = 0xFFFFFFFF;
	currentGLState = 0;
	guiRecursionLevel = 0;
	guiModel = NULL;
	memset(gammaTable, 0, sizeof(gammaTable));
	takingScreenshot = false;

	if (unitSquareTriangles != NULL)
	{
		Mem_Free(unitSquareTriangles);
		unitSquareTriangles = NULL;
	}

	if (zeroOneCubeTriangles != NULL)
	{
		Mem_Free(zeroOneCubeTriangles);
		zeroOneCubeTriangles = NULL;
	}

	if (testImageTriangles != NULL)
	{
		Mem_Free(testImageTriangles);
		testImageTriangles = NULL;
	}

	frontEndJobList = NULL;

#ifdef BUGFIXEDSCREENSHOTRESOLUTION
	// foresthale 2014-03-01: screenshots need to override the results of GetWidth() and GetHeight()
	screenshotOverrideWidth = 0;
	screenshotOverrideHeight = 0;
#endif
}

/*
===============
idDmapRenderSystemLocal::Init
===============
*/
void idDmapRenderSystemLocal::Init()
{

	common->Printf("------- Initializing Dmap RenderSystem --------\n");

	// clear all our internal state
	viewCount = 1;		// so cleared structures never match viewCount
	// we used to memset tr, but now that it is a class, we can't, so
	// there may be other state we need to reset

	ambientLightVector[0] = 0.5f;
	ambientLightVector[1] = 0.5f - 0.385f;
	ambientLightVector[2] = 0.8925f;
	ambientLightVector[3] = 1.0f;

	memset(&backEnd, 0, sizeof(backEnd));

	R_InitCvarsDmap();

	R_InitCommandsDmap();

	guiModel = new(TAG_RENDER)idGuiModel;
	guiModel->Clear();
	tr_dmapGuiModel = guiModel;	// for DeviceContext fast path

	globalImages->Init();
	globalFramebuffers->Init(); // foresthale 2014-02-18: framebuffer objects

	idCinematic::InitCinematic();

	// build brightness translation tables
	R_SetColorMappings();

	R_InitMaterialsDmap();

	dmapRenderModelManager->Init();

	// set the identity space
	identitySpace.modelMatrix[0 * 4 + 0] = 1.0f;
	identitySpace.modelMatrix[1 * 4 + 1] = 1.0f;
	identitySpace.modelMatrix[2 * 4 + 2] = 1.0f;

	// make sure the dmap_tr.unitSquareTriangles data is current in the vertex / index cache
	if (unitSquareTriangles == NULL)
	{
		unitSquareTriangles = R_MakeFullScreenTrisDmap();
	}
	// make sure the dmap_tr.zeroOneCubeTriangles data is current in the vertex / index cache
	if (zeroOneCubeTriangles == NULL)
	{
		zeroOneCubeTriangles = R_MakeZeroOneCubeTrisDmap();
	}
	// make sure the dmap_tr.testImageTriangles data is current in the vertex / index cache
	if (testImageTriangles == NULL)
	{
		testImageTriangles = R_MakeTestImageTrianglesDmap();
	}

	frontEndJobList = parallelJobManager->AllocJobList(JOBLIST_RENDERER_FRONTEND, JOBLIST_PRIORITY_MEDIUM, 2048, 0, NULL);

	// make sure the command buffers are ready to accept the first screen update
	SwapCommandBuffers(NULL, NULL, NULL, NULL);

	common->Printf("Dmap RenderSystem initialized.\n");
	common->Printf("--------------------------------------\n");
}

/*
==================
TakeScreenshot

Move to tr_imagefiles.c...

Downsample is the number of steps to mipmap the image before saving it
If ref == NULL, common->UpdateScreen will be used
==================
*/
void idDmapRenderSystemLocal::TakeScreenshot(int width, int height, const char* fileName, int blends, renderView_t* ref)
{
	assert(0);
}

void idDmapRenderSystemLocal::TakeScreenshot(int width, int height, idFile* outFile, int blends, renderView_t* ref)
{
	assert(0);
}

/*
===============
idDmapRenderSystemLocal::Shutdown
===============
*/
void idDmapRenderSystemLocal::Shutdown()
{
	common->Printf("idDmapRenderSystemLocal::Shutdown()\n");

	fonts.DeleteContents();

	if (R_IsInitialized())
	{
		globalFramebuffers->PurgeAllFramebuffers(); // foresthale 2014-02-18: framebuffer objects
		globalImages->PurgeAllImages();
	}

	dmapRenderModelManager->Shutdown();

	idCinematic::ShutdownCinematic();

	globalFramebuffers->Shutdown(); // foresthale 2014-02-18: framebuffer objects
	globalImages->Shutdown();

	// free frame memory
	R_ShutdownFrameData();

	UnbindBufferObjects();

	// free the vertex cache, which should have nothing allocated now
	dmapVertexCache.Shutdown();

	RB_ShutdownDebugTools();

	delete guiModel;

	parallelJobManager->FreeJobList(frontEndJobList);

	Clear();

	ShutdownOpenGL();
}

/*
========================
idDmapRenderSystemLocal::ResetGuiModels
========================
*/
void idDmapRenderSystemLocal::ResetGuiModels()
{
	delete guiModel;
	guiModel = new(TAG_RENDER)idGuiModel;
	guiModel->Clear();
	guiModel->BeginFrame();
	tr_dmapGuiModel = guiModel;	// for DeviceContext fast path
}

/*
========================
idDmapRenderSystemLocal::BeginLevelLoad
========================
*/
void idDmapRenderSystemLocal::BeginLevelLoad()
{
	// Re-Initialize the Default Materials if needed.
	R_InitMaterialsDmap();
	globalImages->BeginLevelLoad();
	dmapRenderModelManager->BeginLevelLoad();


}

/*
========================
idDmapRenderSystemLocal::LoadLevelImages
========================
*/
void idDmapRenderSystemLocal::LoadLevelImages()
{
	globalImages->LoadLevelImages(false);
}

/*
========================
idDmapRenderSystemLocal::Preload
========================
*/
void idDmapRenderSystemLocal::Preload(const idPreloadManifest& manifest, const char* mapName)
{
	assert(0);
	/*
	globalImages->Preload(manifest, true);
	uiManager->Preload(mapName);
	dmapRenderModelManager->Preload(manifest);
	*/
}

/*
========================
idDmapRenderSystemLocal::EndLevelLoad
========================
*/
void idDmapRenderSystemLocal::EndLevelLoad()
{
	dmapRenderModelManager->EndLevelLoad();
	globalImages->EndLevelLoad();
}

/*
========================
idDmapRenderSystemLocal::BeginAutomaticBackgroundSwaps
========================
*/
void idDmapRenderSystemLocal::BeginAutomaticBackgroundSwaps(autoRenderIconType_t icon)
{
}

/*
========================
idDmapRenderSystemLocal::EndAutomaticBackgroundSwaps
========================
*/
void idDmapRenderSystemLocal::EndAutomaticBackgroundSwaps()
{
}

/*
========================
idDmapRenderSystemLocal::AreAutomaticBackgroundSwapsRunning
========================
*/
bool idDmapRenderSystemLocal::AreAutomaticBackgroundSwapsRunning(autoRenderIconType_t* icon) const
{
	return false;
}

/*
============
idDmapRenderSystemLocal::RegisterFont
============
*/
idFont* idDmapRenderSystemLocal::RegisterFont(const char* fontName)
{

	idStrStatic< MAX_OSPATH > baseFontName = fontName;
	baseFontName.Replace("fonts/", "");
	for (int i = 0; i < fonts.Num(); i++)
	{
		if (idStr::Icmp(fonts[i]->GetName(), baseFontName) == 0)
		{
			fonts[i]->Touch();
			return fonts[i];
		}
	}
	idFont* newFont = new(TAG_FONT)idFont(baseFontName);
	fonts.Append(newFont);
	return newFont;
}

/*
========================
idDmapRenderSystemLocal::ResetFonts
========================
*/
void idDmapRenderSystemLocal::ResetFonts()
{
	fonts.DeleteContents(true);
}
/*
========================
idDmapRenderSystemLocal::InitOpenGL
========================
*/
void idDmapRenderSystemLocal::InitOpenGL()
{
	// if OpenGL isn't started, start it now
	if (!R_IsInitialized())
	{
		R_InitOpenGL();

		// Reloading images here causes the rendertargets to get deleted. Figure out how to handle this properly on 360
		globalImages->ReloadImages(true);

		int err = qglGetError();
		if (err != GL_NO_ERROR)
		{
			common->Printf("glGetError() = 0x%x\n", err);
		}
	}
}

static bool r_initialized = false;

/*
========================
idDmapRenderSystemLocal::ShutdownOpenGL
========================
*/
void idDmapRenderSystemLocal::ShutdownOpenGL()
{
	// free the context and close the window
	R_ShutdownFrameData();
	GLimp_Shutdown();
	r_initialized = false;
}

/*
========================
idDmapRenderSystemLocal::IsOpenGLRunning
========================
*/
bool idDmapRenderSystemLocal::IsOpenGLRunning() const
{
	return R_IsInitialized();
}

/*
========================
idDmapRenderSystemLocal::IsFullScreen
========================
*/
bool idDmapRenderSystemLocal::IsFullScreen() const
{
	return glConfig.isFullscreen != 0;
}

/*
========================
idDmapRenderSystemLocal::GetWidth
========================
*/
int idDmapRenderSystemLocal::GetWidth() const
{
#ifdef BUGFIXEDSCREENSHOTRESOLUTION
	// foresthale 2014-03-01: screenshots need to override the results of GetWidth() and GetHeight()
	if (screenshotOverrideWidth)
	{
		return screenshotOverrideWidth;
	}
#endif
	if (glConfig.stereo3Dmode == STEREO3D_SIDE_BY_SIDE || glConfig.stereo3Dmode == STEREO3D_SIDE_BY_SIDE_COMPRESSED)
	{
		return glConfig.nativeScreenWidth >> 1;
	}
	return glConfig.nativeScreenWidth;
}

/*
========================
idDmapRenderSystemLocal::GetHeight
========================
*/
int idDmapRenderSystemLocal::GetHeight() const
{
#ifdef BUGFIXEDSCREENSHOTRESOLUTION
	// foresthale 2014-03-01: screenshots need to override the results of GetWidth() and GetHeight()
	if (screenshotOverrideHeight)
	{
		return screenshotOverrideHeight;
	}
#endif
	if (glConfig.stereo3Dmode == STEREO3D_HDMI_720)
	{
		return 720;
	}
	extern idCVar stereoRender_warp;
	if (glConfig.stereo3Dmode == STEREO3D_SIDE_BY_SIDE && stereoRender_warp.GetBool())
	{
		// for the Rift, render a square aspect view that will be symetric for the optics
		return glConfig.nativeScreenWidth >> 1;
	}
	if (glConfig.stereo3Dmode == STEREO3D_INTERLACED || glConfig.stereo3Dmode == STEREO3D_TOP_AND_BOTTOM_COMPRESSED)
	{
		return glConfig.nativeScreenHeight >> 1;
	}
	return glConfig.nativeScreenHeight;
}

/*
========================
idDmapRenderSystemLocal::GetStereo3DMode
========================
*/
stereo3DMode_t idDmapRenderSystemLocal::GetStereo3DMode() const
{
	return glConfig.stereo3Dmode;
}

/*
========================
idDmapRenderSystemLocal::IsStereoScopicRenderingSupported
========================
*/
bool idDmapRenderSystemLocal::IsStereoScopicRenderingSupported() const
{
	return true;
}

/*
========================
idDmapRenderSystemLocal::HasQuadBufferSupport
========================
*/
bool idDmapRenderSystemLocal::HasQuadBufferSupport() const
{
	return glConfig.stereoPixelFormatAvailable;
}

/*
========================
idDmapRenderSystemLocal::GetStereoScopicRenderingMode
========================
*/
stereo3DMode_t idDmapRenderSystemLocal::GetStereoScopicRenderingMode() const
{
	return STEREO3D_OFF;
}

/*
========================
idDmapRenderSystemLocal::IsStereoScopicRenderingSupported
========================
*/
void idDmapRenderSystemLocal::EnableStereoScopicRendering(const stereo3DMode_t mode) const
{
	//stereoRender_enable.SetInteger(mode);
}

/*
========================
idDmapRenderSystemLocal::GetPixelAspect
========================
*/
float idDmapRenderSystemLocal::GetPixelAspect() const
{
	switch (glConfig.stereo3Dmode)
	{
	case STEREO3D_SIDE_BY_SIDE_COMPRESSED:
		return glConfig.pixelAspect * 2.0f;
	case STEREO3D_TOP_AND_BOTTOM_COMPRESSED:
	case STEREO3D_INTERLACED:
		return glConfig.pixelAspect * 0.5f;
	default:
		return glConfig.pixelAspect;
	}
}

/*
========================
idDmapRenderSystemLocal::GetPhysicalScreenWidthInCentimeters

This is used to calculate stereoscopic screen offset for a given interocular distance.
========================
*/
float idDmapRenderSystemLocal::GetPhysicalScreenWidthInCentimeters() const
{
	return glConfig.physicalScreenWidthInCentimeters;
}
