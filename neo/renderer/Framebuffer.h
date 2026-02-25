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

// foresthale 2014-02-17: this file derived from Image.h originally

static const int MAX_SHADOWMAP_RESOLUTIONS = 5;
#if 1
//static	int shadowMapResolutions[MAX_SHADOWMAP_RESOLUTIONS] = { 2048, 1024, 512, 512, 256 };
extern int shadowMapResolutions[MAX_SHADOWMAP_RESOLUTIONS];
#else
static	int shadowMapResolutions[MAX_SHADOWMAP_RESOLUTIONS] = { 1024, 1024, 1024, 1024, 1024 };
#endif

/*
====================================================================

FRAMEBUFFER

idFramebuffer has a one to one correspondance with GL framebuffer objects.

No texture is ever used that does not have a corresponding idImage.

====================================================================
*/

class idFramebuffer
{
public:
	idFramebuffer( const char* name, int layer = -1 );
	
	const char* 	GetName() const
	{
		return framebufferName;
	}

	// Makes this Framebuffer the active buffer
	void		Bind();
	// Makes this Framebuffer the active buffer
	void		Bind(int layer); // Use this only on texture arrays.
	// creates the fbo object for this framebuffer
	void		Reload() {}

	// allocates a gl fbo object
	void		AllocFramebuffer();

	// deletes the gl fbo object
	void		PurgeFramebuffer();

	// configure fbo object - requires Bind first
	void		SetDepthStencilAttachment(idImage* image);

	// configure fbo object - requires Bind first
	void		SetDepthAttachment(idImage* image);

	// configure fbo object - requires Bind first
	void		SetColorAttachment(int index, idImage* image);

private:
	friend class idFramebufferManager;

	// parameters that define this framebuffer
	idStr		framebufferName;	// name from code, does not corresond to anything on disk
	int			fbo;				// gl framebuffer object binding
	// foresthale 2014-10-05: when using a GL_TEXTURE_2D_ARRAY we need to pick
	// which layer we bind to - we only store one of these layer indices
	// because it is unlikely that depth and color attachments would bind to
	// different layers
	int			attachmentImageLayer; // when using TD_SHADOW_ARRAY, which layer this fbo binds to, otherwise -1
	// use only one of the following two depth attachments - both would not make sense
	idImage*	depthStencilAttachmentImage; // GL_DEPTH_STENCIL_ATTACHMENT
	idImage*	depthAttachmentImage; // GL_DEPTH_ATTACHMENT
	idImage*	colorAttachmentImage[8];

};

ID_INLINE idFramebuffer::idFramebuffer( const char* name, int layer ) : framebufferName( name )
{
	fbo = 0;
	depthStencilAttachmentImage = NULL;
	depthAttachmentImage = NULL;
	attachmentImageLayer = layer;
	for (int i = 0;i < 8;i++)
		colorAttachmentImage[i] = NULL;
}


class idFramebufferManager
{
public:

	idFramebufferManager()
	{
	}
	
	void				Init();
	void				Shutdown();
	void				InitIntrinsics();

	idFramebuffer*		GetFramebuffer( const char* name ) const;

	// purges all the framebuffers before a vid_restart
	void				PurgeAllFramebuffers();
	
	// reloads all framebuffers after a vid_restart
	void				ReloadFramebuffers();
	
	// restore the system framebuffer
	void				BindSystemFramebuffer();
	
	// built-in framebuffers
	idFramebuffer*		viewFramebuffer;
	idFramebuffer*		glowFramebuffer8[4];
	idFramebuffer*		glowFramebuffer16[4];
	idFramebuffer*		shadowMapFramebuffer[MAX_SHADOWMAP_RESOLUTIONS][6];
	
	// G-Buffer framebuffer for depth pre-pass normal output
	idFramebuffer*		gbufferFramebuffer;
	
	// SSAO framebuffers
	idFramebuffer*		ssaoFramebuffer;
	idFramebuffer*		ssaoBlurTempFramebuffer;

	idFramebuffer*		AllocFramebuffer( const char* name, int layer = -1 );
	idFramebuffer*		AllocStandaloneFramebuffer( const char* name );

	// the system provides an fbo, this is it (on desktop PC this is usually 0)
	int					sysfbo;

	// on first Bind we get the fbo
	bool				gotsysfbo;

	idList<idFramebuffer*, TAG_IDLIB_LIST_FRAMEBUFFER>	framebuffers;
	idHashIndex			framebufferHash;
};

extern idFramebufferManager*	globalFramebuffers;		// pointer to global list for the rest of the system
