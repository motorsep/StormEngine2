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

#include "PredictedValue_impl.h"

idInventory::idInventory()
{
	Clear();
}

idInventory::~idInventory()
{
	Clear();
}

/*
==============
idInventory::Clear
==============
*/
void idInventory::Clear()
{
	maxHealth		= 0;
	weapons			= 0;
	powerups		= 0;
	armor			= 0;
	maxarmor		= 0;
	deplete_armor	= 0;
	deplete_rate	= 0.0f;
	deplete_ammount	= 0;
	nextArmorDepleteTime = 0;

	for( int i = 0; i < ammo.Num(); ++i )
	{
		ammo[i].Set( 0 );
	}

	ClearPowerUps();

	// set to -1 so that the gun knows to have a full clip the first time we get it and at the start of the level
	for( int i = 0; i < clip.Num(); ++i )
	{
		clip[i].Set( -1 );
	}

	items.DeleteContents( true );
	memset( pdasViewed, 0, 4 * sizeof( pdasViewed[0] ) );
	pdas.Clear();
	videos.Clear();
	emails.Clear();
	selVideo = 0;
	selEMail = 0;
	selPDA = 0;
	selAudio = 0;
	pdaOpened = false;

	levelTriggers.Clear();

	nextItemPickup = 0;
	nextItemNum = 1;
	onePickupTime = 0;
	pickupItemNames.Clear();
	objectiveNames.Clear();

	lastGiveTime = 0;

	ammoPulse	= false;
	weaponPulse	= false;
	armorPulse	= false;
}

/*
==============
idInventory::GivePowerUp
==============
*/
void idInventory::GivePowerUp( idPlayer* player, int powerup, int msec )
{
	powerups |= 1 << powerup;
	powerupEndTime[ powerup ] = gameLocal.time + msec;
}

/*
==============
idInventory::ClearPowerUps
==============
*/
void idInventory::ClearPowerUps()
{
	int i;
	for( i = 0; i < MAX_POWERUPS; i++ )
	{
		powerupEndTime[ i ] = 0;
	}
	powerups = 0;
}

/*
==============
idInventory::GetPersistantData
==============
*/
void idInventory::GetPersistantData( idDict& dict )
{
	int		i;
	int		num;
	idDict*	item;
	idStr	key;
	const idKeyValue* kv;
	const char* name;

	// armor
	dict.SetInt( "armor", armor );

	// don't bother with powerups, maxhealth, maxarmor, or the clip

	// ammo
	for( i = 0; i < AMMO_NUMTYPES; i++ )
	{
		name = idWeapon::GetAmmoNameForNum( ( ammo_t )i );
		if( name )
		{
			dict.SetInt( name, ammo[ i ].Get() );
		}
	}

	//Save the clip data
	for( i = 0; i < MAX_WEAPONS; i++ )
	{
		dict.SetInt( va( "clip%i", i ), clip[ i ].Get() );
	}

	// items
	num = 0;
	for( i = 0; i < items.Num(); i++ )
	{
		item = items[ i ];

		// copy all keys with "inv_"
		kv = item->MatchPrefix( "inv_" );
		if( kv )
		{
			while( kv )
			{
				sprintf( key, "item_%i %s", num, kv->GetKey().c_str() );
				dict.Set( key, kv->GetValue() );
				kv = item->MatchPrefix( "inv_", kv );
			}
			num++;
		}
	}
	dict.SetInt( "items", num );

	// pdas viewed
	for( i = 0; i < 4; i++ )
	{
		dict.SetInt( va( "pdasViewed_%i", i ), pdasViewed[i] );
	}

	dict.SetInt( "selPDA", selPDA );
	dict.SetInt( "selVideo", selVideo );
	dict.SetInt( "selEmail", selEMail );
	dict.SetInt( "selAudio", selAudio );
	dict.SetInt( "pdaOpened", pdaOpened );

	// pdas
	for( i = 0; i < pdas.Num(); i++ )
	{
		sprintf( key, "pda_%i", i );
		dict.Set( key, pdas[ i ]->GetName() );
	}
	dict.SetInt( "pdas", pdas.Num() );

	// video cds
	for( i = 0; i < videos.Num(); i++ )
	{
		sprintf( key, "video_%i", i );
		dict.Set( key, videos[ i ]->GetName() );
	}
	dict.SetInt( "videos", videos.Num() );

	// emails
	for( i = 0; i < emails.Num(); i++ )
	{
		sprintf( key, "email_%i", i );
		dict.Set( key, emails[ i ]->GetName() );
	}
	dict.SetInt( "emails", emails.Num() );

	// weapons
	dict.SetInt( "weapon_bits", weapons );

	dict.SetInt( "levelTriggers", levelTriggers.Num() );
	for( i = 0; i < levelTriggers.Num(); i++ )
	{
		sprintf( key, "levelTrigger_Level_%i", i );
		dict.Set( key, levelTriggers[i].levelName );
		sprintf( key, "levelTrigger_Trigger_%i", i );
		dict.Set( key, levelTriggers[i].triggerName );
	}
}

