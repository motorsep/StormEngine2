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

#include "framework/Common_local.h"
#include "tr_local.h"

// motorsep 05-19-2015; bool to determine current colorspace; False is YCoCg, True is RGB
bool skyboxRGBswap;

extern idCVar image_highQualityCompression;

extern idCVar r_useHightQualitySky;

idCVar r_cacheToolImages( "r_cacheToolImages", "1", CVAR_BOOL, "Enable binarization and caching of editor formatted images. Disable will not load from generated or save to generated." );


// certain tools force any loaded materials to be put into low quality editor modes
textureUsage_t CheckEditorUsage( textureUsage_t usage )
{
	const int lowQualityImageTools = EDITOR_RADIANT|EDITOR_MATERIAL|EDITOR_PARTICLE;
	if( (com_editors&lowQualityImageTools)!=0 )
	{
		switch(usage)
		{
		case TD_DIFFUSE: 
			usage = TD_EDITOR_DIFFUSE;
			break;
		case TD_BUMP: 
			usage = TD_EDITOR_BUMP;
			break;
		case TD_COVERAGE: 
			usage = TD_EDITOR_COVERAGE;
			break;
		default:
			usage = TD_EDITOR_DEFAULT;
			break;
		}
	}
	return usage;
}

/*
================
BitsForFormat
================
*/
int BitsForFormat( textureFormat_t format )
{
	switch( format )
	{
		case FMT_NONE:
			return 0;
		case FMT_RGBA8:
			return 32;
		case FMT_XRGB8:
			return 32;
		case FMT_RGB565:
			return 16;
		case FMT_L8A8:
			return 16;
		case FMT_ALPHA:
			return 8;
		case FMT_LUM8:
			return 8;
		case FMT_INT8:
			return 8;
		case FMT_DXT1:
			return 4;
		case FMT_DXT5:
			return 8;
		case FMT_SHADOW_ARRAY:
			return ( 32 * 6 );
		case FMT_DEPTH:
			return 32;
		case FMT_X16:
			return 16;
		case FMT_Y16_X16:
			return 32;
		// foresthale 2014-02-19: added FMT_RGBA16F and FMT_DEPTHSTENCIL for HDR view rendering
		case FMT_RGBA16F:
			return 64;
		case FMT_DEPTHSTENCIL:
			return 32;
		default:
			assert( 0 );
			return 0;
	}
}

