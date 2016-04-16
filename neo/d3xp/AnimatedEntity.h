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
#ifndef __ANIMATED_ENTITY_H__
#define __ANIMATED_ENTITY_H__

/*
===============================================================================

	Animated entity base class.

===============================================================================
*/
typedef struct damageEffect_s
{
	jointHandle_t			jointNum;
	idVec3					localOrigin;
	idVec3					localNormal;
	int						time;
	const idDeclParticle*	type;
	struct damageEffect_s* 	next;
} damageEffect_t;

class idAnimatedEntity : public idEntity
{
public:
	CLASS_PROTOTYPE( idAnimatedEntity );
	
	idAnimatedEntity();
	~idAnimatedEntity();
	
	void					Save( idSaveGame* savefile ) const;
	void					Restore( idRestoreGame* savefile );
	
	virtual void			ClientPredictionThink();
	virtual void			ClientThink( const int curTime, const float fraction, const bool predict );
	virtual void			Think();
	
	void					UpdateAnimation();
	
	virtual idAnimator* 	GetAnimator();
	virtual void			SetModel( const char* modelname );
	
	bool					GetJointWorldTransform( jointHandle_t jointHandle, int currentTime, idVec3& offset, idMat3& axis );
	bool					GetJointTransformForAnim( jointHandle_t jointHandle, int animNum, int currentTime, idVec3& offset, idMat3& axis ) const;
	
	virtual int				GetDefaultSurfaceType() const;
	virtual void			AddDamageEffect( const trace_t& collision, const idVec3& velocity, const char* damageDefName );
	void					AddLocalDamageEffect( jointHandle_t jointNum, const idVec3& localPoint, const idVec3& localNormal, const idVec3& localDir, const idDeclEntityDef* def, const idMaterial* collisionMaterial );
	void					UpdateDamageEffects();

	virtual void			SetAnimState( int channel, const char* statename, int blendFrames ) {}
	virtual const char*		GetAnimState( int channel ) const { return ""; }
	virtual bool			InAnimState( int channel, const char* statename ) const { return false; }
	
	virtual bool			ClientReceiveEvent( int event, int time, const idBitMsg& msg );
	
	enum
	{
		EVENT_ADD_DAMAGE_EFFECT = idEntity::EVENT_MAXEVENTS,
		EVENT_MAXEVENTS
	};
	
protected:
	idAnimator					animator;
	damageEffect_t* 			damageEffects;
private:
	void					Event_GetJointHandle( const char* jointname );
	void 					Event_ClearAllJoints();
	void 					Event_ClearJoint( jointHandle_t jointnum );
	void 					Event_SetJointPos( jointHandle_t jointnum, jointModTransform_t transform_type, const idVec3& pos );
	void 					Event_SetJointAngle( jointHandle_t jointnum, jointModTransform_t transform_type, const idAngles& angles );
	void 					Event_GetJointPos( jointHandle_t jointnum );
	void 					Event_GetJointAngle( jointHandle_t jointnum );
	void					Event_AnimState( int channel, const char* statename, int blendFrames );
	void					Event_GetAnimState( int channel );
	void					Event_InAnimState( int channel, const char* statename );
};

#endif /* !__GAME_ENTITY_H__ */
