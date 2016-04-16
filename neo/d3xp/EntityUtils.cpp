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


/*
===============
turretController_t
===============
*/
turretController_t::turretController_t()
	: jointYaw( INVALID_JOINT )
	, jointPitch( INVALID_JOINT )
	, jointBarrel( INVALID_JOINT )
	, fieldOfView( 180.0f )
	, yawMin( -180.0f )
	, yawMax( 180.0f )
	, yawRate( 60.0 )
	, pitchMin( -45.0f )
	, pitchMax( 90.0f )
	, pitchRate( 60.0 )
	, trackingEnabled( false )
	, anglesIdeal( vec3_zero )
	, anglesCurrent( vec3_zero )
	/*, projectileDecl( NULL )
	, projectileSpeed( 500.0f )
	, projectileGravity( gameLocal.GetGravity() )
	, projectileMaxHeight( 1.0f )
	, projectileClipModel( NULL )
	, projectileClipMask( MASK_SHOT_RENDERMODEL )*/	
{
}

void turretController_t::SetFov( float fov )
{
	fieldOfView = fov;
}

void turretController_t::SetYawJoint( jointHandle_t joint )
{
	jointYaw = joint;
}

void turretController_t::SetPitchJoint( jointHandle_t joint )
{
	jointPitch = joint;
}

void turretController_t::SetBarrelJoint( jointHandle_t joint )
{
	jointBarrel = joint;
}

void turretController_t::SetYawRate( float rate )
{
	yawRate = rate;
}

void turretController_t::SetYawRange( float minYaw, float maxYaw )
{
	yawMin = minYaw;
	yawMax = maxYaw;
}

void turretController_t::SetPitchRate( float rate )
{
	pitchRate = rate;
}

void turretController_t::SetPitchRange( float minPitch, float maxPitch )
{
	pitchMin = minPitch;
	pitchMax = maxPitch;
}

bool turretController_t::WithinAimTolerance( float yawTolerance, float pitchTolerance )
{
	const idAngles diff = anglesIdeal - anglesCurrent;
	return idMath::Abs( diff.yaw ) <= yawTolerance && idMath::Abs( diff.pitch ) <= pitchTolerance;
}

