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
#include "precompiled.h"
#pragma hdrstop

#include "Game_local.h"

/***********************************************************************

	idActor

***********************************************************************/

const idEventDef AI_EnableEyeFocus( "enableEyeFocus" );
const idEventDef AI_DisableEyeFocus( "disableEyeFocus" );
const idEventDef EV_Footstep( "footstep" );
const idEventDef EV_FootstepLeft( "leftFoot" );
const idEventDef EV_FootstepRight( "rightFoot" );
const idEventDef EV_EnableWalkIK( "EnableWalkIK" );
const idEventDef EV_DisableWalkIK( "DisableWalkIK" );
const idEventDef EV_EnableLegIK( "EnableLegIK", "d" );
const idEventDef EV_DisableLegIK( "DisableLegIK", "d" );
const idEventDef AI_StopAnim( "stopAnim", "dd" );
const idEventDef AI_PlayAnim( "playAnim", "ds", 'd' );
const idEventDef AI_PlayCycle( "playCycle", "ds", 'd' );
const idEventDef AI_IdleAnim( "idleAnim", "ds", 'd' );
const idEventDef AI_SetSyncedAnimWeight( "setSyncedAnimWeight", "ddf" );
const idEventDef AI_SetBlendFrames( "setBlendFrames", "dd" );
const idEventDef AI_GetBlendFrames( "getBlendFrames", "d", 'd' );
const idEventDef AI_FinishAction( "finishAction", "s" );
const idEventDef AI_AnimDone( "animDone", "dd", 'd' );
const idEventDef AI_OverrideAnim( "overrideAnim", "d" );
const idEventDef AI_EnableAnim( "enableAnim", "dd" );
const idEventDef AI_PreventPain( "preventPain", "f" );
const idEventDef AI_DisablePain( "disablePain" );
const idEventDef AI_EnablePain( "enablePain" );
const idEventDef AI_GetPainAnim( "getPainAnim", NULL, 's' );
const idEventDef AI_SetAnimPrefix( "setAnimPrefix", "s" );
const idEventDef AI_HasAnim( "hasAnim", "ds", 'f' );
const idEventDef AI_CheckAnim( "checkAnim", "ds" );
const idEventDef AI_ChooseAnim( "chooseAnim", "ds", 's' );
const idEventDef AI_AnimLength( "animLength", "ds", 'f' );
const idEventDef AI_AnimDistance( "animDistance", "ds", 'f' );
const idEventDef AI_HasEnemies( "hasEnemies", NULL, 'd' );
const idEventDef AI_NextEnemy( "nextEnemy", "E", 'e' );
const idEventDef AI_ClosestEnemyToPoint( "closestEnemyToPoint", "v", 'e' );
const idEventDef AI_SetNextState( "setNextState", "s" );
const idEventDef AI_SetState( "setState", "s" );
const idEventDef AI_GetState( "getState", NULL, 's' );
const idEventDef AI_GetHead( "getHead", NULL, 'e' );
const idEventDef EV_SetDamageGroupScale( "setDamageGroupScale", "sf" );
const idEventDef EV_SetDamageGroupScaleAll( "setDamageGroupScaleAll", "f" );
const idEventDef EV_GetDamageGroupScale( "getDamageGroupScale", "s", 'f' );
const idEventDef EV_SetDamageCap( "setDamageCap", "f" );
const idEventDef EV_SetWaitState( "setWaitState" , "s" );
const idEventDef EV_GetWaitState( "getWaitState", NULL, 's' );
const idEventDef AI_SetTeam( "setTeam", "f" );
const idEventDef AI_GetTeam( "getTeam", NULL, 'f' );