/*
========================
idImage::DeriveOpts
========================
*/
ID_INLINE void idImage::DeriveOpts()
{	
	if( cubeFiles != CF_2D && usage != TD_SHADOW_ARRAY ) // foresthale 2014-10-05: we want to hit the TD_SHADOW_ARRAY case below
	{		
		// motorsep 05-17-2015; setting parameters for cubemap / skybox images

		if( usage == TD_HIGHQUALITY_CUBE ) {				
			opts.colorFormat = CFM_DEFAULT;
			opts.format = FMT_RGBA8;
			skyboxRGBswap = true;
		} 
		// motorsep 05-23-2015; due to necessity of having alpha channel in the skyboxes, I decided to drop YCoCg color space and YCoCg compressing altogether;
		// This means compresses skyboxes will be of lower quality than they could have been using YCoCgDXT5 comression. However, they will support alpha channel and will allow to have same effects as
		// HQ RGBA skyboxes. Since we have option to turn on HQ skyboxes, people can always use that to get true high quality vs half-HQ with YCoCgDXT5.
		if( usage == TD_LOWQUALITY_CUBE ) {
			opts.colorFormat = CFM_DEFAULT; // CFM_YCOCG_DXT5;
			opts.format = FMT_DXT5;			
			skyboxRGBswap = false;
		}
	}

	if( opts.format == FMT_NONE )
	{
		opts.colorFormat = CFM_DEFAULT;
		opts.sRGB = false; // foresthale 2014-02-20: fixed r_useSRGB texture handling
		
		switch( usage )
		{
			case TD_COVERAGE:
				opts.format = FMT_DXT1;
				opts.colorFormat = CFM_GREEN_ALPHA;
				break;
			case TD_DEPTH:
				opts.format = FMT_DEPTH;
				break;
				
			case TD_SHADOW_ARRAY:
				opts.format = FMT_SHADOW_ARRAY;
				break;
				
			case TD_DIFFUSE:
				// TD_DIFFUSE gets only set to when its a diffuse texture for an interaction
				opts.gammaMips = true;
				opts.format = FMT_DXT5;
				opts.colorFormat = CFM_YCOCG_DXT5;				
				break;
			case TD_SPECULAR:
				opts.gammaMips = true;
				opts.sRGB = true; // foresthale 2014-02-20: fixed r_useSRGB texture handling
				opts.format = FMT_DXT5;
				opts.colorFormat = CFM_DEFAULT;
				/*opts.gammaMips = true;
				opts.sRGB = true; // foresthale 2014-02-20: fixed r_useSRGB texture handling
				opts.format = FMT_RGBA8;
				opts.colorFormat = CFM_DEFAULT;*/
				break;
			case TD_GLOSS:
				// we want a one-channel image with very precise gradations, so use FMT_INT8 rather than FMT_DXT1				
				opts.format = FMT_INT8;
				break;
			case TD_DEFAULT:
				opts.gammaMips = true;
				opts.sRGB = true; // foresthale 2014-02-20: fixed r_useSRGB texture handling
				opts.format = FMT_DXT5;
				opts.colorFormat = CFM_DEFAULT;
				break;
			case TD_BUMP:
				opts.format = FMT_DXT5;
				opts.colorFormat = CFM_NORMAL_DXT5;
				break;
			case TD_FONT:
				opts.format = FMT_DXT1;
				opts.colorFormat = CFM_GREEN_ALPHA;
				opts.numLevels = 4; // We only support 4 levels because we align to 16 in the exporter
				opts.gammaMips = true;
				break;
			case TD_LIGHT:
				//opts.format = FMT_RGB565;
				//opts.gammaMips = true;
				opts.format = FMT_RGBA8;
				opts.gammaMips = false;				
				break;
			case TD_LOOKUP_TABLE_MONO:
				opts.format = FMT_INT8;
				break;
			case TD_LOOKUP_TABLE_ALPHA:
				opts.format = FMT_ALPHA;
				break;
			case TD_LOOKUP_TABLE_RGB1:
			case TD_LOOKUP_TABLE_RGBA:
				opts.format = FMT_RGBA8;
				break;
			// foresthale 2014-05-17: added TD_EDITOR* image types (uncompressed variants of TD_DEFAULT and such, which always read .tga, and do not write .bimage)
			case TD_EDITOR_DEFAULT:
				opts.colorFormat = CFM_DEFAULT;
				opts.format = FMT_DXT5;
				opts.gammaMips = true;			
				break;
			case TD_EDITOR_DIFFUSE:
				opts.colorFormat = CFM_YCOCG_DXT5;
				opts.format = FMT_DXT5;
				opts.gammaMips = true;
				break;
			case TD_EDITOR_BUMP:
				opts.colorFormat = CFM_NORMAL_DXT5;
				opts.format = FMT_DXT5;
				opts.gammaMips = true;
				break;
			case TD_EDITOR_COVERAGE:
				opts.colorFormat = CFM_GREEN_ALPHA;
				opts.format = FMT_DXT5;
				break;
// ---> sikk - Added - High Quality Texture Depth (full RGBA)
			case TD_HIGHQUALITY:
				opts.colorFormat = CFM_DEFAULT;
				opts.format = FMT_RGBA8;
				opts.gammaMips = true;				
				break;
// <--- sikk - Added - High Quality Texture Depth (full RGBA)
			// motorsep 05-17-2015; added this for uncompressed cubemap/skybox textures
			case TD_HIGHQUALITY_CUBE:
				opts.colorFormat = CFM_DEFAULT;
				opts.format = FMT_RGBA8;
				opts.gammaMips = true;				
				break;
			case TD_LOWQUALITY_CUBE:
				opts.colorFormat = CFM_DEFAULT; // CFM_YCOCG_DXT5;
				opts.format = FMT_DXT5;
				opts.gammaMips = true;
				break;
			// foresthale 2014-02-19: added TD_RGBA16F and TD_DEPTHSTENCIL for HDR view rendering
			case TD_RGBA16F:
				opts.format = FMT_RGBA16F;
				break;
			case TD_DEPTHSTENCIL:
				opts.format = FMT_DEPTHSTENCIL;
				break;			
			default:
				assert( false );
				opts.format = FMT_RGBA8;				
		}
	}
	
	if( opts.numLevels == 0 )
	{
		opts.numLevels = 1;
		
		if( filter == TF_LINEAR || filter == TF_NEAREST )
		{
			// don't create mip maps if we aren't going to be using them
		}
		else
		{
			int	temp_width = opts.width;
			int	temp_height = opts.height;
			while( temp_width > 1 || temp_height > 1 )
			{
				temp_width >>= 1;
				temp_height >>= 1;
				if( ( opts.format == FMT_DXT1 || opts.format == FMT_DXT5 ) &&
						( ( temp_width & 0x3 ) != 0 || ( temp_height & 0x3 ) != 0 ) )
				{
					break;
				}
				opts.numLevels++;
			}
		}
	}
}