void turretController_t::Update( idAI * owner )
{
	const float dt = MS2SEC( gameLocal.time - gameLocal.previousTime );

	idVec3 jointPos;
	idMat3 jointAxis;

	target = FindEnemy(owner, true);
	owner->SetEnemy(target);

	if ( trackingEnabled )
	{
		if ( target != NULL )
		{
			const idVec3 targetPos = target->GetPhysics()->GetOrigin() + target->GetPhysics()->GetBounds().GetCenter();

			if ( jointYaw != INVALID_JOINT )
				owner->GetAnimator()->SetJointAxis( jointYaw, JOINTMOD_NONE, mat3_identity );
			if ( jointPitch != INVALID_JOINT )
				owner->GetAnimator()->SetJointAxis( jointPitch, JOINTMOD_NONE, mat3_identity );

			if ( jointYaw != INVALID_JOINT )
			{				
				if ( owner->GetJointWorldTransform( jointYaw, gameLocal.time, jointPos, jointAxis ) )
				{
					idVec3 worldAimDir = targetPos - jointPos;
					worldAimDir.Normalize();

					const idAngles localAngle = (worldAimDir.ToMat3() * jointAxis).ToAngles();
					anglesIdeal.yaw = localAngle.yaw;

#ifdef _DEBUG
					gameRenderWorld->DebugLine( colorGreen, jointPos, targetPos, 0 );
#endif
				}
			}

			if ( jointPitch != INVALID_JOINT )
			{
				if ( owner->GetJointWorldTransform( jointPitch, gameLocal.time, jointPos, jointAxis ) )
				{
					/*idAI::PredictTrajectory( entPhys->GetOrigin(), lastTargetPos[ i ], speed, entPhys->GetGravity(),
						entPhys->GetClipModel(), entPhys->GetClipMask(), 256.0f, ent, targetEnt, ai_debugTrajectory.GetBool() ? 1 : 0, vel );*/
					
					idVec3 worldAimDir = targetPos - jointPos;
					worldAimDir.Normalize();

					const idAngles localAngle = (worldAimDir.ToMat3() * jointAxis).ToAngles();
					anglesIdeal.pitch = localAngle.pitch;

#ifdef _DEBUG
					gameRenderWorld->DebugLine( colorBlue, jointPos, targetPos, 0 );
#endif
				}
			}
			
			/*idVec3 aimDir, aimPos;
			if ( owner->GetAimDir( owner->GetEyePosition(), target, owner, aimDir, aimPos ) )
			{
			}*/
			
			/*if ( owner->GetJointWorldTransform( jointBarrel, gameLocal.time, jointPos, jointAxis ) )
			{
				idVec3 trajectoryVelocity;
				const bool gotTrajectory = idAI::PredictTrajectory( jointPos, targetPos, projectileSpeed, projectileGravity,
					projectileClipModel, projectileClipMask, 
					projectileMaxHeight, owner, 
					target, 
					ai_debugTrajectory.GetBool() ? 1 : 0, 
					trajectoryVelocity );

				if ( gotTrajectory )
				{

				}
			}*/
		}
	}

	anglesIdeal.Normalize180();
	anglesIdeal.Clamp( idAngles( pitchMin, yawMin, 0.0 ), idAngles( pitchMax, yawMax, 0.0 ) );
	idAngles anglesDiff = anglesIdeal - anglesCurrent;
	anglesDiff.Normalize180();

	// temp
	static float turnScalar = 5.0f;
	anglesCurrent += anglesDiff * dt * turnScalar;
	anglesCurrent.Normalize180();
	anglesCurrent.Clamp( idAngles( pitchMin, yawMin, 0.0 ), idAngles( pitchMax, yawMax, 0.0 ) );
	
	if ( jointYaw != INVALID_JOINT )
		owner->GetAnimator()->SetJointAxis( jointYaw, JOINTMOD_LOCAL, idAngles(0.0,anglesCurrent.yaw,0.0).ToMat3() );
	if ( jointPitch != INVALID_JOINT )
		owner->GetAnimator()->SetJointAxis( jointPitch, JOINTMOD_LOCAL, idAngles(anglesCurrent.pitch,0.0,0.0).ToMat3() );	
}

void turretController_t::SetProjectileParms( const idDeclEntityDef * decl )
{
	//projectileDecl = decl;

	//projectileSpeed		= idProjectile::GetVelocity( &projectileDecl->dict ).Length();
	//projectileGravity	= idProjectile::GetGravity( &projectileDecl->dict );
	//projectileMaxHeight = 1.0f; // todo
	//projectileClipMask = MASK_SHOT_RENDERMODEL;

	//idBounds projectileBounds( vec3_origin );
	//projectileBounds.ExpandSelf( 4.0f );
	//projectileClipModel = new( TAG_MODEL ) idClipModel( idTraceModel( projectileBounds ) );
}

void turretController_t::Save( idSaveGame* savefile ) const
{

}

void turretController_t::Restore( idRestoreGame* savefile )
{

}

/*
========================
FindEnemy
========================
*/
idActor * FindEnemy( idEntity * attacker, bool useFov )
{
	// todo: find enemy AI also?

	// look for players only currently
	if( gameLocal.InPlayerPVS( attacker ) )
	{
		for( int i = 0; i < gameLocal.numClients ; i++ )
		{
			idEntity* ent = gameLocal.entities[ i ];
			if( !ent || !ent->IsType( idActor::Type ) )
				continue;

			idActor * actor = static_cast<idActor*>( ent );
			if( ( actor->health <= 0 ) || !( attacker->ReactionTo( actor ) & ATTACK_ON_SIGHT ) )
			{
				continue;
			}

			if( attacker->CanSee( actor, useFov != 0 ) )
			{
				return actor;
			}
		}
	}
	return NULL;
}