CLASS_DECLARATION( idAFEntity_Gibbable, idActor )
EVENT( AI_EnableEyeFocus,			idActor::Event_EnableEyeFocus )
EVENT( AI_DisableEyeFocus,			idActor::Event_DisableEyeFocus )
EVENT( EV_Footstep,					idActor::Event_Footstep )
EVENT( EV_FootstepLeft,				idActor::Event_Footstep )
EVENT( EV_FootstepRight,			idActor::Event_Footstep )
EVENT( EV_EnableWalkIK,				idActor::Event_EnableWalkIK )
EVENT( EV_DisableWalkIK,			idActor::Event_DisableWalkIK )
EVENT( EV_EnableLegIK,				idActor::Event_EnableLegIK )
EVENT( EV_DisableLegIK,				idActor::Event_DisableLegIK )
EVENT( AI_PreventPain,				idActor::Event_PreventPain )
EVENT( AI_DisablePain,				idActor::Event_DisablePain )
EVENT( AI_EnablePain,				idActor::Event_EnablePain )
EVENT( AI_GetPainAnim,				idActor::Event_GetPainAnim )
EVENT( AI_SetAnimPrefix,			idActor::Event_SetAnimPrefix )
EVENT( AI_StopAnim,					idActor::Event_StopAnim )
EVENT( AI_PlayAnim,					idActor::Event_PlayAnim )
EVENT( AI_PlayCycle,				idActor::Event_PlayCycle )
EVENT( AI_IdleAnim,					idActor::Event_IdleAnim )
EVENT( AI_SetSyncedAnimWeight,		idActor::Event_SetSyncedAnimWeight )
EVENT( AI_SetBlendFrames,			idActor::Event_SetBlendFrames )
EVENT( AI_GetBlendFrames,			idActor::Event_GetBlendFrames )
EVENT( AI_FinishAction,				idActor::Event_FinishAction )
EVENT( AI_AnimDone,					idActor::Event_AnimDone )
EVENT( AI_OverrideAnim,				idActor::Event_OverrideAnim )
EVENT( AI_EnableAnim,				idActor::Event_EnableAnim )
EVENT( AI_HasAnim,					idActor::Event_HasAnim )
EVENT( AI_CheckAnim,				idActor::Event_CheckAnim )
EVENT( AI_ChooseAnim,				idActor::Event_ChooseAnim )
EVENT( AI_AnimLength,				idActor::Event_AnimLength )
EVENT( AI_AnimDistance,				idActor::Event_AnimDistance )
EVENT( AI_HasEnemies,				idActor::Event_HasEnemies )
EVENT( AI_NextEnemy,				idActor::Event_NextEnemy )
EVENT( AI_ClosestEnemyToPoint,		idActor::Event_ClosestEnemyToPoint )
EVENT( EV_StopSound,				idActor::Event_StopSound )
EVENT( AI_SetNextState,				idActor::Event_SetNextState )
EVENT( AI_SetState,					idActor::Event_SetState )
EVENT( AI_GetState,					idActor::Event_GetState )
EVENT( AI_GetHead,					idActor::Event_GetHead )
EVENT( EV_SetDamageGroupScale,		idActor::Event_SetDamageGroupScale )
EVENT( EV_SetDamageGroupScaleAll,	idActor::Event_SetDamageGroupScaleAll )
EVENT( EV_GetDamageGroupScale,		idActor::Event_GetDamageGroupScale )
EVENT( EV_SetDamageCap,				idActor::Event_SetDamageCap )
EVENT( EV_SetWaitState,				idActor::Event_SetWaitState )
EVENT( EV_GetWaitState,				idActor::Event_GetWaitState )
EVENT( AI_SetTeam,					idActor::Event_SetTeam )
EVENT( AI_GetTeam,					idActor::Event_GetTeam )
END_CLASS

/***********************************************************************

	Events

***********************************************************************/