/*
========================
idImage::AllocImage
========================
*/
void idImage::AllocImage( const idImageOpts& imgOpts, textureFilter_t tf, textureRepeat_t tr )
{
	filter = tf;
	repeat = tr;
	opts = imgOpts;
	DeriveOpts();
	AllocImage();
}

/*
================
GenerateImage
================
*/
void idImage::GenerateImage( const byte* pic, int width, int height, textureFilter_t filterParm, textureRepeat_t repeatParm, textureUsage_t usageParm )
{
	PurgeImage();
	
	filter = filterParm;
	repeat = repeatParm;
	usage = usageParm;
	cubeFiles = CF_2D;
	
	opts.textureType = TT_2D;
	opts.width = width;
	opts.height = height;
	opts.numLevels = 0;
	DeriveOpts();
	
	// if we don't have a rendering context, just return after we
	// have filled in the parms.  We must have the values set, or
	// an image match from a shader before the render starts would miss
	// the generated texture
	if( !R_IsInitialized() )
	{
		return;
	}
	
	const bool toolUsage = IsToolUsage( usageParm );

	idBinaryImage im( GetName() );

	// foresthale 2014-05-30: give a nice progress display when binarizing
	commonLocal.LoadPacifierBinarizeFilename(GetName() , "generated image");
	if (opts.numLevels > 1)
		commonLocal.LoadPacifierBinarizeProgressTotal(opts.width * opts.height * 4 / 3);
	else
		commonLocal.LoadPacifierBinarizeProgressTotal(opts.width * opts.height);
	im.Load2DFromMemory( width, height, pic, opts.numLevels, opts.format, opts.colorFormat, opts.gammaMips, toolUsage );
	commonLocal.LoadPacifierBinarizeEnd();
	
	AllocImage();
	
	for( int i = 0; i < im.NumImages(); i++ )
	{
		const bimageImage_t& img = im.GetImageHeader( i );
		const byte* data = im.GetImageData( i );
		SubImageUpload( img.level, 0, 0, img.destZ, img.width, img.height, data );
	}
}

/*
====================
GenerateCubeImage

Non-square cube sides are not allowed
====================
*/
void idImage::GenerateCubeImage( const byte* pic[6], int size, textureFilter_t filterParm, textureUsage_t usageParm )
{
	PurgeImage();
	
	filter = filterParm;
	repeat = TR_CLAMP;
	usage = usageParm;
	cubeFiles = CF_NATIVE;
	
	opts.textureType = TT_CUBIC;
	opts.width = size;
	opts.height = size;
	opts.numLevels = 0;
	opts.format = FMT_DXT5;
	DeriveOpts();
	
	// if we don't have a rendering context, just return after we
	// have filled in the parms.  We must have the values set, or
	// an image match from a shader before the render starts would miss
	// the generated texture
	if( !R_IsInitialized() )
	{
		return;
	}

	const bool toolUsage = IsToolUsage( usageParm );

	idBinaryImage im( GetName() );
	// foresthale 2014-05-30: give a nice progress display when binarizing
	commonLocal.LoadPacifierBinarizeFilename( GetName(), "generated cube image" );
	if (opts.numLevels > 1)
		commonLocal.LoadPacifierBinarizeProgressTotal(opts.width * opts.width * 6 * 4 / 3);
	else
		commonLocal.LoadPacifierBinarizeProgressTotal(opts.width * opts.width * 6);
	im.LoadCubeFromMemory( size, pic, opts.numLevels, opts.format, opts.colorFormat, opts.gammaMips, toolUsage, usage );
	commonLocal.LoadPacifierBinarizeEnd();
	
	AllocImage();
	
	for( int i = 0; i < im.NumImages(); i++ )
	{
		const bimageImage_t& img = im.GetImageHeader( i );
		const byte* data = im.GetImageData( i );
		SubImageUpload( img.level, 0, 0, img.destZ, img.width, img.height, data );
	}
}

