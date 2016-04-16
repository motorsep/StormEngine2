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
#include "EntityUtils.h"

/*
===============================================================================

  idAnimatedEntity

===============================================================================
*/

const idEventDef EV_GetJointHandle( "getJointHandle", "s", 'd' );
const idEventDef EV_ClearAllJoints( "clearAllJoints" );
const idEventDef EV_ClearJoint( "clearJoint", "d" );
const idEventDef EV_SetJointPos( "setJointPos", "ddv" );
const idEventDef EV_SetJointAngle( "setJointAngle", "ddv" );
const idEventDef EV_GetJointPos( "getJointPos", "d", 'v' );
const idEventDef EV_GetJointAngle( "getJointAngle", "d", 'v' );
const idEventDef EV_AnimState( "animState", "dsd" );
const idEventDef EV_GetAnimState( "getAnimState", "d", 's' );
const idEventDef EV_InAnimState( "inAnimState", "ds", 'd' );

CLASS_DECLARATION( idEntity, idAnimatedEntity )
EVENT( EV_GetJointHandle,		idAnimatedEntity::Event_GetJointHandle )
EVENT( EV_ClearAllJoints,		idAnimatedEntity::Event_ClearAllJoints )
EVENT( EV_ClearJoint,			idAnimatedEntity::Event_ClearJoint )
EVENT( EV_SetJointPos,			idAnimatedEntity::Event_SetJointPos )
EVENT( EV_SetJointAngle,		idAnimatedEntity::Event_SetJointAngle )
EVENT( EV_GetJointPos,			idAnimatedEntity::Event_GetJointPos )
EVENT( EV_GetJointAngle,		idAnimatedEntity::Event_GetJointAngle )
EVENT( EV_AnimState,			idAnimatedEntity::Event_AnimState )
EVENT( EV_GetAnimState,			idAnimatedEntity::Event_GetAnimState )
EVENT( EV_InAnimState,			idAnimatedEntity::Event_InAnimState )
END_CLASS

/*
================
idAnimatedEntity::idAnimatedEntity
================
*/
idAnimatedEntity::idAnimatedEntity()
{
	animator.SetEntity( this );
	damageEffects = NULL;
}

/*
================
idAnimatedEntity::~idAnimatedEntity
================
*/
idAnimatedEntity::~idAnimatedEntity()
{
	damageEffect_t*	de;
	
	for( de = damageEffects; de; de = damageEffects )
	{
		damageEffects = de->next;
		delete de;
	}
}

/*
================
idAnimatedEntity::Save

archives object for save game file
================
*/
void idAnimatedEntity::Save( idSaveGame* savefile ) const
{
	animator.Save( savefile );
	
	// Wounds are very temporary, ignored at this time
	//damageEffect_t			*damageEffects;
}

/*
================
idAnimatedEntity::Restore

unarchives object from save game file
================
*/
void idAnimatedEntity::Restore( idRestoreGame* savefile )
{
	animator.Restore( savefile );
	
	// check if the entity has an MD5 model
	if( animator.ModelHandle() )
	{
		// set the callback to update the joints
		renderEntity.callback = idEntity::ModelCallback;
		animator.GetJoints( &renderEntity.numJoints, &renderEntity.joints );
		animator.GetBounds( gameLocal.time, renderEntity.bounds );
		if( modelDefHandle != -1 )
		{
			gameRenderWorld->UpdateEntityDef( modelDefHandle, &renderEntity );
		}
	}
}

/*
================
idAnimatedEntity::ClientPredictionThink
================
*/
void idAnimatedEntity::ClientPredictionThink()
{
	RunPhysics();
	UpdateAnimation();
	Present();
}

/*
================
idAnimatedEntity::ClientThink
================
*/
void idAnimatedEntity::ClientThink( const int curTime, const float fraction, const bool predict )
{
	InterpolatePhysics( fraction );
	UpdateAnimation();
	Present();
}

/*
================
idAnimatedEntity::Think
================
*/
void idAnimatedEntity::Think()
{
	RunPhysics();
	UpdateAnimation();
	Present();
	UpdateDamageEffects();
}

/*
================
idAnimatedEntity::UpdateAnimation
================
*/
void idAnimatedEntity::UpdateAnimation()
{
	// don't do animations if they're not enabled
	if( !( thinkFlags & TH_ANIMATE ) )
	{
		return;
	}
	
	// is the model an MD5?
	if( !animator.ModelHandle() )
	{
		// no, so nothing to do
		return;
	}
	
	// call any frame commands that have happened in the past frame
	if( !fl.hidden )
	{
		animator.ServiceAnims( gameLocal.previousTime, gameLocal.time );
	}
	
	// if the model is animating then we have to update it
	if( !animator.FrameHasChanged( gameLocal.time ) )
	{
		// still fine the way it was
		return;
	}
	
	// get the latest frame bounds
	animator.GetBounds( gameLocal.time, renderEntity.bounds );
	if( renderEntity.bounds.IsCleared() && !fl.hidden )
	{
		gameLocal.DPrintf( "%d: inside out bounds\n", gameLocal.time );
	}
	
	// update the renderEntity
	UpdateVisuals();
	
	// the animation is updated
	animator.ClearForceUpdate();
}