/*
=====================
idActor::Event_EnableEyeFocus
=====================
*/
void idActor::PlayFootStepSound()
{
	const char* sound = NULL;
	const idMaterial* material;
	
	if( !GetPhysics()->HasGroundContacts() )
	{
		return;
	}
	
	// start footstep sound based on material type
	material = GetPhysics()->GetContact( 0 ).material;
	if( material != NULL )
	{
		sound = spawnArgs.GetString( va( "snd_footstep_%s", gameLocal.sufaceTypeNames[ material->GetSurfaceType() ] ) );
	}
	if( sound != NULL && *sound == '\0' )
	{
		sound = spawnArgs.GetString( "snd_footstep" );
	}
	if( sound != NULL && *sound != '\0' )
	{
		StartSoundShader( declManager->FindSound( sound ), SND_CHANNEL_BODY, 0, false, NULL );
	}
}

/*
=====================
idActor::Event_EnableEyeFocus
=====================
*/
void idActor::Event_EnableEyeFocus()
{
	allowEyeFocus = true;
	blink_time = gameLocal.time + blink_min + gameLocal.random.RandomFloat() * ( blink_max - blink_min );
}

/*
=====================
idActor::Event_DisableEyeFocus
=====================
*/
void idActor::Event_DisableEyeFocus()
{
	allowEyeFocus = false;
	
	idEntity* headEnt = head.GetEntity();
	if( headEnt )
	{
		headEnt->GetAnimator()->Clear( ANIMCHANNEL_EYELIDS, gameLocal.time, FRAME2MS( 2 ) );
	}
	else
	{
		animator.Clear( ANIMCHANNEL_EYELIDS, gameLocal.time, FRAME2MS( 2 ) );
	}
}

/*
===============
idActor::Event_Footstep
===============
*/
void idActor::Event_Footstep()
{
	PlayFootStepSound();
}

/*
=====================
idActor::Event_EnableWalkIK
=====================
*/
void idActor::Event_EnableWalkIK()
{
	walkIK.EnableAll();
}

/*
=====================
idActor::Event_DisableWalkIK
=====================
*/
void idActor::Event_DisableWalkIK()
{
	walkIK.DisableAll();
}

/*
=====================
idActor::Event_EnableLegIK
=====================
*/
void idActor::Event_EnableLegIK( int num )
{
	walkIK.EnableLeg( num );
}

/*
=====================
idActor::Event_DisableLegIK
=====================
*/
void idActor::Event_DisableLegIK( int num )
{
	walkIK.DisableLeg( num );
}

/*
=====================
idActor::Event_PreventPain
=====================
*/
void idActor::Event_PreventPain( float duration )
{
	painTime = gameLocal.time + SEC2MS( duration );
}

/*
===============
idActor::Event_DisablePain
===============
*/
void idActor::Event_DisablePain()
{
	allowPain = false;
}

/*
===============
idActor::Event_EnablePain
===============
*/
void idActor::Event_EnablePain()
{
	allowPain = true;
}

/*
=====================
idActor::Event_GetPainAnim
=====================
*/
void idActor::Event_GetPainAnim()
{
	if( !painAnim.Length() )
	{
		idThread::ReturnString( "pain" );
	}
	else
	{
		idThread::ReturnString( painAnim );
	}
}

/*
=====================
idActor::Event_SetAnimPrefix
=====================
*/
void idActor::Event_SetAnimPrefix( const char* prefix )
{
	animPrefix = prefix;
}

/*
===============
idActor::Event_StopAnim
===============
*/
void idActor::Event_StopAnim( int channel, int frames )
{
	switch( channel )
	{
		case ANIMCHANNEL_HEAD :		
			headAnim.StopAnim( frames );
			break;
			
		case ANIMCHANNEL_TORSO :
			torsoAnim.StopAnim( frames );
			break;
			
		case ANIMCHANNEL_LEGS :
			legsAnim.StopAnim( frames );
			break;

		case ANIMCHANNEL_EYELIDS:
			break;
		
		case ANIMCHANNEL_ALL:
			for ( int i = 1; i < ANIMCHANNEL_COUNT; ++i )
				Event_StopAnim( i, frames );
			break;

		default:
			gameLocal.Error( "Unknown anim group" );
			break;
	}
}