// RB begin
void idImage::GenerateShadowArray( int width, int height, textureFilter_t filterParm, textureRepeat_t repeatParm, textureUsage_t usageParm )
{
	PurgeImage();
	
	filter = filterParm;
	repeat = repeatParm;
	usage = usageParm;
	cubeFiles = CF_2D_ARRAY;
	
	opts.textureType = TT_2D_ARRAY;
	opts.width = width;
	opts.height = height;
	opts.numLevels = 0;
	DeriveOpts();
	
	// if we don't have a rendering context, just return after we
	// have filled in the parms.  We must have the values set, or
	// an image match from a shader before the render starts would miss
	// the generated texture
	if( !R_IsInitialized() )
	{
		return;
	}
	
	//idBinaryImage im( GetName() );
	//im.Load2DFromMemory( width, height, pic, opts.numLevels, opts.format, opts.colorFormat, opts.gammaMips );
	
	AllocImage();
	
	/*
	for( int i = 0; i < im.NumImages(); i++ )
	{
		const bimageImage_t& img = im.GetImageHeader( i );
		const byte* data = im.GetImageData( i );
		SubImageUpload( img.level, 0, 0, img.destZ, img.width, img.height, data );
	}
	*/
}
// RB end

/*
===============
BindAttachment
===============
*/
void idImage::BindAttachmentOnFBO(int attachmentType, int layer)
{
	if (layer == -1)
	{
	qglFramebufferTexture2D(GL_FRAMEBUFFER, attachmentType, GL_TEXTURE_2D, texnum, 0);
}
	else
	{
		assert(opts.textureType == TT_2D_ARRAY);
		qglFramebufferTextureLayer( GL_FRAMEBUFFER, attachmentType, texnum, 0, layer );
	}

}

/*
===============
GetGeneratedName

name contains GetName() upon entry
===============
*/
void idImage::GetGeneratedName( idStr& _name, const textureUsage_t& _usage, const cubeFiles_t& _cube )
{
	idStrStatic< 64 > extension;
	
	_name.ExtractFileExtension( extension );
	_name.StripFileExtension();
	
	const char * qualitySuffix = !IsToolUsage( _usage ) && image_highQualityCompression.GetBool() ? "hq" : "lq";

	_name += va( "#__%02d%02d_%s", ( int )_usage, ( int )_cube, qualitySuffix );	
	
	if( extension.Length() > 0 )
	{
		_name.SetFileExtension( extension );
	}
}


