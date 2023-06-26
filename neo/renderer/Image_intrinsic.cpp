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

#define	DEFAULT_SIZE	16

// motorsep 07-18-2015; r_shadowMapImageSize cvar does nothing; resolution is tied to r_shadowMapLodBias cvar (and vLight->shadowLOD respectively). At 0 is uses first parameter from the array, 
// and at 1 it uses second. This issue needs to be resolved!

//int shadowMapResolutions[MAX_SHADOWMAP_RESOLUTIONS] = { 2048, 1024, 512, 512, 256 };
int shadowMapResolutions[MAX_SHADOWMAP_RESOLUTIONS] = { 2048, 2048, 128, 128, 128 };

/*
==================
idImage::MakeDefault

the default image will be grey with a white box outline
to allow you to see the mapping coordinates on a surface
==================
*/
void idImage::MakeDefault()
{
	int		x, y;
	byte	data[DEFAULT_SIZE][DEFAULT_SIZE][4];
	
	if( com_developer.GetBool() )
	{
		// grey center
		for( y = 0 ; y < DEFAULT_SIZE ; y++ )
		{
			for( x = 0 ; x < DEFAULT_SIZE ; x++ )
			{
				data[y][x][0] = 32;
				data[y][x][1] = 32;
				data[y][x][2] = 32;
				data[y][x][3] = 255;
			}
		}
		
		// white border
		for( x = 0 ; x < DEFAULT_SIZE ; x++ )
		{
			data[0][x][0] =
				data[0][x][1] =
					data[0][x][2] =
						data[0][x][3] = 255;
						
			data[x][0][0] =
				data[x][0][1] =
					data[x][0][2] =
						data[x][0][3] = 255;
						
			data[DEFAULT_SIZE - 1][x][0] =
				data[DEFAULT_SIZE - 1][x][1] =
					data[DEFAULT_SIZE - 1][x][2] =
						data[DEFAULT_SIZE - 1][x][3] = 255;
						
			data[x][DEFAULT_SIZE - 1][0] =
				data[x][DEFAULT_SIZE - 1][1] =
					data[x][DEFAULT_SIZE - 1][2] =
						data[x][DEFAULT_SIZE - 1][3] = 255;
		}
	}
	else
	{
		for( y = 0 ; y < DEFAULT_SIZE ; y++ )
		{
			for( x = 0 ; x < DEFAULT_SIZE ; x++ )
			{
				data[y][x][0] = 0;
				data[y][x][1] = 0;
				data[y][x][2] = 0;
				data[y][x][3] = 0;
			}
		}
	}
	
	GenerateImage( ( byte* )data,
				   DEFAULT_SIZE, DEFAULT_SIZE,
				   TF_DEFAULT, TR_REPEAT, TD_DEFAULT );
				   
	defaulted = true;
}

static void R_DefaultImage( idImage* image )
{
	image->MakeDefault();
}

static void R_WhiteImage( idImage* image )
{
	byte	data[DEFAULT_SIZE][DEFAULT_SIZE][4];
	
	// solid white texture
	memset( data, 255, sizeof( data ) );
	image->GenerateImage( ( byte* )data, DEFAULT_SIZE, DEFAULT_SIZE,
						  TF_DEFAULT, TR_REPEAT, TD_DEFAULT );
}

static void R_BlackImage( idImage* image )
{
	byte	data[DEFAULT_SIZE][DEFAULT_SIZE][4];
	
	// solid black texture
	memset( data, 0, sizeof( data ) );
	image->GenerateImage( ( byte* )data, DEFAULT_SIZE, DEFAULT_SIZE,
						  TF_DEFAULT, TR_REPEAT, TD_DEFAULT );
}

static void R_GlossImage( idImage* image )
{
	byte	data[DEFAULT_SIZE][DEFAULT_SIZE][4];
	
	// default 120 specular power texture (this gets multiplied by 32.0, so this is roughly pow(n,15) exponent)
	// TD_GLOSS uses FMT_INT8 so we don't care about the green, blue or alpha bytes, just red
	memset( data, 120, sizeof( data ) );
	image->GenerateImage( ( byte* )data, DEFAULT_SIZE, DEFAULT_SIZE,
						  TF_DEFAULT, TR_REPEAT, TD_GLOSS );
}

