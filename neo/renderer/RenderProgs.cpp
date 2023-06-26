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

#include "tr_local.h"



idRenderProgManager renderProgManager;

/*
================================================================================================
idRenderProgManager::idRenderProgManager()
================================================================================================
*/
idRenderProgManager::idRenderProgManager()
{
}

/*
================================================================================================
idRenderProgManager::~idRenderProgManager()
================================================================================================
*/
idRenderProgManager::~idRenderProgManager()
{
}

/*
================================================================================================
R_ReloadShaders
================================================================================================
*/
static void R_ReloadShaders( const idCmdArgs& args )
{
	renderProgManager.KillAllShaders();
	renderProgManager.LoadAllShaders();
}

/*
================================================================================================
idRenderProgManager::Init()
================================================================================================
*/
void idRenderProgManager::Init()
{
	common->Printf( "----- Initializing Render Shaders -----\n" );
	
	
	for( int i = 0; i < MAX_BUILTINS; i++ )
	{
		builtinShaders[i] = -1;
	}
	struct builtinShaders_t
	{
		int index;
		const char* name;
		const char* nameOutSuffix;
		uint32		shaderFeatures;
		bool		requireGPUSkinningSupport;
	} builtins[] =
	{
		{ BUILTIN_GUI, "gui.vfp", "", 0, false},
		{ BUILTIN_COLOR, "color.vfp" },
		{ BUILTIN_SIMPLESHADE, "simpleshade.vfp" },
		{ BUILTIN_TEXTURED, "texture.vfp" },
		{ BUILTIN_TEXTURE_VERTEXCOLOR, "texture_color.vfp" },
		{ BUILTIN_TEXTURE_VERTEXCOLOR_SKINNED, "texture_color_skinned.vfp" },
		{ BUILTIN_TEXTURE_TEXGEN_VERTEXCOLOR, "texture_color_texgen.vfp" },
		{ BUILTIN_INTERACTION, "interaction.vfp" },
		{ BUILTIN_INTERACTION_SKINNED, "interaction_skinned.vfp" },
		{ BUILTIN_INTERACTION_AMBIENT, "interactionAmbient.vfp" },
		{ BUILTIN_INTERACTION_AMBIENT_SKINNED, "interactionAmbient_skinned.vfp" },
		{ BUILTIN_ENVIRONMENT, "environment.vfp" },
		{ BUILTIN_ENVIRONMENT_SKINNED, "environment_skinned.vfp" },
		{ BUILTIN_BUMPY_ENVIRONMENT, "bumpyEnvironment.vfp" },
		{ BUILTIN_BUMPY_ENVIRONMENT_SKINNED, "bumpyEnvironment_skinned.vfp" },
		
		{ BUILTIN_DEPTH, "depth.vfp" },
		{ BUILTIN_DEPTH_SKINNED, "depth_skinned.vfp" },
		{ BUILTIN_SHADOW_DEBUG, "shadowDebug.vfp" },
		{ BUILTIN_SHADOW_DEBUG_SKINNED, "shadowDebug_skinned.vfp" },
		
		{ BUILTIN_BLENDLIGHT, "blendlight.vfp" },
		{ BUILTIN_FOG, "fog.vfp" },
		{ BUILTIN_FOG_SKINNED, "fog_skinned.vfp" },
		{ BUILTIN_SKYBOX, "skybox.vfp" },
		{ BUILTIN_WOBBLESKY, "wobblesky.vfp" },
		{ BUILTIN_POSTPROCESS, "postprocess.vfp" },
		{ BUILTIN_STEREO_DEGHOST, "stereoDeGhost.vfp" },
		{ BUILTIN_STEREO_WARP, "stereoWarp.vfp" },
		{ BUILTIN_ZCULL_RECONSTRUCT, "zcullReconstruct.vfp" },
		{ BUILTIN_BINK, "bink.vfp" },
		{ BUILTIN_BINK_GUI, "bink_gui.vfp" },
		{ BUILTIN_STEREO_INTERLACE, "stereoInterlace.vfp" },
		{ BUILTIN_MOTION_BLUR, "motionBlur.vfp" },

		// foresthale 2014-02-20: added HDRDither shader
		{ BUILTIN_HDRDITHER, "hdrDither.vfp" },
		// foresthale 2014-04-08: r_glow
		{ BUILTIN_HDRDITHERWITHGLOW, "hdrDitherWithGlow.vfp" },
		{ BUILTIN_HDRGLOWDOWNSCALE, "hdrGlowDownscale.vfp" },
		{ BUILTIN_HDRGLOWBLURDIRECTIONAL, "hdrGlowBlurDirectional.vfp" },
		{ BUILTIN_INTERACTION_SHADOW_MAPPING_SPOT, "interactionSM_spot.vfp" },
		{ BUILTIN_INTERACTION_SHADOW_MAPPING_SPOT_SKINNED, "interactionSM_spot_skinned.vfp" },
		{ BUILTIN_INTERACTION_SHADOW_MAPPING_POINT, "interactionSM_point.vfp" },
		{ BUILTIN_INTERACTION_SHADOW_MAPPING_POINT_SKINNED, "interactionSM_point_skinned.vfp" },
		{ BUILTIN_INTERACTION_SHADOW_MAPPING_PARALLEL, "interactionSM_parallel.vfp" },
		{ BUILTIN_INTERACTION_SHADOW_MAPPING_PARALLEL_SKINNED, "interactionSM_parallel_skinned.vfp" },
		{ BUILTIN_AMBIENT_OCCLUSION, "AmbientOcclusion_AO", "", 0, false },
		{ BUILTIN_AMBIENT_OCCLUSION_AND_OUTPUT, "AmbientOcclusion_AO", "_write", 0, false },
		{ BUILTIN_AMBIENT_OCCLUSION_BLUR, "AmbientOcclusion_blur", "", 0, false },
		{ BUILTIN_AMBIENT_OCCLUSION_BLUR_AND_OUTPUT, "AmbientOcclusion_blur", "_write", 0, false },
		{ BUILTIN_AMBIENT_OCCLUSION_MINIFY, "AmbientOcclusion_minify", "", 0, false },
		{ BUILTIN_AMBIENT_OCCLUSION_RECONSTRUCT_CSZ, "AmbientOcclusion_minify", "_mip0", 0, false },
	};
	int numBuiltins = sizeof( builtins ) / sizeof( builtins[0] );
	vertexShaders.SetNum( numBuiltins );
	fragmentShaders.SetNum( numBuiltins );
	glslPrograms.SetNum( numBuiltins );
	
	for( int i = 0; i < numBuiltins; i++ )
	{
		vertexShaders[i].name = builtins[i].name;
		vertexShaders[i].nameOutSuffix = builtins[i].nameOutSuffix;
		vertexShaders[i].shaderFeatures = builtins[i].shaderFeatures;
		vertexShaders[i].builtin = true;
		fragmentShaders[i].name = builtins[i].name;
		fragmentShaders[i].nameOutSuffix = builtins[i].nameOutSuffix;
		fragmentShaders[i].shaderFeatures = builtins[i].shaderFeatures;
		fragmentShaders[i].builtin = true;
		builtinShaders[builtins[i].index] = i;
		LoadVertexShader( i );
		LoadFragmentShader( i );
		LoadGLSLProgram( i, i, i );
	}
	
	// Special case handling for fastZ shaders
	builtinShaders[BUILTIN_SHADOW] = FindVertexShader( "shadow.vp" );
	builtinShaders[BUILTIN_SHADOW_SKINNED] = FindVertexShader( "shadow_skinned.vp" );
	
	FindGLSLProgram( "shadow.vp", builtinShaders[BUILTIN_SHADOW], -1 );
	FindGLSLProgram( "shadow_skinned.vp", builtinShaders[BUILTIN_SHADOW_SKINNED], -1 );
	
	glslUniforms.SetNum( RENDERPARM_USER + MAX_GLSL_USER_PARMS, vec4_zero );
	
	// motorsep 10-26-2014; any new skinned built-in shader has to have .usesJoints set to true in the section below; 
	// non-built-in skinned shaders are taken care of in idRenderProgManager::FindVertexShader
	vertexShaders[builtinShaders[BUILTIN_TEXTURE_VERTEXCOLOR_SKINNED]].usesJoints = true;
	vertexShaders[builtinShaders[BUILTIN_INTERACTION_SKINNED]].usesJoints = true;
	vertexShaders[builtinShaders[BUILTIN_INTERACTION_AMBIENT_SKINNED]].usesJoints = true;
	vertexShaders[builtinShaders[BUILTIN_ENVIRONMENT_SKINNED]].usesJoints = true;
	vertexShaders[builtinShaders[BUILTIN_BUMPY_ENVIRONMENT_SKINNED]].usesJoints = true;
	vertexShaders[builtinShaders[BUILTIN_DEPTH_SKINNED]].usesJoints = true;
	vertexShaders[builtinShaders[BUILTIN_SHADOW_SKINNED]].usesJoints = true;
	vertexShaders[builtinShaders[BUILTIN_SHADOW_DEBUG_SKINNED]].usesJoints = true;
	vertexShaders[builtinShaders[BUILTIN_FOG_SKINNED]].usesJoints = true;
	vertexShaders[builtinShaders[BUILTIN_INTERACTION_SHADOW_MAPPING_SPOT_SKINNED]].usesJoints = true;
	vertexShaders[builtinShaders[BUILTIN_INTERACTION_SHADOW_MAPPING_POINT_SKINNED]].usesJoints = true;
	vertexShaders[builtinShaders[BUILTIN_INTERACTION_SHADOW_MAPPING_PARALLEL_SKINNED]].usesJoints = true;
	
	cmdSystem->AddCommand( "reloadShaders", R_ReloadShaders, CMD_FL_RENDERER, "reloads shaders" );
}