/*
===============
ActuallyLoadImage

Absolutely every image goes through this path
On exit, the idImage will have a valid OpenGL texture number that can be bound
===============
*/
void idImage::ActuallyLoadImage( bool fromBackEnd )
{

	// if we don't have a rendering context yet, just return
	if( !R_IsInitialized() )
	{
		return;
	}
	
	// this is the ONLY place generatorFunction will ever be called
	if( generatorFunction )
	{
		generatorFunction( this );
		return;
	}
	
	if( com_productionMode.GetInteger() != 0 )
	{
		sourceFileTime = FILE_NOT_FOUND_TIMESTAMP;
		if( cubeFiles != CF_2D )
		{
			opts.textureType = TT_CUBIC;
			repeat = TR_CLAMP;
		}
	}
	else
	{
		// RB begin
		if( cubeFiles == CF_2D_ARRAY )
		{
			opts.textureType = TT_2D_ARRAY;
		}
		// RB end
		else if( cubeFiles != CF_2D )
		{
			opts.textureType = TT_CUBIC;
			repeat = TR_CLAMP;
			R_LoadCubeImages( GetName(), cubeFiles, NULL, NULL, &sourceFileTime );
		}
		else
		{
			opts.textureType = TT_2D;
			R_LoadImageProgram( GetName(), NULL, NULL, NULL, &sourceFileTime, &usage );
		}
	}
	
	const bool toolUsage = IsToolUsage( usage );

	// Figure out opts.colorFormat and opts.format so we can make sure the binary image is up to date
	DeriveOpts();
	
	idStrStatic< MAX_OSPATH > generatedName = GetName();
	GetGeneratedName( generatedName, usage, cubeFiles );
	
	idBinaryImage im( generatedName );
	binaryFileTime = (!toolUsage || r_cacheToolImages.GetBool()) ? im.LoadFromGeneratedFile( sourceFileTime, toolUsage ) : FILE_NOT_FOUND_TIMESTAMP;
	
	const bool binaryFileFound = binaryFileTime != FILE_NOT_FOUND_TIMESTAMP;

	// BFHACK, do not want to tweak on buildgame so catch these images here
	/*if( !binaryFileFound && fileSystem->UsingResourceFiles() )
	{
	int c = 1;
	while( c-- > 0 )
	{
	if( generatedName.Find( "guis/assets/white#__0000", false ) >= 0 )
	{
	generatedName.Replace( "white#__0000", "white#__0200" );
	im.SetName( generatedName );
	binaryFileTime = im.LoadFromGeneratedFile( sourceFileTime, toolUsage );
	break;
	}
	if( generatedName.Find( "guis/assets/white#__0100", false ) >= 0 )
	{
	generatedName.Replace( "white#__0100", "white#__0200" );
	im.SetName( generatedName );
	binaryFileTime = im.LoadFromGeneratedFile( sourceFileTime, toolUsage );
	break;
	}
	if( generatedName.Find( "textures/black#__0100", false ) >= 0 )
	{
	generatedName.Replace( "black#__0100", "black#__0200" );
	im.SetName( generatedName );
	binaryFileTime = im.LoadFromGeneratedFile( sourceFileTime, toolUsage );
	break;
	}
	if( generatedName.Find( "textures/decals/bulletglass1_d#__0100", false ) >= 0 )
	{
	generatedName.Replace( "bulletglass1_d#__0100", "bulletglass1_d#__0200" );
	im.SetName( generatedName );
	binaryFileTime = im.LoadFromGeneratedFile( sourceFileTime, toolUsage );
	break;
	}
	if( generatedName.Find( "models/monsters/skeleton/skeleton01_d#__1000", false ) >= 0 )
	{
	generatedName.Replace( "skeleton01_d#__1000", "skeleton01_d#__0100" );
	im.SetName( generatedName );
	binaryFileTime = im.LoadFromGeneratedFile( sourceFileTime, toolUsage );
	break;
	}
	}
	}*/
	
	const bimageFile_t& header = im.GetFileHeader();

	if( ( (fileSystem->InProductionMode() || sourceFileTime<=0) && binaryFileFound )
		|| ( ( binaryFileFound )
			&& ( header.colorFormat == opts.colorFormat )
			&& ( header.format == opts.format )
			&& ( header.textureType == opts.textureType )
			&& ( !toolUsage || r_cacheToolImages.GetBool() )
			) )
	{
		opts.width = header.width;
		opts.height = header.height;
		opts.numLevels = header.numLevels;
		opts.colorFormat = ( textureColor_t )header.colorFormat;
		opts.format = ( textureFormat_t )header.format;
		opts.textureType = ( textureType_t )header.textureType;
		if( cvarSystem->GetCVarBool( "fs_buildresources" ) )
		{
			// for resource gathering write this image to the preload file for this map
			fileSystem->AddImagePreload( GetName(), filter, repeat, usage, cubeFiles );
		}
	}
	else
	{
		idStr binarizeReason = "binarize: unknown reason";
		if ( !binaryFileFound )
			binarizeReason = va( "binarize: binary file not found '%s'", generatedName.c_str() );
		else if ( header.colorFormat != opts.colorFormat )
			binarizeReason = va( "binarize: mismatch color format '%s'", generatedName.c_str() );
		else if ( header.colorFormat != opts.colorFormat )
			binarizeReason = va( "binarize: mismatched color format '%s'", generatedName.c_str() );
		else if ( header.textureType != opts.textureType )
			binarizeReason = va( "binarize: mismatched texture type '%s'", generatedName.c_str() );
		else if ( toolUsage )
			binarizeReason = va( "binarize: tool usage '%s'", generatedName.c_str() );

		if( cubeFiles != CF_2D )
		{
			int size;
			byte* pics[6];
			
			if( !R_LoadCubeImages( GetName(), cubeFiles, pics, &size, &sourceFileTime ) || size == 0 )
			{
				idLib::Warning( "Couldn't load cube image: %s", GetName() );
				return;
			}
			
			repeat = TR_CLAMP;

			opts.textureType = TT_CUBIC;
			opts.width = size;
			opts.height = size;
			opts.numLevels = 0;

			DeriveOpts();
			
			// foresthale 2014-05-30: give a nice progress display when binarizing
			commonLocal.LoadPacifierBinarizeFilename( generatedName.c_str(), binarizeReason.c_str() );
			if (opts.numLevels > 1)
				commonLocal.LoadPacifierBinarizeProgressTotal(opts.width * opts.width * 6 * 4 / 3);
			else
				commonLocal.LoadPacifierBinarizeProgressTotal(opts.width * opts.width * 6);
			im.LoadCubeFromMemory( size, (const byte**)pics, opts.numLevels, opts.format, opts.colorFormat, opts.gammaMips, toolUsage, usage );
			commonLocal.LoadPacifierBinarizeEnd();
			repeat = TR_CLAMP;
			
			for( int i = 0; i < 6; i++ )
			{
				if( pics[i] )
				{
					Mem_Free( pics[i] );
				}
			}
		}
		else
		{
			int width, height;
			byte* pic;
			
			// load the full specification, and perform any image program calculations
			R_LoadImageProgram( GetName(), &pic, &width, &height, &sourceFileTime, &usage );
			
			if( pic == NULL )
			{
				// motorsep 01-16-2015; only print that debug warning when "developer" cvar is set to 1
				if( com_developer.GetBool() )
					idLib::Warning( "Couldn't load image: %s : %s", GetName(), generatedName.c_str() );

				// create a default so it doesn't get continuously reloaded
				opts.width = 8;
				opts.height = 8;
				opts.numLevels = 1;
				DeriveOpts();
				AllocImage();
				
				// clear the data so it's not left uninitialized
				idTempArray<byte> clear( opts.width * opts.height * 4 );
				memset( clear.Ptr(), 0, clear.Size() );
				for( int level = 0; level < opts.numLevels; level++ )
				{
					SubImageUpload( level, 0, 0, 0, opts.width >> level, opts.height >> level, clear.Ptr() );
				}
				
				return;
			}
			
			opts.width = width;
			opts.height = height;
			opts.numLevels = 0;
			DeriveOpts();

			// foresthale 2014-05-30: give a nice progress display when binarizing
			commonLocal.LoadPacifierBinarizeFilename( generatedName.c_str(), binarizeReason.c_str() );
			if (opts.numLevels > 1)
				commonLocal.LoadPacifierBinarizeProgressTotal(opts.width * opts.height * 4 / 3);
			else
				commonLocal.LoadPacifierBinarizeProgressTotal(opts.width * opts.height);
			im.Load2DFromMemory( opts.width, opts.height, pic, opts.numLevels, opts.format, opts.colorFormat, opts.gammaMips, toolUsage );
			commonLocal.LoadPacifierBinarizeEnd();
			
			Mem_Free( pic );
		}
		
		if ( !toolUsage || r_cacheToolImages.GetBool() )
			binaryFileTime = im.WriteGeneratedFile( sourceFileTime, toolUsage );
	}

	AllocImage();
	
	
	for( int i = 0; i < im.NumImages(); i++ )
	{
		const bimageImage_t& img = im.GetImageHeader( i );
		const byte* data = im.GetImageData( i );
		SubImageUpload( img.level, 0, 0, img.destZ, img.width, img.height, data );
	}
}