static void R_RGBA8Image( idImage* image )
{
	byte	data[DEFAULT_SIZE][DEFAULT_SIZE][4];
	
	memset( data, 0, sizeof( data ) );
	data[0][0][0] = 16;
	data[0][0][1] = 32;
	data[0][0][2] = 48;
	data[0][0][3] = 96;
	
	image->GenerateImage( ( byte* )data, DEFAULT_SIZE, DEFAULT_SIZE, TF_DEFAULT, TR_REPEAT, TD_LOOKUP_TABLE_RGBA );
}

// foresthale 2014-02-20: renamed from R_DepthImage to R_DepthImageNearest to imply TF_NEAREST
static void R_DepthImageNearest( idImage* image )
{
	byte	data[DEFAULT_SIZE][DEFAULT_SIZE][4];
	
	memset( data, 0, sizeof( data ) );
	data[0][0][0] = 16;
	data[0][0][1] = 32;
	data[0][0][2] = 48;
	data[0][0][3] = 96;
	
	image->GenerateImage( ( byte* )data, DEFAULT_SIZE, DEFAULT_SIZE, TF_NEAREST, TR_CLAMP, TD_DEPTH );
}

// foresthale 2014-02-20: modified version of R_RGBA8Image that uses TF_LINEAR for view renders
static void R_RGBA8ImageLinear( idImage* image )
{
	byte	data[DEFAULT_SIZE][DEFAULT_SIZE][4];
	
	memset( data, 0, sizeof( data ) );
	data[0][0][0] = 16;
	data[0][0][1] = 32;
	data[0][0][2] = 48;
	data[0][0][3] = 96;
	
	image->GenerateImage( ( byte* )data, DEFAULT_SIZE, DEFAULT_SIZE, TF_LINEAR, TR_CLAMP, TD_LOOKUP_TABLE_RGBA );
}

// foresthale 2014-02-19: added TD_RGBA16F for HDR view rendering
static void R_RGBA16FImageLinear( idImage* image )
{
	byte	data[DEFAULT_SIZE][DEFAULT_SIZE][8];
	
	memset( data, 0, sizeof( data ) );
	
	image->GenerateImage( ( byte* )data, DEFAULT_SIZE, DEFAULT_SIZE, TF_LINEAR, TR_CLAMP, TD_RGBA16F );
}

// foresthale 2014-02-19: added TD_DEPTHSTENCIL
static void R_DepthStencilImageNearest( idImage* image )
{
	byte	data[DEFAULT_SIZE][DEFAULT_SIZE][4];
	
	memset( data, 0, sizeof( data ) );
	data[0][0][0] = 0;
	data[0][0][1] = 0;
	data[0][0][2] = 0;
	data[0][0][3] = 0;
	
	image->GenerateImage( ( byte* )data, DEFAULT_SIZE, DEFAULT_SIZE, TF_NEAREST, TR_CLAMP, TD_DEPTHSTENCIL );
}

// motorsep 04-23-2023; SSAO from RBDoom 3 1.1.0 preview 3
static void R_SMAAImage_ResNative(idImage* image)
{
	byte	data[DEFAULT_SIZE][DEFAULT_SIZE][8];
	image->GenerateImage((byte*)data, DEFAULT_SIZE, DEFAULT_SIZE, TF_LINEAR, TR_CLAMP, TD_RGBA16F);

	//image->GenerateImage(NULL, glConfig.nativeScreenWidth, glConfig.nativeScreenHeight, TF_LINEAR, TR_CLAMP, TD_LOOKUP_TABLE_RGBA);
}

static void R_HierarchicalZBufferImage_ResNative(idImage* image)
{
	byte	data[DEFAULT_SIZE][DEFAULT_SIZE][8];
	image->GenerateImage((byte*)data, DEFAULT_SIZE, DEFAULT_SIZE, TF_NEAREST_MIPMAP, TR_CLAMP, TD_R32F);
}
// motorsep 04-23-2023; end

