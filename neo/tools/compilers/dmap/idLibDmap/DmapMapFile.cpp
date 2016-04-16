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

/*
=================
idDmapMapPatch::Parse
=================
*/
idDmapMapPatch* idDmapMapPatch::Parse(idLexer& src, const idVec3& origin, bool patchDef3, float version)
{
	float		info[7];
	idDmapDrawVert* vert;
	idToken		token;
	int			i, j;

	if (!src.ExpectTokenString("{"))
	{
		return NULL;
	}

	// read the material (we had an implicit 'textures/' in the old format...)
	if (!src.ReadToken(&token))
	{
		src.Error("idDmapMapPatch::Parse: unexpected EOF");
		return NULL;
	}

	// Parse it
	if (patchDef3)
	{
		if (!src.Parse1DMatrix(7, info))
		{
			src.Error("idDmapMapPatch::Parse: unable to Parse patchDef3 info");
			return NULL;
		}
	}
	else
	{
		if (!src.Parse1DMatrix(5, info))
		{
			src.Error("idDmapMapPatch::Parse: unable to parse patchDef2 info");
			return NULL;
		}
	}

	idDmapMapPatch* patch = new(TAG_IDLIB)idDmapMapPatch(info[0], info[1]);

	patch->SetSize(info[0], info[1]);
	if (version < 2.0f)
	{
		patch->SetMaterial("textures/" + token);
	}
	else
	{
		patch->SetMaterial(token);
	}

	if (patchDef3)
	{
		patch->SetHorzSubdivisions(info[2]);
		patch->SetVertSubdivisions(info[3]);
		patch->SetExplicitlySubdivided(true);
	}

	if (patch->GetWidth() < 0 || patch->GetHeight() < 0)
	{
		src.Error("idDmapMapPatch::Parse: bad size");
		delete patch;
		return NULL;
	}

	// these were written out in the wrong order, IMHO
	if (!src.ExpectTokenString("("))
	{
		src.Error("idDmapMapPatch::Parse: bad patch vertex data");
		delete patch;
		return NULL;
	}


	for (j = 0; j < patch->GetWidth(); j++)
	{
		if (!src.ExpectTokenString("("))
		{
			src.Error("idDmapMapPatch::Parse: bad vertex row data");
			delete patch;
			return NULL;
		}
		for (i = 0; i < patch->GetHeight(); i++)
		{
			float v[5];

			if (!src.Parse1DMatrix(5, v))
			{
				src.Error("idDmapMapPatch::Parse: bad vertex column data");
				delete patch;
				return NULL;
			}

			vert = &((*patch)[i * patch->GetWidth() + j]);
			vert->xyz[0] = v[0] - origin[0];
			vert->xyz[1] = v[1] - origin[1];
			vert->xyz[2] = v[2] - origin[2];
			vert->st[0] = v[3];
			vert->st[1] = v[4];
		}
		if (!src.ExpectTokenString(")"))
		{
			delete patch;
			src.Error("idDmapMapPatch::Parse: unable to parse patch control points");
			return NULL;
		}
	}

	if (!src.ExpectTokenString(")"))
	{
		src.Error("idDmapMapPatch::Parse: unable to parse patch control points, no closure");
		delete patch;
		return NULL;
	}

	// read any key/value pairs
	while (src.ReadToken(&token))
	{
		if (token == "}")
		{
			src.ExpectTokenString("}");
			break;
		}
		if (token.type == TT_STRING)
		{
			idStr key = token;
			src.ExpectTokenType(TT_STRING, 0, &token);
			patch->epairs.Set(key, token);
		}
	}

	return patch;
}

/*
===============
idDmapMapPatch::GetGeometryCRC
===============
*/
void idDmapMapPatch::GetGeometryCRC( unsigned int & crc ) const {
	CRC32_UpdateChecksum( crc, &horzSubdivisions, sizeof(horzSubdivisions) );
	CRC32_UpdateChecksum( crc, &vertSubdivisions, sizeof(vertSubdivisions) );

	for (int i = 0; i < GetWidth(); i++) {
		for (int j = 0; j < GetHeight(); j++) {
			const idVec3 vec = verts[j * GetWidth() + i].xyz;
			CRC32_UpdateChecksum( crc, &vec, sizeof(vec) );
		}
	}

	CRC32_UpdateChecksum( crc, GetMaterial(), strlen(GetMaterial()) );
}