/*
==============
Bind

Automatically enables 2D mapping or cube mapping if needed
==============
*/
void idImage::Bind()
{

	RENDERLOG_PRINTF( "idImage::Bind( %s )\n", GetName() );
	
	// load the image if necessary (FIXME: not SMP safe!)
	if( !IsLoaded() )
	{
		// load the image on demand here, which isn't our normal game operating mode
		ActuallyLoadImage( true );
	}
	
	const int texUnit = backEnd.glState.currenttmu;
	
	tmu_t* tmu = &backEnd.glState.tmu[texUnit];
	// bind the texture
	if( opts.textureType == TT_2D )
	{
		if( tmu->current2DMap != texnum )
		{
			tmu->current2DMap = texnum;
			qglBindMultiTextureEXT( GL_TEXTURE0_ARB + texUnit, GL_TEXTURE_2D, texnum );
		}

		// foresthale 2014-05-10: when using the tools code (which does not use shaders) we have to manage the texture unit enables
		if (com_editors)
		{
			//qglActiveTexture(GL_TEXTURE0_ARB + texUnit);
			qglEnable(GL_TEXTURE_2D);
		}
	}
	else if( opts.textureType == TT_CUBIC )
	{
		if( tmu->currentCubeMap != texnum )
		{
			tmu->currentCubeMap = texnum;
			qglBindMultiTextureEXT( GL_TEXTURE0_ARB + texUnit, GL_TEXTURE_CUBE_MAP_EXT, texnum );
		}

		// foresthale 2014-05-10: when using the tools code (which does not use shaders) we have to manage the texture unit enables
		if (com_editors)
		{
			//qglActiveTexture(GL_TEXTURE0_ARB + texUnit);
			qglEnable(GL_TEXTURE_CUBE_MAP);
		}
	}
	else if( opts.textureType == TT_2D_ARRAY )
	{
		if( tmu->current2DArray != texnum )
		{
			tmu->current2DArray = texnum;
			
			// RB begin
			qglBindMultiTextureEXT( GL_TEXTURE0 + texUnit, GL_TEXTURE_2D_ARRAY, texnum );
		}
	}

	
}