static void R_AlphaNotchImage( idImage* image )
{
	byte	data[2][4];
	
	// this is used for alpha test clip planes
	
	data[0][0] = data[0][1] = data[0][2] = 255;
	data[0][3] = 0;
	data[1][0] = data[1][1] = data[1][2] = 255;
	data[1][3] = 255;
	
	image->GenerateImage( ( byte* )data, 2, 1, TF_NEAREST, TR_CLAMP, TD_LOOKUP_TABLE_ALPHA );
}

static void R_FlatNormalImage( idImage* image )
{
	byte	data[DEFAULT_SIZE][DEFAULT_SIZE][4];
	
	// flat normal map for default bunp mapping
	for( int i = 0 ; i < 4 ; i++ )
	{
		data[0][i][0] = 128;
		data[0][i][1] = 128;
		data[0][i][2] = 255;
		data[0][i][3] = 255;
	}
	image->GenerateImage( ( byte* )data, 2, 2, TF_DEFAULT, TR_REPEAT, TD_BUMP );
}

/*
================
R_CreateNoFalloffImage

This is a solid white texture that is zero clamped.
================
*/
static void R_CreateNoFalloffImage( idImage* image )
{
	int		x, y;
	byte	data[16][FALLOFF_TEXTURE_SIZE][4];
	
	memset( data, 0, sizeof( data ) );
	for( x = 1 ; x < FALLOFF_TEXTURE_SIZE - 1 ; x++ )
	{
		for( y = 1 ; y < 15 ; y++ )
		{
			data[y][x][0] = 255;
			data[y][x][1] = 255;
			data[y][x][2] = 255;
			data[y][x][3] = 255;
		}
	}
	image->GenerateImage( ( byte* )data, FALLOFF_TEXTURE_SIZE, 16, TF_DEFAULT, TR_CLAMP_TO_ZERO, TD_LOOKUP_TABLE_MONO );
}

/*
================
R_FogImage

We calculate distance correctly in two planes, but the
third will still be projection based
================
*/
const int	FOG_SIZE = 128;

void R_FogImage( idImage* image )
{
	int		x, y;
	byte	data[FOG_SIZE][FOG_SIZE][4];
	int		b;
	
	float	step[256];
	int		i;
	float	remaining = 1.0;
	for( i = 0 ; i < 256 ; i++ )
	{
		step[i] = remaining;
		remaining *= 0.982f;
	}
	
	for( x = 0 ; x < FOG_SIZE ; x++ )
	{
		for( y = 0 ; y < FOG_SIZE ; y++ )
		{
			float	d;
			
			d = idMath::Sqrt( ( x - FOG_SIZE / 2 ) * ( x - FOG_SIZE / 2 )
							  + ( y - FOG_SIZE / 2 ) * ( y - FOG_SIZE / 2 ) );
			d /= FOG_SIZE / 2 - 1;
			
			b = ( byte )( d * 255 );
			if( b <= 0 )
			{
				b = 0;
			}
			else if( b > 255 )
			{
				b = 255;
			}
			b = ( byte )( 255 * ( 1.0 - step[b] ) );
			if( x == 0 || x == FOG_SIZE - 1 || y == 0 || y == FOG_SIZE - 1 )
			{
				b = 255;		// avoid clamping issues
			}
			data[y][x][0] =
				data[y][x][1] =
					data[y][x][2] = 255;
			data[y][x][3] = b;
		}
	}
	
	image->GenerateImage( ( byte* )data, FOG_SIZE, FOG_SIZE, TF_LINEAR, TR_CLAMP, TD_LOOKUP_TABLE_ALPHA );
}


