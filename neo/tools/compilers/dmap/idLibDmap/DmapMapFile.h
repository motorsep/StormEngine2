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
#ifndef __DMAPMAPFILE_H__
#define __DMAPMAPFILE_H__

class idDmapMapPatch : public idMapPrimitive, public idDmapSurface_Patch
{
public:
	idDmapMapPatch();
	idDmapMapPatch(int maxPatchWidth, int maxPatchHeight);
	~idDmapMapPatch() { }
	static idDmapMapPatch* 	Parse(idLexer& src, const idVec3& origin, bool patchDef3 = true, float version = CURRENT_MAP_VERSION);
	bool					Write(idFile* fp, int primitiveNum, const idVec3& origin) const;
	const char* 			GetMaterial() const
	{
		return material;
	}
	void					SetMaterial(const char* p)
	{
		material = p;
	}
	int						GetHorzSubdivisions() const
	{
		return horzSubdivisions;
	}
	int						GetVertSubdivisions() const
	{
		return vertSubdivisions;
	}
	bool					GetExplicitlySubdivided() const
	{
		return explicitSubdivisions;
	}
	void					SetHorzSubdivisions(int n)
	{
		horzSubdivisions = n;
	}
	void					SetVertSubdivisions(int n)
	{
		vertSubdivisions = n;
	}
	void					SetExplicitlySubdivided(bool b)
	{
		explicitSubdivisions = b;
	}
	void					GetGeometryCRC( unsigned int & crc ) const;

protected:
	idStr					material;
	int						horzSubdivisions;
	int						vertSubdivisions;
	bool					explicitSubdivisions;
};

ID_INLINE idDmapMapPatch::idDmapMapPatch()
{
	type = TYPE_PATCH;
	horzSubdivisions = vertSubdivisions = 0;
	explicitSubdivisions = false;
	width = height = 0;
	maxWidth = maxHeight = 0;
	expanded = false;
}

ID_INLINE idDmapMapPatch::idDmapMapPatch(int maxPatchWidth, int maxPatchHeight)
{
	type = TYPE_PATCH;
	horzSubdivisions = vertSubdivisions = 0;
	explicitSubdivisions = false;
	width = height = 0;
	maxWidth = maxPatchWidth;
	maxHeight = maxPatchHeight;
	verts.SetNum(maxWidth * maxHeight);
	expanded = false;
}


class idDmapMapEntity {
	friend class			idDmapMapFile;

public:
	idDict					epairs;

public:
	idDmapMapEntity(void) { epairs.SetHashSize(64); }
	~idDmapMapEntity(void) { primitives.DeleteContents(true); }
	static idDmapMapEntity *	Parse(idLexer &src, bool worldSpawn = false, float version = CURRENT_MAP_VERSION);
	bool					Write(idFile *fp, int entityNum) const;
	int						GetNumPrimitives(void) const { return primitives.Num(); }
	idMapPrimitive *		GetPrimitive(int i) const { return primitives[i]; }
	void					AddPrimitive(idMapPrimitive *p) { primitives.Append(p); }
	void					GetGeometryCRC( unsigned int & crc ) const;
	void					RemovePrimitiveData();

protected:
	idList<idMapPrimitive*>	primitives;
};

class idDmapMapFile {
public:
	idDmapMapFile(void);
	~idDmapMapFile(void) { entities.DeleteContents(true); }

	// filename does not require an extension
	// normally this will use a .reg file instead of a .map file if it exists,
	// which is what the game and dmap want, but the editor will want to always
	// load a .map file
	bool					Parse(const char *filename, bool ignoreRegion = false, bool osPath = false);
	bool					Write(const char *fileName, const char *ext, bool fromBasePath = true);
	// get the number of entities in the map
	int						GetNumEntities(void) const { return entities.Num(); }
	// get the specified entity
	idDmapMapEntity *			GetEntity(int i) const { return entities[i]; }
	// get the name without file extension
	const char *			GetName(void) const { return name; }
	// get the file time
	ID_TIME_T					GetFileTime(void) const { return fileTime; }
	// get CRC for the map geometry
	// texture coordinates and entity key/value pairs are not taken into account
	unsigned int			GetGeometryCRC(void) const { return geometryCRC; }
	// returns true if the file on disk changed
	bool					NeedsReload();

	int						AddEntity(idDmapMapEntity *mapentity);
	idDmapMapEntity *		FindEntity(const char *name);
	idDmapMapEntity *		FindEntityByClassName( const char *name );
	void					RemoveEntity(idDmapMapEntity *mapEnt);
	void					RemoveEntities(const char *classname);
	void					RemoveAllEntities();
	void					RemovePrimitiveData();
	bool					HasPrimitiveData() { return hasPrimitiveData; }

protected:
	float					version;
	ID_TIME_T					fileTime;
	unsigned int			geometryCRC;
	idList<idDmapMapEntity *>	entities;
	idStr					name;
	bool					hasPrimitiveData;

private:
	void					SetGeometryCRC(void);
};

ID_INLINE idDmapMapFile::idDmapMapFile(void) {
	version = CURRENT_MAP_VERSION;
	fileTime = 0;
	geometryCRC = 0;
	entities.Resize(1024, 256);
	hasPrimitiveData = false;
}

#endif /* !__DMAPMAPFILE_H__ */