/*
==============
idInventory::RestoreInventory
==============
*/
void idInventory::RestoreInventory( idPlayer* owner, const idDict& dict )
{
	int			i;
	int			num;
	idDict*		item;
	idStr		key;
	idStr		itemname;
	const idKeyValue* kv;
	const char*	name;

	Clear();

	// health/armor
	maxHealth		= dict.GetInt( "maxhealth", "100" );
	armor			= dict.GetInt( "armor", "50" );
	maxarmor		= dict.GetInt( "maxarmor", "100" );
	deplete_armor	= dict.GetInt( "deplete_armor", "0" );
	deplete_rate	= dict.GetFloat( "deplete_rate", "2.0" );
	deplete_ammount	= dict.GetInt( "deplete_ammount", "1" );

	// the clip and powerups aren't restored

	// ammo
	for( i = 0; i < AMMO_NUMTYPES; i++ )
	{
		name = idWeapon::GetAmmoNameForNum( ( ammo_t )i );
		if( name )
		{
			ammo[ i ] = dict.GetInt( name );
		}
	}

	//Restore the clip data
	for( i = 0; i < MAX_WEAPONS; i++ )
	{
		clip[i] = dict.GetInt( va( "clip%i", i ), "-1" );
	}

	// items
	num = dict.GetInt( "items" );
	items.SetNum( num );
	for( i = 0; i < num; i++ )
	{
		item = new( TAG_ENTITY ) idDict();
		items[ i ] = item;
		sprintf( itemname, "item_%i ", i );
		kv = dict.MatchPrefix( itemname );
		while( kv )
		{
			key = kv->GetKey();
			key.Strip( itemname );
			item->Set( key, kv->GetValue() );
			kv = dict.MatchPrefix( itemname, kv );
		}
	}

	// pdas viewed
	for( i = 0; i < 4; i++ )
	{
		pdasViewed[i] = dict.GetInt( va( "pdasViewed_%i", i ) );
	}

	selPDA = dict.GetInt( "selPDA" );
	selEMail = dict.GetInt( "selEmail" );
	selVideo = dict.GetInt( "selVideo" );
	selAudio = dict.GetInt( "selAudio" );
	pdaOpened = dict.GetBool( "pdaOpened" );

	// pdas
	num = dict.GetInt( "pdas" );
	pdas.SetNum( num );
	for( i = 0; i < num; i++ )
	{
		sprintf( itemname, "pda_%i", i );
		pdas[i] = static_cast<const idDeclPDA*>( declManager->FindType( DECL_PDA, dict.GetString( itemname, "default" ) ) );
	}

	// videos
	num = dict.GetInt( "videos" );
	videos.SetNum( num );
	for( i = 0; i < num; i++ )
	{
		sprintf( itemname, "video_%i", i );
		videos[i] = static_cast<const idDeclVideo*>( declManager->FindType( DECL_VIDEO, dict.GetString( itemname, "default" ) ) );
	}

	// emails
	num = dict.GetInt( "emails" );
	emails.SetNum( num );
	for( i = 0; i < num; i++ )
	{
		sprintf( itemname, "email_%i", i );
		emails[i] = static_cast<const idDeclEmail*>( declManager->FindType( DECL_EMAIL, dict.GetString( itemname, "default" ) ) );
	}

	// weapons are stored as a number for persistant data, but as strings in the entityDef
	weapons	= dict.GetInt( "weapon_bits", "0" );

	if( g_skill.GetInteger() >= 3 || cvarSystem->GetCVarBool( "fs_buildresources" ) )
	{
		Give( owner, dict, "weapon", dict.GetString( "weapon_nightmare" ), NULL, false, ITEM_GIVE_FEEDBACK | ITEM_GIVE_UPDATE_STATE );
	}
	else
	{
		Give( owner, dict, "weapon", dict.GetString( "weapon" ), NULL, false, ITEM_GIVE_FEEDBACK | ITEM_GIVE_UPDATE_STATE );
	}

	num = dict.GetInt( "levelTriggers" );
	for( i = 0; i < num; i++ )
	{
		sprintf( itemname, "levelTrigger_Level_%i", i );
		idLevelTriggerInfo lti;
		lti.levelName = dict.GetString( itemname );
		sprintf( itemname, "levelTrigger_Trigger_%i", i );
		lti.triggerName = dict.GetString( itemname );
		levelTriggers.Append( lti );
	}

}