/*
================
FogFraction

Height values below zero are inside the fog volume
================
*/
static const float	RAMP_RANGE =	8;
static const float	DEEP_RANGE =	-30;
static float	FogFraction( float viewHeight, float targetHeight )
{
	float	total = idMath::Fabs( targetHeight - viewHeight );
	
//	return targetHeight >= 0 ? 0 : 1.0;

	// only ranges that cross the ramp range are special
	if( targetHeight > 0 && viewHeight > 0 )
	{
		return 0.0;
	}
	if( targetHeight < -RAMP_RANGE && viewHeight < -RAMP_RANGE )
	{
		return 1.0;
	}
	
	float	above;
	if( targetHeight > 0 )
	{
		above = targetHeight;
	}
	else if( viewHeight > 0 )
	{
		above = viewHeight;
	}
	else
	{
		above = 0;
	}
	
	float	rampTop, rampBottom;
	
	if( viewHeight > targetHeight )
	{
		rampTop = viewHeight;
		rampBottom = targetHeight;
	}
	else
	{
		rampTop = targetHeight;
		rampBottom = viewHeight;
	}
	if( rampTop > 0 )
	{
		rampTop = 0;
	}
	if( rampBottom < -RAMP_RANGE )
	{
		rampBottom = -RAMP_RANGE;
	}
	
	float	rampSlope = 1.0 / RAMP_RANGE;
	
	if( !total )
	{
		return -viewHeight * rampSlope;
	}
	
	float ramp = ( 1.0 - ( rampTop * rampSlope + rampBottom * rampSlope ) * -0.5 ) * ( rampTop - rampBottom );
	
	float	frac = ( total - above - ramp ) / total;
	
	// after it gets moderately deep, always use full value
	float deepest = viewHeight < targetHeight ? viewHeight : targetHeight;
	
	float	deepFrac = deepest / DEEP_RANGE;
	if( deepFrac >= 1.0 )
	{
		return 1.0;
	}
	
	frac = frac * ( 1.0 - deepFrac ) + deepFrac;
	
	return frac;
}

/*
================
R_FogEnterImage

Modulate the fog alpha density based on the distance of the
start and end points to the terminator plane
================
*/
void R_FogEnterImage( idImage* image )
{
	int		x, y;
	byte	data[FOG_ENTER_SIZE][FOG_ENTER_SIZE][4];
	int		b;
	
	for( x = 0 ; x < FOG_ENTER_SIZE ; x++ )
	{
		for( y = 0 ; y < FOG_ENTER_SIZE ; y++ )
		{
			float	d;
			
			d = FogFraction( x - ( FOG_ENTER_SIZE / 2 ), y - ( FOG_ENTER_SIZE / 2 ) );
			
			b = ( byte )( d * 255 );
			if( b <= 0 )
			{
				b = 0;
			}
			else if( b > 255 )
			{
				b = 255;
			}
			data[y][x][0] =
				data[y][x][1] =
					data[y][x][2] = 255;
			data[y][x][3] = b;
		}
	}
	
	// if mipmapped, acutely viewed surfaces fade wrong
	image->GenerateImage( ( byte* )data, FOG_ENTER_SIZE, FOG_ENTER_SIZE, TF_LINEAR, TR_CLAMP, TD_LOOKUP_TABLE_ALPHA );
}


/*
================
R_QuadraticImage

================
*/
static const int	QUADRATIC_WIDTH = 32;
static const int	QUADRATIC_HEIGHT = 4;

void R_QuadraticImage( idImage* image )
{
	int		x, y;
	byte	data[QUADRATIC_HEIGHT][QUADRATIC_WIDTH][4];
	int		b;
	
	
	for( x = 0 ; x < QUADRATIC_WIDTH ; x++ )
	{
		for( y = 0 ; y < QUADRATIC_HEIGHT ; y++ )
		{
			float	d;
			
			d = x - ( QUADRATIC_WIDTH / 2 - 0.5 );
			d = idMath::Fabs( d );
			d -= 0.5;
			d /= QUADRATIC_WIDTH / 2;
			
			d = 1.0 - d;
			d = d * d;
			
			b = ( byte )( d * 255 );
			if( b <= 0 )
			{
				b = 0;
			}
			else if( b > 255 )
			{
				b = 255;
			}
			data[y][x][0] =
				data[y][x][1] =
					data[y][x][2] = b;
			data[y][x][3] = 255;
		}
	}
	
	image->GenerateImage( ( byte* )data, QUADRATIC_WIDTH, QUADRATIC_HEIGHT, TF_DEFAULT, TR_CLAMP, TD_LOOKUP_TABLE_RGB1 );
}