/*
================
MakePowerOfTwo
================
*/
int MakePowerOfTwo( int num )
{
	int	pot;
	for( pot = 1; pot < num; pot <<= 1 )
	{
	}
	return pot;
}

/*
====================
CopyFramebuffer
====================
*/
void idImage::CopyFramebuffer( int x, int y, int imageWidth, int imageHeight )
{
	// foresthale 2014-03-02: fixup incorrectly created render target textures - using a mipmapped texture as destination of a CopyFramebuffer yields a bogus mipmap chain, which works on NVIDIA and older AMD drivers (13.1 and below) but crashes on current AMD drivers (13.12 as of this writing).
	if ( opts.numLevels != 1 || filter != TF_LINEAR || repeat != TR_CLAMP )
	{
		common->Warning("idImage::CopyFramebuffer used on image \"%s\" which was not created for the purpose (should be TF_LINEAR, TR_CLAMP so that it has NO MIPMAPS) - this would render incorrectly (NVIDIA) or crash (AMD) in stock BFG Edition!", GetName());
		opts.numLevels = 1;
		filter = TF_LINEAR;
		repeat = TR_CLAMP;
		AllocImage();
	}

	qglBindTexture( ( opts.textureType == TT_CUBIC ) ? GL_TEXTURE_CUBE_MAP_EXT : GL_TEXTURE_2D, texnum );

	// foresthale 2014-02-20: HDR view rendering - this seems to not be changed anywhere and conflicts with FBO rendering
	//qglReadBuffer( GL_BACK );
	
	opts.width = imageWidth;
	opts.height = imageHeight;
	// foresthale 2014-02-20: HDR view rendering
	qglCopyTexImage2D( GL_TEXTURE_2D, 0, opts.format == FMT_RGBA16F ? GL_RGBA16F : GL_RGBA8, x, y, imageWidth, imageHeight, 0 );
	
	// these shouldn't be necessary if the image was initialized properly
	qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	
	qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	
	backEnd.pc.c_copyFrameBuffer++;
}

/*
====================
CopyDepthbuffer
====================
*/
void idImage::CopyDepthbuffer( int x, int y, int imageWidth, int imageHeight )
{
	qglBindTexture( ( opts.textureType == TT_CUBIC ) ? GL_TEXTURE_CUBE_MAP_EXT : GL_TEXTURE_2D, texnum );
	
	opts.width = imageWidth;
	opts.height = imageHeight;
	qglCopyTexImage2D( GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, x, y, imageWidth, imageHeight, 0 );

	backEnd.pc.c_copyFrameBuffer++;
}

/*
=============
RB_UploadScratchImage

if rows = cols * 6, assume it is a cube map animation
=============
*/
void idImage::UploadScratch( const byte* data, int cols, int rows )
{

	// if rows = cols * 6, assume it is a cube map animation
	if( rows == cols * 6 )
	{
		rows /= 6;
		const byte* pic[6];
		for( int i = 0; i < 6; i++ )
		{
			pic[i] = data + cols * rows * 4 * i;
		}
		
		if( opts.textureType != TT_CUBIC || usage != TD_LOOKUP_TABLE_RGBA )
		{
			GenerateCubeImage( pic, cols, TF_LINEAR, TD_LOOKUP_TABLE_RGBA );
			return;
		}
		if( opts.width != cols || opts.height != rows )
		{
			opts.width = cols;
			opts.height = rows;
			AllocImage();
		}
		SetSamplerState( TF_LINEAR, TR_CLAMP );
		for( int i = 0; i < 6; i++ )
		{
			SubImageUpload( 0, 0, 0, i, opts.width, opts.height, pic[i] );
		}
		
	}
	else
	{
		if( opts.textureType != TT_2D || usage != TD_LOOKUP_TABLE_RGBA )
		{
			GenerateImage( data, cols, rows, TF_LINEAR, TR_REPEAT, TD_LOOKUP_TABLE_RGBA );
			return;
		}
		if( opts.width != cols || opts.height != rows )
		{
			opts.width = cols;
			opts.height = rows;
			AllocImage();
		}
		SetSamplerState( TF_LINEAR, TR_REPEAT );
		SubImageUpload( 0, 0, 0, 0, opts.width, opts.height, data );
	}
}