/*
===============
idActor::Event_PlayAnim
===============
*/
void idActor::Event_PlayAnim( int channel, const char* animname )
{
	const int	anim = GetAnim( channel, animname );
	if( !anim )
	{
		if( ( channel == ANIMCHANNEL_HEAD ) && head.GetEntity() )
		{
			gameLocal.DPrintf( "missing '%s' animation on '%s' (%s)\n", animname, name.c_str(), spawnArgs.GetString( "def_head", "" ) );
		}
		else
		{
			gameLocal.DPrintf( "missing '%s' animation on '%s' (%s)\n", animname, name.c_str(), GetEntityDefName() );
		}
		idThread::ReturnInt( 0 );
		return;
	}
	
	if ( ANIMCHANNEL_ALL == channel )
	{
		// start from 1, skip ANIMCHANNEL_ALL
		for ( int i = 1; i < ANIMCHANNEL_COUNT; ++i )
			InternalPlayAnim( i, anim );
	}
	else
		InternalPlayAnim( channel, anim );

	idThread::ReturnInt( 1 );
}

/*
===============
idActor::Event_PlayCycle
===============
*/
void idActor::Event_PlayCycle( int channel, const char* animname )
{
	animFlags_t	flags;
	int			anim;
	
	anim = GetAnim( channel, animname );
	if( !anim )
	{
		if( ( channel == ANIMCHANNEL_HEAD ) && head.GetEntity() )
		{
			gameLocal.DPrintf( "missing '%s' animation on '%s' (%s)\n", animname, name.c_str(), spawnArgs.GetString( "def_head", "" ) );
		}
		else
		{
			gameLocal.DPrintf( "missing '%s' animation on '%s' (%s)\n", animname, name.c_str(), GetEntityDefName() );
		}
		idThread::ReturnInt( false );
		return;
	}
	
	switch( channel )
	{
		case ANIMCHANNEL_HEAD :
			headAnim.idleAnim = false;
			headAnim.CycleAnim( anim );
			flags = headAnim.GetAnimFlags();
			if( !flags.prevent_idle_override )
			{
				if( torsoAnim.IsIdle() && legsAnim.IsIdle() )
				{
					torsoAnim.animBlendFrames = headAnim.lastAnimBlendFrames;
					SyncAnimChannels( ANIMCHANNEL_TORSO, ANIMCHANNEL_HEAD, headAnim.lastAnimBlendFrames );
					legsAnim.animBlendFrames = headAnim.lastAnimBlendFrames;
					SyncAnimChannels( ANIMCHANNEL_LEGS, ANIMCHANNEL_HEAD, headAnim.lastAnimBlendFrames );
				}
			}
			break;
			
		case ANIMCHANNEL_TORSO :
			torsoAnim.idleAnim = false;
			torsoAnim.CycleAnim( anim );
			flags = torsoAnim.GetAnimFlags();
			if( !flags.prevent_idle_override )
			{
				if( headAnim.IsIdle() )
				{
					headAnim.animBlendFrames = torsoAnim.lastAnimBlendFrames;
					SyncAnimChannels( ANIMCHANNEL_HEAD, ANIMCHANNEL_TORSO, torsoAnim.lastAnimBlendFrames );
				}
				if( legsAnim.IsIdle() )
				{
					legsAnim.animBlendFrames = torsoAnim.lastAnimBlendFrames;
					SyncAnimChannels( ANIMCHANNEL_LEGS, ANIMCHANNEL_TORSO, torsoAnim.lastAnimBlendFrames );
				}
			}
			break;
			
		case ANIMCHANNEL_LEGS :
			legsAnim.idleAnim = false;
			legsAnim.CycleAnim( anim );
			flags = legsAnim.GetAnimFlags();
			if( !flags.prevent_idle_override )
			{
				if( torsoAnim.IsIdle() )
				{
					torsoAnim.animBlendFrames = legsAnim.lastAnimBlendFrames;
					SyncAnimChannels( ANIMCHANNEL_TORSO, ANIMCHANNEL_LEGS, legsAnim.lastAnimBlendFrames );
					if( headAnim.IsIdle() )
					{
						headAnim.animBlendFrames = legsAnim.lastAnimBlendFrames;
						SyncAnimChannels( ANIMCHANNEL_HEAD, ANIMCHANNEL_LEGS, legsAnim.lastAnimBlendFrames );
					}
				}
			}
			break;
			
		default:
			gameLocal.Error( "Unknown anim group" );
	}
	
	idThread::ReturnInt( true );
}

