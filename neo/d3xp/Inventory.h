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
#ifndef __INVENTORY_H__
#define __INVENTORY_H__

// powerups - the "type" in item .def must match
enum
{
	BERSERK = 0,
	INVISIBILITY,
	MEGAHEALTH,
	ADRENALINE,
	INVULNERABILITY,
	HELLTIME,
	ENVIROSUIT,
	//HASTE,
	ENVIROTIME,
	MAX_POWERUPS
};

// powerup modifiers
enum
{
	SPEED = 0,
	PROJECTILE_DAMAGE,
	MELEE_DAMAGE,
	MELEE_DISTANCE
};

typedef struct
{
	int ammo;
	int rechargeTime;
	char ammoName[128];
} RechargeAmmo_t;

struct idLevelTriggerInfo
{
	idStr levelName;
	idStr triggerName;
};

struct idObjectiveInfo
{
	idStr				title;
	idStr				text;
	const idMaterial*	screenshot;
};

const int MAX_WEAPONS = 32;

/*
===============================================================================
	Inventory
===============================================================================
*/

class idInventory
{
public:
	int						maxHealth;
	int						weapons;
	int						powerups;
	int						armor;
	int						maxarmor;
	int						powerupEndTime[ MAX_POWERUPS ];
	
	RechargeAmmo_t			rechargeAmmo[ AMMO_NUMTYPES ];
		
	int						deplete_armor;
	float					deplete_rate;
	int						deplete_ammount;
	int						nextArmorDepleteTime;
	
	int						pdasViewed[4]; // 128 bit flags for indicating if a pda has been viewed
	
	int						selPDA;
	int						selEMail;
	int						selVideo;
	int						selAudio;
	bool					pdaOpened;
	idList<idDict*>		items;
	idList<idStr>			pdaSecurity;
	idList<const idDeclPDA*>	pdas;
	idList<const idDeclVideo*>	videos;
	idList<const idDeclEmail*>	emails;
	
	bool					ammoPulse;
	bool					weaponPulse;
	bool					armorPulse;
	int						lastGiveTime;
	
	idList<idLevelTriggerInfo, TAG_IDLIB_LIST_PLAYER> levelTriggers;
	
	idInventory();
	~idInventory();
	
	// save games
	void					Save( idSaveGame* savefile ) const;					// archives object for save game file
	void					Restore( idRestoreGame* savefile );					// unarchives object from save game file
	
	void					Clear();
	void					GivePowerUp( idPlayer* player, int powerup, int msec );
	void					ClearPowerUps();
	void					GetPersistantData( idDict& dict );
	void					RestoreInventory( idPlayer* owner, const idDict& dict );
	bool					Give( idPlayer* owner, const idDict& spawnArgs, const char* statname, const char* value,
								  idPredictedValue< int >* idealWeapon, bool updateHud, unsigned int giveFlags );
	void					Drop( const idDict& spawnArgs, const char* weapon_classname, int weapon_index );
	ammo_t					AmmoIndexForAmmoClass( const char* ammo_classname ) const;
	int						MaxAmmoForAmmoClass( const idPlayer* owner, const char* ammo_classname ) const;
	int						WeaponIndexForAmmoClass( const idDict& spawnArgs, const char* ammo_classname ) const;
	ammo_t					AmmoIndexForWeaponClass( const char* weapon_classname, int* ammoRequired );
	const char* 			AmmoPickupNameForIndex( ammo_t ammonum ) const;
	void					AddPickupName( const char* name, idPlayer* owner );   //_D3XP
	
	int						HasAmmo( ammo_t type, int amount );
	bool					UseAmmo( ammo_t type, int amount );
	int						HasAmmo( const char* weapon_classname, bool includeClip = false, idPlayer* owner = NULL );			// _D3XP
	
	bool					HasEmptyClipCannotRefill( const char* weapon_classname, idPlayer* owner );
	
	void					UpdateArmor();
	
	void					SetInventoryAmmoForType( const int ammoType, const int amount );
	void					SetClipAmmoForWeapon( const int weapon, const int amount );
	
	int						GetInventoryAmmoForType( const int ammoType ) const;
	int						GetClipAmmoForWeapon( const int weapon ) const;
	
	void					WriteAmmoToSnapshot( idBitMsg& msg ) const;
	void					ReadAmmoFromSnapshot( const idBitMsg& msg, int ownerEntityNumber );
	
	void					SetRemoteClientAmmo( const int ownerEntityNumber );
	
	int						nextItemPickup;
	int						nextItemNum;
	int						onePickupTime;
	idList<idStr>			pickupItemNames;
	idList<idObjectiveInfo>	objectiveNames;
	
	void					InitRechargeAmmo( idPlayer* owner );
	void					RechargeAmmo( idPlayer* owner );
	bool					CanGive( idPlayer* owner, const idDict& spawnArgs, const char* statname, const char* value );
	
private:
	idArray< idPredictedValue< int >, AMMO_NUMTYPES >		ammo;
	idArray< idPredictedValue< int >, MAX_WEAPONS >			clip;
};

#endif /* !__INVENTORY_H__ */