/*
==============
idInventory::Save
==============
*/
void idInventory::Save( idSaveGame* savefile ) const
{
	int i;

	savefile->WriteInt( maxHealth );
	savefile->WriteInt( weapons );
	savefile->WriteInt( powerups );
	savefile->WriteInt( armor );
	savefile->WriteInt( maxarmor );
	savefile->WriteInt( deplete_armor );
	savefile->WriteFloat( deplete_rate );
	savefile->WriteInt( deplete_ammount );
	savefile->WriteInt( nextArmorDepleteTime );

	for( i = 0; i < AMMO_NUMTYPES; i++ )
	{
		savefile->WriteInt( ammo[ i ].Get() );
	}
	for( i = 0; i < MAX_WEAPONS; i++ )
	{
		savefile->WriteInt( clip[ i ].Get() );
	}
	for( i = 0; i < MAX_POWERUPS; i++ )
	{
		savefile->WriteInt( powerupEndTime[ i ] );
	}

	savefile->WriteInt( items.Num() );
	for( i = 0; i < items.Num(); i++ )
	{
		savefile->WriteDict( items[ i ] );
	}

	savefile->WriteInt( pdasViewed[0] );
	savefile->WriteInt( pdasViewed[1] );
	savefile->WriteInt( pdasViewed[2] );
	savefile->WriteInt( pdasViewed[3] );

	savefile->WriteInt( selPDA );
	savefile->WriteInt( selVideo );
	savefile->WriteInt( selEMail );
	savefile->WriteInt( selAudio );
	savefile->WriteBool( pdaOpened );

	savefile->WriteInt( pdas.Num() );
	for( i = 0; i < pdas.Num(); i++ )
	{
		savefile->WriteString( pdas[ i ]->GetName() );
	}

	savefile->WriteInt( pdaSecurity.Num() );
	for( i = 0; i < pdaSecurity.Num(); i++ )
	{
		savefile->WriteString( pdaSecurity[ i ] );
	}

	savefile->WriteInt( videos.Num() );
	for( i = 0; i < videos.Num(); i++ )
	{
		savefile->WriteString( videos[ i ]->GetName() );
	}

	savefile->WriteInt( emails.Num() );
	for( i = 0; i < emails.Num(); i++ )
	{
		savefile->WriteString( emails[ i ]->GetName() );
	}

	savefile->WriteInt( nextItemPickup );
	savefile->WriteInt( nextItemNum );
	savefile->WriteInt( onePickupTime );

	savefile->WriteInt( pickupItemNames.Num() );
	for( i = 0; i < pickupItemNames.Num(); i++ )
	{
		savefile->WriteString( pickupItemNames[i] );
	}

	savefile->WriteInt( objectiveNames.Num() );
	for( i = 0; i < objectiveNames.Num(); i++ )
	{
		savefile->WriteMaterial( objectiveNames[i].screenshot );
		savefile->WriteString( objectiveNames[i].text );
		savefile->WriteString( objectiveNames[i].title );
	}

	savefile->WriteInt( levelTriggers.Num() );
	for( i = 0; i < levelTriggers.Num(); i++ )
	{
		savefile->WriteString( levelTriggers[i].levelName );
		savefile->WriteString( levelTriggers[i].triggerName );
	}

	savefile->WriteBool( ammoPulse );
	savefile->WriteBool( weaponPulse );
	savefile->WriteBool( armorPulse );

	savefile->WriteInt( lastGiveTime );

	for( i = 0; i < AMMO_NUMTYPES; i++ )
	{
		savefile->WriteInt( rechargeAmmo[i].ammo );
		savefile->WriteInt( rechargeAmmo[i].rechargeTime );
		savefile->WriteString( rechargeAmmo[i].ammoName );
	}
}