/*
================
idAnimatedEntity::GetAnimator
================
*/
idAnimator* idAnimatedEntity::GetAnimator()
{
	return &animator;
}

/*
================
idAnimatedEntity::SetModel
================
*/
void idAnimatedEntity::SetModel( const char* modelname )
{
	FreeModelDef();
	
	renderEntity.hModel = animator.SetModel( modelname );
	if( !renderEntity.hModel )
	{
		idEntity::SetModel( modelname );
		return;
	}
	
	if( !renderEntity.customSkin )
	{
		renderEntity.customSkin = animator.ModelDef()->GetDefaultSkin();
	}
	
	// set the callback to update the joints
	renderEntity.callback = idEntity::ModelCallback;
	animator.GetJoints( &renderEntity.numJoints, &renderEntity.joints );
	animator.GetBounds( gameLocal.time, renderEntity.bounds );
	
	UpdateVisuals();
}

/*
=====================
idAnimatedEntity::GetJointWorldTransform
=====================
*/
bool idAnimatedEntity::GetJointWorldTransform( jointHandle_t jointHandle, int currentTime, idVec3& offset, idMat3& axis )
{
	if( !animator.GetJointTransform( jointHandle, currentTime, offset, axis ) )
	{
		return false;
	}
	
	ConvertLocalToWorldTransform( offset, axis );
	return true;
}

/*
==============
idAnimatedEntity::GetJointTransformForAnim
==============
*/
bool idAnimatedEntity::GetJointTransformForAnim( jointHandle_t jointHandle, int animNum, int frameTime, idVec3& offset, idMat3& axis ) const
{
	const idAnim*	anim;
	int				numJoints;
	idJointMat*		frame;
	
	anim = animator.GetAnim( animNum );
	if( !anim )
	{
		assert( 0 );
		return false;
	}
	
	numJoints = animator.NumJoints();
	if( ( jointHandle < 0 ) || ( jointHandle >= numJoints ) )
	{
		assert( 0 );
		return false;
	}
	
	frame = ( idJointMat* )_alloca16( numJoints * sizeof( idJointMat ) );
	gameEdit->ANIM_CreateAnimFrame( animator.ModelHandle(), anim->MD5Anim( 0 ), renderEntity.numJoints, frame, frameTime, animator.ModelDef()->GetVisualOffset(), animator.RemoveOrigin() );
	
	offset = frame[ jointHandle ].ToVec3();
	axis = frame[ jointHandle ].ToMat3();
	
	return true;
}

/*
==============
idAnimatedEntity::AddDamageEffect

  Dammage effects track the animating impact position, spitting out particles.
==============
*/
void idAnimatedEntity::AddDamageEffect( const trace_t& collision, const idVec3& velocity, const char* damageDefName )
{
	jointHandle_t jointNum;
	idVec3 origin, dir, localDir, localOrigin, localNormal;
	idMat3 axis;
	
	if( !g_bloodEffects.GetBool() || renderEntity.joints == NULL )
	{
		return;
	}
	
	const idDeclEntityDef* def = gameLocal.FindEntityDef( damageDefName, false );
	if( def == NULL )
	{
		return;
	}
	
	jointNum = CLIPMODEL_ID_TO_JOINT_HANDLE( collision.c.id );
	if( jointNum == INVALID_JOINT )
	{
		return;
	}
	
	dir = velocity;
	dir.Normalize();
	
	axis = renderEntity.joints[jointNum].ToMat3() * renderEntity.axis;
	origin = renderEntity.origin + renderEntity.joints[jointNum].ToVec3() * renderEntity.axis;
	
	localOrigin = ( collision.c.point - origin ) * axis.Transpose();
	localNormal = collision.c.normal * axis.Transpose();
	localDir = dir * axis.Transpose();
	
	AddLocalDamageEffect( jointNum, localOrigin, localNormal, localDir, def, collision.c.material );
}

/*
==============
idAnimatedEntity::GetDefaultSurfaceType
==============
*/
int	idAnimatedEntity::GetDefaultSurfaceType() const
{
	return SURFTYPE_METAL;
}

