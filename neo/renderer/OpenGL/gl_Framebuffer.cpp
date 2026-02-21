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

/*
================================================================================================
Contains the Framebuffer implementation for OpenGL.
================================================================================================
*/

#include "../tr_local.h"

/*
========================
idFramebuffer::AllocFramebuffer

Every framebuffer will pass through this function.

This should not be done during normal game-play, if you can avoid it.
========================
*/
void idFramebuffer::AllocFramebuffer()
{
	PurgeFramebuffer();

	// if we don't have a rendering context, just return after we
	// have filled in the parms.
	if( !R_IsInitialized() )
	{
		return;
	}

}

/*
========================
idFramebuffer::PurgeFramebuffer
========================
*/
void idFramebuffer::PurgeFramebuffer()
{
	if( fbo != 0 )
	{
		qglDeleteFramebuffers( 1, ( GLuint* )&fbo );	// this should be the ONLY place it is ever called!
		fbo = 0;
		// clear current binding
		globalFramebuffers->BindSystemFramebuffer();
	}
}

/*
========================
idFramebuffer::BindDraw
========================
*/
void idFramebuffer::Bind()
{
	int n = 0;
	static const GLenum drawbuffers[8] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3, GL_COLOR_ATTACHMENT4, GL_COLOR_ATTACHMENT5, GL_COLOR_ATTACHMENT6, GL_COLOR_ATTACHMENT7 };

	for (n = 0;n < 8;n++)
		if (!colorAttachmentImage[n])
			break;

	if (fbo == 0)
	{
		GL_CheckErrors();
	
		if (!globalFramebuffers->gotsysfbo)
		{
			globalFramebuffers->gotsysfbo = true;
			qglGetIntegerv(GL_FRAMEBUFFER_BINDING, (GLint*)&globalFramebuffers->sysfbo);
		}

		// generate the fbo handle
		qglGenFramebuffers( 1, ( GLuint* )&fbo );
		assert( fbo != 0 );

		// bind the fbo handle so that we can configure it
		qglBindFramebuffer(GL_FRAMEBUFFER, fbo);

		// attach things
		if (depthAttachmentImage)
			depthAttachmentImage->BindAttachmentOnFBO(GL_DEPTH_ATTACHMENT, attachmentImageLayer);
		if (depthStencilAttachmentImage)
			depthStencilAttachmentImage->BindAttachmentOnFBO(GL_DEPTH_STENCIL_ATTACHMENT, attachmentImageLayer);
		for (int i = 0;i < 8;i++)
			if (colorAttachmentImage[i])
				colorAttachmentImage[i]->BindAttachmentOnFBO(GL_COLOR_ATTACHMENT0 + i, attachmentImageLayer);

		// configure the drawbuffers/readbuffer correctly so we can use it
		if (n > 1)
		{
			qglDrawBuffers(n, drawbuffers);
			qglReadBuffer(GL_NONE);
		}
		else if (n == 1)
		{
			qglDrawBuffer(GL_COLOR_ATTACHMENT0);
			qglReadBuffer(GL_COLOR_ATTACHMENT0);
		}
		else
		{
			qglDrawBuffer(GL_NONE);
			qglReadBuffer(GL_NONE);
		}

		// check if the framebuffer looks okay to the driver
		int status = qglCheckFramebufferStatus(GL_FRAMEBUFFER);
		assert( status == GL_FRAMEBUFFER_COMPLETE );

		// see if we messed anything up
		GL_CheckErrors();
	}
	else
	{
		qglBindFramebuffer(GL_FRAMEBUFFER, fbo);

		GLenum status = qglCheckFramebufferStatus( GL_FRAMEBUFFER );
		
		// check for errors
		GL_CheckErrors();

		// configure the drawbuffers/readbuffer correctly so we can use it
		if (n > 1)
		{
			qglDrawBuffers(n, drawbuffers);
			qglReadBuffer(GL_NONE);
		}
		else if (n == 1)
		{
			qglDrawBuffer(GL_COLOR_ATTACHMENT0);
			qglReadBuffer(GL_COLOR_ATTACHMENT0);
		}
		else
		{
			qglDrawBuffer(GL_NONE);
			qglReadBuffer(GL_NONE);
		}

		// check for errors
		GL_CheckErrors();
	}
}