/*
===============
idActor::Event_IdleAnim
===============
*/
void idActor::Event_IdleAnim( int channel, const char* animname )
{
	const int anim = GetAnim( channel, animname );
	if( !anim )
	{
		if( ( channel == ANIMCHANNEL_HEAD ) && head.GetEntity() )
		{
			gameLocal.DPrintf( "missing '%s' animation on '%s' (%s)\n", animname, name.c_str(), spawnArgs.GetString( "def_head", "" ) );
		}
		else
		{
			gameLocal.DPrintf( "missing '%s' animation on '%s' (%s)\n", animname, name.c_str(), GetEntityDefName() );
		}
		
		switch( channel )
		{
			case ANIMCHANNEL_HEAD :
				headAnim.BecomeIdle();
				break;
				
			case ANIMCHANNEL_TORSO :
				torsoAnim.BecomeIdle();
				break;
				
			case ANIMCHANNEL_LEGS :
				legsAnim.BecomeIdle();
				break;
				
			default:
				gameLocal.Error( "Unknown anim group" );
		}
		
		idThread::ReturnInt( false );
		return;
	}
		
	if (ANIMCHANNEL_ALL == channel)
	{
		// start from 1, skip ANIMCHANNEL_ALL
		for (int i = 1; i < ANIMCHANNEL_COUNT; ++i)
			InternalIdleAnim(i, anim);
	}
	else
		InternalIdleAnim(channel, anim);
	
	idThread::ReturnInt(1);
}

/*
================
idActor::Event_SetSyncedAnimWeight
================
*/
void idActor::Event_SetSyncedAnimWeight( int channel, int anim, float weight )
{
	idEntity* headEnt;
	
	headEnt = head.GetEntity();
	switch( channel )
	{
		case ANIMCHANNEL_HEAD :
			if( headEnt )
			{
				animator.CurrentAnim( ANIMCHANNEL_ALL )->SetSyncedAnimWeight( anim, weight );
			}
			else
			{
				animator.CurrentAnim( ANIMCHANNEL_HEAD )->SetSyncedAnimWeight( anim, weight );
			}
			if( torsoAnim.IsIdle() )
			{
				animator.CurrentAnim( ANIMCHANNEL_TORSO )->SetSyncedAnimWeight( anim, weight );
				if( legsAnim.IsIdle() )
				{
					animator.CurrentAnim( ANIMCHANNEL_LEGS )->SetSyncedAnimWeight( anim, weight );
				}
			}
			break;
			
		case ANIMCHANNEL_TORSO :
			animator.CurrentAnim( ANIMCHANNEL_TORSO )->SetSyncedAnimWeight( anim, weight );
			if( legsAnim.IsIdle() )
			{
				animator.CurrentAnim( ANIMCHANNEL_LEGS )->SetSyncedAnimWeight( anim, weight );
			}
			if( headEnt && headAnim.IsIdle() )
			{
				animator.CurrentAnim( ANIMCHANNEL_ALL )->SetSyncedAnimWeight( anim, weight );
			}
			break;
			
		case ANIMCHANNEL_LEGS :
			animator.CurrentAnim( ANIMCHANNEL_LEGS )->SetSyncedAnimWeight( anim, weight );
			if( torsoAnim.IsIdle() )
			{
				animator.CurrentAnim( ANIMCHANNEL_TORSO )->SetSyncedAnimWeight( anim, weight );
				if( headEnt && headAnim.IsIdle() )
				{
					animator.CurrentAnim( ANIMCHANNEL_ALL )->SetSyncedAnimWeight( anim, weight );
				}
			}
			break;
			
		default:
			gameLocal.Error( "Unknown anim group" );
	}
}