/*
================
idDmapMapEntity::Parse
================
*/
idDmapMapEntity *idDmapMapEntity::Parse(idLexer &src, bool worldSpawn, float version) {
	idToken	token;
	idDmapMapEntity *mapEnt;
	idDmapMapPatch *mapPatch;
	idMapBrush *mapBrush;
	bool worldent;
	idVec3 origin;
	double v1, v2, v3;

	if (!src.ReadToken(&token)) {
		return NULL;
	}

	if (token != "{") {
		src.Error("idDmapMapEntity::Parse: { not found, found %s", token.c_str());
		return NULL;
	}

	mapEnt = new idDmapMapEntity();

	if (worldSpawn) {
		mapEnt->primitives.Resize(1024, 256);
	}

	origin.Zero();
	worldent = false;
	do {
		if (!src.ReadToken(&token)) {
			src.Error("idDmapMapEntity::Parse: EOF without closing brace");
			return NULL;
		}
		if (token == "}") {
			break;
		}

		if (token == "{") {
			// parse a brush or patch
			if (!src.ReadToken(&token)) {
				src.Error("idDmapMapEntity::Parse: unexpected EOF");
				return NULL;
			}

			if (worldent) {
				origin.Zero();
			}

			// if is it a brush: brush, brushDef, brushDef2, brushDef3
			if (token.Icmpn("brush", 5) == 0) {
				mapBrush = idMapBrush::Parse(src, origin, (!token.Icmp("brushDef2") || !token.Icmp("brushDef3")), version);
				if (!mapBrush) {
					return NULL;
				}
				mapEnt->AddPrimitive(mapBrush);
			}
			// if is it a patch: patchDef2, patchDef3
			else if (token.Icmpn("patch", 5) == 0) {
				mapPatch = idDmapMapPatch::Parse(src, origin, !token.Icmp("patchDef3"), version);
				if (!mapPatch) {
					return NULL;
				}
				mapEnt->AddPrimitive(mapPatch);
			}
			// assume it's a brush in Q3 or older style
			else {
				src.UnreadToken(&token);
				mapBrush = idMapBrush::ParseQ3(src, origin);
				if (!mapBrush) {
					return NULL;
				}
				mapEnt->AddPrimitive(mapBrush);
			}
		}
		else {
			idStr key, value;

			// parse a key / value pair
			key = token;
			src.ReadTokenOnLine(&token);
			value = token;

			// strip trailing spaces that sometimes get accidentally
			// added in the editor
			value.StripTrailingWhitespace();
			key.StripTrailingWhitespace();

			mapEnt->epairs.Set(key, value);

			if (!idStr::Icmp(key, "origin")) {
				// scanf into doubles, then assign, so it is idVec size independent
				v1 = v2 = v3 = 0;
				sscanf(value, "%lf %lf %lf", &v1, &v2, &v3);
				origin.x = v1;
				origin.y = v2;
				origin.z = v3;
			}
			else if (!idStr::Icmp(key, "classname") && !idStr::Icmp(value, "worldspawn")) {
				worldent = true;
			}
		}
	} while (1);

	return mapEnt;
}

/*
===============
idDmapMapEntity::RemovePrimitiveData
===============
*/
void idDmapMapEntity::RemovePrimitiveData() {
	primitives.DeleteContents(true);
}

/*
===============
idDmapMapEntity::GetGeometryCRC
===============
*/
void idDmapMapEntity::GetGeometryCRC( unsigned int & crc ) const {
	for (int i = 0; i < GetNumPrimitives(); i++) {
		idMapPrimitive	*mapPrim = GetPrimitive(i);
		switch (mapPrim->GetType()) {
		case idMapPrimitive::TYPE_BRUSH:
			static_cast<idMapBrush*>(mapPrim)->GetGeometryCRC( crc );
			break;
		case idMapPrimitive::TYPE_PATCH:
			static_cast<idDmapMapPatch*>(mapPrim)->GetGeometryCRC( crc );
			break;
		}
	}
}