/*
================================================================================================
idRenderProgManager::LoadAllShaders()
================================================================================================
*/
void idRenderProgManager::LoadAllShaders()
{
	for( int i = 0; i < vertexShaders.Num(); i++ )
	{
		LoadVertexShader( i );
	}
	for( int i = 0; i < fragmentShaders.Num(); i++ )
	{
		LoadFragmentShader( i );
	}
	
	for( int i = 0; i < glslPrograms.Num(); ++i )
	{
		LoadGLSLProgram( i, glslPrograms[i].vertexShaderIndex, glslPrograms[i].fragmentShaderIndex );
	}
}

/*
================================================================================================
idRenderProgManager::KillAllShaders()
================================================================================================
*/
void idRenderProgManager::KillAllShaders()
{
	Unbind();
	for( int i = 0; i < vertexShaders.Num(); i++ )
	{
		if( vertexShaders[i].progId != INVALID_PROGID )
		{
			qglDeleteShader( vertexShaders[i].progId );
			vertexShaders[i].progId = INVALID_PROGID;
		}
	}
	for( int i = 0; i < fragmentShaders.Num(); i++ )
	{
		if( fragmentShaders[i].progId != INVALID_PROGID )
		{
			qglDeleteShader( fragmentShaders[i].progId );
			fragmentShaders[i].progId = INVALID_PROGID;
		}
	}
	for( int i = 0; i < glslPrograms.Num(); ++i )
	{
		if( glslPrograms[i].progId != INVALID_PROGID )
		{
			qglDeleteProgram( glslPrograms[i].progId );
			glslPrograms[i].progId = INVALID_PROGID;
		}
	}
}