// RB begin
static void R_CreateShadowMapImage_Res0( idImage* image )
{
	int size = shadowMapResolutions[0];
	image->GenerateShadowArray( size, size, TF_LINEAR, TR_CLAMP_TO_ZERO_ALPHA, TD_SHADOW_ARRAY );
}

static void R_CreateShadowMapImage_Res1( idImage* image )
{
	int size = shadowMapResolutions[1];
	image->GenerateShadowArray( size, size, TF_LINEAR, TR_CLAMP_TO_ZERO_ALPHA, TD_SHADOW_ARRAY );
}

static void R_CreateShadowMapImage_Res2( idImage* image )
{
	int size = shadowMapResolutions[2];
	image->GenerateShadowArray( size, size, TF_LINEAR, TR_CLAMP_TO_ZERO_ALPHA, TD_SHADOW_ARRAY );
}

static void R_CreateShadowMapImage_Res3( idImage* image )
{
	int size = shadowMapResolutions[3];
	image->GenerateShadowArray( size, size, TF_LINEAR, TR_CLAMP_TO_ZERO_ALPHA, TD_SHADOW_ARRAY );
}

static void R_CreateShadowMapImage_Res4( idImage* image )
{
	int size = shadowMapResolutions[4];
	image->GenerateShadowArray( size, size, TF_LINEAR, TR_CLAMP_TO_ZERO_ALPHA, TD_SHADOW_ARRAY );
}

const static int JITTER_SIZE = 128;
static void R_CreateJitterImage16( idImage* image )
{
	static byte	data[JITTER_SIZE][JITTER_SIZE * 16][4];
	
	for( int i = 0 ; i < JITTER_SIZE ; i++ )
	{
		for( int s = 0 ; s < 16 ; s++ )
		{
			int sOfs = 64 * ( s & 3 );
			int tOfs = 64 * ( ( s >> 2 ) & 3 );
			
			for( int j = 0 ; j < JITTER_SIZE ; j++ )
			{
				data[i][s * JITTER_SIZE + j][0] = ( rand() & 63 ) | sOfs;
				data[i][s * JITTER_SIZE + j][1] = ( rand() & 63 ) | tOfs;
				data[i][s * JITTER_SIZE + j][2] = rand();
				data[i][s * JITTER_SIZE + j][3] = 0;
			}
		}
	}
	
	image->GenerateImage( ( byte* )data, JITTER_SIZE * 16, JITTER_SIZE, TF_NEAREST, TR_REPEAT, TD_LOOKUP_TABLE_RGBA );
}

static void R_CreateJitterImage4( idImage* image )
{
	byte	data[JITTER_SIZE][JITTER_SIZE * 4][4];
	
	for( int i = 0 ; i < JITTER_SIZE ; i++ )
	{
		for( int s = 0 ; s < 4 ; s++ )
		{
			int sOfs = 128 * ( s & 1 );
			int tOfs = 128 * ( ( s >> 1 ) & 1 );
			
			for( int j = 0 ; j < JITTER_SIZE ; j++ )
			{
				data[i][s * JITTER_SIZE + j][0] = ( rand() & 127 ) | sOfs;
				data[i][s * JITTER_SIZE + j][1] = ( rand() & 127 ) | tOfs;
				data[i][s * JITTER_SIZE + j][2] = rand();
				data[i][s * JITTER_SIZE + j][3] = 0;
			}
		}
	}
	
	image->GenerateImage( ( byte* )data, JITTER_SIZE * 4, JITTER_SIZE, TF_NEAREST, TR_REPEAT, TD_LOOKUP_TABLE_RGBA );
}

