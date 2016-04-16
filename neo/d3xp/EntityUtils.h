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
#ifndef __GAME_ENTITY_UTILS_H__
#define __GAME_ENTITY_UTILS_H__

class idEntity;
class idAI;

enum TurretParm
{
	TURRET_YAW,
	TURRET_PITCH,
};

struct turretController_t
{
public:	
	void SetFov( float fov );

	void SetYawJoint( jointHandle_t joint );
	void SetPitchJoint( jointHandle_t joint );
	void SetBarrelJoint( jointHandle_t jointBarrel );

	void SetYawRate( float rate );
	void SetYawRange( float minYaw, float maxYaw );
	void SetPitchRate( float rate );
	void SetPitchRange( float minPitch, float maxPitch );

	const idAngles & GetAnglesIdeal() const { return anglesIdeal; }
	const idAngles & GetAnglesCurrent() const { return anglesCurrent; }

	void SetAnglesIdeal(const idAngles & angles) { anglesIdeal = angles; }
	void SetAnglesCurrent(const idAngles & angles) { anglesCurrent = angles; }

	bool WithinAimTolerance( float yawTolerance, float pitchTolerance );

	void SetTrackTarget( bool track ) { trackingEnabled = track; }

	void SetProjectileParms( const idDeclEntityDef * decl );

	void Update( idAI * owner );

	void Save( idSaveGame* savefile ) const;
	void Restore( idRestoreGame* savefile );

	turretController_t();
private:
	float					fieldOfView;

	idEntityPtr<idActor>	target;

	jointHandle_t			jointYaw;
	jointHandle_t			jointPitch;
	jointHandle_t			jointBarrel;

	float					yawMin, yawMax, yawRate;
	float					pitchMin, pitchMax, pitchRate;

	idAngles				anglesIdeal;
	idAngles				anglesCurrent;

	bool					trackingEnabled;

	//const idDeclEntityDef * projectileDecl;
	//float					projectileSpeed;	
	//idVec3					projectileGravity;
	//float					projectileMaxHeight;
	//idClipModel*			projectileClipModel;
	//int						projectileClipMask;
	
};

idActor * FindEnemy( idEntity * attacker, bool useFov );

#endif /* !__GAME_ENTITY_UTILS_H__ */