/*
===============
idActor::Event_OverrideAnim
===============
*/
void idActor::Event_OverrideAnim( int channel )
{
	switch( channel )
	{
		case ANIMCHANNEL_HEAD :
			headAnim.Disable();
			if( !torsoAnim.IsIdle() )
			{
				SyncAnimChannels( ANIMCHANNEL_HEAD, ANIMCHANNEL_TORSO, torsoAnim.lastAnimBlendFrames );
			}
			else
			{
				SyncAnimChannels( ANIMCHANNEL_HEAD, ANIMCHANNEL_LEGS, legsAnim.lastAnimBlendFrames );
			}
			break;
			
		case ANIMCHANNEL_TORSO :
			torsoAnim.Disable();
			SyncAnimChannels( ANIMCHANNEL_TORSO, ANIMCHANNEL_LEGS, legsAnim.lastAnimBlendFrames );
			if( headAnim.IsIdle() )
			{
				SyncAnimChannels( ANIMCHANNEL_HEAD, ANIMCHANNEL_TORSO, torsoAnim.lastAnimBlendFrames );
			}
			break;
			
		case ANIMCHANNEL_LEGS :
			legsAnim.Disable();
			SyncAnimChannels( ANIMCHANNEL_LEGS, ANIMCHANNEL_TORSO, torsoAnim.lastAnimBlendFrames );
			break;
			
		default:
			gameLocal.Error( "Unknown anim group" );
			break;
	}
}

/*
===============
idActor::Event_EnableAnim
===============
*/
void idActor::Event_EnableAnim( int channel, int blendFrames )
{
	switch( channel )
	{
		case ANIMCHANNEL_HEAD :
			headAnim.Enable( blendFrames );
			break;
			
		case ANIMCHANNEL_TORSO :
			torsoAnim.Enable( blendFrames );
			break;
			
		case ANIMCHANNEL_LEGS :
			legsAnim.Enable( blendFrames );
			break;
			
		default:
			gameLocal.Error( "Unknown anim group" );
			break;
	}
}

/*
===============
idActor::Event_SetBlendFrames
===============
*/
void idActor::Event_SetBlendFrames( int channel, int blendFrames )
{
	switch( channel )
	{
		case ANIMCHANNEL_HEAD :
			headAnim.animBlendFrames = blendFrames;
			headAnim.lastAnimBlendFrames = blendFrames;
			break;
			
		case ANIMCHANNEL_TORSO :
			torsoAnim.animBlendFrames = blendFrames;
			torsoAnim.lastAnimBlendFrames = blendFrames;
			break;
			
		case ANIMCHANNEL_LEGS :
			legsAnim.animBlendFrames = blendFrames;
			legsAnim.lastAnimBlendFrames = blendFrames;
			break;
			
		default:
			gameLocal.Error( "Unknown anim group" );
			break;
	}
}

/*
===============
idActor::Event_GetBlendFrames
===============
*/
void idActor::Event_GetBlendFrames( int channel )
{
	switch( channel )
	{
		case ANIMCHANNEL_HEAD :
			idThread::ReturnInt( headAnim.animBlendFrames );
			break;
			
		case ANIMCHANNEL_TORSO :
			idThread::ReturnInt( torsoAnim.animBlendFrames );
			break;
			
		case ANIMCHANNEL_LEGS :
			idThread::ReturnInt( legsAnim.animBlendFrames );
			break;
			
		default:
			gameLocal.Error( "Unknown anim group" );
			break;
	}
}

/*
===============
idActor::Event_FinishAction
===============
*/
void idActor::Event_FinishAction( const char* actionname )
{
	if( waitState == actionname )
	{
		SetWaitState( "" );
	}
}