/*
================================================================================================
idRenderProgManager::Shutdown()
================================================================================================
*/
void idRenderProgManager::Shutdown()
{
	KillAllShaders();
}

/*
================================================================================================
idRenderProgManager::FindVertexShader
================================================================================================
*/
int idRenderProgManager::FindVertexShader( const char* name )
{
	for( int i = 0; i < vertexShaders.Num(); i++ )
	{
		if( vertexShaders[i].name.Icmp( name ) == 0 )
		{
			LoadVertexShader( i );
			return i;
		}
	}
	vertexShader_t shader;
	shader.name = name;
	int index = vertexShaders.Append( shader );
	LoadVertexShader( index );
	currentVertexShader = index;
	
	// FIXME: we should really scan the program source code for using rpEnableSkinning but at this
	// point we directly load a binary and the program source code is not available on the consoles
	if(	idStr::Icmp( name, "heatHaze.vfp" ) == 0 ||
			idStr::Icmp( name, "heatHazeWithMask.vfp" ) == 0 ||
			idStr::Icmp( name, "heatHazeWithMaskAndVertex.vfp" ) == 0 )
	{
		vertexShaders[index].usesJoints = true;
		vertexShaders[index].optionalSkinning = true;
	}
	
	// Steel Storm 2 fix for inkEffect on animated skeletal models
	// begins
	if( idStr::FindText( name, "_skinned.glsl" ) > -1 ) 
	{
        vertexShaders[index].usesJoints = true;
        vertexShaders[index].optionalSkinning = true;
    }
	// ends

	return index;
}

/*
================================================================================================
idRenderProgManager::FindFragmentShader
================================================================================================
*/
int idRenderProgManager::FindFragmentShader( const char* name )
{
	for( int i = 0; i < fragmentShaders.Num(); i++ )
	{
		if( fragmentShaders[i].name.Icmp( name ) == 0 )
		{
			LoadFragmentShader( i );
			return i;
		}
	}
	fragmentShader_t shader;
	shader.name = name;
	int index = fragmentShaders.Append( shader );
	LoadFragmentShader( index );
	currentFragmentShader = index;
	return index;
}