/*
==============
idAnimatedEntity::AddLocalDamageEffect
==============
*/
void idAnimatedEntity::AddLocalDamageEffect( jointHandle_t jointNum, const idVec3& localOrigin, const idVec3& localNormal, const idVec3& localDir, const idDeclEntityDef* def, const idMaterial* collisionMaterial )
{
	const char* sound, *splat, *decal, *bleed, *key;
	damageEffect_t*	de;
	idVec3 origin, dir;
	idMat3 axis;
	float flesh_wound_size; // SS2 wound decal size

	SetTimeState ts( timeGroup );
	
	axis = renderEntity.joints[jointNum].ToMat3() * renderEntity.axis;
	origin = renderEntity.origin + renderEntity.joints[jointNum].ToVec3() * renderEntity.axis;
	
	origin = origin + localOrigin * axis;
	dir = localDir * axis;
	
	int type = collisionMaterial->GetSurfaceType();
	if( type == SURFTYPE_NONE )
	{
		type = GetDefaultSurfaceType();
	}
	
	const char* materialType = gameLocal.sufaceTypeNames[ type ];
	
	// start impact sound based on material type
	key = va( "snd_%s", materialType );
	sound = spawnArgs.GetString( key );
	if( *sound == '\0' )
	{
		sound = def->dict.GetString( key );
	}
	if( *sound != '\0' )
	{
		StartSoundShader( declManager->FindSound( sound ), SND_CHANNEL_BODY, 0, false, NULL );
	}
	
	// blood splats are thrown onto nearby surfaces
	key = va( "mtr_splat_%s", materialType );
	splat = spawnArgs.RandomPrefix( key, gameLocal.random );
	if( *splat == '\0' )
	{
		splat = def->dict.RandomPrefix( key, gameLocal.random );
	}
	if( *splat != '\0' )
	{
		gameLocal.BloodSplat( origin, dir, 64.0f, splat );
	}
	
	// SS2 wound decal size
	spawnArgs.GetFloat( "flesh_wound_size", "20", flesh_wound_size );
	flesh_wound_size = idMath::ClampFloat( 0.0f, 30.0f, flesh_wound_size );

	// can't see wounds on the player model in single player mode
	if( !( IsType( idPlayer::Type ) && !common->IsMultiplayer() ) )
	{
		// place a wound overlay on the model
		key = va( "mtr_wound_%s", materialType );
		decal = spawnArgs.RandomPrefix( key, gameLocal.random );
		if( *decal == '\0' )
		{
			decal = def->dict.RandomPrefix( key, gameLocal.random );
		}
		if( *decal != '\0' )
		{
			ProjectOverlay( origin, dir, flesh_wound_size, decal ); // was 20.0f
		}
	}
	
	// a blood spurting wound is added
	key = va( "smoke_wound_%s", materialType );
	bleed = spawnArgs.GetString( key );
	if( *bleed == '\0' )
	{
		bleed = def->dict.GetString( key );
	}
	if( *bleed != '\0' )
	{
		de = new( TAG_ENTITY ) damageEffect_t;
		de->next = this->damageEffects;
		this->damageEffects = de;
		
		de->jointNum = jointNum;
		de->localOrigin = localOrigin;
		de->localNormal = localNormal;
		de->type = static_cast<const idDeclParticle*>( declManager->FindType( DECL_PARTICLE, bleed ) );
		de->time = gameLocal.time;
	}
}

/*
==============
idAnimatedEntity::UpdateDamageEffects
==============
*/
void idAnimatedEntity::UpdateDamageEffects()
{
	damageEffect_t*	de, **prev;
	
	// free any that have timed out
	prev = &this->damageEffects;
	while( *prev )
	{
		de = *prev;
		if( de->time == 0 )  	// FIXME:SMOKE
		{
			*prev = de->next;
			delete de;
		}
		else
		{
			prev = &de->next;
		}
	}
	
	if( !g_bloodEffects.GetBool() )
	{
		return;
	}
	
	// emit a particle for each bleeding wound
	for( de = this->damageEffects; de; de = de->next )
	{
		idVec3 origin, start;
		idMat3 axis;
		
		animator.GetJointTransform( de->jointNum, gameLocal.time, origin, axis );
		axis *= renderEntity.axis;
		origin = renderEntity.origin + origin * renderEntity.axis;
		start = origin + de->localOrigin * axis;
		if( !gameLocal.smokeParticles->EmitSmoke( de->type, de->time, gameLocal.random.CRandomFloat(), start, axis, timeGroup /*_D3XP*/ ) )
		{
			de->time = 0;
		}
	}
}