/*
===============
idActor::Event_AnimDone
===============
*/
void idActor::Event_AnimDone( int channel, int blendFrames )
{
	bool result;
	
	switch( channel )
	{
		case ANIMCHANNEL_HEAD :
			result = headAnim.AnimDone( blendFrames );
			idThread::ReturnInt( result );
			break;
			
		case ANIMCHANNEL_TORSO :
			result = torsoAnim.AnimDone( blendFrames );
			idThread::ReturnInt( result );
			break;
			
		case ANIMCHANNEL_LEGS :
			result = legsAnim.AnimDone( blendFrames );
			idThread::ReturnInt( result );
			break;

		case ANIMCHANNEL_ALL:
			result = headAnim.AnimDone( blendFrames ) &&
				     torsoAnim.AnimDone( blendFrames ) &&
					 legsAnim.AnimDone( blendFrames );
			idThread::ReturnInt(result);
			break;
			
		default:
			gameLocal.Error( "Unknown anim group" );
	}
}

/*
================
idActor::Event_HasAnim
================
*/
void idActor::Event_HasAnim( int channel, const char* animname )
{
	if( GetAnim( channel, animname ) != 0 )
	{
		idThread::ReturnFloat( 1.0f );
	}
	else
	{
		idThread::ReturnFloat( 0.0f );
	}
}

/*
================
idActor::Event_CheckAnim
================
*/
void idActor::Event_CheckAnim( int channel, const char* animname )
{
	if( !GetAnim( channel, animname ) )
	{
		if( animPrefix.Length() )
		{
			gameLocal.Error( "Can't find anim '%s_%s' for '%s'", animPrefix.c_str(), animname, name.c_str() );
		}
		else
		{
			gameLocal.Error( "Can't find anim '%s' for '%s'", animname, name.c_str() );
		}
	}
}

/*
================
idActor::Event_ChooseAnim
================
*/
void idActor::Event_ChooseAnim( int channel, const char* animname )
{
	int anim;
	
	anim = GetAnim( channel, animname );
	if( anim )
	{
		if( channel == ANIMCHANNEL_HEAD )
		{
			if( head.GetEntity() )
			{
				idThread::ReturnString( head.GetEntity()->GetAnimator()->AnimFullName( anim ) );
				return;
			}
		}
		else
		{
			idThread::ReturnString( animator.AnimFullName( anim ) );
			return;
		}
	}
	
	idThread::ReturnString( "" );
}

/*
================
idActor::Event_AnimLength
================
*/
void idActor::Event_AnimLength( int channel, const char* animname )
{
	int anim;
	
	anim = GetAnim( channel, animname );
	if( anim )
	{
		if( channel == ANIMCHANNEL_HEAD )
		{
			if( head.GetEntity() )
			{
				idThread::ReturnFloat( MS2SEC( head.GetEntity()->GetAnimator()->AnimLength( anim ) ) );
				return;
			}
		}
		else
		{
			idThread::ReturnFloat( MS2SEC( animator.AnimLength( anim ) ) );
			return;
		}
	}
	
	idThread::ReturnFloat( 0.0f );
}

/*
================
idActor::Event_AnimDistance
================
*/
void idActor::Event_AnimDistance( int channel, const char* animname )
{
	int anim;
	
	anim = GetAnim( channel, animname );
	if( anim )
	{
		if( channel == ANIMCHANNEL_HEAD )
		{
			if( head.GetEntity() )
			{
				idThread::ReturnFloat( head.GetEntity()->GetAnimator()->TotalMovementDelta( anim ).Length() );
				return;
			}
		}
		else
		{
			idThread::ReturnFloat( animator.TotalMovementDelta( anim ).Length() );
			return;
		}
	}
	
	idThread::ReturnFloat( 0.0f );
}

/*
================
idActor::Event_HasEnemies
================
*/
void idActor::Event_HasEnemies()
{
	bool hasEnemy = HasEnemies();
	idThread::ReturnInt( hasEnemy );
}