/*
========================
idFramebuffer::SetDepthStencilAttachment
========================
*/
void idFramebuffer::SetDepthStencilAttachment(idImage* image)
{
	PurgeFramebuffer();
	depthStencilAttachmentImage = image;
}

/*
========================
idFramebuffer::SetDepthAttachment
========================
*/
void idFramebuffer::SetDepthAttachment(idImage* image)
{
	PurgeFramebuffer();
	depthAttachmentImage = image;
}

/*
========================
idFramebuffer::SetColorAttachment
========================
*/
void idFramebuffer::SetColorAttachment(int index, idImage* image)
{
	PurgeFramebuffer();
	colorAttachmentImage[index] = image;
}


// do this with a pointer, in case we want to make the actual manager
// a private virtual subclass
idFramebufferManager	framebufferManager;
idFramebufferManager* globalFramebuffers = &framebufferManager;

/*
==============
AllocFramebuffer

Allocates an idFramebuffer, adds it to the list,
copies the name, and adds it to the hash chain.
==============
*/
idFramebuffer* idFramebufferManager::AllocFramebuffer( const char* name, int layer )
{
	if( strlen( name ) >= MAX_IMAGE_NAME )
	{
		common->Error( "idFramebufferManager::AllocFramebuffer: \"%s\" is too long\n", name );
	}
	
	int hash = idStr( name ).FileNameHash();
	
	idFramebuffer* framebuffer = new( TAG_IMAGE ) idFramebuffer( name, layer );
	
	framebufferHash.Add( hash, framebuffers.Append( framebuffer ) );
	
	return framebuffer;
}

/*
==============
AllocStandaloneFramebuffer

Allocates an idFramebuffer, does not add it to the list or hash chain

==============
*/
idFramebuffer* idFramebufferManager::AllocStandaloneFramebuffer( const char* name )
{
	if( strlen( name ) >= MAX_IMAGE_NAME )
	{
		common->Error( "idFramebufferManager::AllocFramebuffer: \"%s\" is too long\n", name );
	}
	
	idFramebuffer* framebuffer = new( TAG_IMAGE ) idFramebuffer( name );
	
	return framebuffer;
}

/*
===============
idFramebufferManager::GetFramebuffer
===============
*/
idFramebuffer* idFramebufferManager::GetFramebuffer( const char* _name ) const
{
	idStr name = _name;
	
	//
	// look in loaded framebuffers
	//
	int hash = name.FileNameHash();
	for( int i = framebufferHash.First( hash ); i != -1; i = framebufferHash.Next( i ) )
	{
		idFramebuffer* framebuffer = framebuffers[i];
		if( name.Icmp( framebuffer->GetName() ) == 0 )
		{
			return framebuffer;
		}
	}
	
	return NULL;
}

/*
===============
PurgeAllFramebuffers
===============
*/
void idFramebufferManager::PurgeAllFramebuffers()
{
	int		i;
	idFramebuffer*	framebuffer;
	
	for( i = 0; i < framebuffers.Num() ; i++ )
	{
		framebuffer = framebuffers[i];
		framebuffer->PurgeFramebuffer();
	}

	if (gotsysfbo)
	{
		// return to the system fbo
		BindSystemFramebuffer();

		// we'll get the system fbo again on first Bind after reload
		gotsysfbo = false;
		sysfbo = 0;
	}
}

/*
===============
ReloadFramebuffers
===============
*/
void idFramebufferManager::ReloadFramebuffers()
{
	for( int i = 0 ; i < globalFramebuffers->framebuffers.Num() ; i++ )
	{
		globalFramebuffers->framebuffers[ i ]->Reload();
	}
}