/*
==============
idInventory::Restore
==============
*/
void idInventory::Restore( idRestoreGame* savefile )
{	
	int num;

	savefile->ReadInt( maxHealth );
	savefile->ReadInt( weapons );
	savefile->ReadInt( powerups );
	savefile->ReadInt( armor );
	savefile->ReadInt( maxarmor );
	savefile->ReadInt( deplete_armor );
	savefile->ReadFloat( deplete_rate );
	savefile->ReadInt( deplete_ammount );
	savefile->ReadInt( nextArmorDepleteTime );

	for( int i = 0; i < AMMO_NUMTYPES; i++ )
	{
		int savedAmmo = 0;
		savefile->ReadInt( savedAmmo );
		ammo[ i ].Set( savedAmmo );
	}
	for( int i = 0; i < MAX_WEAPONS; i++ )
	{
		int savedClip = 0;
		savefile->ReadInt( savedClip );
		clip[ i ].Set( savedClip );
	}
	for( int i = 0; i < MAX_POWERUPS; i++ )
	{
		savefile->ReadInt( powerupEndTime[ i ] );
	}

	savefile->ReadInt( num );
	for( int i = 0; i < num; i++ )
	{
		idDict* itemdict = new( TAG_ENTITY ) idDict;

		savefile->ReadDict( itemdict );
		items.Append( itemdict );
	}

	// pdas
	savefile->ReadInt( pdasViewed[0] );
	savefile->ReadInt( pdasViewed[1] );
	savefile->ReadInt( pdasViewed[2] );
	savefile->ReadInt( pdasViewed[3] );

	savefile->ReadInt( selPDA );
	savefile->ReadInt( selVideo );
	savefile->ReadInt( selEMail );
	savefile->ReadInt( selAudio );
	savefile->ReadBool( pdaOpened );

	savefile->ReadInt( num );
	for( int i = 0; i < num; i++ )
	{
		idStr strPda;
		savefile->ReadString( strPda );
		pdas.Append( static_cast<const idDeclPDA*>( declManager->FindType( DECL_PDA, strPda ) ) );
	}

	// pda security clearances
	savefile->ReadInt( num );
	for( int i = 0; i < num; i++ )
	{
		idStr invName;
		savefile->ReadString( invName );
		pdaSecurity.Append( invName );
	}

	// videos
	savefile->ReadInt( num );
	for( int i = 0; i < num; i++ )
	{
		idStr strVideo;
		savefile->ReadString( strVideo );
		videos.Append( static_cast<const idDeclVideo*>( declManager->FindType( DECL_VIDEO, strVideo ) ) );
	}

	// email
	savefile->ReadInt( num );
	for( int i = 0; i < num; i++ )
	{
		idStr strEmail;
		savefile->ReadString( strEmail );
		emails.Append( static_cast<const idDeclEmail*>( declManager->FindType( DECL_EMAIL, strEmail ) ) );
	}

	savefile->ReadInt( nextItemPickup );
	savefile->ReadInt( nextItemNum );
	savefile->ReadInt( onePickupTime );
	savefile->ReadInt( num );
	for( int i = 0; i < num; i++ )
	{
		idStr itemName;
		savefile->ReadString( itemName );
		pickupItemNames.Append( itemName );
	}

	savefile->ReadInt( num );
	for( int i = 0; i < num; i++ )
	{
		idObjectiveInfo obj;

		savefile->ReadMaterial( obj.screenshot );
		savefile->ReadString( obj.text );
		savefile->ReadString( obj.title );

		objectiveNames.Append( obj );
	}

	savefile->ReadInt( num );
	for( int i = 0; i < num; i++ )
	{
		idLevelTriggerInfo lti;
		savefile->ReadString( lti.levelName );
		savefile->ReadString( lti.triggerName );
		levelTriggers.Append( lti );
	}

	savefile->ReadBool( ammoPulse );
	savefile->ReadBool( weaponPulse );
	savefile->ReadBool( armorPulse );

	savefile->ReadInt( lastGiveTime );

	for( int i = 0; i < AMMO_NUMTYPES; i++ )
	{
		savefile->ReadInt( rechargeAmmo[i].ammo );
		savefile->ReadInt( rechargeAmmo[i].rechargeTime );

		idStr name;
		savefile->ReadString( name );
		strcpy( rechargeAmmo[i].ammoName, name );
	}
}

/*
==============
idInventory::AmmoIndexForAmmoClass
==============
*/
ammo_t idInventory::AmmoIndexForAmmoClass( const char* ammo_classname ) const
{
	return idWeapon::GetAmmoNumForName( ammo_classname );
}

/*
==============
idInventory::AmmoIndexForAmmoClass
==============
*/
int idInventory::MaxAmmoForAmmoClass( const idPlayer* owner, const char* ammo_classname ) const
{
	return owner->spawnArgs.GetInt( va( "max_%s", ammo_classname ), "0" );
}

/*
==============
idInventory::AmmoPickupNameForIndex
==============
*/
const char* idInventory::AmmoPickupNameForIndex( ammo_t ammonum ) const
{
	return idWeapon::GetAmmoPickupNameForNum( ammonum );
}

/*
==============
idInventory::WeaponIndexForAmmoClass
mapping could be prepared in the constructor
==============
*/
int idInventory::WeaponIndexForAmmoClass( const idDict& spawnArgs, const char* ammo_classname ) const
{
	int i;
	const char* weapon_classname;
	for( i = 0; i < MAX_WEAPONS; i++ )
	{
		weapon_classname = spawnArgs.GetString( va( "def_weapon%d", i ) );
		if( !weapon_classname )
		{
			continue;
		}
		const idDeclEntityDef* decl = gameLocal.FindEntityDef( weapon_classname, false );
		if( !decl )
		{
			continue;
		}
		if( !idStr::Icmp( ammo_classname, decl->dict.GetString( "ammoType" ) ) )
		{
			return i;
		}
	}
	return -1;
}

/*
==============
idInventory::AmmoIndexForWeaponClass
==============
*/
ammo_t idInventory::AmmoIndexForWeaponClass( const char* weapon_classname, int* ammoRequired )
{
	const idDeclEntityDef* decl = gameLocal.FindEntityDef( weapon_classname, false );
	if( !decl )
	{
		//gameLocal.Error( "Unknown weapon in decl '%s'", weapon_classname );
		return 0;
	}
	if( ammoRequired )
	{
		*ammoRequired = decl->dict.GetInt( "ammoRequired" );
	}
	ammo_t ammo_i = AmmoIndexForAmmoClass( decl->dict.GetString( "ammoType" ) );
	return ammo_i;
}