/*
================
idActor::Event_NextEnemy
================
*/
void idActor::Event_NextEnemy( idEntity* ent )
{
	idActor* actor;
	
	if( !ent || ( ent == this ) )
	{
		actor = enemyList.Next();
	}
	else
	{
		if( !ent->IsType( idActor::Type ) )
		{
			gameLocal.Error( "'%s' cannot be an enemy", ent->name.c_str() );
		}
		
		actor = static_cast<idActor*>( ent );
		if( actor->enemyNode.ListHead() != &enemyList )
		{
			gameLocal.Error( "'%s' is not in '%s' enemy list", actor->name.c_str(), name.c_str() );
		}
	}
	
	for( ; actor != NULL; actor = actor->enemyNode.Next() )
	{
		if( !actor->fl.hidden )
		{
			idThread::ReturnEntity( actor );
			return;
		}
	}
	
	idThread::ReturnEntity( NULL );
}

/*
================
idActor::Event_ClosestEnemyToPoint
================
*/
void idActor::Event_ClosestEnemyToPoint( const idVec3& pos )
{
	idActor* bestEnt = ClosestEnemyToPoint( pos );
	idThread::ReturnEntity( bestEnt );
}

/*
================
idActor::Event_StopSound
================
*/
void idActor::Event_StopSound( int channel, int netSync )
{
	if( channel == SND_CHANNEL_VOICE )
	{
		idEntity* headEnt = head.GetEntity();
		if( headEnt )
		{
			headEnt->StopSound( channel, ( netSync != 0 ) );
		}
	}
	StopSound( channel, ( netSync != 0 ) );
}

/*
=====================
idActor::Event_SetNextState
=====================
*/
void idActor::Event_SetNextState( const char* name )
{
	idealState = GetScriptFunction( name );
	if( idealState == state )
	{
		state = NULL;
	}
}

/*
=====================
idActor::Event_SetState
=====================
*/
void idActor::Event_SetState( const char* name )
{
	idealState = GetScriptFunction( name );
	if( idealState == state )
	{
		state = NULL;
	}
	scriptThread->DoneProcessing();
}

/*
=====================
idActor::Event_GetState
=====================
*/
void idActor::Event_GetState()
{
	if( state )
	{
		idThread::ReturnString( state->Name() );
	}
	else
	{
		idThread::ReturnString( "" );
	}
}

/*
=====================
idActor::Event_GetHead
=====================
*/
void idActor::Event_GetHead()
{
	idThread::ReturnEntity( head.GetEntity() );
}

/*
================
idActor::Event_SetDamageGroupScale
================
*/
void idActor::Event_SetDamageGroupScale( const char* groupName, float scale )
{

	for( int i = 0; i < damageScale.Num(); i++ )
	{
		if( damageGroups[ i ] == groupName )
		{
			damageScale[ i ] = scale;
		}
	}
}

/*
================
idActor::Event_SetDamageGroupScaleAll
================
*/
void idActor::Event_SetDamageGroupScaleAll( float scale )
{

	for( int i = 0; i < damageScale.Num(); i++ )
	{
		damageScale[ i ] = scale;
	}
}

void idActor::Event_GetDamageGroupScale( const char* groupName )
{

	for( int i = 0; i < damageScale.Num(); i++ )
	{
		if( damageGroups[ i ] == groupName )
		{
			idThread::ReturnFloat( damageScale[i] );
			return;
		}
	}
	
	idThread::ReturnFloat( 0 );
}

void idActor::Event_SetDamageCap( float _damageCap )
{
	damageCap = _damageCap;
}

void idActor::Event_SetWaitState( const char* waitState )
{
	SetWaitState( waitState );
}

void idActor::Event_GetWaitState()
{
	if( WaitState() )
	{
		idThread::ReturnString( WaitState() );
	}
	else
	{
		idThread::ReturnString( "" );
	}
}
void idActor::Event_SetTeam( float _team )
{
	team = _team;
}

void idActor::Event_GetTeam()
{
	idThread::ReturnInt( team );
}