/*
===============
idDmapMapFile::Parse
===============
*/
bool idDmapMapFile::Parse(const char *filename, bool ignoreRegion, bool osPath) {
	// no string concatenation for epairs and allow path names for materials
	idLexer src(LEXFL_NOSTRINGCONCAT | LEXFL_NOSTRINGESCAPECHARS | LEXFL_ALLOWPATHNAMES);
	idToken token;
	idStr fullName;
	idDmapMapEntity *mapEnt;
	int i, j, k;

	name = filename;
	name.StripFileExtension();
	fullName = name;
	hasPrimitiveData = false;

	if (!ignoreRegion) {
		// try loading a .reg file first
		fullName.SetFileExtension("reg");
		src.LoadFile(fullName, osPath);
	}

	if (!src.IsLoaded()) {
		// now try a .map file
		fullName.SetFileExtension("map");
		src.LoadFile(fullName, osPath);
		if (!src.IsLoaded()) {
			// didn't get anything at all
			return false;
		}
	}

	version = OLD_MAP_VERSION;
	fileTime = src.GetFileTime();
	entities.DeleteContents(true);

	if (src.CheckTokenString("Version")) {
		src.ReadTokenOnLine(&token);
		version = token.GetFloatValue();
	}

	while (1) {
		mapEnt = idDmapMapEntity::Parse(src, (entities.Num() == 0), version);
		if (!mapEnt) {
			break;
		}
		entities.Append(mapEnt);
	}

	SetGeometryCRC();

	// if the map has a worldspawn
	if (entities.Num()) {

		// "removeEntities" "classname" can be set in the worldspawn to remove all entities with the given classname
		const idKeyValue *removeEntities = entities[0]->epairs.MatchPrefix("removeEntities", NULL);
		while (removeEntities) {
			RemoveEntities(removeEntities->GetValue());
			removeEntities = entities[0]->epairs.MatchPrefix("removeEntities", removeEntities);
		}

		// "overrideMaterial" "material" can be set in the worldspawn to reset all materials
		idStr material;
		if (entities[0]->epairs.GetString("overrideMaterial", "", material)) {
			for (i = 0; i < entities.Num(); i++) {
				mapEnt = entities[i];
				for (j = 0; j < mapEnt->GetNumPrimitives(); j++) {
					idMapPrimitive *mapPrimitive = mapEnt->GetPrimitive(j);
					switch (mapPrimitive->GetType()) {
					case idMapPrimitive::TYPE_BRUSH: {
														 idMapBrush *mapBrush = static_cast<idMapBrush *>(mapPrimitive);
														 for (k = 0; k < mapBrush->GetNumSides(); k++) {
															 mapBrush->GetSide(k)->SetMaterial(material);
														 }
														 break;
					}
					case idMapPrimitive::TYPE_PATCH: {
														 static_cast<idDmapMapPatch *>(mapPrimitive)->SetMaterial(material);
														 break;
					}
					}
				}
			}
		}

		// force all entities to have a name key/value pair
		if (entities[0]->epairs.GetBool("forceEntityNames")) {
			for (i = 1; i < entities.Num(); i++) {
				mapEnt = entities[i];
				if (!mapEnt->epairs.FindKey("name")) {
					mapEnt->epairs.Set("name", va("%s%d", mapEnt->epairs.GetString("classname", "forcedName"), i));
				}
			}
		}

		// move the primitives of any func_group entities to the worldspawn
		if (entities[0]->epairs.GetBool("moveFuncGroups")) {
			for (i = 1; i < entities.Num(); i++) {
				mapEnt = entities[i];
				if (idStr::Icmp(mapEnt->epairs.GetString("classname"), "func_group") == 0) {
					entities[0]->primitives.Append(mapEnt->primitives);
					mapEnt->primitives.Clear();
				}
			}
		}
	}

	hasPrimitiveData = true;
	return true;
}

/*
===============
idDmapMapFile::SetGeometryCRC
===============
*/
void idDmapMapFile::SetGeometryCRC() {
	CRC32_InitChecksum( geometryCRC );

	for ( int i = 0; i < entities.Num(); i++) {
		entities[i]->GetGeometryCRC( geometryCRC );
	}

	CRC32_FinishChecksum( geometryCRC );
}

/*
===============
idDmapMapFile::RemoveEntity
===============
*/
void idDmapMapFile::RemoveEntity(idDmapMapEntity *mapEnt) {
	entities.Remove(mapEnt);
	delete mapEnt;
}

/*
===============
idDmapMapFile::RemoveEntities
===============
*/
void idDmapMapFile::RemoveEntities(const char *classname) {
	for (int i = 0; i < entities.Num(); i++) {
		idDmapMapEntity *ent = entities[i];
		if (idStr::Icmp(ent->epairs.GetString("classname"), classname) == 0) {
			delete entities[i];
			entities.RemoveIndex(i);
			i--;
		}
	}
}

/*
===============
idDmapMapFile::FindEntity
===============
*/
idDmapMapEntity * idDmapMapFile::FindEntity( const char *name )
{
	for ( int i = 0; i < entities.Num(); i++ )
	{
		idDmapMapEntity* ent = entities[ i ];
		if ( idStr::Icmp( ent->epairs.GetString( "name" ), name ) == 0 )
		{
			return ent;
		}
	}
	return NULL;
}

/*
===============
idDmapMapFile::FindEntity
===============
*/
idDmapMapEntity * idDmapMapFile::FindEntityByClassName( const char *name )
{
	for ( int i = 0; i < entities.Num(); i++ )
	{
		idDmapMapEntity* ent = entities[ i ];
		if ( idStr::Icmp( ent->epairs.GetString( "classname" ), name ) == 0 )
		{
			return ent;
		}
	}
	return NULL;
}