/*
==============
idInventory::AddPickupName
==============
*/
void idInventory::AddPickupName( const char* name, idPlayer* owner )     //_D3XP
{
	int num = pickupItemNames.Num();
	if( ( num == 0 ) || ( pickupItemNames[ num - 1 ].Icmp( name ) != 0 ) )
	{
		if( idStr::Cmpn( name, STRTABLE_ID, STRTABLE_ID_LENGTH ) == 0 )
		{
			pickupItemNames.Append( idLocalization::GetString( name ) );
		}
		else
		{
			pickupItemNames.Append( name );
		}
	}
}

/*
==============
idInventory::Give
==============
*/
bool idInventory::Give( idPlayer* owner, const idDict& spawnArgs, const char* statname, const char* value,
					   idPredictedValue< int >* idealWeapon, bool updateHud, unsigned int giveFlags )
{
	int						i;
	const char*				pos;
	const char*				end;
	int						len;
	idStr					weaponString;
	int						max;
	const idDeclEntityDef*	weaponDecl;
	bool					tookWeapon;
	int						amount;
	const char*				name;

	if( !idStr::Icmp( statname, "ammo_bloodstone" ) )
	{
		i = AmmoIndexForAmmoClass( statname );
		max = MaxAmmoForAmmoClass( owner, statname );

		if( max <= 0 )
		{
			if( giveFlags & ITEM_GIVE_UPDATE_STATE )
			{
				//No Max
				ammo[ i ] += atoi( value );
			}
		}
		else
		{
			//Already at or above the max so don't allow the give
			if( ammo[ i ].Get() >= max )
			{
				if( giveFlags & ITEM_GIVE_UPDATE_STATE )
				{
					ammo[ i ] = max;
				}
				return false;
			}
			if( giveFlags & ITEM_GIVE_UPDATE_STATE )
			{
				//We were below the max so accept the give but cap it at the max
				ammo[ i ] += atoi( value );
				if( ammo[ i ].Get() > max )
				{
					ammo[ i ] = max;
				}
			}
		}
	}
	else if( !idStr::Icmpn( statname, "ammo_", 5 ) )
	{
		i = AmmoIndexForAmmoClass( statname );
		max = MaxAmmoForAmmoClass( owner, statname );
		if( ammo[ i ].Get() >= max )
		{
			return false;
		}
		// Add ammo for the feedback flag because it's predicted.
		// If it is a misprediction, the client will be corrected in
		// a snapshot.
		if( giveFlags & ITEM_GIVE_FEEDBACK )
		{
			amount = atoi( value );
			if( amount )
			{
				ammo[ i ] += amount;
				if( ( max > 0 ) && ( ammo[ i ].Get() > max ) )
				{
					ammo[ i ] = max;
				}
				ammoPulse = true;
			}

			name = AmmoPickupNameForIndex( i );
			if( idStr::Length( name ) )
			{
				AddPickupName( name, owner ); //_D3XP
			}
		}
	}
	else if( !idStr::Icmp( statname, "armor" ) )
	{
		if( armor >= maxarmor )
		{
			return false;	// can't hold any more, so leave the item
		}
		if( giveFlags & ITEM_GIVE_UPDATE_STATE )
		{
			amount = atoi( value );
			if( amount )
			{
				armor += amount;
				if( armor > maxarmor )
				{
					armor = maxarmor;
				}
				nextArmorDepleteTime = 0;
				armorPulse = true;
			}
		}
	}
	else if( idStr::FindText( statname, "inclip_" ) == 0 )
	{
		if( giveFlags & ITEM_GIVE_UPDATE_STATE )
		{
			idStr temp = statname;
			i = atoi( temp.Mid( 7, 2 ) );
			if( i != -1 )
			{
				// set, don't add. not going over the clip size limit.
				SetClipAmmoForWeapon( i, atoi( value ) );
			}
		}
	}
	else if( !idStr::Icmp( statname, "invulnerability" ) )
	{
		owner->GivePowerUp( INVULNERABILITY, SEC2MS( atof( value ) ), giveFlags );
	}
	else if( !idStr::Icmp( statname, "helltime" ) )
	{
		owner->GivePowerUp( HELLTIME, SEC2MS( atof( value ) ), giveFlags );
	}
	else if( !idStr::Icmp( statname, "envirosuit" ) )
	{
		owner->GivePowerUp( ENVIROSUIT, SEC2MS( atof( value ) ), giveFlags );
		owner->GivePowerUp( ENVIROTIME, SEC2MS( atof( value ) ), giveFlags );
	}
	else if( !idStr::Icmp( statname, "berserk" ) )
	{
		owner->GivePowerUp( BERSERK, SEC2MS( atof( value ) ), giveFlags );
		//} else if ( !idStr::Icmp( statname, "haste" ) ) {
		//	owner->GivePowerUp( HASTE, SEC2MS( atof( value ) ) );
	}
	else if( !idStr::Icmp( statname, "adrenaline" ) )
	{
		if( giveFlags & ITEM_GIVE_UPDATE_STATE )
		{
			GivePowerUp( owner, ADRENALINE, SEC2MS( atof( value ) ) );
		}
	}
	else if( !idStr::Icmp( statname, "mega" ) )
	{
		if( giveFlags & ITEM_GIVE_UPDATE_STATE )
		{
			GivePowerUp( owner, MEGAHEALTH, SEC2MS( atof( value ) ) );
		}
	}
	else if( !idStr::Icmp( statname, "weapon" ) )
	{
		tookWeapon = false;
		for( pos = value; pos != NULL; pos = end )
		{
			end = strchr( pos, ',' );
			if( end )
			{
				len = end - pos;
				end++;
			}
			else
			{
				len = strlen( pos );
			}

			idStr weaponName( pos, 0, len );

			// find the number of the matching weapon name
			for( i = 0; i < MAX_WEAPONS; i++ )
			{
				if( weaponName == spawnArgs.GetString( va( "def_weapon%d", i ) ) )
				{
					break;
				}
			}

			if( i >= MAX_WEAPONS )
			{
				gameLocal.Warning( "Unknown weapon '%s'", weaponName.c_str() );
				continue;
			}

			// cache the media for this weapon
			weaponDecl = gameLocal.FindEntityDef( weaponName, false );

			// don't pickup "no ammo" weapon types twice
			// not for D3 SP .. there is only one case in the game where you can get a no ammo
			// weapon when you might already have it, in that case it is more conistent to pick it up
			if( common->IsMultiplayer() && ( weapons & ( 1 << i ) ) && ( weaponDecl != NULL ) && !weaponDecl->dict.GetInt( "ammoRequired" ) )
			{
				continue;
			}

			if( !gameLocal.world->spawnArgs.GetBool( "no_Weapons" ) || ( weaponName == "weapon_fists" ) || ( weaponName == "weapon_soulcube" ) )
			{
				if( ( weapons & ( 1 << i ) ) == 0 || common->IsMultiplayer() )
				{
					tookWeapon = true;

					// This is done during "feedback" so that clients can predict the ideal weapon.
					if( giveFlags & ITEM_GIVE_FEEDBACK )
					{
						idLobbyBase& lobby = session->GetActingGameStateLobbyBase();
						lobbyUserID_t& lobbyUserID = gameLocal.lobbyUserIDs[owner->entityNumber];
						if( lobby.GetLobbyUserWeaponAutoSwitch( lobbyUserID ) && idealWeapon != NULL && i != owner->weapon_bloodstone_active1 && i != owner->weapon_bloodstone_active2 && i != owner->weapon_bloodstone_active3 )
						{
							idealWeapon->Set( i );
						}
					}

					if( giveFlags & ITEM_GIVE_UPDATE_STATE )
					{
						if( updateHud && lastGiveTime + 1000 < gameLocal.time )
						{
							if( owner->hud )
							{
								owner->hud->GiveWeapon( owner, i );
							}
							lastGiveTime = gameLocal.time;
						}

						weaponPulse = true;
						weapons |= ( 1 << i );


						if( weaponName != "weapon_pda" )
						{
							for( int index = 0; index < NUM_QUICK_SLOTS; ++index )
							{
								if( owner->GetQuickSlot( index ) == -1 )
								{
									owner->SetQuickSlot( index, i );
									break;
								}
							}
						}
					}
				}
			}
		}
		return tookWeapon;
	}
	else if( !idStr::Icmp( statname, "item" ) || !idStr::Icmp( statname, "icon" ) || !idStr::Icmp( statname, "name" ) )
	{
		// ignore these as they're handled elsewhere
		return false;
	}
	else
	{
		// unknown item
		gameLocal.Warning( "Unknown stat '%s' added to player's inventory", statname );
		return false;
	}

	return true;
}