/*
==================
StorageSize
==================
*/
int idImage::StorageSize() const
{

	if( !IsLoaded() )
	{
		return 0;
	}
	int baseSize = opts.width * opts.height;
	if( opts.numLevels > 1 )
	{
		baseSize *= 4;
		baseSize /= 3;
	}
	baseSize *= BitsForFormat( opts.format );
	baseSize /= 8;
	return baseSize;
}

/*
==================
Print
==================
*/
void idImage::Print() const
{
	if( generatorFunction )
	{
		common->Printf( "F" );
	}
	else
	{
		common->Printf( " " );
	}
	
	switch( opts.textureType )
	{
		case TT_2D:
			common->Printf( " " );
			break;
		case TT_CUBIC:
			common->Printf( "C" );
			break;
		default:
			common->Printf( "<BAD TYPE:%i>", opts.textureType );
			break;
	}
	
	common->Printf( "%4i %4i ",	opts.width, opts.height );
	
	switch( opts.format )
	{
#define NAME_FORMAT( x ) case FMT_##x: common->Printf( "%-6s ", #x ); break;
			NAME_FORMAT( NONE );
			NAME_FORMAT( RGBA8 );
			NAME_FORMAT( XRGB8 );
			NAME_FORMAT( RGB565 );
			NAME_FORMAT( L8A8 );
			NAME_FORMAT( ALPHA );
			NAME_FORMAT( LUM8 );
			NAME_FORMAT( INT8 );
			NAME_FORMAT( DXT1 );
			NAME_FORMAT( DXT5 );
			NAME_FORMAT( DEPTH );
			NAME_FORMAT( X16 );
			NAME_FORMAT( Y16_X16 );
		default:
			common->Printf( "<%3i>", opts.format );
			break;
	}
	
	switch( filter )
	{
		case TF_DEFAULT:
			common->Printf( "mip  " );
			break;
		case TF_LINEAR:
			common->Printf( "linr " );
			break;
		case TF_NEAREST:
			common->Printf( "nrst " );
			break;
		default:
			common->Printf( "<BAD FILTER:%i>", filter );
			break;
	}
	
	switch( repeat )
	{
		case TR_REPEAT:
			common->Printf( "rept " );
			break;
		case TR_CLAMP_TO_ZERO:
			common->Printf( "zero " );
			break;
		case TR_CLAMP_TO_ZERO_ALPHA:
			common->Printf( "azro " );
			break;
		case TR_CLAMP:
			common->Printf( "clmp " );
			break;
		default:
			common->Printf( "<BAD REPEAT:%i>", repeat );
			break;
	}
	
	common->Printf( "%4ik ", StorageSize() / 1024 );
	
	common->Printf( " %s\n", GetName() );
}

/*
===============
idImage::Reload
===============
*/
void idImage::Reload( bool force )
{
	// always regenerate functional images
	if( generatorFunction )
	{
		common->DPrintf( "regenerating %s.\n", GetName() );
		if ( opts.textureType != TT_2D_ARRAY && cubeFiles != CF_2D_ARRAY ) // motorsep 12-18-2014; we don't need to regenerate shadowmaps (maybe even none of the functional images) since they aren't being modified
			generatorFunction( this ); 
		return;
	}
	
	// check file times
	if( !force )
	{
		ID_TIME_T current;
		if( cubeFiles != CF_2D )
		{
			R_LoadCubeImages( imgName, cubeFiles, NULL, NULL, &current );
		}
		else
		{
			// get the current values
			R_LoadImageProgram( imgName, NULL, NULL, NULL, &current );
		}
		if( current <= sourceFileTime )
		{
			return;
		}
	}
	
	common->DPrintf( "reloading %s.\n", GetName() );
	
	PurgeImage();
	
	// Load is from the front end, so the back end must be synced
	ActuallyLoadImage( false );
}

/*
========================
idImage::SetSamplerState
========================
*/
void idImage::SetSamplerState( textureFilter_t tf, textureRepeat_t tr )
{
	if( tf == filter && tr == repeat )
	{
		return;
	}
	filter = tf;
	repeat = tr;
	qglBindTexture( ( opts.textureType == TT_CUBIC ) ? GL_TEXTURE_CUBE_MAP_EXT : GL_TEXTURE_2D, texnum );
	SetTexParameters();
}