/*
===============
BindSystemFramebuffer
===============
*/
void idFramebufferManager::BindSystemFramebuffer()
{
	if ( gotsysfbo )
	{
		qglBindFramebuffer( GL_FRAMEBUFFER, sysfbo );
	}

	GLenum status = qglCheckFramebufferStatus( GL_FRAMEBUFFER );

	// check for errors
	GL_CheckErrors();

	qglDrawBuffer(GL_BACK);
	qglReadBuffer(GL_BACK);

	// check for errors
	GL_CheckErrors();
}

/*
===============
Init
===============
*/
void idFramebufferManager::Init()
{
	framebuffers.Resize( 16, 16 );
	framebufferHash.ResizeIndex( 16 );
	
	viewFramebuffer = AllocFramebuffer( "_viewFramebuffer" );
	viewFramebuffer->SetDepthStencilAttachment( globalImages->viewFramebufferDepthImage );
	viewFramebuffer->SetColorAttachment( 0, globalImages->viewFramebufferRenderImage16 );

	// foresthale 2014-04-08: r_glow
//	glowFramebuffer8[0] = AllocFramebuffer( "_glowFramebuffer0" );
//	glowFramebuffer8[0]->SetColorAttachment( 0, globalImages->glowFramebufferImage[0] );
	glowFramebuffer8[0] = NULL;
	glowFramebuffer8[1] = AllocFramebuffer( "_glowFramebuffer1" );
	glowFramebuffer8[1]->SetColorAttachment( 0, globalImages->glowFramebufferImage8[1] );
	glowFramebuffer8[2] = AllocFramebuffer( "_glowFramebuffer2" );
	glowFramebuffer8[2]->SetColorAttachment( 0, globalImages->glowFramebufferImage8[2] );
	glowFramebuffer8[3] = AllocFramebuffer( "_glowFramebuffer3" );
	glowFramebuffer8[3]->SetColorAttachment( 0, globalImages->glowFramebufferImage8[3] );
//	glowFramebuffer16[0] = AllocFramebuffer( "_glowFramebufferHDR0" );
//	glowFramebuffer16[0]->SetColorAttachment( 0, globalImages->glowFramebufferImage[0] );
	glowFramebuffer16[0] = NULL;
	glowFramebuffer16[1] = AllocFramebuffer( "_glowFramebufferHDR1" );
	glowFramebuffer16[1]->SetColorAttachment( 0, globalImages->glowFramebufferImage16[1] );
	glowFramebuffer16[2] = AllocFramebuffer( "_glowFramebufferHDR2" );
	glowFramebuffer16[2]->SetColorAttachment( 0, globalImages->glowFramebufferImage16[2] );
	glowFramebuffer16[3] = AllocFramebuffer( "_glowFramebufferHDR3" );
	glowFramebuffer16[3]->SetColorAttachment( 0, globalImages->glowFramebufferImage16[3] );

	for ( int i = 0;i < 5;i++ )
	{
		for ( int j = 0;j < 6;j++ )
		{
			shadowMapFramebuffer[i][j] = AllocFramebuffer( va( "_shadowMapFramebuffer%i_%i", i, j ), j );
			shadowMapFramebuffer[i][j]->SetDepthAttachment( globalImages->shadowImage[i] );
		}
	}

	// SSAO/SSR
	ssaoFramebuffer = AllocFramebuffer( "_ssaoFramebuffer" );
	ssaoFramebuffer->SetColorAttachment( 0, globalImages->ssaoImage );
	ssaoBlurFramebuffer = AllocFramebuffer( "_ssaoBlurFramebuffer" );
	ssaoBlurFramebuffer->SetColorAttachment( 0, globalImages->ssaoBlurImage );
	// these will be set by the first call to idFramebuffer::Bind
	sysfbo = 0;
	gotsysfbo = false;
}

/*
===============
Shutdown
===============
*/
void idFramebufferManager::Shutdown()
{
	framebuffers.DeleteContents( true );
	framebufferHash.Clear();
}

/*
===============
InitIntrinsics
===============
*/
void idFramebufferManager::InitIntrinsics()
{
}