/*
===============
idInventoy::Drop
===============
*/
void idInventory::Drop( const idDict& spawnArgs, const char* weapon_classname, int weapon_index )
{
	// remove the weapon bit
	// also remove the ammo associated with the weapon as we pushed it in the item
	assert( weapon_index != -1 || weapon_classname );
	if( weapon_index == -1 )
	{
		for( weapon_index = 0; weapon_index < MAX_WEAPONS; weapon_index++ )
		{
			if( !idStr::Icmp( weapon_classname, spawnArgs.GetString( va( "def_weapon%d", weapon_index ) ) ) )
			{
				break;
			}
		}
		if( weapon_index >= MAX_WEAPONS )
		{
			gameLocal.Error( "Unknown weapon '%s'", weapon_classname );
		}
	}
	else if( !weapon_classname )
	{
		weapon_classname = spawnArgs.GetString( va( "def_weapon%d", weapon_index ) );
	}
	weapons &= ( 0xffffffff ^ ( 1 << weapon_index ) );
	ammo_t ammo_i = AmmoIndexForWeaponClass( weapon_classname, NULL );
	if( ammo_i && ammo_i < AMMO_NUMTYPES )
	{
		clip[ weapon_index ] = -1;
		ammo[ ammo_i ] = 0;
	}
}