/*
================
idAnimatedEntity::ClientReceiveEvent
================
*/
bool idAnimatedEntity::ClientReceiveEvent( int event, int time, const idBitMsg& msg )
{
	int damageDefIndex;
	int materialIndex;
	jointHandle_t jointNum;
	idVec3 localOrigin, localNormal, localDir;
	
	switch( event )
	{
		case EVENT_ADD_DAMAGE_EFFECT:
		{
			jointNum = ( jointHandle_t ) msg.ReadShort();
			localOrigin[0] = msg.ReadFloat();
			localOrigin[1] = msg.ReadFloat();
			localOrigin[2] = msg.ReadFloat();
			localNormal = msg.ReadDir( 24 );
			localDir = msg.ReadDir( 24 );
			damageDefIndex = gameLocal.ClientRemapDecl( DECL_ENTITYDEF, msg.ReadLong() );
			materialIndex = gameLocal.ClientRemapDecl( DECL_MATERIAL, msg.ReadLong() );
			const idDeclEntityDef* damageDef = static_cast<const idDeclEntityDef*>( declManager->DeclByIndex( DECL_ENTITYDEF, damageDefIndex ) );
			const idMaterial* collisionMaterial = static_cast<const idMaterial*>( declManager->DeclByIndex( DECL_MATERIAL, materialIndex ) );
			AddLocalDamageEffect( jointNum, localOrigin, localNormal, localDir, damageDef, collisionMaterial );
			return true;
		}
		default:
		{
			return idEntity::ClientReceiveEvent( event, time, msg );
		}
	}
}

/*
================
idAnimatedEntity::Event_GetJointHandle

looks up the number of the specified joint.  returns INVALID_JOINT if the joint is not found.
================
*/
void idAnimatedEntity::Event_GetJointHandle( const char* jointname )
{
	jointHandle_t joint = animator.GetJointHandle( jointname );
	idThread::ReturnInt( joint );
}

/*
================
idAnimatedEntity::Event_ClearAllJoints

removes any custom transforms on all joints
================
*/
void idAnimatedEntity::Event_ClearAllJoints()
{
	animator.ClearAllJoints();
}

/*
================
idAnimatedEntity::Event_ClearJoint

removes any custom transforms on the specified joint
================
*/
void idAnimatedEntity::Event_ClearJoint( jointHandle_t jointnum )
{
	animator.ClearJoint( jointnum );
}

/*
================
idAnimatedEntity::Event_SetJointPos

modifies the position of the joint based on the transform type
================
*/
void idAnimatedEntity::Event_SetJointPos( jointHandle_t jointnum, jointModTransform_t transform_type, const idVec3& pos )
{
	animator.SetJointPos( jointnum, transform_type, pos );
}

/*
================
idAnimatedEntity::Event_SetJointAngle

modifies the orientation of the joint based on the transform type
================
*/
void idAnimatedEntity::Event_SetJointAngle( jointHandle_t jointnum, jointModTransform_t transform_type, const idAngles& angles )
{
	idMat3 mat = angles.ToMat3();
	animator.SetJointAxis( jointnum, transform_type, mat );
}

/*
================
idAnimatedEntity::Event_GetJointPos

returns the position of the joint in worldspace
================
*/
void idAnimatedEntity::Event_GetJointPos( jointHandle_t jointnum )
{
	idVec3 offset;
	idMat3 axis;
	
	if( !GetJointWorldTransform( jointnum, gameLocal.time, offset, axis ) )
	{
		gameLocal.Warning( "Joint # %d out of range on entity '%s'",  jointnum, name.c_str() );
	}
	
	idThread::ReturnVector( offset );
}

/*
================
idAnimatedEntity::Event_GetJointAngle

returns the orientation of the joint in worldspace
================
*/
void idAnimatedEntity::Event_GetJointAngle( jointHandle_t jointnum )
{
	idVec3 offset;
	idMat3 axis;
	
	if( !GetJointWorldTransform( jointnum, gameLocal.time, offset, axis ) )
	{
		gameLocal.Warning( "Joint # %d out of range on entity '%s'",  jointnum, name.c_str() );
	}
	
	idAngles ang = axis.ToAngles();
	idVec3 vec( ang[ 0 ], ang[ 1 ], ang[ 2 ] );
	idThread::ReturnVector( vec );
}

/*
===============
idAnimatedEntity::Event_AnimState
===============
*/
void idAnimatedEntity::Event_AnimState( int channel, const char* statename, int blendFrames )
{
	SetAnimState( channel, statename, blendFrames );
}

/*
===============
idAnimatedEntity::Event_GetAnimState
===============
*/
void idAnimatedEntity::Event_GetAnimState( int channel )
{
	const char* state = GetAnimState( channel );
	idThread::ReturnString( state );
}

/*
===============
idAnimatedEntity::Event_InAnimState
===============
*/
void idAnimatedEntity::Event_InAnimState( int channel, const char* statename )
{
	idThread::ReturnInt( InAnimState( channel, statename ) );
}