/*
================================================================================================
idRenderProgManager::LoadVertexShader
================================================================================================
*/
void idRenderProgManager::LoadVertexShader( int index )
{
	if( vertexShaders[index].progId != INVALID_PROGID )
	{
		return; // Already loaded
	}
	//vertexShaders[index].progId = ( GLuint ) LoadGLSLShader( GL_VERTEX_SHADER, vertexShaders[index].name, vertexShaders[index].uniforms );
	vertexShader_t& vs = vertexShaders[index];
	vertexShaders[index].progId = (GLuint)LoadGLSLShader(GL_VERTEX_SHADER, vs.name, vs.nameOutSuffix, vs.shaderFeatures, vs.builtin, vs.uniforms);
}

/*
================================================================================================
idRenderProgManager::LoadFragmentShader
================================================================================================
*/
void idRenderProgManager::LoadFragmentShader( int index )
{
	if( fragmentShaders[index].progId != INVALID_PROGID )
	{
		return; // Already loaded
	}
	//fragmentShaders[index].progId = ( GLuint ) LoadGLSLShader( GL_FRAGMENT_SHADER, fragmentShaders[index].name, fragmentShaders[index].uniforms );
	fragmentShader_t& fs = fragmentShaders[index];
	fragmentShaders[index].progId = (GLuint)LoadGLSLShader(GL_FRAGMENT_SHADER, fs.name, fs.nameOutSuffix, fs.shaderFeatures, fs.builtin, fs.uniforms);
}

/*
================================================================================================
idRenderProgManager::BindShader
================================================================================================
*/
/*void idRenderProgManager::BindShader(int vIndex, int fIndex)
{
	if( currentVertexShader == vIndex && currentFragmentShader == fIndex )
	{
		return;
	}
	currentVertexShader = vIndex;
	currentFragmentShader = fIndex;
	// vIndex denotes the GLSL program
	if( vIndex >= 0 && vIndex < glslPrograms.Num() )
	{
		currentRenderProgram = vIndex;
		RENDERLOG_PRINTF( "Binding GLSL Program %s\n", glslPrograms[vIndex].name.c_str() );
		qglUseProgram( glslPrograms[vIndex].progId );
	}
} */
void idRenderProgManager::BindShader(int progIndex, int vIndex, int fIndex, bool builtin)
{
	if (currentVertexShader == vIndex && currentFragmentShader == fIndex)
	{
		return;
	}

	if (builtin)
	{
		currentVertexShader = vIndex;
		currentFragmentShader = fIndex;

		// vIndex denotes the GLSL program
		if (vIndex >= 0 && vIndex < glslPrograms.Num())
		{
			currentRenderProgram = vIndex;
			RENDERLOG_PRINTF("Binding GLSL Program %s\n", glslPrograms[vIndex].name.c_str());
			qglUseProgram(glslPrograms[vIndex].progId);
		}
	}
	else
	{
		if (progIndex == -1)
		{
			// RB: FIXME linear search
			for (int i = 0; i < glslPrograms.Num(); ++i)
			{
				if ((glslPrograms[i].vertexShaderIndex == vIndex) && (glslPrograms[i].fragmentShaderIndex == fIndex))
				{
					progIndex = i;
					break;
				}
			}
		}

		currentVertexShader = vIndex;
		currentFragmentShader = fIndex;

		if (progIndex >= 0 && progIndex < glslPrograms.Num())
		{
			currentRenderProgram = progIndex;
			RENDERLOG_PRINTF("Binding GLSL Program %s\n", glslPrograms[progIndex].name.c_str());
			qglUseProgram(glslPrograms[progIndex].progId);
		}
	}
}

/*
================================================================================================
idRenderProgManager::Unbind
================================================================================================
*/
void idRenderProgManager::Unbind()
{
	currentVertexShader = -1;
	currentFragmentShader = -1;
	
	qglUseProgram( 0 );
}

bool idRenderProgManager::IsShaderBound() const
{
	return (currentVertexShader != -1);
}

/*
================================================================================================
idRenderProgManager::SetRenderParms
================================================================================================
*/
void idRenderProgManager::SetRenderParms( renderParm_t rp, const float* value, int num )
{
	for( int i = 0; i < num; i++ )
	{
		SetRenderParm( ( renderParm_t )( rp + i ), value + ( i * 4 ) );
	}
}

/*
================================================================================================
idRenderProgManager::SetRenderParm
================================================================================================
*/
void idRenderProgManager::SetRenderParm( renderParm_t rp, const float* value )
{
	SetUniformValue( rp, value );
}