/*
===============
idInventory::HasAmmo
===============
*/
int idInventory::HasAmmo( ammo_t type, int amount )
{
	if( ( type == 0 ) || !amount )
	{
		// always allow weapons that don't use ammo to fire
		return -1;
	}

	// check if we have infinite ammo
	if( ammo[ type ].Get() < 0 )
	{
		return -1;
	}

	// return how many shots we can fire
	return ammo[ type ].Get() / amount;

}

/*
===============
idInventory::HasAmmo
===============
*/
int idInventory::HasAmmo( const char* weapon_classname, bool includeClip, idPlayer* owner )  		//_D3XP
{
	int ammoRequired;
	ammo_t ammo_i = AmmoIndexForWeaponClass( weapon_classname, &ammoRequired );

	int ammoCount = HasAmmo( ammo_i, ammoRequired );
	if( includeClip && owner )
	{
		ammoCount += Max( 0, clip[owner->SlotForWeapon( weapon_classname )].Get() );
	}
	return ammoCount;

}

/*
===============
idInventory::HasEmptyClipCannotRefill
===============
*/
bool idInventory::HasEmptyClipCannotRefill( const char* weapon_classname, idPlayer* owner )
{

	int clipSize = clip[owner->SlotForWeapon( weapon_classname )].Get();
	if( clipSize )
	{
		return false;
	}

	const idDeclEntityDef* decl = gameLocal.FindEntityDef( weapon_classname, false );
	if( decl == NULL )
	{
		gameLocal.Error( "Unknown weapon in decl '%s'", weapon_classname );
		return false;
	}
	int minclip = decl->dict.GetInt( "minclipsize" );
	if( !minclip )
	{
		return false;
	}

	ammo_t ammo_i = AmmoIndexForAmmoClass( decl->dict.GetString( "ammoType" ) );
	int ammoRequired = decl->dict.GetInt( "ammoRequired" );
	int ammoCount = HasAmmo( ammo_i, ammoRequired );
	if( ammoCount < minclip )
	{
		return true;
	}
	return false;
}

/*
===============
idInventory::UseAmmo
===============
*/
bool idInventory::UseAmmo( ammo_t type, int amount )
{
	if( g_infiniteAmmo.GetBool() )
	{
		return true;
	}

	if( !HasAmmo( type, amount ) )
	{
		return false;
	}

	// take an ammo away if not infinite
	if( ammo[ type ].Get() >= 0 )
	{
		const int currentAmmo = GetInventoryAmmoForType( type );
		SetInventoryAmmoForType( type, currentAmmo - amount );
	}

	return true;
}

/*
===============
idInventory::UpdateArmor
===============
*/
void idInventory::UpdateArmor()
{
	if( deplete_armor != 0.0f && deplete_armor < armor )
	{
		if( !nextArmorDepleteTime )
		{
			nextArmorDepleteTime = gameLocal.time + deplete_rate * 1000;
		}
		else if( gameLocal.time > nextArmorDepleteTime )
		{
			armor -= deplete_ammount;
			if( armor < deplete_armor )
			{
				armor = deplete_armor;
			}
			nextArmorDepleteTime = gameLocal.time + deplete_rate * 1000;
		}
	}
}

/*
===============
idInventory::InitRechargeAmmo
===============
* Loads any recharge ammo definitions from the ammo_types entity definitions.
*/
void idInventory::InitRechargeAmmo( idPlayer* owner )
{

	memset( rechargeAmmo, 0, sizeof( rechargeAmmo ) );

	const idKeyValue* kv = owner->spawnArgs.MatchPrefix( "ammorecharge_" );
	while( kv )
	{
		idStr key = kv->GetKey();
		idStr ammoname = key.Right( key.Length() - strlen( "ammorecharge_" ) );
		int ammoType = AmmoIndexForAmmoClass( ammoname );
		rechargeAmmo[ammoType].ammo = ( atof( kv->GetValue().c_str() ) * 1000 );
		strcpy( rechargeAmmo[ammoType].ammoName, ammoname );
		kv = owner->spawnArgs.MatchPrefix( "ammorecharge_", kv );
	}
}

