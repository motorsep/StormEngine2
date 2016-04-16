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
#include "../Game_local.h"


/*
========================
idMenuHandler_Cinematic::Update
========================
*/
void idMenuHandler_Cinematic::Update()
{

	if( gui == NULL || !gui->IsActive() )
	{
		return;
	}
	
	if( nextScreen != activeScreen )
	{
	
		if( activeScreen > Cinematic_AREA_INVALID && activeScreen < Cinematic_NUM_AREAS && menuScreens[ activeScreen ] != NULL )
		{
			menuScreens[ activeScreen ]->HideScreen( static_cast<mainMenuTransition_t>( transition ) );
		}
		
		if( nextScreen > Cinematic_AREA_INVALID && nextScreen < Cinematic_NUM_AREAS && menuScreens[ nextScreen ] != NULL )
		{
			menuScreens[ nextScreen ]->ShowScreen( static_cast<mainMenuTransition_t>( transition ) );
		}
		
		transition = MENU_TRANSITION_INVALID;
		activeScreen = nextScreen;
	}
	
	idMenuHandler::Update();
}

/*
========================
idMenuHandler_Cinematic::ActivateMenu
========================
*/
void idMenuHandler_Cinematic::ActivateMenu( bool show )
{

	idMenuHandler::ActivateMenu( show );
	
	idPlayer* player = gameLocal.GetLocalPlayer();
	if( player == NULL )
	{
		return;
	}
	
	if( show )
	{
		activeScreen = Cinematic_AREA_INVALID;
		nextScreen = Cinematic_AREA_PLAYING;
	}
	else
	{
		activeScreen = Cinematic_AREA_INVALID;
		nextScreen = Cinematic_AREA_INVALID;
	}
	
}

/*
========================
idMenuHandler_Cinematic::Initialize
========================
*/
void idMenuHandler_Cinematic::Initialize( const char* swfFile, idSoundWorld* sw )
{
	idMenuHandler::Initialize( swfFile, sw );
	
	//---------------------
	// Initialize the menus
	//---------------------
#define BIND_Cinematic_SCREEN( screenId, className, menuHandler )				\
	menuScreens[ (screenId) ] = new className();						\
	menuScreens[ (screenId) ]->Initialize( menuHandler );				\
	menuScreens[ (screenId) ]->AddRef();
	
	for( int i = 0; i < Cinematic_NUM_AREAS; ++i )
	{
		menuScreens[ i ] = NULL;
	}
	
	BIND_Cinematic_SCREEN( Cinematic_AREA_PLAYING, idMenuScreen_Cinematic, this );
}

/*
========================
idMenuHandler_Cinematic::GetMenuScreen
========================
*/
idMenuScreen* idMenuHandler_Cinematic::GetMenuScreen( int index )
{

	if( index < 0 || index >= Cinematic_NUM_AREAS )
	{
		return NULL;
	}
	
	return menuScreens[ index ];
	
}

/*
========================
idMenuHandler_Cinematic::GetCinematic
========================
*/
idMenuScreen_Cinematic* idMenuHandler_Cinematic::GetCinematic()
{
	idMenuScreen_Cinematic* screen = dynamic_cast< idMenuScreen_Cinematic* >( menuScreens[ Cinematic_AREA_PLAYING ] );
	return screen;
}