static void R_CreateJitterImage1( idImage* image )
{
	byte	data[JITTER_SIZE][JITTER_SIZE][4];
	
	for( int i = 0 ; i < JITTER_SIZE ; i++ )
	{
		for( int j = 0 ; j < JITTER_SIZE ; j++ )
		{
			data[i][j][0] = rand();
			data[i][j][1] = rand();
			data[i][j][2] = rand();
			data[i][j][3] = 0;
		}
	}
	
	image->GenerateImage( ( byte* )data, JITTER_SIZE, JITTER_SIZE, TF_NEAREST, TR_REPEAT, TD_LOOKUP_TABLE_RGBA );
}

static void R_CreateRandom256Image( idImage* image )
{
	byte	data[256][256][4];
	
	for( int i = 0 ; i < 256 ; i++ )
	{
		for( int j = 0 ; j < 256 ; j++ )
		{
			data[i][j][0] = rand();
			data[i][j][1] = rand();
			data[i][j][2] = rand();
			data[i][j][3] = rand();
		}
	}
	
	image->GenerateImage( ( byte* )data, 256, 256, TF_NEAREST, TR_REPEAT, TD_LOOKUP_TABLE_RGBA );
}
// RB end

/*
================
idImageManager::CreateIntrinsicImages
================
*/
void idImageManager::CreateIntrinsicImages()
{
	// create built in images
	defaultImage = ImageFromFunction( "_default", R_DefaultImage );
	whiteImage = ImageFromFunction( "_white", R_WhiteImage );
	blackImage = ImageFromFunction( "_black", R_BlackImage );
	glossImage = ImageFromFunction( "_gloss", R_GlossImage );
	flatNormalMap = ImageFromFunction( "_flat", R_FlatNormalImage );
	alphaNotchImage = ImageFromFunction( "_alphaNotch", R_AlphaNotchImage );
	fogImage = ImageFromFunction( "_fog", R_FogImage );
	fogEnterImage = ImageFromFunction( "_fogEnter", R_FogEnterImage );
	noFalloffImage = ImageFromFunction( "_noFalloff", R_CreateNoFalloffImage );
	ImageFromFunction( "_quadratic", R_QuadraticImage );
	
	cinematicImage = ImageFromFunction( "_cinematic", R_RGBA8Image );

	//int shadowMapSize = r_shadowMapImageSize.GetInteger();
	/* for( int i = 0; i < MAX_SHADOWMAP_RESOLUTIONS; ++i )
	{
		if(!shadowMapSize)
			shadowMapSize = 1;
		shadowMapResolutions[i] = shadowMapSize;
		shadowMapSize >>= 1;
	} */

	// RB begin
	shadowImage[0] = ImageFromFunction( va( "_shadowMapArray0_%i", shadowMapResolutions[0] ), R_CreateShadowMapImage_Res0 );
	shadowImage[1] = ImageFromFunction( va( "_shadowMapArray1_%i", shadowMapResolutions[1] ), R_CreateShadowMapImage_Res1 );
	shadowImage[2] = ImageFromFunction( va( "_shadowMapArray2_%i", shadowMapResolutions[2] ), R_CreateShadowMapImage_Res2 );
	shadowImage[3] = ImageFromFunction( va( "_shadowMapArray3_%i", shadowMapResolutions[3] ), R_CreateShadowMapImage_Res3 );
	shadowImage[4] = ImageFromFunction( va( "_shadowMapArray4_%i", shadowMapResolutions[4] ), R_CreateShadowMapImage_Res4 );

	jitterImage1 = globalImages->ImageFromFunction( "_jitter1", R_CreateJitterImage1 );
	jitterImage4 = globalImages->ImageFromFunction( "_jitter4", R_CreateJitterImage4 );
	jitterImage16 = globalImages->ImageFromFunction( "_jitter16", R_CreateJitterImage16 );

	randomImage256 = globalImages->ImageFromFunction( "_random256", R_CreateRandom256Image );
	// RB end

	// motorsep 04-23-2023; SSAO from RBDoom 3 1.1.0 preview 3
	currentNormalsImage = ImageFromFunction("_currentNormals", R_SMAAImage_ResNative);

	ambientOcclusionImage[0] = ImageFromFunction("_ao0", R_SMAAImage_ResNative);
	ambientOcclusionImage[1] = ImageFromFunction("_ao1", R_SMAAImage_ResNative);

	hierarchicalZbufferImage[0] = ImageFromFunction("_csz0", R_HierarchicalZBufferImage_ResNative);
	hierarchicalZbufferImage[1] = ImageFromFunction("_csz1", R_HierarchicalZBufferImage_ResNative);
	hierarchicalZbufferImage[2] = ImageFromFunction("_csz2", R_HierarchicalZBufferImage_ResNative);
	hierarchicalZbufferImage[3] = ImageFromFunction("_csz3", R_HierarchicalZBufferImage_ResNative);
	hierarchicalZbufferImage[4] = ImageFromFunction("_csz4", R_HierarchicalZBufferImage_ResNative);
	hierarchicalZbufferImage[5] = ImageFromFunction("_csz5", R_HierarchicalZBufferImage_ResNative);
	// motorsep 04-23-2023; end
	
	// scratchImage is used for screen wipes/doublevision etc..
	scratchImage = ImageFromFunction( "_scratch", R_RGBA8Image );
	scratchImage2 = ImageFromFunction( "_scratch2", R_RGBA8Image );
	accumImage = ImageFromFunction( "_accum", R_RGBA8Image );
	viewFramebufferRenderImage16 = ImageFromFunction( "_currentFramebufferRender", R_RGBA16FImageLinear );
//	viewFramebufferRenderImage = ImageFromFunction( "_currentFramebufferRender", R_RGBA8ImageLinear );
	viewFramebufferDepthImage = ImageFromFunction( "_currentFramebufferDepth", R_DepthStencilImageNearest );
	currentRenderImage = ImageFromFunction( "_currentRender", R_RGBA8ImageLinear );
	currentDepthImage = ImageFromFunction( "_currentDepth", R_DepthImageNearest );
	ditherImage = ImageFromFile( "textures/dither4x4.tga", TF_NEAREST, TR_REPEAT, TD_HIGHQUALITY );
	cameraImage = ImageFromFunction( "_camera", R_RGBA8ImageLinear );
	// foresthale 2014-04-08: r_glow
	glowFramebufferImage8[0] = ImageFromFunction( "_glowFramebuffer0", R_RGBA8ImageLinear );
	glowFramebufferImage8[1] = ImageFromFunction( "_glowFramebuffer1", R_RGBA8ImageLinear );
	glowFramebufferImage8[2] = ImageFromFunction( "_glowFramebuffer2", R_RGBA8ImageLinear );
	glowFramebufferImage8[3] = ImageFromFunction( "_glowFramebuffer3", R_RGBA8ImageLinear );
	glowFramebufferImage16[0] = ImageFromFunction( "_glowFramebufferHDR0", R_RGBA16FImageLinear );
	glowFramebufferImage16[1] = ImageFromFunction( "_glowFramebufferHDR1", R_RGBA16FImageLinear );
	glowFramebufferImage16[2] = ImageFromFunction( "_glowFramebufferHDR2", R_RGBA16FImageLinear );
	glowFramebufferImage16[3] = ImageFromFunction( "_glowFramebufferHDR3", R_RGBA16FImageLinear );

	// save a copy of this for material comparison, because currentRenderImage may get
	// reassigned during stereo rendering
	originalCurrentRenderImage = currentRenderImage;
	
	//loadingIconImage = ImageFromFile( "textures/loadingicon2", TF_DEFAULT, TR_CLAMP, TD_DEFAULT, CF_2D );
	//hellLoadingIconImage = ImageFromFile( "textures/loadingicon3", TF_DEFAULT, TR_CLAMP, TD_DEFAULT, CF_2D );
	loadingIconImage = ImageFromFile("textures/da_kat", TF_DEFAULT, TR_CLAMP, TD_HIGHQUALITY, CF_2D);
	hellLoadingIconImage = ImageFromFile("textures/da_kat", TF_DEFAULT, TR_CLAMP, TD_HIGHQUALITY, CF_2D);
	
	release_assert( loadingIconImage->referencedOutsideLevelLoad );
	release_assert( hellLoadingIconImage->referencedOutsideLevelLoad );
}