/*
===============
idInventory::RechargeAmmo
===============
* Called once per frame to update any ammo amount for ammo types that recharge.
*/
void idInventory::RechargeAmmo( idPlayer* owner )
{

	for( int i = 0; i < AMMO_NUMTYPES; i++ )
	{
		if( rechargeAmmo[i].ammo > 0 )
		{
			if( !rechargeAmmo[i].rechargeTime )
			{
				//Initialize the recharge timer.
				rechargeAmmo[i].rechargeTime = gameLocal.time;
			}
			int elapsed = gameLocal.time - rechargeAmmo[i].rechargeTime;
			if( elapsed >= rechargeAmmo[i].ammo )
			{
				int intervals = ( gameLocal.time - rechargeAmmo[i].rechargeTime ) / rechargeAmmo[i].ammo;
				ammo[i] += intervals;

				int max = MaxAmmoForAmmoClass( owner, rechargeAmmo[i].ammoName );
				if( max > 0 )
				{
					if( ammo[i].Get() > max )
					{
						ammo[i] = max;
					}
				}
				rechargeAmmo[i].rechargeTime += intervals * rechargeAmmo[i].ammo;
			}
		}
	}
}

/*
===============
idInventory::CanGive
===============
*/
bool idInventory::CanGive( idPlayer* owner, const idDict& spawnArgs, const char* statname, const char* value )
{

	if( !idStr::Icmp( statname, "ammo_bloodstone" ) )
	{
		int max = MaxAmmoForAmmoClass( owner, statname );
		int i = AmmoIndexForAmmoClass( statname );

		if( max <= 0 )
		{
			//No Max
			return true;
		}
		else
		{
			//Already at or above the max so don't allow the give
			if( ammo[ i ].Get() >= max )
			{
				ammo[ i ] = max;
				return false;
			}
			return true;
		}
	}
	else if( !idStr::Icmp( statname, "item" ) || !idStr::Icmp( statname, "icon" ) || !idStr::Icmp( statname, "name" ) )
	{
		// ignore these as they're handled elsewhere
		//These items should not be considered as succesful gives because it messes up the max ammo items
		return false;
	}
	return true;
}

/*
===============
idInventory::SetClipAmmoForWeapon

Ammo is predicted on clients. This function ensures the frame the prediction occurs
is stored so the predicted value doesn't get overwritten by snapshots. Of course
the snapshot-reading function must check this value.
===============
*/
void idInventory::SetClipAmmoForWeapon( const int weapon, const int amount )
{
	clip[weapon] = amount;
}

/*
===============
idInventory::SetInventoryAmmoForType

Ammo is predicted on clients. This function ensures the frame the prediction occurs
is stored so the predicted value doesn't get overwritten by snapshots. Of course
the snapshot-reading function must check this value.
===============
*/
void idInventory::SetInventoryAmmoForType( int ammoType, const int amount )
{
	ammo[ammoType] = amount;
}

/*
===============
idInventory::GetClipAmmoForWeapon
===============
*/
int idInventory::GetClipAmmoForWeapon( const int weapon ) const
{
	return clip[weapon].Get();
}

/*
===============
idInventory::GetInventoryAmmoForType
===============
*/
int	idInventory::GetInventoryAmmoForType( const int ammoType ) const
{
	return ammo[ammoType].Get();
}

/*
===============
idInventory::WriteAmmoToSnapshot
===============
*/
void idInventory::WriteAmmoToSnapshot( idBitMsg& msg ) const
{
	for( int i = 0; i < AMMO_NUMTYPES; i++ )
	{
		msg.WriteBits( ammo[i].Get(), ASYNC_PLAYER_INV_AMMO_BITS );
	}
	for( int i = 0; i < MAX_WEAPONS; i++ )
	{
		msg.WriteBits( clip[i].Get(), ASYNC_PLAYER_INV_CLIP_BITS );
	}
}

/*
===============
idInventory::ReadAmmoFromSnapshot
===============
*/
void idInventory::ReadAmmoFromSnapshot( const idBitMsg& msg, const int ownerEntityNumber )
{
	for( int i = 0; i < ammo.Num(); i++ )
	{
		const int snapshotAmmo = msg.ReadBits( ASYNC_PLAYER_INV_AMMO_BITS );
		ammo[i].UpdateFromSnapshot( snapshotAmmo, ownerEntityNumber );
	}
	for( int i = 0; i < clip.Num(); i++ )
	{
		const int snapshotClip = msg.ReadBits( ASYNC_PLAYER_INV_CLIP_BITS );
		clip[i].UpdateFromSnapshot( snapshotClip, ownerEntityNumber );
	}
}

/*
===============
idInventory::ReadAmmoFromSnapshot

Doesn't really matter what remote client's ammo count is, so just set it to 999.
===============
*/
void idInventory::SetRemoteClientAmmo( const int ownerEntityNumber )
{
	for( int i = 0; i < ammo.Num(); ++i )
	{
		ammo[i].UpdateFromSnapshot( 999, ownerEntityNumber );
	}
}
